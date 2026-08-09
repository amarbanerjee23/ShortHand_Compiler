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
            return backend == BackendKind::OnnxRuntimeCPU || backend == BackendKind::OnnxRuntimeCUDA || backend == BackendKind::OnnxRuntimeTensorRT || backend == BackendKind::TensorRT;
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

std::set<std::string> SemanticAnalyzer::functionNames(AST_PROGRAM *program) {
    std::set<std::string> names;
    if (program == nullptr || program->functions == nullptr) return names;
    for (AST_FUNCTION_RULE *function : program->functions->functions) {
        if (function != nullptr && function->function_name != nullptr)
            names.insert(function->function_name);
    }
    return names;
}

std::set<std::string> SemanticAnalyzer::globalNames(AST_PROGRAM *program) {
    std::set<std::string> names;
    if (program == nullptr || program->decl_block == nullptr) return names;
    for (const std::string &name : program->decl_block->single_ints) names.insert(name);
    for (const auto &entry : program->decl_block->array_ints) names.insert(entry.first);
    return names;
}

void SemanticAnalyzer::setImportedFunctions(const std::set<std::string> &names,
                                            bool allow_external_calls) {
    imported_functions = names;
    functions = names;
    allow_external_function_calls = allow_external_calls;
}

int SemanticAnalyzer::visit(AST_PROGRAM *p) {
    if (p->decl_block) p->decl_block->accept(*this);
    if (p->functions) p->functions->accept(*this);
    if (p->code_block) p->code_block->accept(*this);
    return diagnostics.hasErrors() ? 1 : 0;
}

int SemanticAnalyzer::visit(AST_DATA_DECLARATION_BLOCK*) { return 0; }

int SemanticAnalyzer::visit(AST_FUNCTION_LIST_RULE *f) {
    for (auto *function : f->functions) {
        if (function && function->function_name) functions.insert(function->function_name);
    }
    for (auto *function : f->functions) {
        if (function) function->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_RULE *n) {
    if (n->block_statement) n->block_statement->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_LOGIC_BLOCK *b) {
    if (b->block_statement) b->block_statement->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_STATEMENTS_BLOCK *b) {
    for (auto *statement : b->statements) {
        if (statement) statement->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_EXPRESSION_STATEMENT_RULE *n) {
    if (n->expression) n->expression->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_FUNCTION_CALL_RULE *n) {
    const std::string name = n->function_name == nullptr ? std::string() : std::string(n->function_name);
    if (name.empty() || !functions.count(name)) {
        diagnostics.errorAtNode(n, diag::LoweringUndefinedFunction, "lowering cannot resolve function: " + name);
    } else if (!allow_external_function_calls && imported_functions.count(name) != 0U) {
        diagnostics.errorAtNode(
            n,
            diag::ModuleExternalRunUnsupported,
            "interpreter execution of imported function is deferred to PR71; compile/native modes are supported: " + name);
    }
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
                n,
                diag::AIModelIncompatibleBackend,
                "model " + d.name + " backend_preference " + backend + " is not compatible with format " + d.format);
        }
    }
    if (d.backend_preference.empty())
        diagnostics.warningAtNode(
            n,
            diag::AIModelMissingBackendPreference,
            "model " + d.name + " has no backend_preference; fallback will be used if allowed");
    if (!d.backend_preference.empty() && !hasCompatibleRealBackend && !hasFallback)
        diagnostics.errorAtNode(
            n,
            diag::AIModelNoCompatibleBackend,
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
            n,
            diag::GreenAIMeasureUnknownContract,
            "greenai_measure references unknown contract: " + n->data.workload);
    if (!models.empty() && !models.count(n->data.backend))
        diagnostics.warningAtNode(
            n,
            diag::GreenAIMeasureExternalBackend,
            "greenai_measure backend " + n->data.backend + " is not a declared model; treating as external measurement source");
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
                n,
                diag::AIInferInputShapeMismatch,
                "infer input tensor shape " + tensor.shape_csv + " does not match model " + model.name + " input_shape " + model.input_shape);
        if (tensors.count(n->output_name)) {
            const auto &outTensor = tensors[n->output_name];
            if (!model.output_shape.empty() && !shapeCompatible(model.output_shape, outTensor.shape_csv))
                diagnostics.errorAtNode(
                    n,
                    diag::AIInferOutputShapeMismatch,
                    "infer output tensor shape " + outTensor.shape_csv + " does not match model " + model.name + " output_shape " + model.output_shape);
        } else {
            diagnostics.warningAtNode(
                n,
                diag::AIInferImplicitOutput,
                "infer output " + n->output_name + " is implicit; declare a tensor to enable output_shape validation");
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
    if (n->condition) n->condition->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_READ_RULE *n) {
    for (auto *variable : n->variables) {
        if (variable) variable->accept(*this);
    }
    return 0;
}

int SemanticAnalyzer::visit(AST_PRINT_RULE*) { return 0; }
int SemanticAnalyzer::visit(AST_LABEL_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_GREENAI_REPORT_RULE *n) {
    if (n->inferences) n->inferences->accept(*this);
    if (n->watts) n->watts->accept(*this);
    if (n->seconds) n->seconds->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_AI_INFER_RULE*) { return 0; }

int SemanticAnalyzer::visit(AST_RETURN_STATEMENT *n) {
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

int SemanticAnalyzer::visit(AST_SIMPLE_VARIABLE*) { return 0; }

int SemanticAnalyzer::visit(AST_ARRAY_VARIABLE *n) {
    if (n->index) n->index->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_STRING_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_BOOL_LITERAL*) { return 0; }
int SemanticAnalyzer::visit(AST_FLOAT_LITERAL*) { return 0; }

int SemanticAnalyzer::visit(AST_FUNCTION_CALL_EXPRESSION *n) {
    if (!functions.count(n->function_name)) {
        diagnostics.errorAtNode(n, diag::LoweringUndefinedFunction, "lowering cannot resolve function: " + n->function_name);
    } else if (!allow_external_function_calls && imported_functions.count(n->function_name) != 0U) {
        diagnostics.errorAtNode(
            n,
            diag::ModuleExternalRunUnsupported,
            "interpreter execution of imported function is deferred to PR71; compile/native modes are supported: " + n->function_name);
    }
    for (auto *argument : n->arguments) {
        if (argument) argument->accept(*this);
    }
    return 0;
}
