#include "AIRuntimeBridgeAdapter.h"

#include <cstddef>
#include <string>

namespace shorthand::runtime_bridge {
namespace {
std::string safe(const char *value) { return value ? std::string(value) : std::string(); }

ai::TensorSpec buildTensorSpec(const RuntimeBridgeTensorInput &input) {
    ai::TensorSpec spec;
    spec.name = safe(input.name);
    spec.element_type = ai::parseElementType(safe(input.element_type));
    spec.shape = ai::parseShapeCsv(safe(input.shape));
    spec.element_count = ai::productOfShape(spec.shape);
    spec.dynamic = spec.shape.empty();
    return spec;
}
} // namespace

const char *bridgeAdapterContractVersion() {
    return "shorthand.runtime.ai_runtime_execution_adapter.v1";
}

ai::ModelSpec buildModelSpec(const RuntimeBridgeModelInput &model,
                             const RuntimeBridgeTensorInput &input,
                             const RuntimeBridgeTensorInput &output) {
    ai::ModelSpec spec;
    spec.name = safe(model.name);
    spec.format = ai::parseModelFormat(safe(model.format));
    spec.path = safe(model.path);
    spec.task = safe(model.task);
    spec.precision = safe(model.precision);
    spec.input = buildTensorSpec(input);
    spec.output = buildTensorSpec(output);
    spec.allow_fallback = model.allow_fallback;

    const std::string backend = safe(model.backend_preference);
    if (!backend.empty()) {
        spec.backend_preference.push_back(ai::parseBackendKind(backend));
    }
    if (spec.backend_preference.empty() && spec.allow_fallback) {
        spec.backend_preference.push_back(ai::BackendKind::Fallback);
    }

    return spec;
}

ai::TensorBuffer buildInputTensorBuffer(const RuntimeBridgeTensorInput &input,
                                        const float *input_values,
                                        int input_count) {
    ai::TensorBuffer buffer;
    buffer.spec = buildTensorSpec(input);
    if (input_values && input_count > 0) {
        buffer.f32_data.assign(input_values, input_values + input_count);
    }
    return buffer;
}

int runtimeStatusFromInferenceStatus(ai::InferenceStatus status) {
    switch (status) {
        case ai::InferenceStatus::Success:
            return SHORTHAND_RUNTIME_OK;
        case ai::InferenceStatus::BackendUnavailable:
            return SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE;
        case ai::InferenceStatus::InvalidInput:
            return SHORTHAND_RUNTIME_INVALID_INPUT;
        case ai::InferenceStatus::RuntimeError:
            return SHORTHAND_RUNTIME_RUNTIME_ERROR;
        case ai::InferenceStatus::NotExecuted:
            return SHORTHAND_RUNTIME_NOT_EXECUTED;
    }
    return SHORTHAND_RUNTIME_RUNTIME_ERROR;
}

bool bridgeRequestIsExecutionReady(const ai::ModelSpec &model,
                                   const ai::TensorBuffer &input,
                                   int output_capacity) {
    if (model.name.empty() || model.path.empty()) return false;
    if (model.format == ai::ModelFormat::Unknown) return false;
    if (model.input.element_type != ai::ElementType::Float32) return false;
    if (model.output.element_type != ai::ElementType::Float32) return false;
    if (!ai::validateShape(model.input.shape) || !ai::validateShape(model.output.shape)) return false;
    if (!ai::validateInputMatchesShape(input)) return false;
    if (input.f32_data.empty()) return false;
    if (output_capacity <= 0) return false;
    return static_cast<std::size_t>(output_capacity) >= model.output.element_count;
}

} // namespace shorthand::runtime_bridge
