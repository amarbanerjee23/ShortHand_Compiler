#include "SemanticAnalyzer.h"
#include "DiagnosticCodes.h"
#include "../ai_runtime/AI_Types.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
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
        case C3EcoDeclarationKind::CertificationProfile:
            return {"profile_version", "certification", "functional_unit", "workload", "boundary",
                    "ai_lifecycle", "guardrails", "valid_from", "valid_until"};
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
    if (kind == C3EcoDeclarationKind::Certification) {
        allowed.insert("release_date");
        allowed.insert("cloud_region");
    }
    if (kind == C3EcoDeclarationKind::FunctionalUnit) {
        allowed.insert("unit");
        allowed.insert("quality_metric");
        allowed.insert("latency_slo_ms");
        allowed.insert("error_rate_slo_percent");
    }
    if (kind == C3EcoDeclarationKind::Boundary) {
        allowed.insert("exclude");
        allowed.insert("evidence");
        allowed.insert("exclusion_reason");
        allowed.insert("exclusion_materiality_percent");
        allowed.insert("materiality_threshold_percent");
        allowed.insert("opaque_provider_treatment");
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
            text += value.text;
        }
    }
    return text;
}

static const C3EcoFieldData *c3EcoField(const C3EcoDeclarationData &data,
                                       const std::string &name) {
    for (const auto &field : data.fields) {
        if (field.name == name) return &field;
    }
    return nullptr;
}

static bool c3EcoParseInteger(const std::string &text, long long &value) {
    if (text.empty()) return false;
    errno = 0;
    char *end = nullptr;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') return false;
    value = parsed;
    return true;
}

static bool c3EcoParseDecimal(const std::string &text, double &value) {
    if (text.empty()) return false;
    errno = 0;
    char *end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (errno != 0 || end == text.c_str() || *end != '\0' || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}

static bool c3EcoLeapYear(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static bool c3EcoIsoDate(const std::string &text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
    }
    const int year = std::atoi(text.substr(0, 4).c_str());
    const int month = std::atoi(text.substr(5, 2).c_str());
    const int day = std::atoi(text.substr(8, 2).c_str());
    if (year < 2000 || month < 1 || month > 12 || day < 1) return false;
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const int limit = month == 2 && c3EcoLeapYear(year) ? 29 : month_days[month - 1];
    return day <= limit;
}

static bool c3EcoSemver(const std::string &text) {
    int dots = 0;
    bool digit_in_part = false;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            digit_in_part = true;
        } else if (c == '.' && digit_in_part && dots < 2) {
            ++dots;
            digit_in_part = false;
        } else {
            return false;
        }
    }
    return dots == 2 && digit_in_part;
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
            type = found->second.type;
            is_array = found->second.is_array;
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
        if (left == ShortType::Void || right == ShortType::Void) {
            diagnostics.errorAtNode(binary, diag::SemanticVoidValueUsed,
                                    "binary operator requires value operands and cannot consume void");
            return ShortType::Void;
        }
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

bool SemanticAnalyzer::definitelyReturns(AST_STATEMENT_RULE *statement) const {
    if (statement == nullptr) return false;
    if (dynamic_cast<AST_RETURN_STATEMENT *>(statement) != nullptr) return true;
    if (auto *conditional = dynamic_cast<AST_IF_ELSE_STATEMENT *>(statement)) {
        return definitelyReturns(conditional->if_block) && definitelyReturns(conditional->else_block);
    }
    if (auto *block = dynamic_cast<AST_STATEMENTS_BLOCK *>(statement)) {
        const bool has_direct_transfer_target = std::any_of(
            block->statements.begin(), block->statements.end(),
            [](AST_STATEMENT_RULE *child) {
                return dynamic_cast<AST_LABEL_RULE *>(child) != nullptr ||
                       dynamic_cast<AST_GOTO_STATEMENT_RULE *>(child) != nullptr;
            });
        if (has_direct_transfer_target) {
            // A label can make statements after an earlier return reachable. Keep
            // the proof deliberately conservative for blocks with unstructured
            // transfers: their final statement must itself close every path.
            return !block->statements.empty() && definitelyReturns(block->statements.back());
        }
        for (AST_STATEMENT_RULE *child : block->statements) {
            if (definitelyReturns(child)) return true;
        }
    }
    return false;
}

void SemanticAnalyzer::declareInCurrentScope(AST_DATA_DECLARATION_BLOCK *block) {
    if (block == nullptr || local_scopes.empty()) return;
    auto &scope = local_scopes.back();
    for (const auto &entry : block->typed_scalars) {
        if (!scope.emplace(entry.name, VariableInfo{entry.type, false}).second) {
            diagnostics.errorAtNode(block, diag::SemanticDuplicateDeclaration,
                                    "duplicate declaration in lexical scope: " + entry.name);
        }
    }
    for (const auto &entry : block->typed_arrays) {
        if (!scope.emplace(entry.name, VariableInfo{entry.type, true}).second) {
            diagnostics.errorAtNode(block, diag::SemanticDuplicateDeclaration,
                                    "duplicate declaration in lexical scope: " + entry.name);
        }
    }
}

void SemanticAnalyzer::validateCall(const void *node, const std::string &name, std::size_t arity) {
    auto signature = function_arity.find(name);
    if (name.empty() || signature == function_arity.end()) {
        diagnostics.errorAtNode(node, diag::SemanticUndefinedFunction,
                                "undefined function: " + name);
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
        if (actual == ShortType::Void) {
            diagnostics.errorAtNode(
                arguments[i], diag::SemanticVoidValueUsed,
                "function " + name + " argument " + std::to_string(i + 1) +
                " requires a value but received void");
        } else if (actual != expected->second[i]) {
            diagnostics.errorAtNode(
                arguments[i], diag::SemanticTypeMismatch,
                "function " + name + " argument " + std::to_string(i + 1) +
                " expects " + shortTypeName(expected->second[i]) +
                " but received " + shortTypeName(actual));
        }
    }
}

int SemanticAnalyzer::visit(AST_PROGRAM *p) {
    c3eco_declarations.clear();
    c3eco_profile_nodes.clear();
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
    validateC3EcoProfiles();
    return diagnostics.hasErrors() ? 1 : 0;
}

int SemanticAnalyzer::visit(AST_DATA_DECLARATION_BLOCK *block) {
    if (block == nullptr) return 0;
    std::set<std::string> names;
    for (const auto &entry : block->typed_scalars) {
        if (local_scopes.empty() && !names.insert(entry.name).second) {
            diagnostics.errorAtNode(block, diag::SemanticDuplicateDeclaration,
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
        if (local_scopes.empty() && !names.insert(entry.name).second) {
            diagnostics.errorAtNode(block, diag::SemanticDuplicateDeclaration,
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
    declareInCurrentScope(block);
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

    std::map<std::string, VariableInfo> scope;
    if (n->parameters) {
        for (const auto &entry : n->parameters->typed_scalars)
            scope.emplace(entry.name, VariableInfo{entry.type, false});
        for (const auto &entry : n->parameters->typed_arrays)
            scope.emplace(entry.name, VariableInfo{entry.type, true});
    }
    local_scopes.push_back(scope);
    return_types.push_back(n->type);
    const int saved_loop_depth = loopDepth;
    loopDepth = 0;
    if (n->block_statement) n->block_statement->accept(*this);
    if (n->type != ShortType::Void && !definitelyReturns(n->block_statement)) {
        diagnostics.errorAtNode(
            n, diag::SemanticMissingReturn,
            std::string("non-void function `") +
            (n->function_name == nullptr ? "" : n->function_name) +
            "` does not return a value on every structured path");
    }
    loopDepth = saved_loop_depth;
    return_types.pop_back();
    local_scopes.pop_back();
    return 0;
}

int SemanticAnalyzer::visit(AST_LOGIC_BLOCK *b) {
    local_scopes.push_back({});
    if (b->block_statement) b->block_statement->accept(*this);
    local_scopes.pop_back();
    return 0;
}

int SemanticAnalyzer::visit(AST_STATEMENTS_BLOCK *b) {
    if (b == nullptr) return 0;
    if (b->lexical_scope) local_scopes.push_back({});

    std::set<std::string> labels;
    std::map<std::string, std::size_t> label_positions;
    for (std::size_t i = 0; i < b->statements.size(); ++i) {
        auto *label = dynamic_cast<AST_LABEL_RULE *>(b->statements[i]);
        if (label == nullptr) continue;
        if (!labels.insert(label->label).second) {
            diagnostics.errorAtNode(label, diag::SemanticDuplicateLabel,
                                    "duplicate label in lexical block: " + label->label);
        } else {
            label_positions[label->label] = i;
        }
    }
    block_label_scopes.push_back(labels);

    for (std::size_t i = 0; i < b->statements.size(); ++i) {
        auto *jump = dynamic_cast<AST_GOTO_STATEMENT_RULE *>(b->statements[i]);
        const auto target = jump == nullptr ? label_positions.end() : label_positions.find(jump->label);
        if (target != label_positions.end()) {
            const std::size_t low = std::min(i, target->second);
            const std::size_t high = std::max(i, target->second);
            for (std::size_t k = low + 1; k < high; ++k) {
                if (dynamic_cast<AST_DATA_DECLARATION_BLOCK *>(b->statements[k]) != nullptr) {
                    diagnostics.errorAtNode(
                        jump, diag::SemanticInvalidGotoScope,
                        "goto cannot cross a lexical declaration boundary: " + jump->label);
                    break;
                }
            }
        }
        if (b->statements[i]) b->statements[i]->accept(*this);
    }

    block_label_scopes.pop_back();
    if (b->lexical_scope) local_scopes.pop_back();
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
    if (target != ShortType::Void && source == ShortType::Void) {
        diagnostics.errorAtNode(n->expression, diag::SemanticVoidValueUsed,
                                "assignment source requires a value but received void");
    } else if (target != ShortType::Void && target != source) {
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
    c3eco_profile_nodes.push_back(n);
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

    if (data.kind == C3EcoDeclarationKind::Boundary && c3EcoHasField(data, "exclude") &&
        !c3EcoHasField(data, "exclusion_reason")) {
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

void SemanticAnalyzer::validateC3EcoTypedDeclaration(AST_C3ECO_DECLARATION *node) {
    if (node == nullptr) return;
    const auto &data = node->data;
    const std::string declaration = std::string(c3EcoDeclarationKindName(data.kind)) +
                                    " `" + data.name + "`";

    auto single = [&](const std::string &name, C3EcoValueKind kind) -> const C3EcoValueData * {
        const C3EcoFieldData *field = c3EcoField(data, name);
        if (field == nullptr) return nullptr;
        if (field->values.size() != 1U || field->values.front().kind != kind) {
            diagnostics.errorAtNode(
                node, diag::C3EcoInvalidTypedLiteral,
                declaration + " field `" + name + "` requires exactly one " +
                    c3EcoValueKindName(kind) + " literal");
            return nullptr;
        }
        return &field->values.front();
    };
    auto requireV2Field = [&](const std::string &name) {
        if (!c3EcoHasField(data, name)) {
            diagnostics.errorAtNode(
                node, diag::C3EcoProfileIncomplete,
                declaration + " is linked by profile v2 and requires field `" + name + "`");
        }
    };
    auto enumValue = [&](const std::string &name, const std::set<std::string> &allowed) {
        const C3EcoValueData *value = single(name, C3EcoValueKind::Identifier);
        if (value != nullptr && allowed.count(value->text) == 0U) {
            diagnostics.errorAtNode(
                node, diag::C3EcoInvalidDomainValue,
                declaration + " field `" + name + "` has unsupported domain value `" +
                    value->text + "`");
        }
    };
    auto integerRange = [&](const std::string &name, long long minimum, long long maximum) {
        const C3EcoValueData *value = single(name, C3EcoValueKind::Integer);
        if (value == nullptr) return;
        long long parsed = 0;
        if (!c3EcoParseInteger(value->text, parsed)) {
            diagnostics.errorAtNode(node, diag::C3EcoInvalidTypedLiteral,
                                    declaration + " field `" + name + "` is not a valid integer");
        } else if (parsed < minimum || parsed > maximum) {
            diagnostics.errorAtNode(
                node, diag::C3EcoValueOutOfRange,
                declaration + " field `" + name + "` is outside the allowed range");
        }
    };
    auto decimalRange = [&](const std::string &name, double minimum, double maximum) {
        const C3EcoValueData *value = single(name, C3EcoValueKind::Decimal);
        if (value == nullptr) return;
        double parsed = 0.0;
        if (!c3EcoParseDecimal(value->text, parsed)) {
            diagnostics.errorAtNode(node, diag::C3EcoInvalidTypedLiteral,
                                    declaration + " field `" + name + "` is not a valid decimal");
        } else if (parsed < minimum || parsed > maximum) {
            diagnostics.errorAtNode(
                node, diag::C3EcoValueOutOfRange,
                declaration + " field `" + name + "` is outside the allowed range");
        }
    };
    auto nonEmptyString = [&](const std::string &name) -> const C3EcoValueData * {
        const C3EcoValueData *value = single(name, C3EcoValueKind::String);
        if (value != nullptr && value->text.empty()) {
            diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                    declaration + " field `" + name + "` must not be empty");
        }
        return value;
    };
    auto requiredBoolean = [&](const std::string &name) {
        const C3EcoValueData *value = single(name, C3EcoValueKind::Boolean);
        if (value != nullptr && value->text != "true") {
            diagnostics.errorAtNode(
                node, diag::C3EcoInvalidDomainValue,
                declaration + " safeguard field `" + name + "` must be true in profile v2");
        }
    };

    switch (data.kind) {
        case C3EcoDeclarationKind::CertificationProfile:
            integerRange("profile_version", 2, 2);
            nonEmptyString("valid_from");
            nonEmptyString("valid_until");
            break;
        case C3EcoDeclarationKind::Certification: {
            const C3EcoValueData *version = nonEmptyString("version");
            nonEmptyString("owner");
            const C3EcoValueData *geography = nonEmptyString("geography");
            requireV2Field("release_date");
            const C3EcoValueData *release = nonEmptyString("release_date");
            if (c3EcoHasField(data, "cloud_region")) nonEmptyString("cloud_region");
            if (version != nullptr && !c3EcoSemver(version->text)) {
                diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                        declaration + " version must use numeric MAJOR.MINOR.PATCH form");
            }
            if (release != nullptr && !c3EcoIsoDate(release->text)) {
                diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                        declaration + " release_date must be a valid YYYY-MM-DD date");
            }
            if (geography != nullptr) {
                const std::string &code = geography->text;
                const bool valid = (code.size() == 2U || code.size() == 3U) &&
                    std::all_of(code.begin(), code.end(), [](char c) {
                        return std::isupper(static_cast<unsigned char>(c));
                    });
                if (!valid) {
                    diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                            declaration + " geography must be a two- or three-letter uppercase code");
                }
            }
            enumValue("software_class", {"S1", "S2", "S3", "S4", "S5", "S6", "S7",
                                             "S8", "S9", "S10", "S11", "S12", "S13", "S14",
                                             "S6_AI_GENAI", "S9_DEVELOPER_TOOLS_CI_CD",
                                             "S12_INFRASTRUCTURE_PLATFORM"});
            enumValue("deployment_mode", {"kubernetes", "container", "vm", "bare_metal",
                                             "edge", "serverless", "hybrid"});
            integerRange("validity_period", 1, 366);
            break;
        }
        case C3EcoDeclarationKind::FunctionalUnit:
            requireV2Field("unit");
            requireV2Field("quality_metric");
            integerRange("denominator", 1, 1000000000000LL);
            nonEmptyString("success_condition");
            decimalRange("quality_threshold", 0.0, 1.0);
            enumValue("unit", {"inference", "successful_inference", "request", "successful_request",
                                  "training_run", "token", "document", "gb_processed", "build"});
            enumValue("quality_metric", {"accuracy", "f1", "precision", "recall", "pass_rate",
                                            "quality_score"});
            if (c3EcoHasField(data, "latency_slo_ms")) integerRange("latency_slo_ms", 1, 86400000);
            if (c3EcoHasField(data, "error_rate_slo_percent"))
                decimalRange("error_rate_slo_percent", 0.0, 100.0);
            break;
        case C3EcoDeclarationKind::Workload:
            enumValue("traffic_profile", {"production_representative", "steady", "bursty", "trace_replay"});
            integerRange("batch_size", 1, 1000000);
            integerRange("concurrency", 1, 1000000);
            integerRange("warmup_runs", 0, 1000000000);
            integerRange("measured_runs", 1, 1000000000);
            enumValue("cache_state", {"cold", "warm", "mixed", "disabled", "declared"});
            break;
        case C3EcoDeclarationKind::Boundary: {
            requireV2Field("materiality_threshold_percent");
            const std::set<std::string> components = {
                "compute", "accelerator", "memory", "storage", "network", "ci_cd", "thirdparty",
                "thirdparty_ai_api", "client_device", "training", "fine_tuning"};
            const C3EcoFieldData *include = c3EcoField(data, "include");
            if (include != nullptr) {
                for (const auto &value : include->values) {
                    if (value.kind != C3EcoValueKind::Identifier) {
                        diagnostics.errorAtNode(node, diag::C3EcoInvalidTypedLiteral,
                                                declaration + " include entries must be identifiers");
                    } else if (components.count(value.text) == 0U) {
                        diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                                declaration + " includes unknown component `" + value.text + "`");
                    }
                }
            }
            decimalRange("materiality_threshold_percent", 0.0, 100.0);
            const C3EcoFieldData *exclude = c3EcoField(data, "exclude");
            if (exclude != nullptr) {
                const C3EcoFieldData *reasons = c3EcoField(data, "exclusion_reason");
                const C3EcoFieldData *materiality = c3EcoField(data, "exclusion_materiality_percent");
                if (reasons == nullptr || materiality == nullptr ||
                    reasons->values.size() != exclude->values.size() ||
                    materiality->values.size() != exclude->values.size()) {
                    diagnostics.errorAtNode(
                        node, diag::C3EcoMaterialityViolation,
                        declaration + " requires one string reason and decimal materiality for every exclusion");
                } else {
                    double total = 0.0;
                    for (std::size_t i = 0; i < exclude->values.size(); ++i) {
                        if (exclude->values[i].kind != C3EcoValueKind::Identifier ||
                            components.count(exclude->values[i].text) == 0U ||
                            reasons->values[i].kind != C3EcoValueKind::String ||
                            materiality->values[i].kind != C3EcoValueKind::Decimal) {
                            diagnostics.errorAtNode(node, diag::C3EcoInvalidTypedLiteral,
                                                    declaration + " has an invalid typed exclusion entry");
                            continue;
                        }
                        double amount = 0.0;
                        if (!c3EcoParseDecimal(materiality->values[i].text, amount) ||
                            amount < 0.0 || amount > 100.0) {
                            diagnostics.errorAtNode(node, diag::C3EcoValueOutOfRange,
                                                    declaration + " exclusion materiality is outside 0..100 percent");
                        } else {
                            total += amount;
                        }
                    }
                    const C3EcoValueData *threshold = single("materiality_threshold_percent",
                                                              C3EcoValueKind::Decimal);
                    double maximum = 0.0;
                    if (threshold != nullptr && c3EcoParseDecimal(threshold->text, maximum) && total > maximum) {
                        diagnostics.errorAtNode(
                            node, diag::C3EcoMaterialityViolation,
                            declaration + " cumulative exclusions exceed materiality_threshold_percent");
                    }
                }
                bool excludes_opaque_provider = false;
                for (const auto &value : exclude->values) {
                    excludes_opaque_provider = excludes_opaque_provider || value.text == "thirdparty_ai_api";
                }
                if (excludes_opaque_provider) {
                    requireV2Field("opaque_provider_treatment");
                    enumValue("opaque_provider_treatment", {"conservative_estimate", "included", "provider_evidence"});
                }
            }
            break;
        }
        case C3EcoDeclarationKind::AILifecycle: {
            enumValue("role", {"application_provider", "model_provider", "hosted_model_consumer",
                                 "trainer", "fine_tuner", "inference_operator"});
            nonEmptyString("model_provider");
            enumValue("lifecycle_scope", {"inference", "training", "fine_tuning", "end_to_end"});
            const C3EcoValueData *training = single("training_included", C3EcoValueKind::Boolean);
            const C3EcoValueData *fine_tuning = single("fine_tuning_included", C3EcoValueKind::Boolean);
            requiredBoolean("evaluation_included");
            const C3EcoValueData *role = c3EcoField(data, "role") != nullptr
                ? single("role", C3EcoValueKind::Identifier) : nullptr;
            if (role != nullptr && role->text == "trainer" && training != nullptr && training->text != "true") {
                diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                        declaration + " trainer role must include training lifecycle evidence");
            }
            if (role != nullptr && role->text == "fine_tuner" && fine_tuning != nullptr &&
                fine_tuning->text != "true") {
                diagnostics.errorAtNode(node, diag::C3EcoInvalidDomainValue,
                                        declaration + " fine_tuner role must include fine-tuning lifecycle evidence");
            }
            break;
        }
        case C3EcoDeclarationKind::Guardrails:
            requiredBoolean("functional_tests");
            decimalRange("accuracy", 0.0, 1.0);
            integerRange("p95_latency_ms", 1, 86400000);
            decimalRange("error_rate_percent", 0.0, 100.0);
            requiredBoolean("security_scan");
            requiredBoolean("accessibility");
            requiredBoolean("privacy_telemetry");
            break;
        case C3EcoDeclarationKind::MeasurementPlan:
        case C3EcoDeclarationKind::RAGPipeline:
        case C3EcoDeclarationKind::TokenBudget:
        case C3EcoDeclarationKind::ModelRouting:
            break;
    }
}

void SemanticAnalyzer::validateC3EcoProfiles() {
    std::map<C3EcoDeclarationKind, std::map<std::string, AST_C3ECO_DECLARATION *>> declarations;
    std::map<std::string, std::set<C3EcoDeclarationKind>> kinds_by_name;
    std::vector<AST_C3ECO_DECLARATION *> profiles;
    for (AST_C3ECO_DECLARATION *node : c3eco_profile_nodes) {
        if (node == nullptr) continue;
        declarations[node->data.kind].emplace(node->data.name, node);
        kinds_by_name[node->data.name].insert(node->data.kind);
        if (node->data.kind == C3EcoDeclarationKind::CertificationProfile) profiles.push_back(node);
    }
    std::set<AST_C3ECO_DECLARATION *> validated;
    const std::vector<std::pair<std::string, C3EcoDeclarationKind>> references = {
        {"certification", C3EcoDeclarationKind::Certification},
        {"functional_unit", C3EcoDeclarationKind::FunctionalUnit},
        {"workload", C3EcoDeclarationKind::Workload},
        {"boundary", C3EcoDeclarationKind::Boundary},
        {"ai_lifecycle", C3EcoDeclarationKind::AILifecycle},
        {"guardrails", C3EcoDeclarationKind::Guardrails}};

    for (AST_C3ECO_DECLARATION *profile : profiles) {
        validateC3EcoTypedDeclaration(profile);
        const C3EcoValueData *from = nullptr;
        const C3EcoValueData *until = nullptr;
        if (const C3EcoFieldData *field = c3EcoField(profile->data, "valid_from")) {
            if (field->values.size() == 1U && field->values.front().kind == C3EcoValueKind::String)
                from = &field->values.front();
        }
        if (const C3EcoFieldData *field = c3EcoField(profile->data, "valid_until")) {
            if (field->values.size() == 1U && field->values.front().kind == C3EcoValueKind::String)
                until = &field->values.front();
        }
        if (from != nullptr && until != nullptr) {
            if (!c3EcoIsoDate(from->text) || !c3EcoIsoDate(until->text) || from->text >= until->text) {
                diagnostics.errorAtNode(
                    profile, diag::C3EcoInvalidValidityWindow,
                    "certification_profile `" + profile->data.name +
                        "` requires valid YYYY-MM-DD dates with valid_from before valid_until");
            }
        }

        for (const auto &reference : references) {
            const C3EcoFieldData *field = c3EcoField(profile->data, reference.first);
            if (field == nullptr || field->values.size() != 1U ||
                field->values.front().kind != C3EcoValueKind::Identifier) {
                if (field != nullptr) {
                    diagnostics.errorAtNode(
                        profile, diag::C3EcoInvalidTypedLiteral,
                        "certification_profile `" + profile->data.name + "` field `" +
                            reference.first + "` requires exactly one identifier reference");
                }
                continue;
            }
            const std::string &name = field->values.front().text;
            const auto by_kind = declarations.find(reference.second);
            AST_C3ECO_DECLARATION *target = nullptr;
            if (by_kind != declarations.end()) {
                const auto found = by_kind->second.find(name);
                if (found != by_kind->second.end()) target = found->second;
            }
            if (target == nullptr) {
                if (kinds_by_name.count(name) != 0U) {
                    diagnostics.errorAtNode(
                        profile, diag::C3EcoReferenceKindMismatch,
                        "certification_profile `" + profile->data.name + "` field `" +
                            reference.first + "` references `" + name + "` with the wrong declaration kind");
                } else {
                    diagnostics.errorAtNode(
                        profile, diag::C3EcoUnknownReference,
                        "certification_profile `" + profile->data.name + "` field `" +
                            reference.first + "` references unknown declaration `" + name + "`");
                }
                continue;
            }
            if (validated.insert(target).second) validateC3EcoTypedDeclaration(target);
        }
    }
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
    bool resolved = false;
    if (!block_label_scopes.empty()) {
        resolved = block_label_scopes.back().count(n->label) != 0U;
    }
    if (!resolved) {
        bool found_in_outer_scope = false;
        if (block_label_scopes.size() > 1) {
            for (auto it = block_label_scopes.rbegin() + 1; it != block_label_scopes.rend(); ++it) {
                if (it->count(n->label) != 0U) {
                    found_in_outer_scope = true;
                    break;
                }
            }
        }
        diagnostics.errorAtNode(
            n,
            found_in_outer_scope ? diag::SemanticInvalidGotoScope : diag::SemanticUndefinedLabel,
            found_in_outer_scope
                ? "goto cannot leave its lexical block: " + n->label
                : "goto target is not defined in the current lexical block: " + n->label);
    }
    if (n->condition) requireConditionType(n, n->condition);
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
        if (item.expression) {
            if (expressionType(item.expression) == ShortType::Void) {
                diagnostics.errorAtNode(item.expression, diag::SemanticVoidValueUsed,
                                        "print requires a value and cannot consume void");
            }
        }
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
    if (return_types.back() != ShortType::Void && n->expression == nullptr) {
        diagnostics.errorAtNode(n, diag::SemanticReturnTypeMismatch,
                                "non-void function must return a value");
    } else if (n->expression && return_types.back() != ShortType::Void) {
        const ShortType actual = expressionType(n->expression);
        if (actual == ShortType::Void) {
            diagnostics.errorAtNode(n->expression, diag::SemanticVoidValueUsed,
                                    "return expression requires a value but received void");
        } else if (actual != return_types.back()) {
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
