#include "SemanticAnalyzer.h"
#include "DiagnosticCodes.h"
#include "../ai_runtime/AI_Types.h"
#include <set>

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

int SemanticAnalyzer::visit(AST_PROGRAM *p) {
    if (p->decl_block) p->decl_block->accept(*this);
    if (p->functions) p->functions->accept(*this);
    if (p->code_block) p->code_block->accept(*this);
    return diagnostics.hasErrors() ? 1 : 0;
}

int SemanticAnalyzer::visit(AST_DATA_DECLARATION_BLOCK*) { return 0; }

int SemanticAnalyzer::visit(AST_FUNCTION_LIST_RULE *f) {
    for (auto x : f->functions) x->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_LOGIC_BLOCK *b) {
    if (b->block_statement) b->block_statement->accept(*this);
    return 0;
}

int SemanticAnalyzer::visit(AST_STATEMENTS_BLOCK *b) {
    for (auto s : b->statements) s->accept(*this);
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
    for (const auto &b : d.backend_preference) {
        auto kind = shorthand::ai::parseBackendKind(b);
        hasFallback = hasFallback || kind == shorthand::ai::BackendKind::Fallback;
        if (kind == shorthand::ai::BackendKind::Fallback) continue;
        if (backendCompatibleWithFormat(format, kind)) {
            hasCompatibleRealBackend = true;
        } else {
            diagnostics.warningAtNode(
                n,
                diag::AIModelIncompatibleBackend,
                "model " + d.name + " backend_preference " + b + " is not compatible with format " + d.format);
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

#define STUB(T) int SemanticAnalyzer::visit(T*){ return 0; }
STUB(AST_EXPRESSION_STATEMENT_RULE) STUB(AST_FUNCTION_RULE) STUB(AST_FUNCTION_CALL_RULE) STUB(AST_ASSIGNMENT_RULE) STUB(AST_IF_STATEMENT) STUB(AST_IF_ELSE_STATEMENT) STUB(AST_GOTO_STATEMENT_RULE) STUB(AST_READ_RULE) STUB(AST_PRINT_RULE) STUB(AST_LABEL_RULE) STUB(AST_GREENAI_REPORT_RULE) STUB(AST_AI_INFER_RULE) STUB(AST_RETURN_STATEMENT) STUB(AST_BINARY_EXPRESSION_RULE) STUB(AST_UNARY_EXPRESSION_RULE) STUB(AST_SIMPLE_VARIABLE) STUB(AST_ARRAY_VARIABLE) STUB(AST_LITERAL) STUB(AST_STRING_LITERAL) STUB(AST_BOOL_LITERAL) STUB(AST_FLOAT_LITERAL) STUB(AST_FUNCTION_CALL_EXPRESSION)

int SemanticAnalyzer::visit(AST_FOR_LOOP_STATEMENT_RULE *n) {
    loopDepth++;
    if (n->for_block) n->for_block->accept(*this);
    loopDepth--;
    return 0;
}

int SemanticAnalyzer::visit(AST_WHILE_LOOP_STATEMENT_RULE *n) {
    loopDepth++;
    if (n->while_block) n->while_block->accept(*this);
    loopDepth--;
    return 0;
}
