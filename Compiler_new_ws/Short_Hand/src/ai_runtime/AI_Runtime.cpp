#include "AI_Runtime.h"
#include "backends/FallbackBackend.h"
#include "backends/LibTorchBackend.h"
#include "backends/LlamaCppBackend.h"
#include "backends/OnnxRuntimeBackend.h"
#include "backends/OpenVINOBackend.h"
#include "backends/TensorRTBackend.h"

namespace shorthand::ai {
AIRuntime::AIRuntime(){ registry.registerBackend(std::make_unique<TensorRTBackend>()); registry.registerBackend(std::make_unique<OnnxRuntimeBackend>()); registry.registerBackend(std::make_unique<OpenVINOBackend>()); registry.registerBackend(std::make_unique<LibTorchBackend>()); registry.registerBackend(std::make_unique<LlamaCppBackend>()); registry.registerBackend(std::make_unique<FallbackBackend>()); }
std::vector<BackendCapabilities> AIRuntime::capabilities() const { return registry.capabilities(); }
InferenceResult AIRuntime::infer(const ModelSpec &model, const TensorBuffer &input){ if(!validateInputMatchesShape(input)){ InferenceResult r; r.status=InferenceStatus::InvalidInput; r.reason="input_shape_mismatch"; return r; } auto *b=registry.select(model); if(!b){ InferenceResult r; r.status=InferenceStatus::BackendUnavailable; r.reason="no_compatible_backend"; return r; } auto r=b->infer(model,input); if(b->kind()==BackendKind::Fallback){ r.status=InferenceStatus::NotExecuted; r.backend=BackendKind::Fallback; r.backend_name="fallback"; r.provider_name="none"; r.reason="backend_not_available"; r.output_f32.clear(); } return r; }
}

bool AI_Runtime::loadModel(const std::string &model_path){
    model_path_=model_path;
    last_error_.clear();
    if(model_path_.empty()){
        last_error_="missing_model_path";
        return false;
    }
    return true;
}

bool AI_Runtime::run(const TensorData &input_tensor, std::vector<float> &output){
    output.clear();

    shorthand::ai::TensorBuffer input;
    input.spec.name="input";
    input.spec.element_type=shorthand::ai::ElementType::Float32;
    input.spec.shape=input_tensor.shape;
    input.spec.element_count=shorthand::ai::productOfShape(input.spec.shape);
    input.f32_data=input_tensor.data;

    shorthand::ai::ModelSpec model;
    model.name="legacy_onnx_model";
    model.path=model_path_;
    model.format=shorthand::ai::ModelFormat::Onnx;
    model.task="inference";
    model.precision="float32";
    model.input=input.spec;
    model.output.name="output";
    model.output.element_type=shorthand::ai::ElementType::Float32;
    model.backend_preference={shorthand::ai::BackendKind::OnnxRuntimeCPU};
    model.allow_fallback=false;

    shorthand::ai::AIRuntime runtime;
    auto result=runtime.infer(model,input);
    if(result.status==shorthand::ai::InferenceStatus::Success){
        output=result.output_f32;
        last_error_.clear();
        return true;
    }

    last_error_=result.backend_name+":"+shorthand::ai::inferenceStatusToString(result.status)+":"+result.reason;
    return false;
}

std::string AI_Runtime::getLastError() const { return last_error_; }

// shorthand_runtime_hook_integration_ready:
// AI_Runtime owns SDK-backed C++ inference. The public compiled-hook C ABI is
// owned by runtime/ShorthandRuntime.* so future linked builds do not get duplicate
// C symbols such as short_ai_infer or short_ai_infer_f32.
