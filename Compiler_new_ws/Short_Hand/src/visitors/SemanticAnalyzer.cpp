#include "SemanticAnalyzer.h"
#include "DiagnosticCodes.h"
#include "../ai_runtime/AI_Types.h"

#include <utility>

namespace diag = shorthand::diagnostics;

static bool validShape(const std::string &s) {
    if (s == "dynamic") return true;
    return shorthand::ai::validateShape(shorthand::ai::parseShapeCsv(s));
}

static bool shapeCompatible(const std::string &modelShape, const std::string &tensorShape) {
    if (modelShape.empty() || tensorShape.empty()) return true;
    if (modelShape == "dynamic" || tensorShape == "dynamic") return true;
    return shorthand::ai::parseShapeCsv(modelShape) == shorthand::ai::parseShapeCsv(tensorShape);
}

static bool backendCompatibleWithFormat(shorthand::ai::ModelFormat format, shorthand::ai::BackendKind backend) {
    using shorthand::ai::BackendKind;
    using shorthand::ai::ModelFormat;
    if (backend == BackendKind::Fallback) return true;
    switch (format) {
        case ModelFormat::Onnx:
            return backend == BackendKind::OnnxRuntimeCPU || backend == BackendKind::OnnxRuntimeCUDA ||
                   backend == BackendKind::OnnxRuntimeTensorRT || backend == BackendKind::TensorRT;
        case ModelFormat::TensorRTEngine:
            return backend == BackendKind::TensorRT || backend == BackendKind::OnnxRuntimeTensorRT;
        case ModelFormat::TorchScript:
            return backend == BackendKind::LibTorch;
        case ModelFormat::OpenVINOIR:
            return backend == BackendKind::OpenVINO;
        case ModelFormat::GGUF:
            return backend == BackendKind::LlamaCpp;
        case ModelFormat::Unknown:
            return false;
    }
    return false;
}

static const char *shortTypeName(ShortType type) {
    switch (type) {
        case ShortType::Int: return "int";
        case ShortType::Boolean: return "bool";
        case ShortType::Float: return "float/double";
        case ShortType::String: return "string";
        case ShortType::Void: return "void";
    }
    return "unknown";
}

static shorthand::types::TypeKind productionTypeKind(ShortType type) {
    using shorthand::types::TypeKind;
    switch (type) {
        case ShortType::Int: return TypeKind::Int32;
        case ShortType::Boolean: return TypeKind::Bool;
        case ShortType::Float: return TypeKind::Float64;
        case ShortType::String: return TypeKind::String;
        case ShortType::Void: return TypeKind::Void;
    }
    return TypeKind::Void;
}

static std::set<std::string> c3EcoRequiredFields(C3EcoDeclarationKind kind) {
    switch (kind) {
        case C3EcoDeclarationKind::Certification:
            return {"version", "owner", "software_class", "deployment_mode", "geography", "validity_period"};
        case C3EcoDeclarationKind::FunctionalUnit:
            return {"denominator", "success_condition", "quality_threshold"};
        case C3EcoDeclarationKind::Workload:
            return {"traffic_profile", "batch_size", "concurrency", "warmup_runs", "measured_runs", "cache_state"};
        case C3EcoDeclarationKind::Boundary:
            return {"include"};
        case C3EcoDeclarationKind::MeasurementPlan:
            return {"instrument", "carbon_factor", "sampling", "uncertainty", "retention"};
        case C3EcoDeclarationKind::AILifecycle:
            return {"role", "model_provider", "lifecycle_scope", "training_included", "fine_tuning_included", "evaluation_included"};
        case C3EcoDeclarationKind::RAGPipeline:
            return {"embedding_model", "vector_db", "retrieval_top_k", "cache_policy"};
        case C3EcoDeclarationKind::TokenBudget:
            return {"input_tokens_p95", "output_tokens_p95", "cache_hit_rate_min_percent"};
        case C3EcoDeclarationKind::ModelRouting:
            return {"route", "fallback"};
        case C3EcoDeclarationKind::Guardrails:
            return {"functional_tests", "accuracy", "p95_latency_ms", "error_rate_percent", "security_scan", "accessibility", "privacy_telemetry"};
    }
    return {};
}

static std::set<std::string> c3EcoAllowedFields(C3EcoDeclarationKind kind) {
    std::set<std::string> allowed = c3EcoRequiredFields(kind);
    if (kind == C3EcoDeclarationKind::Boundary) {
        allowed.insert("exclude");
        allowed.insert("evidence");
    }
    return allowed;
}

static bool c3EcoHasField(const C3EcoDeclarationData &data, const std::string &name) {
    for (const auto &field : data.fields) if (field.name == name) return true;
    return false;
}

static std::string c3EcoFieldText(const C3EcoDeclarationData &data, const std::string &name) {
    std::string text;
    for (const auto &field : data.fields) {
        if (field.name != name) continue;
        for (const auto &value : field.values) {
            if (!text.empty()) text += " ";
            text += value;
        }
    }
    return text;
}

std::set<std::string> SemanticAnalyzer::functionNames(AST_PROGRAM *program) {
    std::set<std::string> names;
    if (program == nullptr || program->functions == nullptr) return names;
    for (AST_FUNCTION_RULE *function : program->functions->functions) {
        if (function != nullptr && function->function_name != nullptr)
            names.insert(function->function_name);
    }
    return names;
}

std::map<std::string, std::size_t> SemanticAnalyzer::functionArities(AST_PROGRAM *program) {
    std::map<std::string, std::size_t> arities;
    if (program == nullptr || program->functions == nullptr) return arities;
    for (AST_FUNCTION_RULE *function : program->functions->functions) {
        if (function == nullptr || function->function_name == nullptr || function->parameters == nullptr) continue;
        arities[function->function_name] =
            function->parameters->single_ints.size() + function->parameters->array_ints.size();
    }
    return arities;
}

std::map<std::string, SemanticAnalyzer::FunctionSignature>
SemanticAnalyzer::functionSignatures(AST_PROGRAM *program) {
    std::map<std::string, FunctionSignature> signatures;
    if (program == nullptr || program->functions == nullptr) return signatures;
    for (AST_FUNCTION_RULE *function : program->functions->functions) {
        if (function == nullptr || function->function_name == nullptr || function->parameters == nullptr) continue;
        FunctionSignature signature;
        signature.return_type = function->type;
        for (const auto &parameter : function->parameters->typed_scalars)
            signature.parameter_types.push_back(parameter.type);
        for (const auto &parameter : function->parameters->typed_arrays)
            signature.parameter_types.push_back(parameter.type);
        signatures[function->function_name] = std::move(signature);
    }
    return signatures;
}

std::set<std::string> SemanticAnalyzer::globalNames(AST_PROGRAM *program) {
    std::set<std::string> names;
    if (program == nullptr || program->decl_block == nullptr) return names;
    for (const std::string &name : program->decl_block->single_ints) names.insert(name);
    for (const auto &entry : program->decl_block->array_ints) names.insert(entry.first);
    return names;
}

void SemanticAnalyzer::setImportedFunctions(const std::map<std::string, std::size_t> &arities,
                                            bool allow_external_calls) {
    imported_functions.clear();
    for (const auto &entry : arities) {
        imported_functions.insert(entry.first);
        functions.insert(entry.first);
        function_arity[entry.first] = entry.second;
    }
    allow_external_function_calls = allow_external_calls;
}

void SemanticAnalyzer::setImportedFunctions(
    const std::map<std::string, FunctionSignature> &signatures,
    bool allow_external_calls) {
    imported_functions.clear();
    for (const auto &entry : signatures) {
        imported_functions.insert(entry.first);
        functions.insert(entry.first);
        function_arity[entry.first] = entry.second.parameter_types.size();
        function_return_types[entry.first] = entry.second.return_type;
        function_parameter_types[entry.first] = entry.second.parameter_types;
    }
    allow_external_function_calls = allow_external_calls;
}

bool SemanticAnalyzer::isExecutableScalarType(ShortType type) {
    return type == ShortType::Int || type == ShortType::Boolean ||
           type == ShortType::Float || type == ShortType::String;
}

bool SemanticAnalyzer::isDeclared(const std::string &name) const {
    for (auto it = local_scopes.rbegin(); it != local_scopes.rend(); ++it) {
        if (it->count(name) != 0U) return true;
    }
    return globals.count(name) != 0U;
}

bool SemanticAnalyzer::lookupType(const std::string &name,
                                  ShortType &type,
                                  bool &is_array) const {
    for (auto it = local_scopes.rbegin(); it != local_scopes.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) {
            type = found->second;
            is_array = false;
            return true;
        }
    }
    const auto scalar = global_types.find(name);
    if (scalar != global_types.end()) {
        type = scalar->second;
        is_array = false;
        return true;
    }
    const auto array = global_array_types.find(name);
    if (array != global_array_types.end()) {
        type = array->second;
        is_array = true;
        return true;
    }
    return false;
}

ShortType SemanticAnalyzer::expressionType(AST_EXPRESSION_RULE *expression) {
    if (expression == nullptr) return ShortType::Void;
    if (dynamic_cast<AST_LITERAL *>(expression) != nullptr) return ShortType::Int;
    if (dynamic_cast<AST_BOOL_LITERAL *>(expression) != nullptr) return ShortType::Boolean;
    if (dynamic_cast<AST_FLOAT_LITERAL *>(expression) != nullptr) return ShortType::Float;
    if (dynamic_cast<AST_STRING_LITERAL *>(expression) != nullptr) return ShortType::String;

    if (auto *variable = dynamic_cast<AST_SIMPLE_VARIABLE *>(expression)) {
        ShortType type = ShortType::Void;
        bool is_array = false;
        if (!lookupType(variable->variable_name, type, is_array)) {
            diagnostics.errorAtNode(variable, diag::SemanticUndeclaredVariable,
                                    "undeclared variable: " + variable->variable_name);
            return ShortType::Void;
        }
        if (is_array) {
            diagnostics.errorAtNode(variable, diag::SemanticTypeMismatch,
                                    "fixed array `" + variable->variable_name +
                                    "` requires an index before use as a scalar value");
            return ShortType::Void;
        }
        return type;
    }

    if (auto *array = dynamic_cast<AST_ARRAY_VARIABLE *>(expression)) {
        ShortType type = ShortType::Void;
        bool is_array = false;
        if (!lookupType(array->array_name, type, is_array)) {
            diagnostics.errorAtNode(array, diag::SemanticUndeclaredVariable,
                                    "undeclared array: " + array->array_name);
            return ShortType::Void;
        }
        if (!is_array) {
            diagnostics.errorAtNode(array, diag::SemanticTypeMismatch,
                                    "scalar `" + array->array_name + "` cannot be indexed");
            return ShortType::Void;
        }
        const ShortType index_type = expressionType(array->index);
        if (index_type != ShortType::Int) {
            diagnostics.errorAtNode(array->index, diag::SemanticTypeMismatch,
                                    "array index must have type int");
        }
        return type;
    }

    if (auto *unary = dynamic_cast<AST_UNARY_EXPRESSION_RULE *>(expression)) {
        const ShortType operand = expressionType(unary->expression);
        if (operand != ShortType::Int && operand != ShortType::Float) {
            diagnostics.errorAtNode(unary, diag::SemanticInvalidOperator,
                                    "unary minus requires int or float operand");
            return ShortType::Void;
        }
        return operand;
    }

    if (auto *binary = dynamic_cast<AST_BINARY_EXPRESSION_RULE *>(expression)) {
        const ShortType left = expressionType(binary->left);
        const ShortType right = expressionType(binary->right);
        if (left == ShortType::Void || right == ShortType::Void) return ShortType::Void;
        const int op = binary->op;
        const bool arithmetic = op == AST_BINARY_EXPRESSION_RULE::PLUS ||
                                op == AST_BINARY_EXPRESSION_RULE::MINUS ||
                                op == AST_BINARY_EXPRESSION_RULE::MULTIPLY ||
                                op == AST_BINARY_EXPRESSION_RULE::DIVIDE ||
                                op == AST_BINARY_EXPRESSION_RULE::MODULO;
        const bool equality = op == AST_BINARY_EXPRESSION_RULE::EQUAL ||
                              op == AST_BINARY_EXPRESSION_RULE::NOT_EQUAL;
        const bool ordering = op == AST_BINARY_EXPRESSION_RULE::LESS ||
                              op == AST_BINARY_EXPRESSION_RULE::GREATER ||
                              op == AST_BINARY_EXPRESSION_RULE::LESS_OR_EQUAL ||
                              op == AST_BINARY_EXPRESSION_RULE::GREATER_OR_EQUAL;
        const bool logical = op == AST_BINARY_EXPRESSION_RULE::OR ||
                             op == AST_BINARY_EXPRESSION_RULE::AND;
        if (left != right) {
            diagnostics.errorAtNode(binary, diag::SemanticTypeMismatch,
                                    std::string("operator operands must have the same type; left=") +
                                    shortTypeName(left) + " right=" + shortTypeName(right));
            return ShortType::Void;
        }
        if (arithmetic) {
            if (left != ShortType::Int && left != ShortType::Float) {
                diagnostics.errorAtNode(binary, diag::SemanticInvalidOperator,
                                        "arithmetic operators require int or float operands");
                return ShortType::Void;
            }
            if (op == AST_BINARY_EXPRESSION_RULE::MODULO && left != ShortType::Int) {
                diagnostics.errorAtNode(binary, diag::SemanticInvalidOperator,
                                        "modulo requires int operands");
                return ShortType::Void;
            }
            return left;
        }
        if (equality) return ShortType::Boolean;
        if (ordering) {
            if (left != ShortType::Int && left != ShortType::Float) {
                diagnostics.errorAtNode(binary, diag::SemanticInvalidOperator,
                                        "ordering operators require int or float operands");
                return ShortType::Void;
            }
            return ShortType::Boolean;
        }
        if (logical) {
            if (left != ShortType::Boolean && left != ShortType::Int) {
                diagnostics.errorAtNode(binary, diag::SemanticInvalidOperator,
                                        "logical operators require bool or int operands");
                return ShortType::Void;
            }
            return ShortType::Boolean;
        }
        diagnostics.errorAtNode(binary, diag::SemanticInvalidOperator,
                                "unknown expression operator");
        return ShortType::Void;
    }

    if (auto *call = dynamic_cast<AST_FUNCTION_CALL_EXPRESSION *>(expression)) {
        validateCallTypes(call, call->function_name, call->arguments);
        const auto result = function_return_types.find(call->function_name);
        return result == function_return_types.end() ? ShortType::Void : result->second;
    }
    diagnostics.errorAtNode(expression, diag::SemanticInvalidType,
                            "expression has no production type");
    return ShortType::Void;
}

bool SemanticAnalyzer::requireConditionType(const void *node, AST_EXPRESSION_RULE *expression) {
    const ShortType type = expressionType(expression);
    if (type == ShortType::Boolean || type == ShortType::Int) return true;
    diagnostics.errorAtNode(node, diag::SemanticInvalidCondition,
                            "condition must have type bool or int");
    return false;
}

void SemanticAnalyzer::validateCall(const void *node, const std::string &name, std::size_t arity) {
    auto signature = function_arity.find(name);
    if (name.empty() || signature == function_arity.end()) {
        diagnostics.errorAtNode(node, diag::LoweringUndefinedFunction,
                                "lowering cannot resolve function: " + name);
        return;
    }
    if (signature->second != arity) {
        diagnostics.errorAtNode(
            node, diag::SemanticFunctionArityMismatch,
            "function " + name + " expects " + std::to_string(signature->second) +
            " argument(s), got " + std::to_string(arity));
    }
    if (!allow_external_function_calls && imported_functions.count(name) != 0U) {
        diagnostics.errorAtNode(
            node, diag::ModuleExternalRunUnsupported,
            "interpreter execution of imported function is disabled by this execution profile: " + name);
    }
}

void SemanticAnalyzer::validateCallTypes(
    const void *node,
    const std::string &name,
    const std::vector<AST_EXPRESSION_RULE *> &arguments) {
    validateCall(node, name, arguments.size());
    const auto expected = function_parameter_types.find(name);
    if (expected == function_parameter_types.end() || expected->second.size() != arguments.size()) return;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const ShortType actual = expressionType(arguments[i]);
        if (actual != ShortType::Void && actual != expected->second[i]) {
            diagnostics.errorAtNode(
                arguments[i], diag::SemanticTypeMismatch,
                "function " + name + " argument " + std::to_string(i + 1) +
                " expects " + shortTypeName(expected->second[i]) +
                " but received " + shortTypeName(actual));
        }
    }
}

int SemanticAnalyzer::visit(AST_PROGRAM *p) {
    globals = globalNames(p);
    global_types.clear();
    global_array_types.clear();
    if (p != nullptr && p->decl_block != nullptr) {
        for (const auto &entry : p->decl_block->typed_scalars) global_types[entry.name] = entry.type;
        for (const auto &entry : p->decl_block->typed_arrays) global_array_types[entry.name] = entry.type;
    }
    if (p->decl_block) p->decl_block->accept(*this);
    if (p->functions) p->functions->accept(*this);
    if (p->code_block) p->code_block->accept(*this);
    return diagnostics.hasErrors() ? 1 : 0;
}

int SemanticAnalyzer::visit(AST_DATA_DECLARATION_BLOCK *block) {
    if (block == nullptr) return 0;
    std::set<std::string> names;
    for (const auto &entry : block->typed_scalars) {
        if (!names.insert(entry.name).second) {
            diagnostics.errorAtNode(block, diag::SemanticInvalidType,
                                    "duplicate declaration: " + entry.name);
        }
        if (!isExecutableScalarType(entry.type)) {
            diagnostics.errorAtNode(
                block, diag::SemanticUnsupportedExecutableType,
                std::string("production executable semantics do not accept ") +
                shortTypeName(entry.type) + " declaration `" + entry.name + "`");
        }
        std::string type_diagnostic;
        const auto descriptor = shorthand::types::TypeDescriptor::scalar(productionTypeKind(entry.type));
        if (!descriptor.validate(type_diagnostic))
            diagnostics.errorAtNode(block, diag::SemanticInvalidType, type_diagnostic);
    }
    for (const auto &entry : block->typed_arrays) {
        if (!names.insert(entry.name).second) {
            diagnostics.errorAtNode(block, diag::SemanticInvalidType,
                                    "duplicate declaration: " + entry.name);
        }
        if (!isExecutableScalarType(entry.type)) {
            diagnostics.errorAtNode(
                block, diag::SemanticUnsupportedExecutableType,
                std::string("production executable semantics do not accept ") +
                shortTypeName(entry.type) + " array `" + entry.name + "`");
        }
        if (entry.type == ShortType::String) {
            diagnostics.errorAtNode(block, diag::SemanticUnsupportedExecutableType,
                                    "string arrays require owned element destruction and are reserved by shorthand.type_memory.v1");
        }
        std::string type_diagnostic;
        const auto descriptor = shorthand::types::TypeDescriptor::array(
            productionTypeKind(entry.type), entry.size < 0 ? 0U : static_cast<std::size_t>(entry.size));
        if (!descriptor.validate(type_diagnostic))
            diagnostics.errorAtNode(block, diag::SemanticInvalidType, type_diagnostic);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_LIST_RULE *f) {
    std::set<std::string> local_names;
    for (auto *function : f->functions) {
        if (function == nullptr || function->function_name == nullptr) continue;
        const std::string name(function->function_name);
        if (!local_names.insert(name).second || imported_functions.count(name) != 0U) {
            diagnostics.errorAtNode(function, diag::SemanticDuplicateFunction,
                                    "duplicate function definition: " + name);
        }
        functions.insert(name);
        const std::size_t arity = function->parameters == nullptr ? 0U :
            function->parameters->single_ints.size() + function->parameters->array_ints.size();
        function_arity[name] = arity;
        function_return_types[name] = function->type;
        std::vector<ShortType> parameter_types;
        if (function->parameters != nullptr) {
            for (const auto &parameter : function->parameters->typed_scalars)
                parameter_types.push_back(parameter.type);
            for (const auto &parameter : function->parameters->typed_arrays)
                parameter_types.push_back(parameter.type);
        }
        function_parameter_types[name] = parameter_types;
    }
    for (auto *function : f->functions) {
        if (function) function->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_RULE *n) {
    if (n == nullptr) return 0;
    if (n->parameters) {
        n->parameters->accept(*this);
        if (!n->parameters->array_ints.empty()) {
            diagnostics.errorAtNode(
                n, diag::SemanticUnsupportedExecutableType,
                "array parameters are not part of the beta-0.4 executable function contract");
        }
    }

    std::map<std::string, ShortType> scope;
    if (n->parameters) {
        for (const auto &entry : n->parameters->typed_scalars) scope[entry.name] = entry.type;
        for (const auto &entry : n->parameters->typed_arrays) scope[entry.name] = entry.type;
    }
    local_scopes.push_back(scope);
    return_types.push_back(n->type);
    const int saved_loop_depth = loopDepth;
    loopDepth = 0;
    if (n->block_statement) n->block_statement->accept(*this);
    loopDepth = saved_loop_depth;
    return_types.pop_back();
    local_scopes.pop_back();
    return 0;
}

int SemanticAnalyzer::visit(AST_LOGIC_BLOCK *b) {
    if (b->block_statement) b->block_statement->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_STATEMENTS_BLOCK *b) {
    for (auto *statement : b->statements) if (statement) statement->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_EXPRESSION_STATEMENT_RULE *n) {
    if (n->expression) n->expression->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_CALL_RULE *n) {
    const std::string name = n->function_name == nullptr ? std::string() : std::string(n->function_name);
    std::vector<AST_EXPRESSION_RULE *> arguments;
    if (n->parameters) {
        for (AST_VARIABLE_RULE *argument : n->parameters->variables) arguments.push_back(argument);
    }
    validateCallTypes(n, name, arguments);
    return 0;
}

int SemanticAnalyzer::visit(AST_ASSIGNMENT_RULE *n) {
    const ShortType target = expressionType(n->variable);
    const ShortType source = expressionType(n->expression);
    if (target != ShortType::Void && source != ShortType::Void && target != source) {
        diagnostics.errorAtNode(
            n, diag::SemanticTypeMismatch,
            std::string("assignment requires identical types; target=") + shortTypeName(target) +
            " source=" + shortTypeName(source) +
            ". Use an explicit checked conversion when conversion syntax is available.");
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_IF_STATEMENT *n) {
    requireConditionType(n, n->condition);
    if (n->if_block) n->if_block->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_IF_ELSE_STATEMENT *n) {
    requireConditionType(n, n->condition);
    if (n->if_block) n->if_block->accept(*this);
    if (n->else_block) n->else_block->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_MODEL_DECLARATION *n) {
    auto &d = n->data;
    if (models.count(d.name))
        diagnostics.errorAtNode(n, diag::AIModelRedeclared, "model redeclared: " + d.name);
    models[d.name] = d;

    auto format = shorthand::ai::parseModelFormat(d.format);
    if (d.format.empty() || format == shorthand::ai::ModelFormat::Unknown)
        diagnostics.errorAtNode(n, diag::AIModelInvalidFormat, "model " + d.name + " missing or invalid format");
    if (shorthand::ai::parseElementType(d.precision) == shorthand::ai::ElementType::Unknown)
        diagnostics.errorAtNode(n, diag::AIModelInvalidPrecision, "model " + d.name + " has invalid precision");
    if (d.input_shape.empty() || !validShape(d.input_shape))
        diagnostics.errorAtNode(n, diag::AIModelInvalidInputShape, "model " + d.name + " has invalid input_shape");
    if (!d.output_shape.empty() && !validShape(d.output_shape))
        diagnostics.errorAtNode(n, diag::AIModelInvalidOutputShape, "model " + d.name + " has invalid output_shape");
    if (!d.has_quality_guardrail)
        diagnostics.errorAtNode(n, diag::AIModelMissingQualityGuardrail, "model " + d.name + " requires quality_guardrail");

    bool hasFallback = false;
    bool hasCompatibleRealBackend = false;
    for (const auto &backend : d.backend_preference) {
        auto kind = shorthand::ai::parseBackendKind(backend);
        hasFallback = hasFallback || kind == shorthand::ai::BackendKind::Fallback;
        if (kind == shorthand::ai::BackendKind::Fallback) continue;
        if (backendCompatibleWithFormat(format, kind)) {
            hasCompatibleRealBackend = true;
        } else {
            diagnostics.warningAtNode(
                n, diag::AIModelIncompatibleBackend,
                "model " + d.name + " backend_preference " + backend +
                " is not compatible with format " + d.format);
        }
    }
    if (d.backend_preference.empty())
        diagnostics.warningAtNode(
            n, diag::AIModelMissingBackendPreference,
            "model " + d.name + " has no backend_preference; fallback will be used if allowed");
    if (!d.backend_preference.empty() && !hasCompatibleRealBackend && !hasFallback)
        diagnostics.errorAtNode(
            n, diag::AIModelNoCompatibleBackend,
            "model " + d.name + " has no compatible backend_preference for format " + d.format);
    return 0;
}

int SemanticAnalyzer::visit(AST_TENSOR_DECLARATION *n) {
    if (tensors.count(n->data.name))
        diagnostics.errorAtNode(n, diag::AITensorRedeclared, "tensor redeclared: " + n->data.name);
    tensors[n->data.name] = n->data;
    if (!validShape(n->data.shape_csv))
        diagnostics.errorAtNode(n, diag::AITensorInvalidShape, "invalid tensor shape for " + n->data.name);
    return 0;
}

int SemanticAnalyzer::visit(AST_GREENAI_CONTRACT *n) {
    auto &d = n->data;
    contracts[d.name] = d;
    if (!d.has_functional_unit)
        diagnostics.errorAtNode(n, diag::GreenAIMissingFunctionalUnit, "greenai_contract " + d.name + " missing functional_unit");
    if (!d.has_success_criteria)
        diagnostics.errorAtNode(n, diag::GreenAIMissingSuccessCriteria, "greenai_contract " + d.name + " missing success_criteria");
    if (!d.has_boundary)
        diagnostics.errorAtNode(n, diag::GreenAIMissingBoundary, "greenai_contract " + d.name + " missing boundary");
    if (!d.has_mq || !d.has_dq)
        diagnostics.errorAtNode(n, diag::GreenAIMissingMeasurementOrDataQuality, "greenai_contract " + d.name + " requires MQ/DQ");
    if (!d.has_carbon_factor || d.carbon_factor <= 0)
        diagnostics.errorAtNode(n, diag::GreenAIInvalidCarbonFactor, "greenai_contract " + d.name + " requires positive carbon_factor");
    if (!d.has_quality_guardrail)
        diagnostics.errorAtNode(n, diag::GreenAIMissingQualityGuardrail, "greenai_contract " + d.name + " requires quality_guardrail");
    if (d.claims_mode != "evidence_only")
        diagnostics.errorAtNode(n, diag::GreenAIInvalidClaimsMode, "claims_mode must be evidence_only");
    if (d.energy_budget_j < 0 || d.carbon_budget_gco2e < 0)
        diagnostics.errorAtNode(n, diag::GreenAIInvalidBudget, "budgets must be non-negative");
    return 0;
}

int SemanticAnalyzer::visit(AST_GREENAI_MEASUREMENT *n) {
    if (!contracts.empty() && !contracts.count(n->data.workload))
        diagnostics.errorAtNode(
            n, diag::GreenAIMeasureUnknownContract,
            "greenai_measure references unknown contract: " + n->data.workload);
    if (!models.empty() && !models.count(n->data.backend))
        diagnostics.warningAtNode(
            n, diag::GreenAIMeasureExternalBackend,
            "greenai_measure backend " + n->data.backend +
            " is not a declared model; treating as external measurement source");
    return 0;
}

int SemanticAnalyzer::visit(AST_C3ECO_DECLARATION *n) {
    const auto &data = n->data;
    const std::string kind = c3EcoDeclarationKindName(data.kind);
    const std::string key = kind + ":" + data.name;
    if (!c3eco_declarations.insert(key).second) {
        diagnostics.errorAtNode(n, diag::C3EcoDuplicateDeclaration,
                                "duplicate C3-ECO declaration: " + key);
    }

    const std::set<std::string> required = c3EcoRequiredFields(data.kind);
    const std::set<std::string> allowed = c3EcoAllowedFields(data.kind);
    static const std::set<std::string> unsafe = {
        "official_certification_granted", "certified_level", "certificate_id", "certified"};

    for (const auto &field : data.fields) {
        if (unsafe.count(field.name) != 0U) {
            diagnostics.errorAtNode(
                n, diag::C3EcoUnsafeCertificationClaim,
                "C3-ECO declaration must not self-assert certification field `" + field.name + "`");
            continue;
        }
        if (allowed.count(field.name) == 0U) {
            diagnostics.errorAtNode(
                n, diag::C3EcoInvalidField,
                "invalid field `" + field.name + "` for C3-ECO " + kind + " declaration");
        }
    }

    for (const auto &field : required) {
        if (!c3EcoHasField(data, field)) {
            diagnostics.errorAtNode(
                n, diag::C3EcoMissingRequiredField,
                "C3-ECO " + kind + " declaration `" + data.name + "` missing required field `" + field + "`");
        }
    }

    if (data.kind == C3EcoDeclarationKind::MeasurementPlan && c3EcoHasField(data, "carbon_factor")) {
        const std::string carbon = c3EcoFieldText(data, "carbon_factor");
        if (carbon.find("source") == std::string::npos || carbon.find("unit") == std::string::npos) {
            diagnostics.errorAtNode(
                n, diag::C3EcoInvalidField,
                "measurement_plan carbon_factor must name both source and unit");
        }
    }

    if (data.kind == C3EcoDeclarationKind::Boundary && c3EcoHasField(data, "exclude")) {
        const std::string evidence = c3EcoFieldText(data, "evidence");
        if (evidence.find("component") == std::string::npos ||
            evidence.find("reason") == std::string::npos ||
            evidence.find("materiality") == std::string::npos) {
            diagnostics.errorAtNode(
                n, diag::C3EcoInvalidField,
                "boundary exclusions require evidence naming component, reason and materiality");
        }
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_INFER_STATEMENT *n) {
    if (!models.count(n->model_name))
        diagnostics.errorAtNode(n, diag::AIInferUnknownModel, "infer references unknown model: " + n->model_name);
    if (!tensors.empty() && !tensors.count(n->input_name))
        diagnostics.errorAtNode(n, diag::AIInferUnknownInputTensor, "infer references unknown input tensor: " + n->input_name);
    if (models.count(n->model_name) && tensors.count(n->input_name)) {
        const auto &model = models[n->model_name];
        const auto &tensor = tensors[n->input_name];
        if (!shapeCompatible(model.input_shape, tensor.shape_csv))
            diagnostics.errorAtNode(
                n, diag::AIInferInputShapeMismatch,
                "infer input tensor shape " + tensor.shape_csv + " does not match model " +
                model.name + " input_shape " + model.input_shape);
        if (tensors.count(n->output_name)) {
            const auto &outTensor = tensors[n->output_name];
            if (!model.output_shape.empty() && !shapeCompatible(model.output_shape, outTensor.shape_csv))
                diagnostics.errorAtNode(
                    n, diag::AIInferOutputShapeMismatch,
                    "infer output tensor shape " + outTensor.shape_csv + " does not match model " +
                    model.name + " output_shape " + model.output_shape);
        } else {
            diagnostics.warningAtNode(
                n, diag::AIInferImplicitOutput,
                "infer output " + n->output_name +
                " is implicit; declare a tensor to enable output_shape validation");
        }
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_BREAK *n) {
    if (loopDepth == 0)
        diagnostics.errorAtNode(n, diag::SemanticBreakOutsideLoop, "break outside loop");
    return 0;
}

int SemanticAnalyzer::visit(AST_CONTINUE *n) {
    if (loopDepth == 0)
        diagnostics.errorAtNode(n, diag::SemanticContinueOutsideLoop, "continue outside loop");
    return 0;
}

int SemanticAnalyzer::visit(AST_FOR_LOOP_STATEMENT_RULE *n) {
    const ShortType variable = expressionType(n->variable);
    const ShortType from = expressionType(n->from);
    const ShortType step = expressionType(n->step);
    const ShortType to = expressionType(n->to);
    if (variable != ShortType::Int || from != ShortType::Int ||
        step != ShortType::Int || to != ShortType::Int) {
        diagnostics.errorAtNode(n, diag::SemanticTypeMismatch,
                                "counted loop variable, start, step and bound must all have type int");
    }
    loopDepth++;
    if (n->for_block) n->for_block->accept(*this);
    loopDepth--;
    return 0;
}

int SemanticAnalyzer::visit(AST_WHILE_LOOP_STATEMENT_RULE *n) {
    requireConditionType(n, n->condition);
    loopDepth++;
    if (n->while_block) n->while_block->accept(*this);
    loopDepth--;
    return 0;
}

int SemanticAnalyzer::visit(AST_GOTO_STATEMENT_RULE *n) {
    diagnostics.errorAtNode(
        n, diag::SemanticGotoUnsupported,
        "goto is parser-valid but not executable until interpreter and LLVM jump semantics are identical");
    if (n->condition) n->condition->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_READ_RULE *n) {
    for (auto *variable : n->variables) {
        const ShortType type = expressionType(variable);
        if (type == ShortType::String) {
            diagnostics.errorAtNode(variable, diag::SemanticUnsupportedExecutableType,
                                    "read into string is unavailable until a bounded input-capacity syntax is declared");
        }
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_PRINT_RULE *n) {
    for (auto &item : n->printables) {
        if (item.expression) expressionType(item.expression);
        else if (item.string_literal) expressionType(item.string_literal);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_LABEL_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_GREENAI_REPORT_RULE *n) {
    const ShortType inferences = expressionType(n->inferences);
    const ShortType watts = expressionType(n->watts);
    const ShortType seconds = expressionType(n->seconds);
    if (inferences != ShortType::Int || watts != ShortType::Int || seconds != ShortType::Int) {
        diagnostics.errorAtNode(
            n, diag::SemanticTypeMismatch,
            "greenai report inferences, watts and seconds require exact int values; implicit measurement conversion is forbidden");
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_AI_INFER_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_RETURN_STATEMENT *n) {
    if (return_types.empty()) {
        diagnostics.errorAtNode(n, diag::SemanticReturnOutsideFunction, "return outside function");
        if (n->expression) expressionType(n->expression);
        return 0;
    }
    if (return_types.back() == ShortType::Void && n->expression != nullptr) {
        diagnostics.errorAtNode(n, diag::SemanticReturnTypeMismatch,
                                "void function cannot return a value");
    }
    if (n->expression && return_types.back() != ShortType::Void) {
        const ShortType actual = expressionType(n->expression);
        if (actual != ShortType::Void && actual != return_types.back()) {
            diagnostics.errorAtNode(
                n, diag::SemanticReturnTypeMismatch,
                std::string("return expression has type ") + shortTypeName(actual) +
                " but function returns " + shortTypeName(return_types.back()));
        }
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_BINARY_EXPRESSION_RULE *n) {
    expressionType(n);
    return 0;
}

int SemanticAnalyzer::visit(AST_UNARY_EXPRESSION_RULE *n) {
    expressionType(n);
    return 0;
}

int SemanticAnalyzer::visit(AST_SIMPLE_VARIABLE *n) {
    expressionType(n);
    return 0;
}

int SemanticAnalyzer::visit(AST_ARRAY_VARIABLE *n) {
    expressionType(n);
    return 0;
}

int SemanticAnalyzer::visit(AST_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_STRING_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_BOOL_LITERAL*) { return 0; }

int SemanticAnalyzer::visit(AST_FLOAT_LITERAL *) { return 0; }

int SemanticAnalyzer::visit(AST_FUNCTION_CALL_EXPRESSION *n) {
    validateCallTypes(n, n->function_name, n->arguments);
    return 0;
}
