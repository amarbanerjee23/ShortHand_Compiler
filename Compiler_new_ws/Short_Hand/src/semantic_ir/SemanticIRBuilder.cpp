#include "SemanticIRBuilder.h"
#include "../ast/SourceRange.h"
#include <charconv>
#include <iomanip>
#include <sstream>

namespace sir = shorthand::semantic_ir;
namespace {
sir::ScalarType scalar(ShortType type) {
    switch (type) {
    case ShortType::Int: return sir::ScalarType::Int32;
    case ShortType::Boolean: return sir::ScalarType::Bool;
    case ShortType::Float: return sir::ScalarType::Float64;
    case ShortType::String: return sir::ScalarType::String;
    case ShortType::Void: return sir::ScalarType::Void;
    }
    return sir::ScalarType::Void;
}
std::string unquote(std::string s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size()-2);
    return s;
}
sir::TensorShape shape(const std::string &text) {
    sir::TensorShape result;
    std::istringstream in(text);
    std::string item;
    while (std::getline(in, item, ',')) {
        std::int64_t n = 0;
        auto parsed = std::from_chars(item.data(), item.data()+item.size(), n);
        if (parsed.ec != std::errc{} || parsed.ptr != item.data()+item.size() || n <= 0) return {};
        result.dims.push_back(n);
    }
    return result;
}
sir::ElementType element(const std::string &s) {
    if (s == "float32" || s == "f32" || s == "fp32") return sir::ElementType::Float32;
    if (s == "float16" || s == "f16" || s == "fp16") return sir::ElementType::Float16;
    if (s == "bfloat16" || s == "bf16") return sir::ElementType::BFloat16;
    if (s == "int8" || s == "i8") return sir::ElementType::Int8;
    if (s == "int4" || s == "i4") return sir::ElementType::Int4;
    if (s == "int32" || s == "i32") return sir::ElementType::Int32;
    return sir::ElementType::Unknown;
}
sir::ModelFormat format(const std::string &s) {
    if (s == "onnx") return sir::ModelFormat::Onnx;
    if (s == "tensorrt" || s == "tensorrt_engine") return sir::ModelFormat::TensorRTEngine;
    if (s == "torchscript") return sir::ModelFormat::TorchScript;
    if (s == "openvino" || s == "openvino_ir") return sir::ModelFormat::OpenVINOIR;
    if (s == "gguf") return sir::ModelFormat::GGUF;
    return sir::ModelFormat::Unknown;
}
bool backend(const std::string &s, sir::BackendKind &result) {
    const std::map<std::string, sir::BackendKind> kinds = {
        {"fallback", sir::BackendKind::Fallback}, {"onnxruntime_cpu", sir::BackendKind::OnnxRuntimeCPU},
        {"onnxruntime_cuda", sir::BackendKind::OnnxRuntimeCUDA}, {"onnxruntime_tensorrt", sir::BackendKind::OnnxRuntimeTensorRT},
        {"tensorrt", sir::BackendKind::TensorRT}, {"openvino", sir::BackendKind::OpenVINO},
        {"libtorch", sir::BackendKind::LibTorch}, {"llamacpp", sir::BackendKind::LlamaCpp}};
    auto found = kinds.find(s);
    if (found == kinds.end()) return false;
    result = found->second;
    return true;
}
std::string guardrail(const QualityGuardrail &g) {
    std::ostringstream out;
    out << unquote(g.metric) << ' ' << g.op << ' ' << std::setprecision(17) << g.threshold;
    return out.str();
}
}

sir::SourceRange SemanticIRBuilder::range(const void *node) const {
    const auto r = shorthand_get_ast_source_range(node);
    return {{file_, r.begin.line, r.begin.column}, {file_, r.end.line, r.end.column}};
}
std::vector<SemanticIRBuilder::Var> SemanticIRBuilder::declarations(AST_DATA_DECLARATION_BLOCK *node) {
    std::vector<Var> result;
    if (!node) return result;
    for (const auto &v : node->typed_scalars) result.push_back({v.name, scalar(v.type), 0, range(node), {}});
    for (const auto &v : node->typed_arrays) result.push_back({v.name, scalar(v.type), v.size, range(node), {}});
    return result;
}
void SemanticIRBuilder::signatures(AST_PROGRAM *node) {
    if (!node) { error_ = "missing semantic program"; return; }
    for (const auto &v : declarations(node->decl_block)) globals_[v.name] = v.type;
    if (node->functions) for (auto *f : node->functions->functions) returns_[f->function_name] = scalar(f->type);
}
void SemanticIRBuilder::functions(AST_PROGRAM *node) {
    auto vars = declarations(node->decl_block);
    result_.globals.insert(result_.globals.end(), vars.begin(), vars.end());
    if (!node->functions) return;
    for (auto *f : node->functions->functions) {
        sir::Function function;
        function.name = f->function_name;
        function.result = scalar(f->type);
        function.parameters = declarations(f->parameters);
        function.source_range = range(f);
        scopes_.push_back({});
        for (const auto &v : function.parameters) scopes_.back()[v.name] = v.type;
        function.body = statement(f->block_statement);
        scopes_.pop_back();
        result_.functions.push_back(std::move(function));
    }
}
SemanticIRBuilder::Type SemanticIRBuilder::variableType(const std::string &name) {
    for (auto it=scopes_.rbegin(); it!=scopes_.rend(); ++it) {
        auto found=it->find(name); if (found!=it->end()) return found->second;
    }
    auto found=globals_.find(name);
    if (found!=globals_.end()) return found->second;
    error_="unresolved variable in SemanticIR: "+name;
    return Type::Void;
}
SemanticIRBuilder::Expr SemanticIRBuilder::expression(AST_EXPRESSION_RULE *node) {
    Expr e; e.source_range=range(node);
    if (auto *n=dynamic_cast<AST_LITERAL *>(node)) { e.integer=n->int_literal; }
    else if (auto *n=dynamic_cast<AST_BOOL_LITERAL *>(node)) { e.kind=Expr::Kind::Boolean; e.type=Type::Bool; e.integer=n->value?1:0; }
    else if (auto *n=dynamic_cast<AST_FLOAT_LITERAL *>(node)) { e.kind=Expr::Kind::Float; e.type=Type::Float64; e.decimal=n->value; }
    else if (auto *n=dynamic_cast<AST_STRING_LITERAL *>(node)) { e.kind=Expr::Kind::String; e.type=Type::String; e.name=unquote(n->string_literal); }
    else if (auto *n=dynamic_cast<AST_SIMPLE_VARIABLE *>(node)) { e.kind=Expr::Kind::Variable; e.name=n->variable_name; e.type=variableType(e.name); }
    else if (auto *n=dynamic_cast<AST_ARRAY_VARIABLE *>(node)) { e.kind=Expr::Kind::Index; e.name=n->array_name; e.type=variableType(e.name); e.operands.push_back(expression(n->index)); }
    else if (auto *n=dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(node)) {
        e.kind=Expr::Kind::Binary; e.opcode=n->op;
        e.operands.push_back(expression(n->left)); e.operands.push_back(expression(n->right));
        e.type=e.opcode>=AST_BINARY_EXPRESSION_RULE::LESS?Type::Bool:e.operands[0].type;
    } else if (auto *n=dynamic_cast<AST_UNARY_EXPRESSION_RULE *>(node)) {
        e.kind=Expr::Kind::Unary; e.opcode=n->op; e.operands.push_back(expression(n->expression)); e.type=e.operands[0].type;
    } else if (auto *n=dynamic_cast<AST_FUNCTION_CALL_EXPRESSION *>(node)) {
        e.kind=Expr::Kind::Call; e.name=n->function_name;
        auto found=returns_.find(e.name);
        if(found==returns_.end()) error_="unresolved function in SemanticIR: "+e.name;
        else e.type=found->second;
        for(auto *arg:n->arguments) e.operands.push_back(expression(arg));
    } else { error_="unsupported expression in SemanticIR"; }
    return e;
}
SemanticIRBuilder::Stmt SemanticIRBuilder::statement(AST_STATEMENT_RULE *node) {
    Stmt s; s.source_range=range(node);
    if (auto *n=dynamic_cast<AST_STATEMENTS_BLOCK *>(node)) {
        s.lexical_scope=n->lexical_scope;
        if(s.lexical_scope) scopes_.push_back({});
        for(auto *child:n->statements) s.body.push_back(statement(child));
        if(s.lexical_scope) scopes_.pop_back();
    } else if (auto *n=dynamic_cast<AST_DATA_DECLARATION_BLOCK *>(node)) {
        s.kind=Stmt::Kind::Declare; s.variables=declarations(n);
        for(const auto &v:s.variables) scopes_.back()[v.name]=v.type;
    } else if (auto *n=dynamic_cast<AST_ASSIGNMENT_RULE *>(node)) {
        s.kind=Stmt::Kind::Assign; s.expressions={expression(n->variable),expression(n->expression)};
    } else if (auto *n=dynamic_cast<AST_EXPRESSION_STATEMENT_RULE *>(node)) {
        s.kind=Stmt::Kind::Expression; s.expressions={expression(n->expression)};
    } else if (auto *n=dynamic_cast<AST_FUNCTION_CALL_RULE *>(node)) {
        s.kind=Stmt::Kind::Expression; Expr e; e.kind=Expr::Kind::Call; e.name=n->function_name; e.source_range=range(n);
        auto found=returns_.find(e.name); if(found==returns_.end()) error_="unresolved function in SemanticIR: "+e.name; else e.type=found->second;
        for(auto *arg:n->parameters->variables) e.operands.push_back(expression(arg));
        s.expressions.push_back(std::move(e));
    } else if (auto *n=dynamic_cast<AST_IF_STATEMENT *>(node)) {
        s.kind=Stmt::Kind::If; s.expressions={expression(n->condition)}; s.body={statement(n->if_block)};
    } else if (auto *n=dynamic_cast<AST_IF_ELSE_STATEMENT *>(node)) {
        s.kind=Stmt::Kind::If; s.expressions={expression(n->condition)}; s.body={statement(n->if_block)}; s.alternative={statement(n->else_block)};
    } else if (auto *n=dynamic_cast<AST_WHILE_LOOP_STATEMENT_RULE *>(node)) {
        s.kind=Stmt::Kind::While; s.expressions={expression(n->condition)}; s.body={statement(n->while_block)};
    } else if (auto *n=dynamic_cast<AST_FOR_LOOP_STATEMENT_RULE *>(node)) {
        s.kind=Stmt::Kind::For; s.expressions={expression(n->variable),expression(n->from),expression(n->step),expression(n->to)}; s.body={statement(n->for_block)};
    } else if (dynamic_cast<AST_BREAK *>(node)) s.kind=Stmt::Kind::Break;
    else if (dynamic_cast<AST_CONTINUE *>(node)) s.kind=Stmt::Kind::Continue;
    else if (auto *n=dynamic_cast<AST_RETURN_STATEMENT *>(node)) { s.kind=Stmt::Kind::Return; if(n->expression) s.expressions={expression(n->expression)}; }
    else if (auto *n=dynamic_cast<AST_LABEL_RULE *>(node)) { s.kind=Stmt::Kind::Label; s.name=n->label; }
    else if (auto *n=dynamic_cast<AST_GOTO_STATEMENT_RULE *>(node)) { s.kind=Stmt::Kind::Goto; s.name=n->label; if(n->condition) s.expressions={expression(n->condition)}; }
    else if (auto *n=dynamic_cast<AST_READ_RULE *>(node)) { s.kind=Stmt::Kind::Read; for(auto *v:n->variables) s.expressions.push_back(expression(v)); }
    else if (auto *n=dynamic_cast<AST_PRINT_RULE *>(node)) {
        s.kind=Stmt::Kind::Print;
        for(const auto &p:n->printables) s.expressions.push_back(expression(p.expression?p.expression:static_cast<AST_EXPRESSION_RULE *>(p.string_literal)));
    } else if (auto *n=dynamic_cast<AST_MODEL_DECLARATION *>(node)) {
        s.kind=Stmt::Kind::Model; s.declaration=result_.models.size(); const auto &d=n->data;
        sir::ModelOp op(d.name,s.source_range); op.format=format(d.format); op.path=unquote(d.path); op.task=unquote(d.task);
        op.precision=element(d.precision); op.input_shape=shape(d.input_shape); op.output_shape=shape(d.output_shape);
        op.quality_guardrail=d.has_quality_guardrail?guardrail(d.quality_guardrail):"";
        for(const auto &b:d.backend_preference) { sir::BackendKind kind; if(!backend(b,kind)) error_="unknown SemanticIR backend: "+b; else op.backend_preference.push_back(kind); }
        result_.models.push_back(std::move(op));
    } else if (auto *n=dynamic_cast<AST_TENSOR_DECLARATION *>(node)) {
        s.kind=Stmt::Kind::Tensor; s.declaration=result_.tensors.size(); const auto &d=n->data;
        result_.tensors.emplace_back(d.name,element(d.element_type),shape(d.shape_csv),s.source_range);
    } else if (auto *n=dynamic_cast<AST_INFER_STATEMENT *>(node)) {
        s.kind=Stmt::Kind::Infer; s.declaration=result_.inferences.size(); result_.inferences.emplace_back(n->model_name,n->input_name,n->output_name,s.source_range);
    } else if (auto *n=dynamic_cast<AST_GREENAI_CONTRACT *>(node)) {
        s.kind=Stmt::Kind::Contract; s.declaration=result_.contracts.size(); const auto &d=n->data;
        sir::GreenAIContractOp op(d.name,s.source_range); op.functional_unit=unquote(d.functional_unit); op.success_criteria=unquote(d.success_criteria);
        op.boundary=d.boundary; op.measurement_quality=d.measurement_quality; op.data_quality=d.data_quality; op.carbon_factor=d.carbon_factor;
        op.claims_mode=d.claims_mode; op.quality_guardrail=d.has_quality_guardrail?guardrail(d.quality_guardrail):"";
        op.energy_budget_j=d.energy_budget_j; op.carbon_budget_gco2e=d.carbon_budget_gco2e; result_.contracts.push_back(std::move(op));
    } else if (auto *n=dynamic_cast<AST_GREENAI_MEASUREMENT *>(node)) {
        s.kind=Stmt::Kind::Measurement; s.declaration=result_.measurements.size(); const auto &d=n->data;
        sir::GreenAIMeasurementOp op(d.workload,s.source_range); op.backend=d.backend; op.inferences=d.inferences; op.watts=d.watts; op.seconds=d.seconds;
        result_.measurements.push_back(std::move(op));
    } else if (auto *n=dynamic_cast<AST_AI_INFER_RULE *>(node)) {
        s.kind=Stmt::Kind::LegacyInfer; s.strings={unquote(n->model_path),unquote(n->shape_csv),unquote(n->input_csv)};
    } else if (auto *n=dynamic_cast<AST_GREENAI_REPORT_RULE *>(node)) {
        s.kind=Stmt::Kind::GreenReport; s.name=n->workload_name; s.expressions={expression(n->inferences),expression(n->watts),expression(n->seconds)};
    } else if (auto *n=dynamic_cast<AST_C3ECO_DECLARATION *>(node)) {
        s.kind=Stmt::Kind::Evidence; s.name=n->data.name; s.strings.push_back(c3EcoDeclarationKindName(n->data.kind));
        for(const auto &f:n->data.fields) { s.strings.push_back(f.name); for(const auto &v:f.values) s.strings.push_back(std::string(c3EcoValueKindName(v.kind))+":"+v.text); }
    } else { error_="unsupported statement in SemanticIR"; }
    return s;
}
bool SemanticIRBuilder::build(const std::vector<std::pair<AST_PROGRAM *, std::string>> &libraries,
                              AST_PROGRAM *entry,const std::string &file,sir::ProgramIR &result,std::string &error) {
    result_={}; globals_.clear(); returns_.clear(); scopes_.clear(); error_.clear();
    for(const auto &unit:libraries) signatures(unit.first);
    signatures(entry);
    if(!error_.empty()) { error=error_; return false; }
    for(const auto &unit:libraries) { file_=unit.second; functions(unit.first); }
    file_=file; functions(entry); scopes_.push_back({}); result_.body=statement(entry->code_block->block_statement); scopes_.pop_back();
    // Compact model declarations derive a signature from their checked uses.
    for(auto &model:result_.models) for(const auto &infer:result_.inferences) if(infer.model_name==model.name) {
        for(const auto &tensor:result_.tensors) {
            if(model.input_shape.dims.empty() && tensor.name==infer.input_tensor_name) model.input_shape=tensor.shape;
            if(model.output_shape.dims.empty() && tensor.name==infer.output_tensor_name) model.output_shape=tensor.shape;
        }
    }
    if(!error_.empty()) { error=error_; return false; }
    result=std::move(result_); error.clear(); return true;
}
