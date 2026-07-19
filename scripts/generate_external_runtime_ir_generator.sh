#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 2 ]]; then
  echo "usage: $0 <IR_Generator.cpp> <output.cpp>" >&2
  exit 2
fi

src="$1"
out="$2"
mkdir -p "$(dirname "$out")"

python3 - "$src" "$out" <<'PY'
import sys
from pathlib import Path

src = Path(sys.argv[1])
out = Path(sys.argv[2])
text = src.read_text()

old_hook = '''static Function *ensureShortHandRuntimeHook(const std::string &name, size_t argc) {
    if (!module) return nullptr;
    if (Function *existing = module->getFunction(name)) return existing;
    std::vector<Type*> args(argc, i8PtrTy());
    FunctionType *ftype = FunctionType::get(i32Ty(), args, false);
    Function *fn = Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);
    BasicBlock *entry = BasicBlock::Create(ShortGlobalContext, "entry", fn);
    IRBuilder<> stubBuilder(entry);
    stubBuilder.CreateRet(ConstantInt::get(i32Ty(), 0, true));
    return fn;
}
'''

new_hook = '''static Function *ensureShortHandRuntimeHook(const std::string &name, size_t argc) {
    if (!module) return nullptr;
    if (Function *existing = module->getFunction(name)) return existing;
    std::vector<Type*> args(argc, i8PtrTy());
    FunctionType *ftype = FunctionType::get(i32Ty(), args, false);
    return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);
}
'''

if old_hook not in text:
    raise SystemExit("expected local runtime-hook stub implementation was not found")
text = text.replace(old_hook, new_hook)

helper_anchor = '''bool IR_Generator::dumpNativeBinary() {
'''
helpers = r'''static bool fileExists(const std::string &path) {
    std::ifstream in(path.c_str());
    return in.good();
}

static std::string shellQuote(const std::string &value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static std::string resolveShortHandRuntimeLibrary() {
    const char *env = std::getenv("SHORTHAND_RUNTIME_LIB");
    if (env && *env) return std::string(env);

    const std::vector<std::string> candidates = {
        "Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a",
        "../build/libshorthand_runtime.a",
        "build/libshorthand_runtime.a"
    };
    for (const auto &candidate : candidates) {
        if (fileExists(candidate)) return candidate;
    }
    return "";
}

'''

if helpers not in text:
    text = text.replace(helper_anchor, helpers + helper_anchor)

start = text.find('bool IR_Generator::dumpNativeBinary() {')
end_marker = '\nValue *IR_Generator::get_expression() {'
end = text.find(end_marker, start)
if start == -1 or end == -1:
    raise SystemExit("could not locate dumpNativeBinary block")

new_dump = r'''bool IR_Generator::dumpNativeBinary() {
    if (!dumpBitcode()) return false;
    std::string base = this->getModuleName();
    std::string cmd_obj = "llc -filetype=obj " + shellQuote(base + ".bc") + " -o " + shellQuote(base + ".o");
    std::string runtime_lib = resolveShortHandRuntimeLibrary();
    std::string cmd_bin = "clang -no-pie " + shellQuote(base + ".o");
    if (!runtime_lib.empty()) {
        cmd_bin += " " + shellQuote(runtime_lib);
    } else {
        cerr << "No ShortHand runtime library found. Native linking will fail if AI/GreenAI runtime hooks are referenced.\n";
    }
    cmd_bin += " -o " + shellQuote(base);

    if (std::system(cmd_obj.c_str()) != 0) {
        cerr << "Failed to run llc. Ensure LLVM tools are installed and in PATH.\n";
        return false;
    }
    if (std::system(cmd_bin.c_str()) != 0) {
        cerr << "Failed to run clang linker step. Build libshorthand_runtime.a or set SHORTHAND_RUNTIME_LIB for AI/GreenAI programs.\n";
        return false;
    }
    if (!runtime_lib.empty()) {
        cerr << "Linked ShortHand runtime library: " << runtime_lib << "\n";
    }
    cerr << "Generated native binary: " << base << "\n";
    return true;
}
'''

text = text[:start] + new_dump + text[end:]

if 'BasicBlock *entry = BasicBlock::Create(ShortGlobalContext, "entry", fn);' in text:
    raise SystemExit("local runtime-hook stub body still present after transform")

out.write_text(text)
PY
