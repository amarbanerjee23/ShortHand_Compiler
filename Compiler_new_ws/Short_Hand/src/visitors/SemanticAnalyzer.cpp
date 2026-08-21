#include "SemanticAnalyzer.h"
#include "DiagnosticCodes.h"
#include "../ai_runtime/AI_Types.h"

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

bool SemanticAnalyzer::isExecutableScalarType(ShortType type) {
    return type == ShortType::Int || type == ShortType::Boolean;
}

bool SemanticAnalyzer::isDeclared(const std::string &name) const {
    for (auto it = local_scopes.rbegin(); it != local_scopes.rend(); ++it) {
        if (it->count(name) != 0U) return true;
    }
    return globals.count(name) != 0U;
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

int SemanticAnalyzer::visit(AST_PROGRAM *p) {
    globals = globalNames(p);
    if (p->decl_block) p->decl_block->accept(*this);
    if (p->functions) p->functions->accept(*this);
    if (p->code_block) p->code_block->accept(*this);
    return diagnostics.hasErrors() ? 1 : 0;
}

int SemanticAnalyzer::visit(AST_DATA_DECLARATION_BLOCK *block) {
    if (block == nullptr) return 0;
    for (const auto &entry : block->typed_scalars) {
        if (!isExecutableScalarType(entry.type)) {
            diagnostics.errorAtNode(
                block, diag::SemanticUnsupportedExecutableType,
                std::string("beta-0.3 executable semantics do not silently coerce ") +
                shortTypeName(entry.type) + " declaration `" + entry.name + "`");
        }
    }
    for (const auto &entry : block->typed_arrays) {
        if (!isExecutableScalarType(entry.type)) {
            diagnostics.errorAtNode(
                block, diag::SemanticUnsupportedExecutableType,
                std::string("beta-0.3 executable semantics do not silently coerce ") +
                shortTypeName(entry.type) + " array `" + entry.name + "`");
        }
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
    }
    for (auto *function : f->functions) {
        if (function) function->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_RULE *n) {
    if (n == nullptr) return 0;
    if (n->type == ShortType::Float || n->type == ShortType::String) {
        diagnostics.errorAtNode(
            n, diag::SemanticUnsupportedExecutableType,
            std::string("function return type ") + shortTypeName(n->type) +
            " is not executable in the beta-0.3 differential contract");
    }
    if (n->parameters) {
        n->parameters->accept(*this);
        if (!n->parameters->array_ints.empty()) {
            diagnostics.errorAtNode(
                n, diag::SemanticUnsupportedExecutableType,
                "array parameters are not part of the beta-0.3 executable function contract");
        }
    }

    std::set<std::string> scope;
    if (n->parameters) {
        for (const std::string &name : n->parameters->single_ints) scope.insert(name);
        for (const auto &entry : n->parameters->array_ints) scope.insert(entry.first);
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
    const std::size_t arity = n->parameters == nullptr ? 0U : n->parameters->variables.size();
    validateCall(n, name, arity);
    if (n->parameters) n->parameters->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_ASSIGNMENT_RULE *n) {
    if (n->variable) n->variable->accept(*this);
    if (n->expression) n->expression->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_IF_STATEMENT *n) {
    if (n->condition) n->condition->accept(*this);
    if (n->if_block) n->if_block->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_IF_ELSE_STATEMENT *n) {
    if (n->condition) n->condition->accept(*this);
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
    if (n->variable) n->variable->accept(*this);
    if (n->from) n->from->accept(*this);
    if (n->step) n->step->accept(*this);
    if (n->to) n->to->accept(*this);
    loopDepth++;
    if (n->for_block) n->for_block->accept(*this);
    loopDepth--;
    return 0;
}

int SemanticAnalyzer::visit(AST_WHILE_LOOP_STATEMENT_RULE *n) {
    if (n->condition) n->condition->accept(*this);
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
    for (auto *variable : n->variables) if (variable) variable->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_PRINT_RULE *n) {
    for (auto &item : n->printables) {
        if (item.expression) item.expression->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_LABEL_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_GREENAI_REPORT_RULE *n) {
    if (n->inferences) n->inferences->accept(*this);
    if (n->watts) n->watts->accept(*this);
    if (n->seconds) n->seconds->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_AI_INFER_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_RETURN_STATEMENT *n) {
    if (return_types.empty()) {
        diagnostics.errorAtNode(n, diag::SemanticReturnOutsideFunction, "return outside function");
        if (n->expression) n->expression->accept(*this);
        return 0;
    }
    if (return_types.back() == ShortType::Void && n->expression != nullptr) {
        diagnostics.errorAtNode(n, diag::SemanticReturnTypeMismatch,
                                "void function cannot return a value");
    }
    if (n->expression) n->expression->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_BINARY_EXPRESSION_RULE *n) {
    if (n->left) n->left->accept(*this);
    if (n->right) n->right->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_UNARY_EXPRESSION_RULE *n) {
    if (n->expression) n->expression->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_SIMPLE_VARIABLE *n) {
    if (!isDeclared(n->variable_name)) {
        diagnostics.errorAtNode(n, diag::SemanticUndeclaredVariable,
                                "undeclared variable: " + n->variable_name);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_ARRAY_VARIABLE *n) {
    if (!isDeclared(n->array_name)) {
        diagnostics.errorAtNode(n, diag::SemanticUndeclaredVariable,
                                "undeclared array: " + n->array_name);
    }
    if (n->index) n->index->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_STRING_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_BOOL_LITERAL*) { return 0; }

int SemanticAnalyzer::visit(AST_FLOAT_LITERAL *n) {
    diagnostics.errorAtNode(
        n, diag::SemanticUnsupportedExecutableType,
        "float literals are parser-valid but not silently truncated in beta-0.3 executable semantics");
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_CALL_EXPRESSION *n) {
    validateCall(n, n->function_name, n->arguments.size());
    for (auto *argument : n->arguments) if (argument) argument->accept(*this);
    return 0;
}
