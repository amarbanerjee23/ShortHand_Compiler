#include "IR_Generator.h"
#include "Symbol_Table.h"
#include "../util/util.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace llvm;
using namespace std;

static Module *module = nullptr;
static map<string, Value*> NamedValues;
static LLVMContext ShortGlobalContext;
static IRBuilder<> Builder(ShortGlobalContext);
static Symbol_Table symbol_table_obj;
static FunctionCallee CalleeF;
static FunctionCallee CalleeR;

static Type *i32Ty() { return Type::getInt32Ty(ShortGlobalContext); }
static Type *i1Ty() { return Type::getInt1Ty(ShortGlobalContext); }
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
    return Builder.CreateIntCast(value, i32Ty(), true, "toint32");
}

static Value *asBool(Value *value) {
    if (!value) return value;
    if (value->getType()->isIntegerTy(1)) return value;
    return Builder.CreateICmpNE(asI32(value), ConstantInt::get(i32Ty(), 0, true), "tobool");
}

static AllocaInst *createEntryBlockAlloca(Function *function, const std::string &name) {
    IRBuilder<> tmp(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmp.CreateAlloca(i32Ty(), nullptr, name);
}

Value *ShowError(const char *str) {
    cerr << "Error:\n" << str << "\n";
    return nullptr;
}

Value *ShowError(string str) {
    cerr << "Error:\n" << str << "\n";
    return nullptr;
}

IR_Generator::IR_Generator() {
    module = new Module(this->getModuleName().c_str(), ShortGlobalContext);
    FunctionType *printfType = FunctionType::get(i32Ty(), {i8PtrTy()}, true);
    FunctionType *scanfType = FunctionType::get(i32Ty(), {i8PtrTy()}, true);
    CalleeF = module->getOrInsertFunction("printf", printfType);
    CalleeR = module->getOrInsertFunction("scanf", scanfType);
    load_variable = 0;
    is_condition = 0;
    is_expression = 0;
    ret = nullptr;
    main_function = nullptr;
}

IR_Generator::~IR_Generator() {
    delete module;
    module = nullptr;
}

void IR_Generator::dump() {
    if (verifyModule(*module, &errs())) {
        cerr << "LLVM module verification failed. IR was still written for inspection.\n";
    }
    cerr << "Generating LLVM IR Code\n\n";
    std::string Str;
    raw_string_ostream OS(Str);
    OS << *module;
    OS.flush();
    cerr << Str;
    std::string ir_file = this->getModuleName() + ".ir";
    std::ofstream out(ir_file.c_str());
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
    std::string base = this->getModuleName();
    std::string cmd_obj = "llc -filetype=obj " + base + ".bc -o " + base + ".o";
    std::string cmd_bin = "clang " + base + ".o -o " + base;
    if (std::system(cmd_obj.c_str()) != 0) {
        cerr << "Failed to run llc. Ensure LLVM tools are installed and in PATH.\n";
        return false;
    }
    if (std::system(cmd_bin.c_str()) != 0) {
        cerr << "Failed to run clang linker step.\n";
        return false;
    }
    cerr << "Generated native binary: " << base << "\n";
    return true;
}

Value *IR_Generator::get_expression() {
    Value *v = ret;
    if (load_variable) {
        v = typedLoad(v, i32Ty());
        load_variable = 0;
    }
    v = asI32(v);
    is_condition = 0;
    is_expression = 1;
    return v;
}

Value *IR_Generator::get_condition() {
    Value *v = ret;
    if (load_variable) {
        v = typedLoad(v, i32Ty());
        load_variable = 0;
    }
    v = asBool(v);
    is_condition = 1;
    is_expression = 0;
    return v;
}

int IR_Generator::visit(AST_PROGRAM *program) {
    program->decl_block->accept(*this);
    program->functions->accept(*this);

    FunctionType *ftype = FunctionType::get(i32Ty(), false);
    main_function = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
    BasicBlock *BB = BasicBlock::Create(ShortGlobalContext, "entry", main_function);
    Builder.SetInsertPoint(BB);
    program->code_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(ConstantInt::get(i32Ty(), 0, true));
    }
    return 0;
}

int IR_Generator::visit(AST_DATA_DECLARATION_BLOCK *decl_block) {
    for (int i = 0; i < (int)decl_block->single_ints.size(); i++) {
        GlobalVariable *gv = new GlobalVariable(*module, i32Ty(), false, GlobalValue::CommonLinkage,
                                                ConstantInt::get(i32Ty(), 0, true), decl_block->single_ints[i]);
        (void)gv;
    }
    for (int i = 0; i < (int)decl_block->array_ints.size(); i++) {
        ArrayType *arr_type = ArrayType::get(i32Ty(), decl_block->array_ints[i].second);
        new GlobalVariable(*module, arr_type, false, GlobalValue::ExternalLinkage,
                           ConstantAggregateZero::get(arr_type), decl_block->array_ints[i].first);
    }
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_LIST_RULE *functions) {
    for (auto function : functions->functions) function->accept(*this);
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_RULE *function) {
    std::vector<Type*> args(function->parameters->single_ints.size(), i32Ty());
    FunctionType *ftype = FunctionType::get(i32Ty(), args, false);
    Function *llvm_function = Function::Create(ftype, GlobalValue::ExternalLinkage, function->function_name, module);
    BasicBlock *BB = BasicBlock::Create(ShortGlobalContext, "entry", llvm_function);
    symbol_table_obj.push_block(BB);
    Builder.SetInsertPoint(BB);

    unsigned idx = 0;
    for (Function::arg_iterator ai = llvm_function->arg_begin(); ai != llvm_function->arg_end(); ++ai, ++idx) {
        std::string name = function->parameters->single_ints[idx];
        ai->setName(name);
        AllocaInst *alloca = createEntryBlockAlloca(llvm_function, name);
        Builder.CreateStore(&*ai, alloca);
        symbol_table_obj.declare_locals(name, alloca);
    }

    function->block_statement->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(ConstantInt::get(i32Ty(), 0, true));
    }
    symbol_table_obj.pop_block();
    return 0;
}

int IR_Generator::visit(AST_LOGIC_BLOCK *code_block) {
    code_block->block_statement->accept(*this);
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_BREAK *break_statement) {
    (void)break_statement;
    return 0;
}

int IR_Generator::visit(AST_FUNCTION_CALL_RULE *function_called) {
    Function *func = module->getFunction(function_called->function_name);
    if (!func) {
        ShowError("Function Not Defined");
        return 0;
    }
    vector<Value*> args;
    for (auto &i : function_called->parameters->variables) {
        i->accept(*this);
        Value *arg = get_expression();
        if (!arg) return 0;
        args.push_back(arg);
    }
    ret = Builder.CreateCall(func, args);
    load_variable = 0;
    is_expression = 1;
    return 0;
}

int IR_Generator::visit(AST_EXPRESSION_STATEMENT_RULE *expression_statement) {
    expression_statement->expression->accept(*this);
    get_expression();
    return 0;
}

int IR_Generator::visit(AST_ASSIGNMENT_RULE *assignment_statement) {
    assignment_statement->expression->accept(*this);
    Value *expr_value = get_expression();
    assignment_statement->variable->accept(*this);
    Value *variable_value = ret;
    ret = Builder.CreateStore(expr_value, variable_value);
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_STATEMENTS_BLOCK *block_statement) {
    for (int i = 0; i < (int)block_statement->statements.size(); i++) {
        if (Builder.GetInsertBlock() && Builder.GetInsertBlock()->getTerminator()) break;
        block_statement->statements[i]->accept(*this);
        load_variable = 0;
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

    BasicBlock *cond_BB = BasicBlock::Create(ShortGlobalContext, "for_condition", funct);
    BasicBlock *body_BB = BasicBlock::Create(ShortGlobalContext, "for_body", funct);
    BasicBlock *after_BB = BasicBlock::Create(ShortGlobalContext, "for_after", funct);
    Builder.CreateBr(cond_BB);

    Builder.SetInsertPoint(cond_BB);
    Value *cur_val = typedLoad(variable, i32Ty(), "for_current");
    for_statement->to->accept(*this);
    Value *to_val = get_expression();
    Value *cond = Builder.CreateICmpSLT(cur_val, to_val, "forcond");
    Builder.CreateCondBr(cond, body_BB, after_BB);

    Builder.SetInsertPoint(body_BB);
    for_statement->for_block->accept(*this);
    for_statement->step->accept(*this);
    Value *step = get_expression();
    Value *next = Builder.CreateAdd(typedLoad(variable, i32Ty()), step, "for_next");
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
    while_statement->while_block->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(cond_BB);
    Builder.SetInsertPoint(after_BB);
    return 0;
}

int IR_Generator::visit(AST_GOTO_STATEMENT_RULE *goto_statement) {
    Value *cond = nullptr;
    if (goto_statement->condition) {
        goto_statement->condition->accept(*this);
        cond = get_condition();
    }
    string &label = goto_statement->label;
    Function *funct = Builder.GetInsertBlock()->getParent();
    BasicBlock *label_BB;
    if (goto_labels.find(label) == goto_labels.end()) {
        label_BB = BasicBlock::Create(ShortGlobalContext, label, funct);
        goto_labels[label] = label_BB;
    } else {
        label_BB = goto_labels[label];
    }
    BasicBlock *next_BB = BasicBlock::Create(ShortGlobalContext, "goto_next", funct);
    if (goto_statement->condition) Builder.CreateCondBr(cond, label_BB, next_BB);
    else Builder.CreateBr(label_BB);
    Builder.SetInsertPoint(next_BB);
    return 0;
}

int IR_Generator::visit(AST_READ_RULE *read_statement) {
    for (auto *var : read_statement->variables) {
        vector<Value*> args;
        var->accept(*this);
        Value *v = ret;
        args.push_back(Builder.CreateGlobalStringPtr("%d"));
        args.push_back(v);
        ret = Builder.CreateCall(CalleeR, args, "scanfCall");
    }
    return 0;
}

int IR_Generator::visit(AST_PRINT_RULE *print_statement) {
    int sz = (int)print_statement->printables.size();
    for (int i = 0; i < sz; i++) {
        vector<Value*> args;
        AST_PRINTABLE_ITEM &p = print_statement->printables[i];
        if (p.expression) {
            p.expression->accept(*this);
            Value *v = get_expression();
            args.push_back(Builder.CreateGlobalStringPtr("%d"));
            args.push_back(v);
        } else {
            print_statement->printables[i].string_literal->accept(*this);
            string printable = str_;
            if (printable.size() >= 2 && printable.front() == '"' && printable.back() == '"') {
                printable = printable.substr(1, printable.length() - 2);
            }
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
    string &label = label_statement->label;
    Function *funct = Builder.GetInsertBlock()->getParent();
    BasicBlock *label_BB;
    if (goto_labels.find(label) == goto_labels.end()) {
        label_BB = BasicBlock::Create(ShortGlobalContext, label, funct);
        goto_labels[label] = label_BB;
    } else {
        label_BB = goto_labels[label];
    }
    if (!Builder.GetInsertBlock()->getTerminator()) Builder.CreateBr(label_BB);
    Builder.SetInsertPoint(label_BB);
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
    Value *safe_energy = Builder.CreateSelect(Builder.CreateICmpEQ(energy, ConstantInt::get(i32Ty(), 0, true), "greenai_zero_energy"),
                                             ConstantInt::get(i32Ty(), 1, true), energy);
    Value *efficiency = Builder.CreateSDiv(inferences, safe_energy, "greenai_inf_per_j");
    std::string workload = greenai_report->workload_name;
    if (workload.size() >= 2 && workload.front() == '"' && workload.back() == '"') workload = workload.substr(1, workload.length() - 2);
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
    std::string model_path = ai_infer->model_path;
    std::string shape_csv = ai_infer->shape_csv;
    std::string input_csv = ai_infer->input_csv;
    if (model_path.size() >= 2 && model_path.front() == '"' && model_path.back() == '"') model_path = model_path.substr(1, model_path.length() - 2);
    if (shape_csv.size() >= 2 && shape_csv.front() == '"' && shape_csv.back() == '"') shape_csv = shape_csv.substr(1, shape_csv.length() - 2);
    if (input_csv.size() >= 2 && input_csv.front() == '"' && input_csv.back() == '"') input_csv = input_csv.substr(1, input_csv.length() - 2);
    vector<Value*> args;
    args.push_back(Builder.CreateGlobalStringPtr("AI inference request: model=%s shape=%s input=%s\n"));
    args.push_back(Builder.CreateGlobalStringPtr(model_path));
    args.push_back(Builder.CreateGlobalStringPtr(shape_csv));
    args.push_back(Builder.CreateGlobalStringPtr(input_csv));
    ret = Builder.CreateCall(CalleeF, args, "aiInferPrintfCall");
    return 0;
}

int IR_Generator::visit(AST_BINARY_EXPRESSION_RULE *binary_operator_expression) {
    int op = binary_operator_expression->op;
    binary_operator_expression->left->accept(*this);
    Value *L = get_expression();
    binary_operator_expression->right->accept(*this);
    Value *R = get_expression();
    is_condition = is_expression = 0;
    if (op == PLUS) ret = Builder.CreateAdd(L, R, "addtmp"), is_expression = 1;
    else if (op == MINUS) ret = Builder.CreateSub(L, R, "subtmp"), is_expression = 1;
    else if (op == MULTIPLY) ret = Builder.CreateMul(L, R, "multmp"), is_expression = 1;
    else if (op == DIVIDE) ret = Builder.CreateSDiv(L, R, "divtmp"), is_expression = 1;
    else if (op == MODULO) ret = Builder.CreateSRem(L, R, "modtmp"), is_expression = 1;
    else if (op == LESS) ret = Builder.CreateICmpSLT(L, R, "lttmp"), is_condition = 1;
    else if (op == GREATER) ret = Builder.CreateICmpSGT(L, R, "gttmp"), is_condition = 1;
    else if (op == LESS_OR_EQUAL) ret = Builder.CreateICmpSLE(L, R, "letmp"), is_condition = 1;
    else if (op == GREATER_OR_EQUAL) ret = Builder.CreateICmpSGE(L, R, "getmp"), is_condition = 1;
    else if (op == EQUAL) ret = Builder.CreateICmpEQ(L, R, "eqtmp"), is_condition = 1;
    else if (op == NOT_EQUAL) ret = Builder.CreateICmpNE(L, R, "netmp"), is_condition = 1;
    else if (op == OR) ret = Builder.CreateOr(asBool(L), asBool(R), "ortmp"), is_condition = 1;
    else if (op == AND) ret = Builder.CreateAnd(asBool(L), asBool(R), "andtmp"), is_condition = 1;
    else ret = ShowError("Not a supported binary operator");
    load_variable = 0;
    return 0;
}

int IR_Generator::visit(AST_UNARY_EXPRESSION_RULE *unary_operator_expression) {
    int op = unary_operator_expression->op;
    unary_operator_expression->expression->accept(*this);
    Value *R = get_expression();
    if (op == UMINUS) ret = Builder.CreateNeg(R, "negtmp");
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
    load_variable = 1;
    is_expression = 1;
    return 0;
}

int IR_Generator::visit(AST_ARRAY_VARIABLE *variable_array_int) {
    string &array_name = variable_array_int->array_name;
    variable_array_int->index->accept(*this);
    Value *index = get_expression();
    GlobalVariable *array_global = module->getNamedGlobal(array_name);
    if (!array_global) {
        ret = ShowError("Unknown Array name " + array_name);
        return 0;
    }
    vector<Value*> array_index;
    array_index.push_back(ConstantInt::get(i32Ty(), 0, true));
    array_index.push_back(index);
    ret = Builder.CreateInBoundsGEP(array_global->getValueType(), array_global, array_index, array_name + "_IDX");
    load_variable = 1;
    is_expression = 1;
    return 0;
}

int IR_Generator::visit(AST_LITERAL *int_literal) {
    is_expression = 1;
    ret = ConstantInt::get(i32Ty(), int_literal->int_literal, true);
    return 0;
}

int IR_Generator::visit(AST_STRING_LITERAL *string_literal) {
    str_ = string_literal->string_literal;
    return 0;
}

llvm::Type* IR_Generator::parseType(ShortType type) {
    switch (type) {
    case ShortType::Boolean: return i32Ty();
    case ShortType::Int: return i32Ty();
    case ShortType::Void: return voidTy();
    case ShortType::Float: return i32Ty();
    case ShortType::String: return i32Ty();
    }
    return i32Ty();
}
