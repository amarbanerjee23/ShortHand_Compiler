#include "IR_Generator.h"
#include "DiagnosticCodes.h"
#include "Symbol_Table.h"
#include "../util/util.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
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
        v = typedLoad(v, parseType(load_type));
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

int IR_Generator::visit(AST_PROGRAM *program) {
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
    program->code_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateRet(ConstantInt::get(i32Ty(), 0, true));
    local_type_scopes.pop_back();
    symbol_table_obj.pop_block();
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
        }
        for (const auto &entry : decl_block->typed_arrays) {
            ArrayType *array_type = ArrayType::get(parseType(entry.type), entry.size);
            AllocaInst *alloca = createEntryBlockAlloca(function, array_type, entry.name);
            Builder.CreateStore(ConstantAggregateZero::get(array_type), alloca);
            symbol_table_obj.declare_locals(entry.name, alloca);
            local_type_scopes.back()[entry.name] = LocalTypeInfo{entry.type, true};
        }
        return 0;
    }
    for (const auto &entry : decl_block->typed_scalars) {
        global_scalar_types[entry.name] = entry.type;
        if (module->getNamedGlobal(entry.name)) continue;
        new GlobalVariable(*module, parseType(entry.type), false, GlobalValue::ExternalLinkage,
                           zeroValue(entry.type), entry.name);
    }
    for (const auto &entry : decl_block->typed_arrays) {
        global_array_types[entry.name] = entry.type;
        if (module->getNamedGlobal(entry.name)) continue;
        ArrayType *arr_type = ArrayType::get(parseType(entry.type), entry.size);
        new GlobalVariable(*module, arr_type, false, GlobalValue::ExternalLinkage,
                           ConstantAggregateZero::get(arr_type), entry.name);
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
    assignment_statement->expression->accept(*this);
    Value *expr_value = get_expression();
    assignment_statement->variable->accept(*this);
    Value *variable_value = ret;
    const ShortType target_type = ret_type;
    expr_value = asExactLoweredType(expr_value, target_type);
    if (!expr_value) return 0;
    ret = Builder.CreateStore(expr_value, variable_value);
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
    Builder.SetInsertPoint(if_BB);
    if_statement->if_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);
    Builder.SetInsertPoint(next_BB);
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
    Builder.SetInsertPoint(if_BB);
    ifelse_statement->if_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);
    Builder.SetInsertPoint(else_BB);
    ifelse_statement->else_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(next_BB);
    Builder.SetInsertPoint(next_BB);
    return 0;
}

int IR_Generator::visit(AST_FOR_LOOP_STATEMENT_RULE *for_statement) {
    Function *funct = Builder.GetInsertBlock()->getParent();
    for_statement->from->accept(*this);
    Value *start = get_expression();
    for_statement->variable->accept(*this);
    Value *variable = ret;
    Builder.CreateStore(start, variable);
    load_variable = 0;

    AllocaInst *step_slot = createEntryBlockAlloca(funct, i32Ty(), "for_step");
    BasicBlock *cond_BB = BasicBlock::Create(ShortGlobalContext, "for_condition", funct);
    BasicBlock *body_BB = BasicBlock::Create(ShortGlobalContext, "for_body", funct);
    BasicBlock *step_BB = BasicBlock::Create(ShortGlobalContext, "for_step_block", funct);
    BasicBlock *after_BB = BasicBlock::Create(ShortGlobalContext, "for_after", funct);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(cond_BB);
    Value *cur_val = typedLoad(variable, i32Ty(), "for_current");
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
    Value *cond = Builder.CreateSelect(positive, positive_cond, negative_cond, "forcond");
    Builder.CreateCondBr(cond, body_BB, after_BB);

    Builder.SetInsertPoint(body_BB);
    break_targets.push_back(after_BB);
    continue_targets.push_back(step_BB);
    for_statement->for_block->accept(*this);
    continue_targets.pop_back();
    break_targets.pop_back();
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(step_BB);

    Builder.SetInsertPoint(step_BB);
    Value *saved_step = typedLoad(step_slot, i32Ty(), "for_saved_step");
    Value *next = Builder.CreateAdd(typedLoad(variable, i32Ty()), saved_step, "for_next");
    Builder.CreateStore(next, variable);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(after_BB);
    return 0;
}

int IR_Generator::visit(AST_WHILE_LOOP_STATEMENT_RULE *while_statement) {
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
        vector<Value*> args;
        var->accept(*this);
        Value *v = ret;
        const ShortType type = load_type;
        args.push_back(Builder.CreateGlobalStringPtr(type == ShortType::Float ? "%lf" : "%d"));
        args.push_back(v);
        ret = Builder.CreateCall(CalleeR, args, "scanfCall");
    }
    return 0;
}

int IR_Generator::visit(AST_PRINT_RULE *print_statement) {
    const int sz = static_cast<int>(print_statement->printables.size());
    for (int i = 0; i < sz; i++) {
        vector<Value*> args;
        AST_PRINTABLE_ITEM &p = print_statement->printables[i];
        if (p.expression) {
            p.expression->accept(*this);
            Value *v = get_expression();
            if (ret_type == ShortType::Float) {
                args.push_back(Builder.CreateGlobalStringPtr("%.17g"));
                args.push_back(v);
            } else if (ret_type == ShortType::String) {
                args.push_back(Builder.CreateGlobalStringPtr("%s"));
                args.push_back(safeStringPointer(v));
            } else {
                args.push_back(Builder.CreateGlobalStringPtr("%d"));
                Value *print_value = asExactLoweredType(v, ret_type);
                if (!print_value) return 0;
                args.push_back(print_value);
            }
        } else {
            p.string_literal->accept(*this);
            string printable = str_;
            if (printable.size() >= 2 && printable.front() == '"' && printable.back() == '"')
                printable = printable.substr(1, printable.length() - 2);
            args.push_back(Builder.CreateGlobalStringPtr("%s"));
            args.push_back(Builder.CreateGlobalStringPtr(printable));
        }
        ret = Builder.CreateCall(CalleeF, args, "printfCall");
        vector<Value*> sepArgs;
        sepArgs.push_back(Builder.CreateGlobalStringPtr("%s"));
        sepArgs.push_back(Builder.CreateGlobalStringPtr(i != sz - 1 ? " " : "\n"));
        ret = Builder.CreateCall(CalleeF, sepArgs, "printfSepCall");
    }
    return 0;
}

int IR_Generator::visit(AST_LABEL_RULE *label_statement) {
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
        Value *zero = Builder.CreateICmpEQ(R, ConstantInt::get(i32Ty(), 0, true), "arith_zero_divisor");
        Value *min_value = Builder.CreateICmpEQ(L, ConstantInt::get(i32Ty(), std::numeric_limits<std::int32_t>::min(), true), "arith_min_value");
        Value *negative_one = Builder.CreateICmpEQ(R, ConstantInt::get(i32Ty(), -1, true), "arith_negative_one");
        Value *overflow = Builder.CreateAnd(min_value, negative_one, "arith_div_overflow");
        emitRuntimeFailureIf(Builder.CreateOr(zero, overflow, "arith_domain_error"),
                             diag::RuntimeArithmeticDomainError,
                             op == DIVIDE ? "integer division domain error" : "integer remainder domain error");
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
    variable_array_int->index->accept(*this);
    Value *index = get_expression();
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
    Value *negative = Builder.CreateICmpSLT(index, ConstantInt::get(i32Ty(), 0, true), "array_index_negative");
    Value *too_large = Builder.CreateICmpSGE(
        index, ConstantInt::get(i32Ty(), array_type->getNumElements(), false), "array_index_too_large");
    emitRuntimeFailureIf(Builder.CreateOr(negative, too_large, "array_bounds_error"),
                         diag::RuntimeArrayBounds,
                         "array index is outside declared bounds for `" + array_name + "`");
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
        d.name, d.format, stripQuotes(d.path), stripQuotes(d.task), d.precision,
        d.input_shape, d.output_shape, backend_preference});
    return 0;
}

int IR_Generator::visit(AST_TENSOR_DECLARATION *n){
    const auto &d = n->data;
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
    for (const auto &field : d.fields)
        payload += ";" + field.name + "=" + joinStrings(field.values, "|");
    emitShortHandMetadata("c3eco_declaration", d.name, payload);
    return 0;
}

int IR_Generator::visit(AST_INFER_STATEMENT *n){
    emitShortHandMetadata("infer", n->model_name,
        "model=" + n->model_name + ";input=" + n->input_name +
        ";output=" + n->output_name + ";compiled_runtime_hook=true");
    emitShortHandRuntimeCall("short_ai_infer", {n->model_name, n->input_name, n->output_name});
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
