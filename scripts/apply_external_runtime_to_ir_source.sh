#!/usr/bin/env bash
set -Eeuo pipefail

if [[ "$#" -ne 1 ]]; then
  echo "usage: $0 <IR_Generator.cpp>" >&2
  exit 2
fi

src="$1"
[[ -s "${src}" ]] || { echo "error: IR generator source is missing or empty: ${src}" >&2; exit 1; }

require_literal() {
  local needle="$1"
  grep -Fq -- "${needle}" "${src}" || {
    echo "error: canonical external-runtime lowering is missing required source: ${needle}" >&2
    exit 1
  }
}

forbid_literal() {
  local needle="$1"
  if grep -Fq -- "${needle}" "${src}"; then
    echo "error: source-level runtime lowering still contains forbidden local-stub/native-link text: ${needle}" >&2
    exit 1
  fi
}

# PR63/PR72 permanently integrated external runtime lowering into the checked-in
# source. This target remains intentionally named runtime-source-lowering for
# compatibility with older build and enterprise-hardening contracts, but it is
# now validation-only. The build must not require Python to mutate source files.
require_literal 'return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);'
require_literal 'static std::string resolveShortHandRuntimeLibrary()'
require_literal 'static std::string resolveShortHandNativeLinker()'
require_literal 'Linked ShortHand runtime library:'
require_literal 'Native linker:'
require_literal 'std::string cmd_obj = "llc -filetype=obj " + shellQuote(base + ".bc")'
require_literal 'std::string cmd_bin = shellQuote(native_linker);'
require_literal '#if !defined(_WIN32)'
require_literal 'cmd_bin += " -no-pie";'
require_literal 'const std::string binary_file = base + ".exe";'
require_literal 'module->getOrInsertFunction("_write", writeType)'
require_literal 'module->getOrInsertFunction("write", writeType)'

forbid_literal 'BasicBlock *entry = BasicBlock::Create(ShortGlobalContext, "entry", fn);'
forbid_literal 'IRBuilder<> stubBuilder(entry);'
forbid_literal 'stubBuilder.CreateRet(ConstantInt::get(i32Ty(), 0, true));'
forbid_literal 'std::string cmd_bin = "clang -no-pie " + base + ".o -o " + base;'

if grep -Eq 'python(3)?[[:space:]]+-' "$0"; then
  echo "error: runtime source-lowering guard must remain Python-free" >&2
  exit 1
fi

printf 'PASS external runtime source lowering is canonical, target-aware and Python-free: %s\n' "${src}"
