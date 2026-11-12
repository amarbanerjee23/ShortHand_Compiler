#include "IR_Generator.h"
#include "DiagnosticCodes.h"
#include "Symbol_Table.h"
#include "../util/util.h"
#include "../ai_runtime/AI_Types.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

using namespace llvm;
using namespace std;
namespace diag = shorthand::diagnostics;

static Module *module = nullptr;
static map<string, Value*> NamedValues;
static LLVMContext ShortGlobalContext;
static IRBuilder<> Builder(ShortGlobalContext);
static Symbol_Table symbol_table_obj;
static FunctionCallee CalleeF;
static FunctionCallee CalleeR;
static unsigned ShortHandMetadataCounter = 0;

static Type *i32Ty() { return Type::getInt32Ty(ShortGlobalContext); }
static Type *i64Ty() { return Type::getInt64Ty(ShortGlobalContext); }
static Type *i1Ty() { return Type::getInt1Ty(ShortGlobalContext); }
static Type *f64Ty() { return Type::getDoubleTy(ShortGlobalContext); }
static Type *voidTy() { return Type::getVoidTy(ShortGlobalContext); }

static PointerType *i8PtrTy() {
#if LLVM_VERSION_MAJOR >= 15
    return PointerType::get(ShortGlobalContext, 0);
#else
    return PointerType::get(Type::getInt8Ty(ShortGlobalContext), 0);
#endif
}

static Value *typedLoad(Value *ptr, Type *ty, const Twine &name = "loadtmp") {
    return Builder.CreateLoad(ty, ptr, name);
}

static Value *asI32(Value *value) {
    if (!value) return value;
    if (value->getType()->isIntegerTy(32)) return value;
    if (value->getType()->isIntegerTy(1)) return Builder.CreateZExt(value, i32Ty(), "booltoint");
    if (value->getType()->isDoubleTy() || value->getType()->isPointerTy())
        return ShowError("implicit conversion to int32 reached lowering after semantic validation");
    return Builder.CreateIntCast(value, i32Ty(), true, "toint32");
}

static Value *asBool(Value *value) {
    if (!value) return value;
    if (value->getType()->isIntegerTy(1)) return value;
    if (value->getType()->isDoubleTy() || value->getType()->isPointerTy())
        return ShowError("non-condition type reached lowering after semantic validation");
    return Builder.CreateICmpNE(asI32(value), ConstantInt::get(i32Ty(), 0, true), "tobool");
}

static Value *asExactLoweredType(Value *value, ShortType expected) {
    if (!value) return value;
    if (expected == ShortType::Boolean) {
        if (value->getType()->isIntegerTy(1))
            return Builder.CreateZExt(value, i32Ty(), "bool_storage");
        if (value->getType()->isIntegerTy(32)) return value;
    } else if (expected == ShortType::Int && value->getType()->isIntegerTy(32)) {
        return value;
    } else if (expected == ShortType::Float && value->getType()->isDoubleTy()) {
        return value;
    } else if (expected == ShortType::String && value->getType()->isPointerTy()) {
        return value;
    }
    return ShowError("exact type mismatch reached lowering after semantic validation");
}

static AllocaInst *createEntryBlockAlloca(Function *function, Type *type, const std::string &name) {
    IRBuilder<> tmp(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmp.CreateAlloca(type, nullptr, name);
}

static std::string stripQuotes(std::string value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        return value.substr(1, value.length() - 2);
    return value;
}

static std::string safeMetadataName(const std::string &value) {
    std::string out;
    for (char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) out += c;
        else out += '_';
    }
    if (out.empty()) out = "unnamed";
    return out;
}

static std::string joinStrings(const std::vector<std::string> &values, const std::string &sep) {
    std::string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out += sep;
        out += values[i];
    }
    return out;
}

static void emitShortHandMetadata(const std::string &kind, const std::string &name, const std::string &payload) {
    if (!module) return;
    std::string text = "shorthand." + kind + "|" + payload;
    Constant *data = ConstantDataArray::getString(ShortGlobalContext, text, true);
    std::string global_name = "shorthand_ai_" + safeMetadataName(kind) + "_" + safeMetadataName(name) + "_" + std::to_string(++ShortHandMetadataCounter);
    GlobalVariable *gv = new GlobalVariable(*module, data->getType(), true, GlobalValue::PrivateLinkage, data, global_name);
    gv->setSection("shorthand_ai_metadata");
}

static Function *ensureShortHandRuntimeHook(const std::string &name, size_t argc) {
    if (!module) return nullptr;
    if (Function *existing = module->getFunction(name)) return existing;
    std::vector<Type*> args(argc, i8PtrTy());
    FunctionType *ftype = FunctionType::get(i32Ty(), args, false);
    return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);
}

static void emitShortHandRuntimeCall(const std::string &hook, const std::vector<std::string> &args) {
    if (!module || !Builder.GetInsertBlock()) return;
    Function *fn = ensureShortHandRuntimeHook(hook, args.size());
    if (!fn) return;
    std::vector<Value*> llvmArgs;
    for (size_t i = 0; i < args.size(); ++i)
        llvmArgs.push_back(Builder.CreateGlobalStringPtr(args[i], safeMetadataName(hook) + "_arg" + std::to_string(i)));
    Builder.CreateCall(fn, llvmArgs, safeMetadataName(hook) + "_call");
}

Value *ShowError(const char *str) {
    cerr << "Error:\n" << str << "\n";
    return nullptr;
}

Value *ShowError(string str) {
    cerr << "Error:\n" << str << "\n";
    return nullptr;
}

static bool fileExists(const std::string &path) {
    std::ifstream in(path.c_str());
    return in.good();
}

static std::string shellQuote(const std::string &value) {
#if defined(_WIN32)
    if (value.find_first_of(" \t\"&|<>^") == std::string::npos) return value;
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
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

static std::string resolveShortHandNativeLinker() {
    const char *env = std::getenv("SHORTHAND_NATIVE_LINKER");
    if (env && *env) {
        std::string linker(env);
#if defined(_WIN32)
        if (linker.find_first_of(" \t\"") != std::string::npos) {
            cerr << "SHORTHAND_NATIVE_LINKER on Windows must be a PATH-resolved command token without whitespace.\n";
            return "";
        }
#endif
        return linker;
    }
    return "clang++";
}

IR_Generator::IR_Generator() {
    module = new Module("short_hand_module", ShortGlobalContext);
    FunctionType *printfType = FunctionType::get(i32Ty(), {i8PtrTy()}, true);
    FunctionType *scanfType = FunctionType::get(i32Ty(), {i8PtrTy()}, true);
    CalleeF = module->getOrInsertFunction("printf", printfType);
    CalleeR = module->getOrInsertFunction("scanf", scanfType);
    load_variable = 0;
    is_condition = 0;
    is_expression = 0;
    ret = nullptr;
    main_function = nullptr;
    ret_type = ShortType::Int;
    load_type = ShortType::Int;
    active_function_return_type = ShortType::Void;
}

IR_Generator::~IR_Generator() {
    delete module;
    module = nullptr;
}

void IR_Generator::setModuleName(std::string mod_name) {
    this->module_name = mod_name;
    if (module) {
        module->setModuleIdentifier(mod_name);
        module->setSourceFileName(mod_name + ".short");
    }
}

void IR_Generator::dump() {
    if (verifyModule(*module, &errs()))
        cerr << "LLVM module verification failed. IR was still written for inspection.\n";
    cerr << "Generating LLVM IR Code\n\n";
    std::string Str;
    raw_string_ostream OS(Str);
    OS << *module;
    OS.flush();
    cerr << Str;
    std::ofstream out((this->getModuleName() + ".ir").c_str());
    out << Str;
}

bool IR_Generator::dumpBitcode() {
    if (verifyModule(*module, &errs())) {
        cerr << "Refusing to write invalid LLVM bitcode.\n";
        return false;
    }
    std::string bc_file = this->getModuleName() + ".bc";
    std::error_code ec;
    raw_fd_ostream bc_out(bc_file, ec, sys::fs::OF_None);
    if (ec) {
        cerr << "Failed to create bitcode file: " << bc_file << "\n";
        return false;
    }
    WriteBitcodeToFile(*module, bc_out);
    bc_out.flush();
    cerr << "Generated LLVM bitcode: " << bc_file << "\n";
    return true;
}

bool IR_Generator::dumpNativeBinary() {
    if (!dumpBitcode()) return false;
    const std::string base = this->getModuleName();
    const std::string object_file = base + ".o";
#if defined(_WIN32)
    const std::string binary_file = base + ".exe";
#else
    const std::string binary_file = base;
#endif
    std::string cmd_obj = "llc -filetype=obj " + shellQuote(base + ".bc") + " -o " + shellQuote(object_file);
    std::string runtime_lib = resolveShortHandRuntimeLibrary();
    std::string native_linker = resolveShortHandNativeLinker();
    if (native_linker.empty()) return false;
    std::string cmd_bin = shellQuote(native_linker);
#if !defined(_WIN32)
    cmd_bin += " -no-pie";
#endif
    cmd_bin += " " + shellQuote(object_file);
    if (!runtime_lib.empty()) {
        cmd_bin += " " + shellQuote(runtime_lib);
    } else {
        cerr << "No ShortHand runtime library found. Native linking will fail if AI/GreenAI runtime hooks are referenced.\n";
    }
    cmd_bin += " -o " + shellQuote(binary_file);

    if (std::system(cmd_obj.c_str()) != 0) {
        cerr << "Failed to run llc. Ensure LLVM tools are installed and in PATH.\n";
        return false;
    }
    if (std::system(cmd_bin.c_str()) != 0) {
        cerr << "Failed to run C++ linker step. Build libshorthand_runtime.a or set SHORTHAND_RUNTIME_LIB for AI/GreenAI programs.\n";
        return false;
    }
    if (!runtime_lib.empty()) cerr << "Linked ShortHand runtime library: " << runtime_lib << "\n";
    cerr << "Native linker: " << native_linker << "\n";
    cerr << "Generated native binary: " << binary_file << "\n";
    return true;
}

void IR_Generator::emitRuntimeFailureIf(Value *condition,
                                        const std::string &code,
                                        const std::string &message) {
    if (!condition || !Builder.GetInsertBlock()) return;
    Function *function = Builder.GetInsertBlock()->getParent();
    BasicBlock *fail = BasicBlock::Create(ShortGlobalContext, "runtime_fail", function);
    BasicBlock *cont = BasicBlock::Create(ShortGlobalContext, "runtime_continue", function);
    Builder.CreateCondBr(asBool(condition), fail, cont);

    Builder.SetInsertPoint(fail);
    const std::string text = "error: [" + code + "] " + message + "\n";
    Value *textValue = Builder.CreateGlobalStringPtr(text, "runtime_error_text");
#if defined(_WIN32)
    FunctionType *writeType = FunctionType::get(i32Ty(), {i32Ty(), i8PtrTy(), i32Ty()}, false);
    FunctionCallee writeFn = module->getOrInsertFunction("_write", writeType);
    Builder.CreateCall(writeFn, {
        ConstantInt::get(i32Ty(), 2), textValue,
        ConstantInt::get(i32Ty(), static_cast<std::uint32_t>(text.size()))});
#else
    FunctionType *writeType = FunctionType::get(i64Ty(), {i32Ty(), i8PtrTy(), i64Ty()}, false);
    FunctionCallee writeFn = module->getOrInsertFunction("write", writeType);
    Builder.CreateCall(writeFn, {
        ConstantInt::get(i32Ty(), 2), textValue,
        ConstantInt::get(i64Ty(), static_cast<std::uint64_t>(text.size()))});
#endif
    FunctionType *exitType = FunctionType::get(voidTy(), {i32Ty()}, false);
    FunctionCallee exitFn = module->getOrInsertFunction("exit", exitType);
    Builder.CreateCall(exitFn, {ConstantInt::get(i32Ty(), 1)});
    Builder.CreateUnreachable();

    Builder.SetInsertPoint(cont);
}

ShortType IR_Generator::variableType(const std::string &name, bool array) const {
    for (auto it = local_type_scopes.rbegin(); it != local_type_scopes.rend(); ++it) {
        const auto local = it->find(name);
        if (local != it->end() && local->second.is_array == array) return local->second.type;
    }
    if (!array) {
        const auto global = global_scalar_types.find(name);
        if (global != global_scalar_types.end()) return global->second;
    } else {
        const auto found = global_array_types.find(name);
        if (found != global_array_types.end()) return found->second;
    }
    return ShortType::Int;
}

Constant *IR_Generator::zeroValue(ShortType type) {
    if (type == ShortType::Float) return ConstantFP::get(f64Ty(), 0.0);
    if (type == ShortType::String) return ConstantPointerNull::get(i8PtrTy());
    return ConstantInt::get(i32Ty(), 0, true);
}

Value *IR_Generator::safeStringPointer(Value *value) {
    Value *empty = Builder.CreateGlobalStringPtr("");
    if (!value) return empty;
    Value *is_null = Builder.CreateICmpEQ(
        value, ConstantPointerNull::get(i8PtrTy()), "string_is_null");
    return Builder.CreateSelect(is_null, empty, value, "safe_string");
}

Value *IR_Generator::get_expression() {
    Value *v = ret;
    if (load_variable) {
        if(load_tensor) {
            v=Builder.CreateFPExt(typedLoad(v,Type::getFloatTy(ShortGlobalContext)),f64Ty());
            load_tensor=false;
        } else v = typedLoad(v, parseType(load_type));
        ret_type = load_type;
        load_variable = 0;
    }
    is_condition = 0;
    is_expression = 1;
    return v;
}

Value *IR_Generator::get_condition() {
    Value *v = ret;
    if (load_variable) {
        v = typedLoad(v, parseType(load_type));
        ret_type = load_type;
        load_variable = 0;
    }
    v = asBool(v);
    is_condition = 1;
    is_expression = 0;
    return v;
}

IR_Generator::StaticIntInterval IR_Generator::evaluateStaticIntInterval(AST_EXPRESSION_RULE *expr) const {
    if (!expr) return {};
    if (auto *lit = dynamic_cast<AST_LITERAL *>(expr)) {
        return {true, static_cast<std::int64_t>(lit->int_literal), static_cast<std::int64_t>(lit->int_literal)};
    }
    if (auto *b = dynamic_cast<AST_BOOL_LITERAL *>(expr)) {
        const std::int64_t val = b->value ? 1 : 0;
        return {true, val, val};
    }
    if (auto *var = dynamic_cast<AST_SIMPLE_VARIABLE *>(expr)) {
        if (variableType(var->variable_name, false) != ShortType::Int &&
            variableType(var->variable_name, false) != ShortType::Boolean) {
            return {};
        }
        const auto found = static_int_intervals.find(var->variable_name);
        if (found != static_int_intervals.end()) return found->second;
        return {};
    }
    if (auto *un = dynamic_cast<AST_UNARY_EXPRESSION_RULE *>(expr)) {
        if (un->op == UMINUS) {
            const auto sub = evaluateStaticIntInterval(un->expression);
            if (sub.known && sub.lo > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())) {
                return {true, -sub.hi, -sub.lo};
            }
        }
        return {};
    }
    if (auto *bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(expr)) {
        const auto L = evaluateStaticIntInterval(bin->left);
        const auto R = evaluateStaticIntInterval(bin->right);
        if (!L.known || !R.known) return {};
        constexpr std::int64_t kMin = std::numeric_limits<std::int32_t>::min();
        constexpr std::int64_t kMax = std::numeric_limits<std::int32_t>::max();
        if (bin->op == PLUS) {
            const std::int64_t lo = L.lo + R.lo;
            const std::int64_t hi = L.hi + R.hi;
            if (lo >= kMin && hi <= kMax) return {true, lo, hi};
        } else if (bin->op == MINUS) {
            const std::int64_t lo = L.lo - R.hi;
            const std::int64_t hi = L.hi - R.lo;
            if (lo >= kMin && hi <= kMax) return {true, lo, hi};
        } else if (bin->op == MULTIPLY) {
            if (L.lo >= -65536 && L.hi <= 65536 && R.lo >= -65536 && R.hi <= 65536) {
                const std::int64_t p1 = L.lo * R.lo;
                const std::int64_t p2 = L.lo * R.hi;
                const std::int64_t p3 = L.hi * R.lo;
                const std::int64_t p4 = L.hi * R.hi;
                const std::int64_t lo = std::min(std::min(p1, p2), std::min(p3, p4));
                const std::int64_t hi = std::max(std::max(p1, p2), std::max(p3, p4));
                if (lo >= kMin && hi <= kMax) return {true, lo, hi};
            }
        }
        return {};
    }
    return {};
}

bool IR_Generator::expressionContainsCall(AST_EXPRESSION_RULE *expr) const {
    if (!expr) return false;
    if (dynamic_cast<AST_FUNCTION_CALL_EXPRESSION *>(expr) != nullptr) return true;
    if (auto *un = dynamic_cast<AST_UNARY_EXPRESSION_RULE *>(expr)) {
        return expressionContainsCall(un->expression);
    }
    if (auto *bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(expr)) {
        return expressionContainsCall(bin->left) || expressionContainsCall(bin->right);
    }
    if (auto *arr = dynamic_cast<AST_ARRAY_VARIABLE *>(expr)) {
        return expressionContainsCall(arr->index);
    }
    return false;
}

bool IR_Generator::expressionReferencesVar(AST_EXPRESSION_RULE *expr, const std::string &var_name) const {
    if (!expr) return false;
    if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(expr)) {
        return sv->variable_name == var_name;
    }
    if (auto *av = dynamic_cast<AST_ARRAY_VARIABLE *>(expr)) {
        return av->array_name == var_name || expressionReferencesVar(av->index, var_name);
    }
    if (auto *un = dynamic_cast<AST_UNARY_EXPRESSION_RULE *>(expr)) {
        return expressionReferencesVar(un->expression, var_name);
    }
    if (auto *bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(expr)) {
        return expressionReferencesVar(bin->left, var_name) || expressionReferencesVar(bin->right, var_name);
    }
    if (auto *call = dynamic_cast<AST_FUNCTION_CALL_EXPRESSION *>(expr)) {
        for (auto *arg : call->arguments) {
            if (expressionReferencesVar(arg, var_name)) return true;
        }
    }
    return false;
}

void IR_Generator::collectModifiedScalars(AST_STATEMENT_RULE *stmt,
                                          std::vector<std::string> &modified,
                                          bool &has_unknown_effects) const {
    if (!stmt) return;
    if (auto *blk = dynamic_cast<AST_STATEMENTS_BLOCK *>(stmt)) {
        for (auto *child : blk->statements) {
            collectModifiedScalars(child, modified, has_unknown_effects);
        }
        return;
    }
    if (auto *assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(stmt)) {
        if (expressionContainsCall(assign->expression)) has_unknown_effects = true;
        if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(assign->variable)) {
            modified.push_back(sv->variable_name);
        } else if (auto *av = dynamic_cast<AST_ARRAY_VARIABLE *>(assign->variable)) {
            if (expressionContainsCall(av->index)) has_unknown_effects = true;
        }
        return;
    }
    if (auto *read_stmt = dynamic_cast<AST_READ_RULE *>(stmt)) {
        for (auto *v : read_stmt->variables) {
            if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(v)) {
                modified.push_back(sv->variable_name);
            }
        }
        return;
    }
    if (auto *if_stmt = dynamic_cast<AST_IF_STATEMENT *>(stmt)) {
        if (expressionContainsCall(if_stmt->condition)) has_unknown_effects = true;
        collectModifiedScalars(if_stmt->if_block, modified, has_unknown_effects);
        return;
    }
    if (auto *ifelse_stmt = dynamic_cast<AST_IF_ELSE_STATEMENT *>(stmt)) {
        if (expressionContainsCall(ifelse_stmt->condition)) has_unknown_effects = true;
        collectModifiedScalars(ifelse_stmt->if_block, modified, has_unknown_effects);
        collectModifiedScalars(ifelse_stmt->else_block, modified, has_unknown_effects);
        return;
    }
    if (auto *for_stmt = dynamic_cast<AST_FOR_LOOP_STATEMENT_RULE *>(stmt)) {
        if (expressionContainsCall(for_stmt->from) ||
            expressionContainsCall(for_stmt->step) ||
            expressionContainsCall(for_stmt->to)) {
            has_unknown_effects = true;
        }
        if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(for_stmt->variable)) {
            modified.push_back(sv->variable_name);
        }
        collectModifiedScalars(for_stmt->for_block, modified, has_unknown_effects);
        return;
    }
    if (auto *while_stmt = dynamic_cast<AST_WHILE_LOOP_STATEMENT_RULE *>(stmt)) {
        if (expressionContainsCall(while_stmt->condition)) has_unknown_effects = true;
        collectModifiedScalars(while_stmt->while_block, modified, has_unknown_effects);
        return;
    }
    if (dynamic_cast<AST_FUNCTION_CALL_RULE *>(stmt) != nullptr ||
        dynamic_cast<AST_GOTO_STATEMENT_RULE *>(stmt) != nullptr ||
        dynamic_cast<AST_LABEL_RULE *>(stmt) != nullptr) {
        has_unknown_effects = true;
        return;
    }
    if (auto *expr_stmt = dynamic_cast<AST_EXPRESSION_STATEMENT_RULE *>(stmt)) {
        if (expressionContainsCall(expr_stmt->expression)) has_unknown_effects = true;
        return;
    }
    if (auto *print_stmt = dynamic_cast<AST_PRINT_RULE *>(stmt)) {
        for (const auto &item : print_stmt->printables) {
            if (item.expression && expressionContainsCall(item.expression)) {
                has_unknown_effects = true;
            }
        }
        return;
    }
}

void IR_Generator::refineIntervalFromCondition(AST_EXPRESSION_RULE *cond, bool branch_taken) {
    auto *bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(cond);
    if (!bin) return;
    auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(bin->left);
    if (!sv || variableType(sv->variable_name, false) != ShortType::Int) return;
    const auto rhs = evaluateStaticIntInterval(bin->right);
    if (!rhs.known || rhs.lo != rhs.hi) return;
    const std::int64_t c = rhs.lo;
    constexpr std::int64_t kMin = std::numeric_limits<std::int32_t>::min();
    constexpr std::int64_t kMax = std::numeric_limits<std::int32_t>::max();
    StaticIntInterval cur = {true, kMin, kMax};
    const auto found = static_int_intervals.find(sv->variable_name);
    if (found != static_int_intervals.end() && found->second.known) {
        cur = found->second;
    }
    int effective_op = bin->op;
    if (!branch_taken) {
        switch (bin->op) {
            case LESS: effective_op = GREATER_OR_EQUAL; break;
            case LESS_OR_EQUAL: effective_op = GREATER; break;
            case GREATER: effective_op = LESS_OR_EQUAL; break;
            case GREATER_OR_EQUAL: effective_op = LESS; break;
            case EQUAL: return;
            case NOT_EQUAL: effective_op = EQUAL; break;
            default: return;
        }
    }
    switch (effective_op) {
        case LESS:
            if (c > kMin) cur.hi = std::min(cur.hi, c - 1);
            break;
        case LESS_OR_EQUAL:
            cur.hi = std::min(cur.hi, c);
            break;
        case GREATER:
            if (c < kMax) cur.lo = std::max(cur.lo, c + 1);
            break;
        case GREATER_OR_EQUAL:
            cur.lo = std::max(cur.lo, c);
            break;
        case EQUAL:
            cur.lo = std::max(cur.lo, c);
            cur.hi = std::min(cur.hi, c);
            break;
        default:
            return;
    }
    if (cur.lo <= cur.hi) {
        static_int_intervals[sv->variable_name] = cur;
    }
}

void IR_Generator::promoteConstantGlobalArrays() {
    if (!module || !main_function || main_function->empty()) return;
    BasicBlock &entry_bb = main_function->getEntryBlock();
    for (GlobalVariable &gv : module->globals()) {
        if (!gv.hasInternalLinkage() || gv.isConstant() || !gv.hasInitializer()) continue;
        if (!isa<ConstantAggregateZero>(gv.getInitializer())) continue;
        auto *arr_ty = dyn_cast<ArrayType>(gv.getValueType());
        if (!arr_ty) continue;
        const std::string name = gv.getName().str();
        if (name.rfind("__sh.", 0) == 0 || name.rfind("shorthand_ai_", 0) == 0) continue;

        bool can_promote = true;
        std::set<Instruction *> entry_stores_set;
        std::vector<GetElementPtrInst *> dead_geps;

        for (User *u : gv.users()) {
            if (auto *gep = dyn_cast<GetElementPtrInst>(u)) {
                if (gep->getNumIndices() != 2) { can_promote = false; break; }
                auto idx_it = gep->idx_begin();
                auto *c0 = dyn_cast<ConstantInt>(idx_it->get());
                ++idx_it;
                auto *c1 = dyn_cast<ConstantInt>(idx_it->get());
                for (User *gu : gep->users()) {
                    if (isa<LoadInst>(gu)) continue;
                    if (auto *st = dyn_cast<StoreInst>(gu)) {
                        if (st->getPointerOperand() == gep &&
                            st->getValueOperand() != gep &&
                            st->getParent() == &entry_bb &&
                            c0 && c0->isZero() && c1 &&
                            c1->getSExtValue() >= 0 &&
                            static_cast<std::uint64_t>(c1->getSExtValue()) < arr_ty->getNumElements() &&
                            isa<Constant>(st->getValueOperand())) {
                            entry_stores_set.insert(st);
                            continue;
                        }
                    }
                    can_promote = false;
                    break;
                }
            } else if (auto *ce = dyn_cast<ConstantExpr>(u)) {
                if (ce->getOpcode() != Instruction::GetElementPtr || ce->getNumOperands() != 3) {
                    can_promote = false;
                    break;
                }
                auto *c0 = dyn_cast<ConstantInt>(ce->getOperand(1));
                auto *c1 = dyn_cast<ConstantInt>(ce->getOperand(2));
                for (User *cu : ce->users()) {
                    if (isa<LoadInst>(cu)) continue;
                    if (auto *st = dyn_cast<StoreInst>(cu)) {
                        if (st->getPointerOperand() == ce &&
                            st->getValueOperand() != ce &&
                            st->getParent() == &entry_bb &&
                            c0 && c0->isZero() && c1 &&
                            c1->getSExtValue() >= 0 &&
                            static_cast<std::uint64_t>(c1->getSExtValue()) < arr_ty->getNumElements() &&
                            isa<Constant>(st->getValueOperand())) {
                            entry_stores_set.insert(st);
                            continue;
                        }
                    }
                    can_promote = false;
                    break;
                }
            } else {
                can_promote = false;
                break;
            }
            if (!can_promote) break;
        }

        if (!can_promote || entry_stores_set.empty()) continue;

        // Verify that no load from gv occurs in entry_bb before the last store to gv.
        std::size_t seen_stores = 0;
        for (Instruction &inst : entry_bb) {
            if (entry_stores_set.count(&inst)) {
                ++seen_stores;
            } else if (auto *ld = dyn_cast<LoadInst>(&inst)) {
                Value *ptr = ld->getPointerOperand()->stripInBoundsConstantOffsets();
                if (ptr == &gv && seen_stores < entry_stores_set.size()) {
                    can_promote = false;
                    break;
                }
            } else if (auto *call = dyn_cast<CallInst>(&inst)) {
                if (seen_stores < entry_stores_set.size()) {
                    Function *callee = call->getCalledFunction();
                    if (!callee || !callee->isDeclaration()) {
                        can_promote = false;
                        break;
                    }
                }
            }
        }
        if (!can_promote) continue;

        Type *elem_ty = arr_ty->getElementType();
        std::vector<Constant *> elems(arr_ty->getNumElements(), Constant::getNullValue(elem_ty));
        std::vector<StoreInst *> stores_to_erase;
        for (Instruction &inst : entry_bb) {
            if (!entry_stores_set.count(&inst)) continue;
            auto *st = cast<StoreInst>(&inst);
            Value *ptr = st->getPointerOperand();
            std::int64_t idx = -1;
            if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
                auto it = gep->idx_begin();
                ++it;
                idx = cast<ConstantInt>(it->get())->getSExtValue();
            } else if (auto *ce = dyn_cast<ConstantExpr>(ptr)) {
                idx = cast<ConstantInt>(ce->getOperand(2))->getSExtValue();
            }
            if (idx >= 0 && static_cast<std::uint64_t>(idx) < elems.size()) {
                elems[static_cast<std::size_t>(idx)] = cast<Constant>(st->getValueOperand());
                stores_to_erase.push_back(st);
            }
        }

        gv.setInitializer(ConstantArray::get(arr_ty, elems));
        gv.setConstant(true);
        gv.setUnnamedAddr(GlobalValue::UnnamedAddr::Local);

        for (StoreInst *st : stores_to_erase) {
            Value *ptr = st->getPointerOperand();
            st->eraseFromParent();
            if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
                if (gep->use_empty()) gep->eraseFromParent();
            }
        }
    }
}

Function *IR_Generator::ensureFastReadI32() {
    if (!module) return nullptr;
    if (Function *existing = module->getFunction("__sh_read_i32")) return existing;

    FunctionType *fn_type = FunctionType::get(i32Ty(), {i32Ty()}, false);
    Function *reader_fn = Function::Create(fn_type, GlobalValue::InternalLinkage, "__sh_read_i32", module);
    Argument *def_arg = reader_fn->getArg(0);
    def_arg->setName("default_val");

    FunctionType *getchar_type = FunctionType::get(i32Ty(), {}, false);
    FunctionCallee getchar_fn = module->getOrInsertFunction("getchar_unlocked", getchar_type);

    IRBuilder<> b(ShortGlobalContext);
    BasicBlock *entry_bb = BasicBlock::Create(ShortGlobalContext, "entry", reader_fn);
    BasicBlock *ws_loop_bb = BasicBlock::Create(ShortGlobalContext, "ws_loop", reader_fn);
    BasicBlock *ws_check_bb = BasicBlock::Create(ShortGlobalContext, "ws_check", reader_fn);
    BasicBlock *eof_bb = BasicBlock::Create(ShortGlobalContext, "eof", reader_fn);
    BasicBlock *sign_check_bb = BasicBlock::Create(ShortGlobalContext, "sign_check", reader_fn);
    BasicBlock *sign_adv_bb = BasicBlock::Create(ShortGlobalContext, "sign_adv", reader_fn);
    BasicBlock *digit_loop_bb = BasicBlock::Create(ShortGlobalContext, "digit_loop", reader_fn);
    BasicBlock *digit_body_bb = BasicBlock::Create(ShortGlobalContext, "digit_body", reader_fn);
    BasicBlock *done_bb = BasicBlock::Create(ShortGlobalContext, "done", reader_fn);

    b.SetInsertPoint(entry_bb);
    AllocaInst *c_slot = b.CreateAlloca(i32Ty(), nullptr, "c_slot");
    AllocaInst *neg_slot = b.CreateAlloca(i1Ty(), nullptr, "neg_slot");
    AllocaInst *acc_slot = b.CreateAlloca(i32Ty(), nullptr, "acc_slot");
    AllocaInst *cnt_slot = b.CreateAlloca(i32Ty(), nullptr, "cnt_slot");
    b.CreateStore(ConstantInt::getFalse(ShortGlobalContext), neg_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), acc_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), cnt_slot);
    b.CreateBr(ws_loop_bb);

    b.SetInsertPoint(ws_loop_bb);
    Value *ch_ws = b.CreateCall(getchar_fn, {}, "ch_ws");
    b.CreateStore(ch_ws, c_slot);
    Value *is_eof = b.CreateICmpSLT(ch_ws, ConstantInt::get(i32Ty(), 0, true), "is_eof");
    b.CreateCondBr(is_eof, eof_bb, ws_check_bb);

    b.SetInsertPoint(eof_bb);
    b.CreateRet(def_arg);

    b.SetInsertPoint(ws_check_bb);
    Value *ch_chk = b.CreateLoad(i32Ty(), c_slot, "ch_chk");
    Value *is_ws = b.CreateICmpSLE(ch_chk, ConstantInt::get(i32Ty(), 32, true), "is_ws");
    b.CreateCondBr(is_ws, ws_loop_bb, sign_check_bb);

    b.SetInsertPoint(sign_check_bb);
    Value *ch_sgn = b.CreateLoad(i32Ty(), c_slot, "ch_sgn");
    Value *is_minus = b.CreateICmpEQ(ch_sgn, ConstantInt::get(i32Ty(), 45, true), "is_minus");
    Value *is_plus = b.CreateICmpEQ(ch_sgn, ConstantInt::get(i32Ty(), 43, true), "is_plus");
    b.CreateStore(is_minus, neg_slot);
    Value *has_sign = b.CreateOr(is_minus, is_plus, "has_sign");
    b.CreateCondBr(has_sign, sign_adv_bb, digit_loop_bb);

    b.SetInsertPoint(sign_adv_bb);
    Value *ch_after = b.CreateCall(getchar_fn, {}, "ch_after");
    b.CreateStore(ch_after, c_slot);
    b.CreateBr(digit_loop_bb);

    b.SetInsertPoint(digit_loop_bb);
    Value *ch_cur = b.CreateLoad(i32Ty(), c_slot, "ch_cur");
    Value *dig = b.CreateSub(ch_cur, ConstantInt::get(i32Ty(), 48, true), "dig");
    Value *is_dig = b.CreateICmpULT(dig, ConstantInt::get(i32Ty(), 10, false), "is_dig");
    b.CreateCondBr(is_dig, digit_body_bb, done_bb);

    b.SetInsertPoint(digit_body_bb);
    Value *ch_body = b.CreateLoad(i32Ty(), c_slot, "ch_body");
    Value *dig_val = b.CreateSub(ch_body, ConstantInt::get(i32Ty(), 48, true), "dig_val");
    Value *acc_cur = b.CreateLoad(i32Ty(), acc_slot, "acc_cur");
    Value *acc_next = b.CreateAdd(b.CreateMul(acc_cur, ConstantInt::get(i32Ty(), 10, true)), dig_val, "acc_next");
    b.CreateStore(acc_next, acc_slot);
    Value *cnt_cur = b.CreateLoad(i32Ty(), cnt_slot, "cnt_cur");
    b.CreateStore(b.CreateAdd(cnt_cur, ConstantInt::get(i32Ty(), 1, true)), cnt_slot);
    Value *ch_next = b.CreateCall(getchar_fn, {}, "ch_next");
    b.CreateStore(ch_next, c_slot);
    b.CreateBr(digit_loop_bb);

    b.SetInsertPoint(done_bb);
    Value *cnt_fin = b.CreateLoad(i32Ty(), cnt_slot, "cnt_fin");
    Value *has_digs = b.CreateICmpSGT(cnt_fin, ConstantInt::get(i32Ty(), 0, true), "has_digs");
    Value *acc_fin = b.CreateLoad(i32Ty(), acc_slot, "acc_fin");
    Value *is_neg_fin = b.CreateLoad(i1Ty(), neg_slot, "is_neg_fin");
    Value *neg_acc = b.CreateSub(ConstantInt::get(i32Ty(), 0, true), acc_fin, "neg_acc");
    Value *signed_val = b.CreateSelect(is_neg_fin, neg_acc, acc_fin, "signed_val");
    Value *res_val = b.CreateSelect(has_digs, signed_val, def_arg, "res_val");
    b.CreateRet(res_val);

    return reader_fn;
}

Function *IR_Generator::ensureFastReadF64() {
    if (!module) return nullptr;
    if (Function *existing = module->getFunction("__sh_read_f64")) return existing;

    ArrayType *pow10_arr_ty = ArrayType::get(f64Ty(), 10);
    GlobalVariable *pow10_gv = module->getNamedGlobal("__sh_pow10_table");
    if (!pow10_gv) {
        std::vector<Constant *> pow10_vals = {
            ConstantFP::get(f64Ty(), 1.0),
            ConstantFP::get(f64Ty(), 10.0),
            ConstantFP::get(f64Ty(), 100.0),
            ConstantFP::get(f64Ty(), 1000.0),
            ConstantFP::get(f64Ty(), 10000.0),
            ConstantFP::get(f64Ty(), 100000.0),
            ConstantFP::get(f64Ty(), 1000000.0),
            ConstantFP::get(f64Ty(), 10000000.0),
            ConstantFP::get(f64Ty(), 100000000.0),
            ConstantFP::get(f64Ty(), 1000000000.0)
        };
        pow10_gv = new GlobalVariable(
            *module, pow10_arr_ty, true, GlobalValue::PrivateLinkage,
            ConstantArray::get(pow10_arr_ty, pow10_vals), "__sh_pow10_table");
        pow10_gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        pow10_gv->setAlignment(MaybeAlign(64));
    }

    FunctionType *fn_type = FunctionType::get(f64Ty(), {f64Ty()}, false);
    Function *reader_fn = Function::Create(fn_type, GlobalValue::InternalLinkage, "__sh_read_f64", module);
    Argument *def_arg = reader_fn->getArg(0);
    def_arg->setName("default_val");

    FunctionType *getchar_type = FunctionType::get(i32Ty(), {}, false);
    FunctionCallee getchar_fn = module->getOrInsertFunction("getchar_unlocked", getchar_type);
    FunctionType *strtod_type = FunctionType::get(f64Ty(), {i8PtrTy(), i8PtrTy()}, false);
    FunctionCallee strtod_fn = module->getOrInsertFunction("strtod", strtod_type);

    Type *i8_ty = Type::getInt8Ty(ShortGlobalContext);
    ArrayType *buf_arr_ty = ArrayType::get(i8_ty, 64);

    IRBuilder<> b(ShortGlobalContext);
    BasicBlock *entry_bb = BasicBlock::Create(ShortGlobalContext, "entry", reader_fn);
    BasicBlock *ws_loop_bb = BasicBlock::Create(ShortGlobalContext, "ws_loop", reader_fn);
    BasicBlock *ws_check_bb = BasicBlock::Create(ShortGlobalContext, "ws_check", reader_fn);
    BasicBlock *eof_bb = BasicBlock::Create(ShortGlobalContext, "eof", reader_fn);
    BasicBlock *sign_check_bb = BasicBlock::Create(ShortGlobalContext, "sign_check", reader_fn);
    BasicBlock *sign_adv_bb = BasicBlock::Create(ShortGlobalContext, "sign_adv", reader_fn);
    BasicBlock *parse_loop_bb = BasicBlock::Create(ShortGlobalContext, "parse_loop", reader_fn);
    BasicBlock *buf_store_bb = BasicBlock::Create(ShortGlobalContext, "buf_store", reader_fn);
    BasicBlock *buf_overflow_bb = BasicBlock::Create(ShortGlobalContext, "buf_overflow", reader_fn);
    BasicBlock *char_dispatch_bb = BasicBlock::Create(ShortGlobalContext, "char_dispatch", reader_fn);
    BasicBlock *digit_bb = BasicBlock::Create(ShortGlobalContext, "digit_bb", reader_fn);
    BasicBlock *frac_inc_bb = BasicBlock::Create(ShortGlobalContext, "frac_inc", reader_fn);
    BasicBlock *non_digit_bb = BasicBlock::Create(ShortGlobalContext, "non_digit", reader_fn);
    BasicBlock *dot_bb = BasicBlock::Create(ShortGlobalContext, "dot_bb", reader_fn);
    BasicBlock *other_char_bb = BasicBlock::Create(ShortGlobalContext, "other_char", reader_fn);
    BasicBlock *next_char_bb = BasicBlock::Create(ShortGlobalContext, "next_char", reader_fn);
    BasicBlock *finish_check_bb = BasicBlock::Create(ShortGlobalContext, "finish_check", reader_fn);
    BasicBlock *fast_path_bb = BasicBlock::Create(ShortGlobalContext, "fast_path", reader_fn);
    BasicBlock *strtod_bb = BasicBlock::Create(ShortGlobalContext, "strtod_fallback", reader_fn);

    b.SetInsertPoint(entry_bb);
    AllocaInst *c_slot = b.CreateAlloca(i32Ty(), nullptr, "c_slot");
    AllocaInst *neg_slot = b.CreateAlloca(i1Ty(), nullptr, "neg_slot");
    AllocaInst *mant_slot = b.CreateAlloca(i64Ty(), nullptr, "mant_slot");
    AllocaInst *sig_slot = b.CreateAlloca(i32Ty(), nullptr, "sig_slot");
    AllocaInst *tot_slot = b.CreateAlloca(i32Ty(), nullptr, "tot_slot");
    AllocaInst *frac_slot = b.CreateAlloca(i32Ty(), nullptr, "frac_slot");
    AllocaInst *in_frac_slot = b.CreateAlloca(i1Ty(), nullptr, "in_frac_slot");
    AllocaInst *fallback_slot = b.CreateAlloca(i1Ty(), nullptr, "fallback_slot");
    AllocaInst *len_slot = b.CreateAlloca(i32Ty(), nullptr, "len_slot");
    AllocaInst *buf_slot = b.CreateAlloca(buf_arr_ty, nullptr, "buf_slot");

    b.CreateStore(ConstantInt::getFalse(ShortGlobalContext), neg_slot);
    b.CreateStore(ConstantInt::get(i64Ty(), 0, false), mant_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), sig_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), tot_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), frac_slot);
    b.CreateStore(ConstantInt::getFalse(ShortGlobalContext), in_frac_slot);
    b.CreateStore(ConstantInt::getFalse(ShortGlobalContext), fallback_slot);
    b.CreateStore(ConstantInt::get(i32Ty(), 0, true), len_slot);
    b.CreateBr(ws_loop_bb);

    b.SetInsertPoint(ws_loop_bb);
    Value *ch_ws = b.CreateCall(getchar_fn, {}, "ch_ws");
    b.CreateStore(ch_ws, c_slot);
    Value *is_eof = b.CreateICmpSLT(ch_ws, ConstantInt::get(i32Ty(), 0, true), "is_eof");
    b.CreateCondBr(is_eof, eof_bb, ws_check_bb);

    b.SetInsertPoint(eof_bb);
    b.CreateRet(def_arg);

    b.SetInsertPoint(ws_check_bb);
    Value *ch_chk = b.CreateLoad(i32Ty(), c_slot, "ch_chk");
    Value *is_ws = b.CreateICmpSLE(ch_chk, ConstantInt::get(i32Ty(), 32, true), "is_ws");
    b.CreateCondBr(is_ws, ws_loop_bb, sign_check_bb);

    b.SetInsertPoint(sign_check_bb);
    Value *ch_sgn = b.CreateLoad(i32Ty(), c_slot, "ch_sgn");
    Value *is_minus = b.CreateICmpEQ(ch_sgn, ConstantInt::get(i32Ty(), 45, true), "is_minus");
    Value *is_plus = b.CreateICmpEQ(ch_sgn, ConstantInt::get(i32Ty(), 43, true), "is_plus");
    b.CreateStore(is_minus, neg_slot);
    Value *has_sign = b.CreateOr(is_minus, is_plus, "has_sign");
    b.CreateCondBr(has_sign, sign_adv_bb, parse_loop_bb);

    b.SetInsertPoint(sign_adv_bb);
    Value *sgn_i8 = b.CreateTrunc(ch_sgn, i8_ty, "sgn_i8");
    Value *buf0_ptr = b.CreateInBoundsGEP(buf_arr_ty, buf_slot, {ConstantInt::get(i32Ty(), 0, true), ConstantInt::get(i32Ty(), 0, true)}, "buf0");
    b.CreateStore(sgn_i8, buf0_ptr);
    b.CreateStore(ConstantInt::get(i32Ty(), 1, true), len_slot);
    Value *ch_after = b.CreateCall(getchar_fn, {}, "ch_after");
    b.CreateStore(ch_after, c_slot);
    b.CreateBr(parse_loop_bb);

    b.SetInsertPoint(parse_loop_bb);
    Value *ch_cur = b.CreateLoad(i32Ty(), c_slot, "ch_cur");
    Value *is_delim = b.CreateICmpSLE(ch_cur, ConstantInt::get(i32Ty(), 32, true), "is_delim");
    Value *len_cur = b.CreateLoad(i32Ty(), len_slot, "len_cur");
    Value *len_ok = b.CreateICmpSLT(len_cur, ConstantInt::get(i32Ty(), 62, true), "len_ok");
    b.CreateCondBr(is_delim, finish_check_bb, buf_store_bb);

    b.SetInsertPoint(buf_store_bb);
    b.CreateCondBr(len_ok, char_dispatch_bb, buf_overflow_bb);

    b.SetInsertPoint(buf_overflow_bb);
    b.CreateStore(ConstantInt::getTrue(ShortGlobalContext), fallback_slot);
    b.CreateBr(next_char_bb);

    b.SetInsertPoint(char_dispatch_bb);
    Value *ch_i8 = b.CreateTrunc(ch_cur, i8_ty, "ch_i8");
    Value *buf_idx_ptr = b.CreateInBoundsGEP(buf_arr_ty, buf_slot, {ConstantInt::get(i32Ty(), 0, true), len_cur}, "buf_idx");
    b.CreateStore(ch_i8, buf_idx_ptr);
    b.CreateStore(b.CreateAdd(len_cur, ConstantInt::get(i32Ty(), 1, true)), len_slot);
    Value *dig = b.CreateSub(ch_cur, ConstantInt::get(i32Ty(), 48, true), "dig");
    Value *is_dig = b.CreateICmpULT(dig, ConstantInt::get(i32Ty(), 10, false), "is_dig");
    b.CreateCondBr(is_dig, digit_bb, non_digit_bb);

    b.SetInsertPoint(digit_bb);
    Value *tot_cur = b.CreateLoad(i32Ty(), tot_slot, "tot_cur");
    b.CreateStore(b.CreateAdd(tot_cur, ConstantInt::get(i32Ty(), 1, true)), tot_slot);
    Value *d64 = b.CreateZExt(dig, i64Ty(), "d64");
    Value *mant_cur = b.CreateLoad(i64Ty(), mant_slot, "mant_cur");
    Value *sig_cur = b.CreateLoad(i32Ty(), sig_slot, "sig_cur");
    Value *mant_nz = b.CreateICmpNE(mant_cur, ConstantInt::get(i64Ty(), 0, false), "mant_nz");
    Value *dig_nz = b.CreateICmpNE(d64, ConstantInt::get(i64Ty(), 0, false), "dig_nz");
    Value *inc_sig = b.CreateOr(mant_nz, dig_nz, "inc_sig");
    Value *sig_next = b.CreateSelect(inc_sig, b.CreateAdd(sig_cur, ConstantInt::get(i32Ty(), 1, true)), sig_cur, "sig_next");
    b.CreateStore(sig_next, sig_slot);
    Value *sig_fits = b.CreateICmpSLE(sig_next, ConstantInt::get(i32Ty(), 18, true), "sig_fits");
    Value *mant_mul = b.CreateAdd(b.CreateMul(mant_cur, ConstantInt::get(i64Ty(), 10, false)), d64, "mant_mul");
    b.CreateStore(b.CreateSelect(sig_fits, mant_mul, mant_cur), mant_slot);
    Value *in_frac = b.CreateLoad(i1Ty(), in_frac_slot, "in_frac");
    b.CreateCondBr(in_frac, frac_inc_bb, next_char_bb);

    b.SetInsertPoint(frac_inc_bb);
    Value *frac_cur = b.CreateLoad(i32Ty(), frac_slot, "frac_cur");
    b.CreateStore(b.CreateAdd(frac_cur, ConstantInt::get(i32Ty(), 1, true)), frac_slot);
    b.CreateBr(next_char_bb);

    b.SetInsertPoint(non_digit_bb);
    Value *is_dot = b.CreateICmpEQ(ch_cur, ConstantInt::get(i32Ty(), 46, true), "is_dot");
    Value *already_frac = b.CreateLoad(i1Ty(), in_frac_slot, "already_frac");
    Value *valid_dot = b.CreateAnd(is_dot, b.CreateNot(already_frac), "valid_dot");
    b.CreateCondBr(valid_dot, dot_bb, other_char_bb);

    b.SetInsertPoint(dot_bb);
    b.CreateStore(ConstantInt::getTrue(ShortGlobalContext), in_frac_slot);
    b.CreateBr(next_char_bb);

    b.SetInsertPoint(other_char_bb);
    b.CreateStore(ConstantInt::getTrue(ShortGlobalContext), fallback_slot);
    b.CreateBr(next_char_bb);

    b.SetInsertPoint(next_char_bb);
    Value *ch_nxt = b.CreateCall(getchar_fn, {}, "ch_nxt");
    b.CreateStore(ch_nxt, c_slot);
    b.CreateBr(parse_loop_bb);

    b.SetInsertPoint(finish_check_bb);
    Value *fb = b.CreateLoad(i1Ty(), fallback_slot, "fb");
    Value *tot_fin = b.CreateLoad(i32Ty(), tot_slot, "tot_fin");
    Value *sig_fin = b.CreateLoad(i32Ty(), sig_slot, "sig_fin");
    Value *frac_fin = b.CreateLoad(i32Ty(), frac_slot, "frac_fin");
    Value *has_tot = b.CreateICmpSGT(tot_fin, ConstantInt::get(i32Ty(), 0, true), "has_tot");
    Value *sig_ok = b.CreateICmpSLE(sig_fin, ConstantInt::get(i32Ty(), 15, true), "sig_ok");
    Value *frac_ok = b.CreateICmpSLE(frac_fin, ConstantInt::get(i32Ty(), 9, true), "frac_ok");
    Value *use_fast = b.CreateAnd(b.CreateAnd(b.CreateNot(fb), has_tot), b.CreateAnd(sig_ok, frac_ok), "use_fast");
    b.CreateCondBr(use_fast, fast_path_bb, strtod_bb);

    b.SetInsertPoint(fast_path_bb);
    Value *mant_fin = b.CreateLoad(i64Ty(), mant_slot, "mant_fin");
    Value *fp_mant = b.CreateUIToFP(mant_fin, f64Ty(), "fp_mant");
    Value *pow10_ptr = b.CreateInBoundsGEP(pow10_arr_ty, pow10_gv, {ConstantInt::get(i32Ty(), 0, true), frac_fin}, "pow10_ptr");
    Value *scale = b.CreateLoad(f64Ty(), pow10_ptr, "scale");
    Value *scaled = b.CreateFDiv(fp_mant, scale, "scaled");
    Value *neg_fin = b.CreateLoad(i1Ty(), neg_slot, "neg_fin");
    Value *neg_scaled = b.CreateFNeg(scaled, "neg_scaled");
    Value *fast_res = b.CreateSelect(neg_fin, neg_scaled, scaled, "fast_res");
    b.CreateRet(fast_res);

    b.SetInsertPoint(strtod_bb);
    Value *len_fin = b.CreateLoad(i32Ty(), len_slot, "len_fin");
    Value *term_ptr = b.CreateInBoundsGEP(buf_arr_ty, buf_slot, {ConstantInt::get(i32Ty(), 0, true), len_fin}, "term_ptr");
    b.CreateStore(ConstantInt::get(i8_ty, 0, false), term_ptr);
    Value *buf_base = b.CreateInBoundsGEP(buf_arr_ty, buf_slot, {ConstantInt::get(i32Ty(), 0, true), ConstantInt::get(i32Ty(), 0, true)}, "buf_base");
    Value *strtod_res = b.CreateCall(strtod_fn, {buf_base, ConstantPointerNull::get(i8PtrTy())}, "strtod_res");
    Value *has_any = b.CreateICmpSGT(len_fin, ConstantInt::get(i32Ty(), 0, true), "has_any");
    b.CreateRet(b.CreateSelect(has_any, strtod_res, def_arg, "fallback_res"));

    return reader_fn;
}

bool IR_Generator::tryEmitUnrollAndJamReductionLoop(AST_FOR_LOOP_STATEMENT_RULE *for_statement) {
    if (!for_statement || !for_statement->for_block || for_statement->for_block->lexical_scope) return false;
    auto *outer_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(for_statement->variable);
    auto *outer_from = dynamic_cast<AST_LITERAL *>(for_statement->from);
    auto *outer_step = dynamic_cast<AST_LITERAL *>(for_statement->step);
    auto *outer_to = dynamic_cast<AST_LITERAL *>(for_statement->to);
    if (!outer_var || !outer_from || !outer_step || !outer_to) return false;
    if (outer_from->int_literal != 0 || outer_step->int_literal != 1) return false;
    const int M = outer_to->int_literal;
    if (M < 2 || M > 32) return false;
    if (variableType(outer_var->variable_name, false) != ShortType::Int) return false;

    const auto &stmts = for_statement->for_block->statements;
    if (stmts.size() != 3 && stmts.size() != 4) return false;

    auto *init_assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(stmts[0]);
    if (!init_assign) return false;
    auto *acc_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(init_assign->variable);
    if (!acc_var || variableType(acc_var->variable_name, false) != ShortType::Float) return false;
    if (expressionContainsCall(init_assign->expression) ||
        expressionReferencesVar(init_assign->expression, acc_var->variable_name)) {
        return false;
    }

    std::size_t idx_pos = 1;
    AST_SIMPLE_VARIABLE *pre_inner_var = nullptr;
    if (stmts.size() == 4) {
        auto *pre_assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(stmts[1]);
        if (!pre_assign) return false;
        pre_inner_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(pre_assign->variable);
        auto *pre_lit = dynamic_cast<AST_LITERAL *>(pre_assign->expression);
        if (!pre_inner_var || !pre_lit || pre_lit->int_literal != 0) return false;
        idx_pos = 2;
    }

    auto *inner_loop = dynamic_cast<AST_FOR_LOOP_STATEMENT_RULE *>(stmts[idx_pos]);
    auto *argmax_if = dynamic_cast<AST_IF_STATEMENT *>(stmts[idx_pos + 1]);
    if (!inner_loop || !argmax_if || !inner_loop->for_block || inner_loop->for_block->lexical_scope) return false;

    auto *inner_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(inner_loop->variable);
    auto *inner_from = dynamic_cast<AST_LITERAL *>(inner_loop->from);
    auto *inner_step = dynamic_cast<AST_LITERAL *>(inner_loop->step);
    auto *inner_to = dynamic_cast<AST_LITERAL *>(inner_loop->to);
    if (!inner_var || !inner_from || !inner_step || !inner_to) return false;
    if (pre_inner_var && pre_inner_var->variable_name != inner_var->variable_name) return false;
    if (inner_from->int_literal != 0 || inner_step->int_literal != 1 || inner_to->int_literal < 1) return false;
    if (variableType(inner_var->variable_name, false) != ShortType::Int) return false;
    const int K = inner_to->int_literal;

    const auto &inner_stmts = inner_loop->for_block->statements;
    if (inner_stmts.size() != 1 && inner_stmts.size() != 2) return false;

    AST_ASSIGNMENT_RULE *idx_assign = nullptr;
    AST_SIMPLE_VARIABLE *idx_var = nullptr;
    AST_ASSIGNMENT_RULE *acc_update = nullptr;
    if (inner_stmts.size() == 2) {
        idx_assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(inner_stmts[0]);
        acc_update = dynamic_cast<AST_ASSIGNMENT_RULE *>(inner_stmts[1]);
        if (!idx_assign) return false;
        idx_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(idx_assign->variable);
        if (!idx_var || variableType(idx_var->variable_name, false) != ShortType::Int) return false;
        if (expressionContainsCall(idx_assign->expression) ||
            expressionReferencesVar(idx_assign->expression, idx_var->variable_name) ||
            expressionReferencesVar(idx_assign->expression, acc_var->variable_name)) {
            return false;
        }
    } else {
        acc_update = dynamic_cast<AST_ASSIGNMENT_RULE *>(inner_stmts[0]);
    }
    if (!acc_update) return false;
    auto *acc_lhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(acc_update->variable);
    auto *acc_bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(acc_update->expression);
    if (!acc_lhs || acc_lhs->variable_name != acc_var->variable_name || !acc_bin || acc_bin->op != PLUS) return false;
    auto *acc_add_lhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(acc_bin->left);
    if (!acc_add_lhs || acc_add_lhs->variable_name != acc_var->variable_name) return false;
    AST_EXPRESSION_RULE *term_expr = acc_bin->right;
    if (!term_expr || expressionContainsCall(term_expr) ||
        expressionReferencesVar(term_expr, acc_var->variable_name) ||
        expressionReferencesVar(term_expr, outer_var->variable_name)) {
        // Note: if term_expr references idx_var (which depends on outer_var), that is valid;
        // we also allow term_expr to reference outer_var directly, so only forbid acc_var.
    }
    if (!term_expr || expressionContainsCall(term_expr) ||
        expressionReferencesVar(term_expr, acc_var->variable_name)) {
        return false;
    }

    // Match argmax_if: if (acc_var > max_var) { max_var = acc_var; best_var = outer_var; }
    if (!argmax_if->if_block || argmax_if->if_block->lexical_scope) return false;
    auto *cmp_bin = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(argmax_if->condition);
    if (!cmp_bin || cmp_bin->op != GREATER) return false;
    auto *cmp_lhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(cmp_bin->left);
    auto *max_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(cmp_bin->right);
    if (!cmp_lhs || !max_var || cmp_lhs->variable_name != acc_var->variable_name) return false;
    if (variableType(max_var->variable_name, false) != ShortType::Float) return false;

    const auto &if_stmts = argmax_if->if_block->statements;
    if (if_stmts.size() != 2) return false;
    auto *max_assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(if_stmts[0]);
    auto *best_assign = dynamic_cast<AST_ASSIGNMENT_RULE *>(if_stmts[1]);
    if (!max_assign || !best_assign) return false;
    auto *max_lhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(max_assign->variable);
    auto *max_rhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(max_assign->expression);
    auto *best_var = dynamic_cast<AST_SIMPLE_VARIABLE *>(best_assign->variable);
    auto *best_rhs = dynamic_cast<AST_SIMPLE_VARIABLE *>(best_assign->expression);
    if (!max_lhs || !max_rhs || !best_var || !best_rhs) return false;
    if (max_lhs->variable_name != max_var->variable_name ||
        max_rhs->variable_name != acc_var->variable_name ||
        best_rhs->variable_name != outer_var->variable_name ||
        variableType(best_var->variable_name, false) != ShortType::Int) {
        return false;
    }

    // Ensure all scalar variables involved are pairwise distinct and not read across roles.
    std::set<std::string> roles = {
        outer_var->variable_name,
        inner_var->variable_name,
        acc_var->variable_name,
        max_var->variable_name,
        best_var->variable_name
    };
    if (roles.size() != 5) return false;
    if (idx_var && !roles.insert(idx_var->variable_name).second) return false;
    if (expressionReferencesVar(init_assign->expression, inner_var->variable_name) ||
        expressionReferencesVar(init_assign->expression, max_var->variable_name) ||
        expressionReferencesVar(init_assign->expression, best_var->variable_name) ||
        expressionReferencesVar(term_expr, max_var->variable_name) ||
        expressionReferencesVar(term_expr, best_var->variable_name)) {
        return false;
    }
    if (idx_assign &&
        (expressionReferencesVar(idx_assign->expression, max_var->variable_name) ||
         expressionReferencesVar(idx_assign->expression, best_var->variable_name))) {
        return false;
    }

    // Pattern verified! Emit Loop Fission + Unroll-and-Jam + Branchless Argmax Epilogue.
    Function *funct = Builder.GetInsertBlock()->getParent();
    const auto saved_intervals = static_int_intervals;

    outer_var->accept(*this);
    Value *outer_var_ptr = ret;
    load_variable = 0;

    inner_var->accept(*this);
    Value *inner_var_ptr = ret;
    load_variable = 0;

    acc_var->accept(*this);
    Value *acc_var_ptr = ret;
    load_variable = 0;

    max_var->accept(*this);
    Value *max_var_ptr = ret;
    load_variable = 0;

    best_var->accept(*this);
    Value *best_var_ptr = ret;
    load_variable = 0;

    std::vector<AllocaInst *> acc_slots(static_cast<std::size_t>(M));
    for (int c = 0; c < M; ++c) {
        acc_slots[static_cast<std::size_t>(c)] =
            createEntryBlockAlloca(funct, f64Ty(), "jam_acc_" + std::to_string(c));
    }

    // Stage 1: Fissioned accumulator initialization across c = 0 .. M-1.
    for (int c = 0; c < M; ++c) {
        Builder.CreateStore(ConstantInt::get(i32Ty(), c, true), outer_var_ptr);
        static_int_intervals[outer_var->variable_name] = {true, c, c};
        init_assign->expression->accept(*this);
        Value *init_val = asExactLoweredType(get_expression(), ShortType::Float);
        Builder.CreateStore(init_val, acc_slots[static_cast<std::size_t>(c)]);
    }

    // Stage 2: Unroll-and-jammed reduction loop over inner_var = 0 .. K-1.
    Builder.CreateStore(ConstantInt::get(i32Ty(), 0, true), inner_var_ptr);
    BasicBlock *cond_BB = BasicBlock::Create(ShortGlobalContext, "jam_for_cond", funct);
    BasicBlock *body_BB = BasicBlock::Create(ShortGlobalContext, "jam_for_body", funct);
    BasicBlock *after_BB = BasicBlock::Create(ShortGlobalContext, "jam_for_after", funct);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(cond_BB);
    Value *cur_i = typedLoad(inner_var_ptr, i32Ty(), "jam_i");
    Value *cond = Builder.CreateICmpSLT(cur_i, ConstantInt::get(i32Ty(), K, true), "jam_cond");
    Builder.CreateCondBr(cond, body_BB, after_BB);

    Builder.SetInsertPoint(body_BB);
    static_int_intervals[inner_var->variable_name] = {true, 0, K - 1};
    for (int c = 0; c < M; ++c) {
        Builder.CreateStore(ConstantInt::get(i32Ty(), c, true), outer_var_ptr);
        static_int_intervals[outer_var->variable_name] = {true, c, c};
        if (idx_assign) {
            idx_assign->accept(*this);
        }
        term_expr->accept(*this);
        Value *term_val = asExactLoweredType(get_expression(), ShortType::Float);
        Value *prev_acc = typedLoad(acc_slots[static_cast<std::size_t>(c)], f64Ty(), "jam_acc_load");
        Value *next_acc = Builder.CreateFAdd(prev_acc, term_val, "jam_acc_add");
        Builder.CreateStore(next_acc, acc_slots[static_cast<std::size_t>(c)]);
    }
    Value *next_i = Builder.CreateAdd(typedLoad(inner_var_ptr, i32Ty()), ConstantInt::get(i32Ty(), 1, true), "jam_next_i");
    Builder.CreateStore(next_i, inner_var_ptr);
    Builder.CreateBr(cond_BB);

    // Stage 3: Branchless argmax reduction epilogue across c = 0 .. M-1.
    Builder.SetInsertPoint(after_BB);
    Value *cur_max = typedLoad(max_var_ptr, f64Ty(), "jam_max_init");
    Value *cur_best = typedLoad(best_var_ptr, i32Ty(), "jam_best_init");
    Value *last_acc = nullptr;
    for (int c = 0; c < M; ++c) {
        Value *acc_c = typedLoad(acc_slots[static_cast<std::size_t>(c)], f64Ty(), "jam_acc_final");
        last_acc = acc_c;
        Value *is_greater = Builder.CreateFCmpOGT(acc_c, cur_max, "jam_argmax_cmp");
        cur_max = Builder.CreateSelect(is_greater, acc_c, cur_max, "jam_max_sel");
        cur_best = Builder.CreateSelect(is_greater, ConstantInt::get(i32Ty(), c, true), cur_best, "jam_best_sel");
    }
    if (last_acc) Builder.CreateStore(last_acc, acc_var_ptr);
    Builder.CreateStore(cur_max, max_var_ptr);
    Builder.CreateStore(cur_best, best_var_ptr);
    Builder.CreateStore(ConstantInt::get(i32Ty(), M, true), outer_var_ptr);
    Builder.CreateStore(ConstantInt::get(i32Ty(), K, true), inner_var_ptr);

    static_int_intervals = saved_intervals;
    static_int_intervals[outer_var->variable_name] = {true, M, M};
    static_int_intervals[inner_var->variable_name] = {true, K, K};
    static_int_intervals[best_var->variable_name] = {false, 0, 0};
    if (idx_var) static_int_intervals.erase(idx_var->variable_name);
    load_variable = 0;
    return true;
}

int IR_Generator::visit(AST_PROGRAM *program) {
    static_int_intervals.clear();
    emitting_global_declarations = true;
    program->decl_block->accept(*this);
    emitting_global_declarations = false;
    program->functions->accept(*this);

    FunctionType *ftype = FunctionType::get(i32Ty(), false);
    main_function = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
    BasicBlock *BB = BasicBlock::Create(ShortGlobalContext, "entry", main_function);
    Builder.SetInsertPoint(BB);
    symbol_table_obj.push_block(BB);
    local_type_scopes.push_back({});
    static_int_intervals.clear();
    program->code_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateRet(ConstantInt::get(i32Ty(), 0, true));
    local_type_scopes.pop_back();
    symbol_table_obj.pop_block();
    static_int_intervals.clear();
    promoteConstantGlobalArrays();
    return 0;
}

int IR_Generator::visit(AST_DATA_DECLARATION_BLOCK *decl_block) {
    if (!emitting_global_declarations) {
        if (Builder.GetInsertBlock() == nullptr || local_type_scopes.empty()) {
            ShowError("local declaration reached lowering without an active lexical scope");
            return 0;
        }
        Function *function = Builder.GetInsertBlock()->getParent();
        for (const auto &entry : decl_block->typed_scalars) {
            AllocaInst *alloca = createEntryBlockAlloca(function, parseType(entry.type), entry.name);
            Builder.CreateStore(zeroValue(entry.type), alloca);
            symbol_table_obj.declare_locals(entry.name, alloca);
            local_type_scopes.back()[entry.name] = LocalTypeInfo{entry.type, false};
            if (entry.type == ShortType::Int || entry.type == ShortType::Boolean) {
                static_int_intervals[entry.name] = {true, 0, 0};
            }
        }
        for (const auto &entry : decl_block->typed_arrays) {
            ArrayType *array_type = ArrayType::get(parseType(entry.type), entry.size);
            AllocaInst *alloca = createEntryBlockAlloca(function, array_type, entry.name);
            alloca->setAlignment(Align(64));
            Builder.CreateStore(ConstantAggregateZero::get(array_type), alloca);
            symbol_table_obj.declare_locals(entry.name, alloca);
            local_type_scopes.back()[entry.name] = LocalTypeInfo{entry.type, true};
        }
        return 0;
    }
    for (const auto &entry : decl_block->typed_scalars) {
        global_scalar_types[entry.name] = entry.type;
        if (module->getNamedGlobal(entry.name)) continue;
        auto *gv = new GlobalVariable(*module, parseType(entry.type), false, GlobalValue::InternalLinkage,
                                      zeroValue(entry.type), entry.name);
        gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Local);
    }
    for (const auto &entry : decl_block->typed_arrays) {
        global_array_types[entry.name] = entry.type;
        if (module->getNamedGlobal(entry.name)) continue;
        ArrayType *arr_type = ArrayType::get(parseType(entry.type), entry.size);
        auto *gv = new GlobalVariable(*module, arr_type, false, GlobalValue::InternalLinkage,
                                      ConstantAggregateZero::get(arr_type), entry.name);
        gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Local);
        gv->setAlignment(MaybeAlign(64));
    }
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_LIST_RULE *functions) {
    for (AST_FUNCTION_RULE *function : functions->functions) {
        if (!function || !function->function_name) continue;
        function_return_types[function->function_name] = function->type;
        std::vector<ShortType> parameter_types;
        for (const auto &parameter : function->parameters->typed_scalars)
            parameter_types.push_back(parameter.type);
        function_parameter_types[function->function_name] = parameter_types;
        if (module->getFunction(function->function_name)) continue;
        std::vector<Type*> args;
        for (const auto &parameter : function->parameters->typed_scalars)
            args.push_back(parseType(parameter.type));
        FunctionType *ftype = FunctionType::get(parseType(function->type), args, false);
        Function::Create(ftype, GlobalValue::ExternalLinkage, function->function_name, module);
    }
    for (AST_FUNCTION_RULE *function : functions->functions) {
        if (function) function->accept(*this);
    }
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_RULE *function) {
    function_return_types[function->function_name] = function->type;
    std::vector<ShortType> parameter_types;
    for (const auto &parameter : function->parameters->typed_scalars)
        parameter_types.push_back(parameter.type);
    function_parameter_types[function->function_name] = parameter_types;
    Function *llvm_function = module->getFunction(function->function_name);
    if (!llvm_function) {
        std::vector<Type*> args;
        for (const auto &parameter : function->parameters->typed_scalars)
            args.push_back(parseType(parameter.type));
        FunctionType *ftype = FunctionType::get(parseType(function->type), args, false);
        llvm_function = Function::Create(ftype, GlobalValue::ExternalLinkage, function->function_name, module);
    }
    if (!llvm_function->empty()) return 0;

    BasicBlock *BB = BasicBlock::Create(ShortGlobalContext, "entry", llvm_function);
    symbol_table_obj.push_block(BB);
    Builder.SetInsertPoint(BB);
    local_type_scopes.push_back({});
    static_int_intervals.clear();
    const ShortType saved_return_type = active_function_return_type;
    active_function_return_type = function->type;

    unsigned idx = 0;
    for (Function::arg_iterator ai = llvm_function->arg_begin(); ai != llvm_function->arg_end(); ++ai, ++idx) {
        const auto &parameter = function->parameters->typed_scalars[idx];
        std::string name = parameter.name;
        ai->setName(name);
        AllocaInst *alloca = createEntryBlockAlloca(llvm_function, parseType(parameter.type), name);
        Builder.CreateStore(&*ai, alloca);
        symbol_table_obj.declare_locals(name, alloca);
        local_type_scopes.back()[name] = LocalTypeInfo{parameter.type, false};
    }

    const std::size_t saved_break = break_targets.size();
    const std::size_t saved_continue = continue_targets.size();
    function->block_statement->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) {
        if (llvm_function->getReturnType()->isVoidTy()) Builder.CreateRetVoid();
        else Builder.CreateUnreachable();
    }
    break_targets.resize(saved_break);
    continue_targets.resize(saved_continue);
    symbol_table_obj.pop_block();
    local_type_scopes.pop_back();
    static_int_intervals.clear();
    active_function_return_type = saved_return_type;
    return 0;
}

int IR_Generator::visit(AST_LOGIC_BLOCK *code_block) {
    code_block->block_statement->accept(*this);
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_CALL_RULE *function_called) {
    Function *func = module->getFunction(function_called->function_name);
    if (!func) {
        ShowError("Function Not Defined");
        return 0;
    }
    if (func->arg_size() != function_called->parameters->variables.size()) {
        ShowError("Function arity mismatch after semantic validation");
        return 0;
    }
    vector<Value*> args;
    const auto parameter_types = function_parameter_types.find(function_called->function_name);
    std::size_t argument_index = 0;
    for (auto *variable : function_called->parameters->variables) {
        variable->accept(*this);
        Value *arg = get_expression();
        if (!arg) return 0;
        if (parameter_types != function_parameter_types.end() &&
            argument_index < parameter_types->second.size())
            arg = asExactLoweredType(arg, parameter_types->second[argument_index]);
        if (!arg) return 0;
        args.push_back(arg);
        ++argument_index;
    }
    ret = Builder.CreateCall(func, args);
    const auto return_type = function_return_types.find(function_called->function_name);
    ret_type = return_type == function_return_types.end() ? ShortType::Int : return_type->second;
    for (const auto &g : global_scalar_types) static_int_intervals.erase(g.first);
    load_variable = 0;
    is_expression = !func->getReturnType()->isVoidTy();
    return 0;
}

int IR_Generator::visit(AST_EXPRESSION_STATEMENT_RULE *expression_statement) {
    expression_statement->expression->accept(*this);
    if (ret && !ret->getType()->isVoidTy()) get_expression();
    return 0;
}

int IR_Generator::visit(AST_ASSIGNMENT_RULE *assignment_statement) {
    const StaticIntInterval rhs_interval = evaluateStaticIntInterval(assignment_statement->expression);
    assignment_statement->expression->accept(*this);
    Value *expr_value = get_expression();
    assignment_statement->variable->accept(*this);
    Value *variable_value = ret;
    const ShortType target_type = ret_type;
    if(load_tensor) {
        Value *maximum=ConstantFP::get(f64Ty(),std::numeric_limits<float>::max());
        Value *minimum=ConstantFP::get(f64Ty(),-std::numeric_limits<float>::max());
        emitRuntimeFailureIf(Builder.CreateOr(Builder.CreateFCmpUGT(expr_value,maximum),Builder.CreateFCmpULT(expr_value,minimum)),
            diag::RuntimeInvalidState,"tensor store is outside the finite FP32 range");
        ret=Builder.CreateStore(Builder.CreateFPTrunc(expr_value,Type::getFloatTy(ShortGlobalContext)),variable_value);
        load_tensor=false; load_variable=0; return 0;
    }
    expr_value = asExactLoweredType(expr_value, target_type);
    if (!expr_value) return 0;
    ret = Builder.CreateStore(expr_value, variable_value);
    if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(assignment_statement->variable)) {
        if (rhs_interval.known && (target_type == ShortType::Int || target_type == ShortType::Boolean)) {
            static_int_intervals[sv->variable_name] = rhs_interval;
        } else {
            static_int_intervals.erase(sv->variable_name);
        }
    }
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_STATEMENTS_BLOCK *block_statement) {
    if (block_statement == nullptr || Builder.GetInsertBlock() == nullptr) return 0;
    if (block_statement->lexical_scope) {
        symbol_table_obj.push_block(Builder.GetInsertBlock());
        local_type_scopes.push_back({});
    }

    Function *function = Builder.GetInsertBlock()->getParent();
    std::map<std::string, BasicBlock*> labels;
    for (AST_STATEMENT_RULE *statement : block_statement->statements) {
        auto *label = dynamic_cast<AST_LABEL_RULE *>(statement);
        if (label != nullptr) {
            labels.emplace(label->label,
                           BasicBlock::Create(ShortGlobalContext, "label_" + label->label, function));
        }
    }
    if (!labels.empty()) static_int_intervals.clear();
    goto_label_scopes.push_back(labels);

    for (AST_STATEMENT_RULE *statement : block_statement->statements) {
        if (Builder.GetInsertBlock() && Builder.GetInsertBlock()->getTerminator() &&
            dynamic_cast<AST_LABEL_RULE *>(statement) == nullptr) {
            continue;
        }
        if (statement) statement->accept(*this);
        load_variable = 0;
    }

    goto_label_scopes.pop_back();
    if (block_statement->lexical_scope) {
        for (const auto &entry : local_type_scopes.back()) {
            static_int_intervals.erase(entry.first);
        }
        local_type_scopes.pop_back();
        symbol_table_obj.pop_block();
    }
    return 0;
}

int IR_Generator::visit(AST_IF_STATEMENT *if_statement) {
    Function *funct = Builder.GetInsertBlock()->getParent();
    BasicBlock *if_BB = BasicBlock::Create(ShortGlobalContext, "if", funct);
    BasicBlock *next_BB = BasicBlock::Create(ShortGlobalContext, "ifnext", funct);
    if_statement->condition->accept(*this);
    Value *condition_value = get_condition();
    Builder.CreateCondBr(condition_value, if_BB, next_BB);

    const auto saved_intervals = static_int_intervals;
    std::vector<std::string> modified;
    bool has_unknown = false;
    collectModifiedScalars(if_statement->if_block, modified, has_unknown);

    Builder.SetInsertPoint(if_BB);
    refineIntervalFromCondition(if_statement->condition, true);
    if_statement->if_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);

    Builder.SetInsertPoint(next_BB);
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        static_int_intervals = saved_intervals;
        for (const auto &name : modified) static_int_intervals.erase(name);
    }
    return 0;
}

int IR_Generator::visit(AST_IF_ELSE_STATEMENT *ifelse_statement) {
    Function *funct = Builder.GetInsertBlock()->getParent();
    BasicBlock *if_BB = BasicBlock::Create(ShortGlobalContext, "if", funct);
    BasicBlock *else_BB = BasicBlock::Create(ShortGlobalContext, "else", funct);
    BasicBlock *next_BB = BasicBlock::Create(ShortGlobalContext, "ifnext", funct);
    ifelse_statement->condition->accept(*this);
    Value *condition_value = get_condition();
    Builder.CreateCondBr(condition_value, if_BB, else_BB);

    const auto saved_intervals = static_int_intervals;
    std::vector<std::string> modified;
    bool has_unknown = false;
    collectModifiedScalars(ifelse_statement->if_block, modified, has_unknown);
    collectModifiedScalars(ifelse_statement->else_block, modified, has_unknown);

    Builder.SetInsertPoint(if_BB);
    refineIntervalFromCondition(ifelse_statement->condition, true);
    ifelse_statement->if_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);

    Builder.SetInsertPoint(else_BB);
    static_int_intervals = saved_intervals;
    refineIntervalFromCondition(ifelse_statement->condition, false);
    ifelse_statement->else_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);

    Builder.SetInsertPoint(next_BB);
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        static_int_intervals = saved_intervals;
        for (const auto &name : modified) static_int_intervals.erase(name);
    }
    return 0;
}

int IR_Generator::visit(AST_FOR_LOOP_STATEMENT_RULE *for_statement) {
    if (tryEmitUnrollAndJamReductionLoop(for_statement)) {
        return 0;
    }

    Function *funct = Builder.GetInsertBlock()->getParent();
    const StaticIntInterval from_interval = evaluateStaticIntInterval(for_statement->from);
    const StaticIntInterval step_interval = evaluateStaticIntInterval(for_statement->step);
    const StaticIntInterval to_interval = evaluateStaticIntInterval(for_statement->to);

    for_statement->from->accept(*this);
    Value *start = get_expression();
    for_statement->variable->accept(*this);
    Value *variable = ret;
    Builder.CreateStore(start, variable);
    load_variable = 0;

    std::vector<std::string> modified;
    bool has_unknown = false;
    collectModifiedScalars(for_statement->for_block, modified, has_unknown);
    if (expressionContainsCall(for_statement->step) || expressionContainsCall(for_statement->to)) {
        has_unknown = true;
    }
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        for (const auto &name : modified) static_int_intervals.erase(name);
    }

    std::string loop_var_name;
    bool loop_var_modified_in_body = false;
    if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(for_statement->variable)) {
        loop_var_name = sv->variable_name;
        for (const auto &name : modified) {
            if (name == loop_var_name) {
                loop_var_modified_in_body = true;
                break;
            }
        }
        static_int_intervals.erase(loop_var_name);
    }

    auto *const_step_lit = dynamic_cast<AST_LITERAL *>(for_statement->step);
    const bool has_nonzero_const_step = (const_step_lit != nullptr && const_step_lit->int_literal != 0);
    const int const_step_val = has_nonzero_const_step ? const_step_lit->int_literal : 0;

    AllocaInst *step_slot = has_nonzero_const_step ? nullptr : createEntryBlockAlloca(funct, i32Ty(), "for_step");
    BasicBlock *cond_BB = BasicBlock::Create(ShortGlobalContext, "for_condition", funct);
    BasicBlock *body_BB = BasicBlock::Create(ShortGlobalContext, "for_body", funct);
    BasicBlock *step_BB = BasicBlock::Create(ShortGlobalContext, "for_step_block", funct);
    BasicBlock *after_BB = BasicBlock::Create(ShortGlobalContext, "for_after", funct);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(cond_BB);
    Value *cur_val = typedLoad(variable, i32Ty(), "for_current");
    Value *cond = nullptr;
    if (has_nonzero_const_step) {
        for_statement->to->accept(*this);
        Value *to_val = get_expression();
        if (const_step_val > 0) {
            cond = Builder.CreateICmpSLT(cur_val, to_val, "for_forward_cond");
        } else {
            cond = Builder.CreateICmpSGT(cur_val, to_val, "for_reverse_cond");
        }
    } else {
        for_statement->step->accept(*this);
        Value *step = get_expression();
        emitRuntimeFailureIf(Builder.CreateICmpEQ(step, ConstantInt::get(i32Ty(), 0, true), "for_step_zero"),
                             diag::RuntimeLoopStepZero, "loop step must be non-zero");
        Builder.CreateStore(step, step_slot);
        for_statement->to->accept(*this);
        Value *to_val = get_expression();
        Value *positive = Builder.CreateICmpSGT(step, ConstantInt::get(i32Ty(), 0, true), "for_step_positive");
        Value *positive_cond = Builder.CreateICmpSLT(cur_val, to_val, "for_forward_cond");
        Value *negative_cond = Builder.CreateICmpSGT(cur_val, to_val, "for_reverse_cond");
        cond = Builder.CreateSelect(positive, positive_cond, negative_cond, "forcond");
    }
    Builder.CreateCondBr(cond, body_BB, after_BB);

    Builder.SetInsertPoint(body_BB);
    if (!loop_var_name.empty() && !loop_var_modified_in_body && !has_unknown &&
        from_interval.known && to_interval.known && step_interval.known &&
        step_interval.lo == step_interval.hi && step_interval.lo != 0) {
        const std::int64_t step_c = step_interval.lo;
        constexpr std::int64_t kMin = std::numeric_limits<std::int32_t>::min();
        constexpr std::int64_t kMax = std::numeric_limits<std::int32_t>::max();
        if (step_c > 0 && from_interval.lo < to_interval.hi &&
            to_interval.hi - 1 <= kMax - step_c) {
            static_int_intervals[loop_var_name] = {true, from_interval.lo, to_interval.hi - 1};
        } else if (step_c < 0 && from_interval.hi > to_interval.lo &&
                   to_interval.lo + 1 >= kMin - step_c) {
            static_int_intervals[loop_var_name] = {true, to_interval.lo + 1, from_interval.hi};
        }
    }
    break_targets.push_back(after_BB);
    continue_targets.push_back(step_BB);
    for_statement->for_block->accept(*this);
    continue_targets.pop_back();
    break_targets.pop_back();
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(step_BB);

    Builder.SetInsertPoint(step_BB);
    Value *saved_step = has_nonzero_const_step
        ? ConstantInt::get(i32Ty(), const_step_val, true)
        : typedLoad(step_slot, i32Ty(), "for_saved_step");
    Value *next = Builder.CreateAdd(typedLoad(variable, i32Ty()), saved_step, "for_next");
    Builder.CreateStore(next, variable);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(after_BB);
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        for (const auto &name : modified) static_int_intervals.erase(name);
        if (!loop_var_name.empty()) static_int_intervals.erase(loop_var_name);
    }
    return 0;
}

int IR_Generator::visit(AST_WHILE_LOOP_STATEMENT_RULE *while_statement) {
    std::vector<std::string> modified;
    bool has_unknown = false;
    collectModifiedScalars(while_statement->while_block, modified, has_unknown);
    if (expressionContainsCall(while_statement->condition)) has_unknown = true;
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        for (const auto &name : modified) static_int_intervals.erase(name);
    }

    Function *funct = Builder.GetInsertBlock()->getParent();
    BasicBlock *cond_BB = BasicBlock::Create(ShortGlobalContext, "while_condition", funct);
    BasicBlock *body_BB = BasicBlock::Create(ShortGlobalContext, "while_body", funct);
    BasicBlock *after_BB = BasicBlock::Create(ShortGlobalContext, "while_after", funct);
    Builder.CreateBr(cond_BB);
    Builder.SetInsertPoint(cond_BB);
    while_statement->condition->accept(*this);
    Value *condition_value = get_condition();
    Builder.CreateCondBr(condition_value, body_BB, after_BB);
    Builder.SetInsertPoint(body_BB);
    break_targets.push_back(after_BB);
    continue_targets.push_back(cond_BB);
    while_statement->while_block->accept(*this);
    continue_targets.pop_back();
    break_targets.pop_back();
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(cond_BB);
    Builder.SetInsertPoint(after_BB);
    if (has_unknown) {
        static_int_intervals.clear();
    } else {
        for (const auto &name : modified) static_int_intervals.erase(name);
    }
    return 0;
}

int IR_Generator::visit(AST_BREAK *) {
    if (break_targets.empty()) {
        ShowError("break reached lowering outside loop");
        return 0;
    }
    Builder.CreateBr(break_targets.back());
    return 0;
}

int IR_Generator::visit(AST_CONTINUE *) {
    if (continue_targets.empty()) {
        ShowError("continue reached lowering outside loop");
        return 0;
    }
    Builder.CreateBr(continue_targets.back());
    return 0;
}

int IR_Generator::visit(AST_RETURN_STATEMENT *statement) {
    Function *function = Builder.GetInsertBlock()->getParent();
    if (function->getReturnType()->isVoidTy()) {
        Builder.CreateRetVoid();
        return 0;
    }
    if (statement->expression) {
        statement->expression->accept(*this);
        Value *value = asExactLoweredType(get_expression(), active_function_return_type);
        if (value) Builder.CreateRet(value);
    } else {
        ShowError("non-void return reached lowering without a value");
        Builder.CreateUnreachable();
    }
    return 0;
}

int IR_Generator::visit(AST_GOTO_STATEMENT_RULE *goto_statement) {
    static_int_intervals.clear();
    if (goto_label_scopes.empty()) {
        ShowError("goto reached lowering without a lexical label scope");
        return 0;
    }
    const auto target = goto_label_scopes.back().find(goto_statement->label);
    if (target == goto_label_scopes.back().end()) {
        ShowError("goto target was not resolved by semantic analysis");
        return 0;
    }
    if (goto_statement->condition == nullptr) {
        Builder.CreateBr(target->second);
        return 0;
    }
    goto_statement->condition->accept(*this);
    Value *condition = get_condition();
    Function *function = Builder.GetInsertBlock()->getParent();
    BasicBlock *next = BasicBlock::Create(ShortGlobalContext, "goto_next", function);
    Builder.CreateCondBr(condition, target->second, next);
    Builder.SetInsertPoint(next);
    return 0;
}

int IR_Generator::visit(AST_READ_RULE *read_statement) {
    for (auto *var : read_statement->variables) {
        var->accept(*this);
        Value *v = ret;
        const ShortType type = load_type;
        load_variable = 0;
        if (auto *sv = dynamic_cast<AST_SIMPLE_VARIABLE *>(var)) {
            static_int_intervals.erase(sv->variable_name);
        }
#if !defined(_WIN32)
        if (type == ShortType::Float) {
            Function *reader = ensureFastReadF64();
            Value *cur = typedLoad(v, f64Ty(), "read_f64_default");
            Value *parsed = Builder.CreateCall(reader, {cur}, "fastReadF64Call");
            ret = Builder.CreateStore(parsed, v);
            continue;
        } else if (type == ShortType::Int || type == ShortType::Boolean) {
            Function *reader = ensureFastReadI32();
            Value *cur = typedLoad(v, i32Ty(), "read_i32_default");
            Value *parsed = Builder.CreateCall(reader, {cur}, "fastReadI32Call");
            ret = Builder.CreateStore(parsed, v);
            continue;
        }
#endif
        vector<Value*> args;
        args.push_back(Builder.CreateGlobalStringPtr(type == ShortType::Float ? "%lf" : "%d"));
        args.push_back(v);
        ret = Builder.CreateCall(CalleeR, args, "scanfCall");
    }
    return 0;
}

int IR_Generator::visit(AST_PRINT_RULE *print_statement) {
    const int sz = static_cast<int>(print_statement->printables.size());
    for (int i = 0; i < sz; i++) {
        const char *sep = (i != sz - 1) ? " " : "\n";
        vector<Value*> args;
        AST_PRINTABLE_ITEM &p = print_statement->printables[i];
        if (p.expression) {
            p.expression->accept(*this);
            Value *v = get_expression();
            if (ret_type == ShortType::Float) {
                args.push_back(Builder.CreateGlobalStringPtr(std::string("%.17g") + sep));
                args.push_back(v);
            } else if (ret_type == ShortType::String) {
                args.push_back(Builder.CreateGlobalStringPtr(std::string("%s") + sep));
                args.push_back(safeStringPointer(v));
            } else {
                args.push_back(Builder.CreateGlobalStringPtr(std::string("%d") + sep));
                Value *print_value = asExactLoweredType(v, ret_type);
                if (!print_value) return 0;
                args.push_back(print_value);
            }
        } else {
            p.string_literal->accept(*this);
            string printable = str_;
            if (printable.size() >= 2 && printable.front() == '"' && printable.back() == '"')
                printable = printable.substr(1, printable.length() - 2);
            args.push_back(Builder.CreateGlobalStringPtr(std::string("%s") + sep));
            args.push_back(Builder.CreateGlobalStringPtr(printable));
        }
        ret = Builder.CreateCall(CalleeF, args, "printfCall");
    }
    return 0;
}

int IR_Generator::visit(AST_LABEL_RULE *label_statement) {
    static_int_intervals.clear();
    if (goto_label_scopes.empty()) {
        ShowError("label reached lowering without a lexical label scope");
        return 0;
    }
    const auto target = goto_label_scopes.back().find(label_statement->label);
    if (target == goto_label_scopes.back().end()) {
        ShowError("label was not registered before lowering");
        return 0;
    }
    if (Builder.GetInsertBlock() && !Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(target->second);
    Builder.SetInsertPoint(target->second);
    return 0;
}

int IR_Generator::visit(AST_GREENAI_REPORT_RULE *greenai_report) {
    greenai_report->inferences->accept(*this);
    Value *inferences = get_expression();
    greenai_report->watts->accept(*this);
    Value *watts = get_expression();
    greenai_report->seconds->accept(*this);
    Value *seconds = get_expression();
    Value *energy = Builder.CreateMul(watts, seconds, "greenai_energy_j");
    Value *zero_energy = Builder.CreateICmpEQ(energy, ConstantInt::get(i32Ty(), 0, true), "greenai_zero_energy");
    Value *safe_energy = Builder.CreateSelect(zero_energy, ConstantInt::get(i32Ty(), 1, true), energy);
    Value *raw_efficiency = Builder.CreateSDiv(inferences, safe_energy, "greenai_raw_inf_per_j");
    Value *efficiency = Builder.CreateSelect(zero_energy, ConstantInt::get(i32Ty(), 0, true), raw_efficiency, "greenai_inf_per_j");
    std::string workload = stripQuotes(greenai_report->workload_name);
    vector<Value*> args;
    args.push_back(Builder.CreateGlobalStringPtr("GreenAI workload %s: inferences=%d energy_j=%d inf_per_j=%d\n"));
    args.push_back(Builder.CreateGlobalStringPtr(workload));
    args.push_back(inferences);
    args.push_back(energy);
    args.push_back(efficiency);
    ret = Builder.CreateCall(CalleeF, args, "greenaiPrintfCall");
    return 0;
}

int IR_Generator::visit(AST_AI_INFER_RULE *ai_infer) {
    std::string model_path = stripQuotes(ai_infer->model_path);
    std::string shape_csv = stripQuotes(ai_infer->shape_csv);
    std::string input_csv = stripQuotes(ai_infer->input_csv);
    vector<Value*> args;
    args.push_back(Builder.CreateGlobalStringPtr("AI inference request: model=%s shape=%s input=%s\n"));
    args.push_back(Builder.CreateGlobalStringPtr(model_path));
    args.push_back(Builder.CreateGlobalStringPtr(shape_csv));
    args.push_back(Builder.CreateGlobalStringPtr(input_csv));
    ret = Builder.CreateCall(CalleeF, args, "aiInferPrintfCall");
    emitShortHandRuntimeCall("short_ai_infer_legacy", {model_path, shape_csv, input_csv});
    return 0;
}

int IR_Generator::visit(AST_BINARY_EXPRESSION_RULE *binary_operator_expression) {
    const int op = binary_operator_expression->op;
    binary_operator_expression->left->accept(*this);
    Value *L = get_expression();
    const ShortType left_type = ret_type;
    binary_operator_expression->right->accept(*this);
    Value *R = get_expression();
    const ShortType right_type = ret_type;

    if (left_type == ShortType::Boolean && right_type == ShortType::Boolean) {
        L = asExactLoweredType(L, ShortType::Boolean);
        R = asExactLoweredType(R, ShortType::Boolean);
    }
    is_condition = is_expression = 0;
    if (left_type != right_type) {
        ret = ShowError("type mismatch reached lowering after semantic validation");
        return 0;
    }
    if (left_type == ShortType::String && (op == EQUAL || op == NOT_EQUAL)) {
        FunctionType *strcmpType = FunctionType::get(i32Ty(), {i8PtrTy(), i8PtrTy()}, false);
        FunctionCallee strcmpFn = module->getOrInsertFunction("strcmp", strcmpType);
        Value *comparison = Builder.CreateCall(
            strcmpFn, {safeStringPointer(L), safeStringPointer(R)}, "strcmp_result");
        Value *equal = Builder.CreateICmpEQ(comparison, ConstantInt::get(i32Ty(), 0, true), "string_equal");
        ret = op == EQUAL ? equal : Builder.CreateNot(equal, "string_not_equal");
        ret_type = ShortType::Boolean;
        is_condition = 1;
    }
    else if (left_type == ShortType::Float) {
        if (op == PLUS) ret = Builder.CreateFAdd(L, R, "faddtmp"), ret_type = ShortType::Float, is_expression = 1;
        else if (op == MINUS) ret = Builder.CreateFSub(L, R, "fsubtmp"), ret_type = ShortType::Float, is_expression = 1;
        else if (op == MULTIPLY) ret = Builder.CreateFMul(L, R, "fmultmp"), ret_type = ShortType::Float, is_expression = 1;
        else if (op == DIVIDE) {
            if (auto *cfp = dyn_cast<ConstantFP>(R)) {
                const double divisor = cfp->getValueAPF().convertToDouble();
                if (std::isfinite(divisor) && divisor != 0.0) {
                    int exp = 0;
                    const double mant = std::frexp(divisor, &exp);
                    if (std::abs(mant) == 0.5) {
                        ret = Builder.CreateFMul(L, ConstantFP::get(f64Ty(), 1.0 / divisor), "fdivtmp");
                    } else {
                        ret = Builder.CreateFDiv(L, R, "fdivtmp");
                    }
                    ret_type = ShortType::Float;
                    is_expression = 1;
                    load_variable = 0;
                    return 0;
                }
            }
            Value *zero = Builder.CreateFCmpOEQ(R, ConstantFP::get(f64Ty(), 0.0), "float_zero_divisor");
            emitRuntimeFailureIf(zero, diag::RuntimeArithmeticDomainError,
                                 "float division by zero is forbidden by shorthand.type_memory.v1");
            ret = Builder.CreateFDiv(L, R, "fdivtmp");
            ret_type = ShortType::Float;
            is_expression = 1;
        }
        else if (op == LESS) ret = Builder.CreateFCmpOLT(L, R, "flttmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else if (op == GREATER) ret = Builder.CreateFCmpOGT(L, R, "fgttmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else if (op == LESS_OR_EQUAL) ret = Builder.CreateFCmpOLE(L, R, "fletmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else if (op == GREATER_OR_EQUAL) ret = Builder.CreateFCmpOGE(L, R, "fgetmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else if (op == EQUAL) ret = Builder.CreateFCmpOEQ(L, R, "feqtmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else if (op == NOT_EQUAL) ret = Builder.CreateFCmpUNE(L, R, "fnetmp"), ret_type = ShortType::Boolean, is_condition = 1;
        else ret = ShowError("unsupported float operator reached lowering");
    }
    else if (op == PLUS) ret = Builder.CreateAdd(L, R, "addtmp"), ret_type = ShortType::Int, is_expression = 1;
    else if (op == MINUS) ret = Builder.CreateSub(L, R, "subtmp"), ret_type = ShortType::Int, is_expression = 1;
    else if (op == MULTIPLY) ret = Builder.CreateMul(L, R, "multmp"), ret_type = ShortType::Int, is_expression = 1;
    else if (op == DIVIDE || op == MODULO) {
        bool proven_safe_int_div = false;
        if (auto *ci = dyn_cast<ConstantInt>(R)) {
            const std::int64_t div_val = ci->getSExtValue();
            if (div_val != 0 && div_val != -1) proven_safe_int_div = true;
        }
        if (!proven_safe_int_div) {
            Value *zero = Builder.CreateICmpEQ(R, ConstantInt::get(i32Ty(), 0, true), "arith_zero_divisor");
            Value *min_value = Builder.CreateICmpEQ(L, ConstantInt::get(i32Ty(), std::numeric_limits<std::int32_t>::min(), true), "arith_min_value");
            Value *negative_one = Builder.CreateICmpEQ(R, ConstantInt::get(i32Ty(), -1, true), "arith_negative_one");
            Value *overflow = Builder.CreateAnd(min_value, negative_one, "arith_div_overflow");
            emitRuntimeFailureIf(Builder.CreateOr(zero, overflow, "arith_domain_error"),
                                 diag::RuntimeArithmeticDomainError,
                                 op == DIVIDE ? "integer division domain error" : "integer remainder domain error");
        }
        ret = op == DIVIDE ? Builder.CreateSDiv(L, R, "divtmp") : Builder.CreateSRem(L, R, "modtmp");
        ret_type = ShortType::Int;
        is_expression = 1;
    }
    else if (op == LESS) ret = Builder.CreateICmpSLT(L, R, "lttmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == GREATER) ret = Builder.CreateICmpSGT(L, R, "gttmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == LESS_OR_EQUAL) ret = Builder.CreateICmpSLE(L, R, "letmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == GREATER_OR_EQUAL) ret = Builder.CreateICmpSGE(L, R, "getmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == EQUAL) ret = Builder.CreateICmpEQ(L, R, "eqtmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == NOT_EQUAL) ret = Builder.CreateICmpNE(L, R, "netmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == OR) ret = Builder.CreateOr(asBool(L), asBool(R), "ortmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else if (op == AND) ret = Builder.CreateAnd(asBool(L), asBool(R), "andtmp"), ret_type = ShortType::Boolean, is_condition = 1;
    else ret = ShowError("Not a supported binary operator");
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_UNARY_EXPRESSION_RULE *unary_operator_expression) {
    unary_operator_expression->expression->accept(*this);
    Value *R = get_expression();
    if (unary_operator_expression->op == UMINUS) {
        if (ret_type == ShortType::Float) ret = Builder.CreateFNeg(R, "fnegtmp");
        else ret = Builder.CreateNeg(R, "negtmp");
    }
    else ret = ShowError("Not a supported unary operator");
    is_expression = 1;
    is_condition = 0;
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_SIMPLE_VARIABLE *variable_single_int) {
    load_tensor=false;
    string &var_name = variable_single_int->variable_name;
    ret = symbol_table_obj.return_locals(var_name);
    if (!ret) ret = module->getNamedGlobal(var_name);
    if (!ret) ret = ShowError("Unknown Variable name " + var_name);
    load_type = variableType(var_name, false);
    ret_type = load_type;
    load_variable = 1;
    is_expression = 1;
    return 0;
}

int IR_Generator::visit(AST_ARRAY_VARIABLE *variable_array_int) {
    string &array_name = variable_array_int->array_name;
    const StaticIntInterval idx_interval = evaluateStaticIntInterval(variable_array_int->index);
    variable_array_int->index->accept(*this);
    Value *index = get_expression();
    load_tensor=false;
    if(variable_array_int->tensor_element) {
        auto *tensor=module->getNamedGlobal("__sh.tensor."+array_name);
        auto *ready=module->getNamedGlobal("__sh.tensor.ready."+array_name);
        if(!tensor || !ready) { ret=ShowError("Unknown tensor "+array_name); return 0; }
        auto *array_type=cast<ArrayType>(tensor->getValueType());
        emitRuntimeFailureIf(Builder.CreateNot(typedLoad(ready,i1Ty())),diag::RuntimeInvalidState,"tensor declaration has not executed");
        const auto num_elems = static_cast<std::int64_t>(array_type->getNumElements());
        if (!(idx_interval.known && idx_interval.lo >= 0 && idx_interval.hi < num_elems)) {
            emitRuntimeFailureIf(Builder.CreateICmpUGE(index,ConstantInt::get(i32Ty(),array_type->getNumElements())),
                diag::RuntimeArrayBounds,"tensor index is outside bounds");
        }
        ret=Builder.CreateInBoundsGEP(array_type,tensor,{ConstantInt::get(i32Ty(),0),index});
        load_tensor=true; load_variable=1; load_type=ShortType::Float; ret_type=ShortType::Float; is_expression=1;
        return 0;
    }
    Value *array_pointer = symbol_table_obj.return_locals(array_name);
    GlobalVariable *array_global = module->getNamedGlobal(array_name);
    if (array_pointer == nullptr) array_pointer = array_global;
    if (!array_pointer) {
        ret = ShowError("Unknown Array name " + array_name);
        return 0;
    }
    Type *stored_type = nullptr;
    if (auto *alloca = dyn_cast<AllocaInst>(array_pointer)) stored_type = alloca->getAllocatedType();
    else if (array_global != nullptr) stored_type = array_global->getValueType();
    ArrayType *array_type = dyn_cast_or_null<ArrayType>(stored_type);
    if (!array_type) {
        ret = ShowError("Variable is not an array " + array_name);
        return 0;
    }
    const auto num_elems = static_cast<std::int64_t>(array_type->getNumElements());
    const bool proven_in_bounds = idx_interval.known && idx_interval.lo >= 0 && idx_interval.hi < num_elems;
    if (!proven_in_bounds) {
        Value *negative = Builder.CreateICmpSLT(index, ConstantInt::get(i32Ty(), 0, true), "array_index_negative");
        Value *too_large = Builder.CreateICmpSGE(
            index, ConstantInt::get(i32Ty(), array_type->getNumElements(), false), "array_index_too_large");
        emitRuntimeFailureIf(Builder.CreateOr(negative, too_large, "array_bounds_error"),
                             diag::RuntimeArrayBounds,
                             "array index is outside declared bounds for `" + array_name + "`");
    }
    vector<Value*> array_index;
    array_index.push_back(ConstantInt::get(i32Ty(), 0, true));
    array_index.push_back(index);
    ret = Builder.CreateInBoundsGEP(array_type, array_pointer, array_index, array_name + "_IDX");
    load_type = variableType(array_name, true);
    ret_type = load_type;
    load_variable = 1;
    is_expression = 1;
    return 0;
}

int IR_Generator::visit(AST_LITERAL *int_literal) {
    is_expression = 1;
    ret = ConstantInt::get(i32Ty(), int_literal->int_literal, true);
    ret_type = ShortType::Int;
    return 0;
}

int IR_Generator::visit(AST_STRING_LITERAL *string_literal) {
    str_ = string_literal->string_literal;
    ret = Builder.CreateGlobalStringPtr(stripQuotes(str_), "string_literal");
    ret_type = ShortType::String;
    load_variable = 0;
    is_expression = 1;
    return 0;
}

llvm::Type* IR_Generator::parseType(ShortType type) {
    switch (type) {
        case ShortType::Boolean: return i32Ty();
        case ShortType::Int: return i32Ty();
        case ShortType::Void: return voidTy();
        case ShortType::Float: return f64Ty();
        case ShortType::String: return i8PtrTy();
    }
    return i32Ty();
}

int IR_Generator::visit(AST_MODEL_DECLARATION *n){
    const auto &d = n->data;
    std::string backend_preference = joinStrings(d.backend_preference, ",");
    emitShortHandMetadata("model", d.name,
        "name=" + d.name + ";format=" + d.format + ";path=" + stripQuotes(d.path) +
        ";task=" + stripQuotes(d.task) + ";precision=" + d.precision +
        ";input_shape=" + d.input_shape + ";output_shape=" + d.output_shape +
        ";backend_preference=" + backend_preference +
        ";compact=" + std::string(d.compact ? "true" : "false"));
    emitShortHandRuntimeCall("short_ai_register_model", {
        d.name, d.format, stripQuotes(d.path), stripQuotes(d.task), d.source_tensor_values?"float32":d.precision,
        d.input_shape, d.output_shape, backend_preference});
    return 0;
}

int IR_Generator::visit(AST_TENSOR_DECLARATION *n){
    const auto &d = n->data;
    if(shorthand::ai::parseElementType(d.element_type)==shorthand::ai::ElementType::Float32 &&
       !d.dynamic && d.total_elements>0 && d.total_elements<=65536) {
        auto *array=ArrayType::get(Type::getFloatTy(ShortGlobalContext),d.total_elements);
        auto *tensor=module->getNamedGlobal("__sh.tensor."+d.name);
        if(!tensor) tensor=new GlobalVariable(*module,array,false,GlobalValue::InternalLinkage,
            ConstantAggregateZero::get(array),"__sh.tensor."+d.name);
        auto *ready=module->getNamedGlobal("__sh.tensor.ready."+d.name);
        if(!ready) ready=new GlobalVariable(*module,i1Ty(),false,GlobalValue::InternalLinkage,
            ConstantInt::getFalse(ShortGlobalContext),"__sh.tensor.ready."+d.name);
        std::vector<Constant *> values;
        for(double value:d.values) values.push_back(ConstantFP::get(Type::getFloatTy(ShortGlobalContext),value));
        const auto bytes=static_cast<std::uint64_t>(d.total_elements)*sizeof(float);
        if(values.empty()) Builder.CreateMemSet(tensor,Builder.getInt8(0),bytes,MaybeAlign(alignof(float)));
        else {
            auto *initial=ConstantArray::get(array,values);
            auto *constant=new GlobalVariable(*module,array,true,GlobalValue::PrivateLinkage,initial,"__sh.tensor.initial");
            Builder.CreateMemCpy(tensor,MaybeAlign(alignof(float)),constant,MaybeAlign(alignof(float)),bytes);
        }
        Builder.CreateStore(ConstantInt::getTrue(ShortGlobalContext),ready);
    }
    emitShortHandMetadata("tensor", d.name,
        "name=" + d.name + ";element_type=" + d.element_type +
        ";shape=" + d.shape_csv + ";rank=" + std::to_string(d.rank) +
        ";total_elements=" + std::to_string(d.total_elements));
    emitShortHandRuntimeCall("short_ai_register_tensor", {
        d.name, d.element_type, d.shape_csv,
        std::to_string(d.rank), std::to_string(d.total_elements)});
    return 0;
}

int IR_Generator::visit(AST_GREENAI_CONTRACT *n){
    const auto &d = n->data;
    std::string boundary = joinStrings(d.boundary, ",");
    emitShortHandMetadata("greenai_contract", d.name,
        "name=" + d.name + ";functional_unit=" + stripQuotes(d.functional_unit) +
        ";success_criteria=" + stripQuotes(d.success_criteria) +
        ";boundary=" + boundary +
        ";measurement_quality=" + d.measurement_quality +
        ";data_quality=" + d.data_quality +
        ";carbon_factor=" + std::to_string(d.carbon_factor) +
        ";claims_mode=" + d.claims_mode);
    emitShortHandRuntimeCall("short_greenai_register_contract", {
        d.name, stripQuotes(d.functional_unit), stripQuotes(d.success_criteria), boundary,
        d.measurement_quality, d.data_quality, std::to_string(d.carbon_factor), d.claims_mode});
    return 0;
}

int IR_Generator::visit(AST_GREENAI_MEASUREMENT *n){
    const auto &d = n->data;
    emitShortHandMetadata("greenai_measurement", d.workload,
        "workload=" + d.workload + ";backend=" + d.backend +
        ";inferences=" + std::to_string(d.inferences) +
        ";watts=" + std::to_string(d.watts) +
        ";seconds=" + std::to_string(d.seconds));
    emitShortHandRuntimeCall("short_greenai_record_measurement", {
        d.workload, d.backend, std::to_string(d.inferences),
        std::to_string(d.watts), std::to_string(d.seconds)});
    return 0;
}

int IR_Generator::visit(AST_C3ECO_DECLARATION *n){
    const auto &d = n->data;
    std::string payload = "kind=" + std::string(c3EcoDeclarationKindName(d.kind)) + ";name=" + d.name;
    for (const auto &field : d.fields) {
        std::vector<std::string> values;
        for (const auto &value : field.values)
            values.push_back(std::string(c3EcoValueKindName(value.kind)) + ":" + value.text);
        payload += ";" + field.name + "=" + joinStrings(values, "|");
    }
    emitShortHandMetadata("c3eco_declaration", d.name, payload);
    return 0;
}

int IR_Generator::visit(AST_INFER_STATEMENT *n){
    emitShortHandMetadata("infer", n->model_name,
        "model=" + n->model_name + ";input=" + n->input_name +
        ";output=" + n->output_name + ";compiled_runtime_hook=true");
    const bool typed=(n->input_declaration && (n->input_declaration->initialized || n->input_declaration->accessed)) ||
                     (n->output_declaration && (n->output_declaration->initialized || n->output_declaration->accessed));
    if(typed) {
        auto *input=module->getNamedGlobal("__sh.tensor."+n->input_name);
        auto *output=module->getNamedGlobal("__sh.tensor."+n->output_name);
        if(!input || !output) { ShowError("Value inference requires declared bounded tensors"); return 0; }
        for(const auto &name:{n->input_name,n->output_name}) {
            auto *ready=module->getNamedGlobal("__sh.tensor.ready."+name);
            emitRuntimeFailureIf(Builder.CreateNot(typedLoad(ready,i1Ty())),diag::RuntimeInvalidState,"tensor declaration has not executed");
        }
        auto *input_type=cast<ArrayType>(input->getValueType()),*output_type=cast<ArrayType>(output->getValueType());
        auto *count=createEntryBlockAlloca(Builder.GetInsertBlock()->getParent(),i32Ty(),"tensor_output_count");
        Builder.CreateStore(ConstantInt::get(i32Ty(),0),count);
        auto callee=module->getOrInsertFunction("short_ai_infer_f32",FunctionType::get(i32Ty(),
            {i8PtrTy(),i8PtrTy(),i8PtrTy(),i32Ty(),i8PtrTy(),i8PtrTy(),i32Ty(),i8PtrTy()},false));
        auto result=Builder.CreateCall(callee,{Builder.CreateGlobalStringPtr(n->model_name),Builder.CreateGlobalStringPtr(n->input_name),
            input,ConstantInt::get(i32Ty(),input_type->getNumElements()),Builder.CreateGlobalStringPtr(n->output_name),
            output,ConstantInt::get(i32Ty(),output_type->getNumElements()),count});
        emitRuntimeFailureIf(Builder.CreateICmpNE(result,ConstantInt::get(i32Ty(),0)),diag::RuntimeInferenceFailure,"typed inference failed");
        emitRuntimeFailureIf(Builder.CreateICmpNE(typedLoad(count,i32Ty()),ConstantInt::get(i32Ty(),output_type->getNumElements())),
            diag::RuntimeInferenceFailure,"typed inference returned unexpected tensor size");
        load_variable=0; load_tensor=false;
    } else emitShortHandRuntimeCall("short_ai_infer", {n->model_name, n->input_name, n->output_name});
    return 0;
}

int IR_Generator::visit(AST_BOOL_LITERAL *n){
    ret = ConstantInt::get(i32Ty(), n->value ? 1 : 0, true);
    load_variable = 0;
    is_expression = 1;
    ret_type = ShortType::Boolean;
    return 0;
}

int IR_Generator::visit(AST_FLOAT_LITERAL *n){
    ret = ConstantFP::get(f64Ty(), n->value);
    load_variable = 0;
    is_expression = 1;
    ret_type = ShortType::Float;
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_CALL_EXPRESSION *call){
    Function *func = module->getFunction(call->function_name);
    if (!func) {
        ret = ShowError("Function Not Defined");
        return 0;
    }
    if (func->arg_size() != call->arguments.size()) {
        ret = ShowError("Function arity mismatch after semantic validation");
        return 0;
    }
    vector<Value*> args;
    const auto parameter_types = function_parameter_types.find(call->function_name);
    std::size_t argument_index = 0;
    for (AST_EXPRESSION_RULE *argument : call->arguments) {
        argument->accept(*this);
        Value *value = get_expression();
        if (!value) return 0;
        if (parameter_types != function_parameter_types.end() &&
            argument_index < parameter_types->second.size())
            value = asExactLoweredType(value, parameter_types->second[argument_index]);
        if (!value) return 0;
        args.push_back(value);
        ++argument_index;
    }
    ret = func->getReturnType()->isVoidTy()
        ? Builder.CreateCall(func, args)
        : Builder.CreateCall(func, args, "calltmp");
    const auto return_type = function_return_types.find(call->function_name);
    ret_type = return_type == function_return_types.end() ? ShortType::Int : return_type->second;
    load_variable = 0;
    is_expression = !func->getReturnType()->isVoidTy();
    return 0;
}
