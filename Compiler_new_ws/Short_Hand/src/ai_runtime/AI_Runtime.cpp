#include "AI_Runtime.h"

#include "backends/FallbackBackend.h"
#include "backends/LibTorchBackend.h"
#include "backends/LlamaCppBackend.h"
#include "backends/OnnxRuntimeBackend.h"
#include "backends/OpenVINOBackend.h"
#include "backends/TensorRTBackend.h"

#include <sstream>

bool AI_Runtime::loadModel(const std::string& model_path) {
    model_path_ = model_path;
    last_error_.clear();
    return true;
}

bool AI_Runtime::run(const TensorData& input_tensor, std::vector<float>& output) {
    shorthand::ai::ModelSpec model;
    model.name = "legacy_ai_infer";
    model.path = model_path_;
    model.format = shorthand::ai::ModelFormat::Onnx;
    model.input.name = "legacy_input";
    model.input.element_type = shorthand::ai::ElementType::Float32;
    model.input.shape = input_tensor.shape;
    model.input.element_count = shorthand::ai::productOfShape(input_tensor.shape);
    model.backend_preference = {shorthand::ai::BackendKind::OnnxRuntimeCPU, shorthand::ai::BackendKind::Fallback};

    shorthand::ai::TensorBuffer input;
    input.spec = model.input;
    input.f32_data = input_tensor.data;

    shorthand::ai::AIRuntime runtime;
    const shorthand::ai::InferenceResult result = runtime.infer(model, input);
    if (result.status != shorthand::ai::InferenceStatus::Success) {
        last_error_ = result.reason;
        return false;
    }
    output = result.output_f32;
    last_error_.clear();
    return true;
}

std::string AI_Runtime::getLastError() const {
    return last_error_;
}

namespace shorthand::ai {

AIRuntime::AIRuntime() {
    registry_.registerBackend(std::make_unique<TensorRTBackend>());
    registry_.registerBackend(std::make_unique<OnnxRuntimeBackend>(BackendKind::OnnxRuntimeTensorRT));
    registry_.registerBackend(std::make_unique<OnnxRuntimeBackend>(BackendKind::OnnxRuntimeCUDA));
    registry_.registerBackend(std::make_unique<OpenVINOBackend>());
    registry_.registerBackend(std::make_unique<OnnxRuntimeBackend>(BackendKind::OnnxRuntimeCPU));
    registry_.registerBackend(std::make_unique<LibTorchBackend>());
    registry_.registerBackend(std::make_unique<LlamaCppBackend>());
    registry_.registerBackend(std::make_unique<FallbackBackend>());
}

InferenceResult AIRuntime::infer(const ModelSpec& model, const TensorBuffer& input) {
    std::string validation_error;
    if (!validateInputMatchesShape(model.input, input, &validation_error)) {
        InferenceResult result;
        result.status = InferenceStatus::InvalidInput;
        result.backend = BackendKind::Fallback;
        result.backend_name = "none";
        result.provider_name = "none";
        result.reason = validation_error;
        return result;
    }

    AIBackend* backend = registry_.select(model);
    if (!backend) {
        return unavailableResult(BackendKind::Fallback, "none", "backend_not_available");
    }
    InferenceResult result = backend->infer(model, input);
    if (result.status == InferenceStatus::Success && backend->kind() == BackendKind::Fallback) {
        result.status = InferenceStatus::NotExecuted;
        result.reason = "backend_not_available";
    }
    return result;
}

std::vector<BackendCapabilities> AIRuntime::capabilities() const {
    return registry_.capabilities();
}

}  // namespace shorthand::ai

extern "C" int short_ai_infer(const char* model_name, const char* model_path, const char* backend_policy, const char* input_shape, const char* input_values) {
    (void)backend_policy;
    bool dynamic = false;
    std::string error;
    shorthand::ai::ModelSpec model;
    model.name = model_name ? model_name : "native_model";
    model.path = model_path ? model_path : "";
    model.format = shorthand::ai::ModelFormat::Onnx;
    model.input.name = "native_input";
    model.input.element_type = shorthand::ai::ElementType::Float32;
    model.input.shape = shorthand::ai::parseShapeCsv(input_shape ? input_shape : "dynamic", &dynamic, &error);
    model.input.dynamic = dynamic;
    model.input.element_count = shorthand::ai::productOfShape(model.input.shape);
    model.backend_preference = {shorthand::ai::BackendKind::OnnxRuntimeCPU, shorthand::ai::BackendKind::Fallback};

    shorthand::ai::TensorBuffer input;
    input.spec = model.input;
    input.f32_data = shorthand::ai::parseFloatCsv(input_values ? input_values : "", &error);

    shorthand::ai::AIRuntime runtime;
    const shorthand::ai::InferenceResult result = runtime.infer(model, input);
    return result.status == shorthand::ai::InferenceStatus::Success ? 0 : 2;
}

extern "C" int short_greenai_emit_event(const char* workload, const char* event_json) {
    (void)workload;
    (void)event_json;
    return 0;
}
