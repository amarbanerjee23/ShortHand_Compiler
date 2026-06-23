#include "FallbackBackend.h"

namespace shorthand::ai {

BackendKind FallbackBackend::kind() const { return BackendKind::Fallback; }
std::string FallbackBackend::name() const { return "fallback"; }

BackendCapabilities FallbackBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = BackendKind::Fallback;
    caps.name = "fallback";
    caps.available = true;
    caps.supports_onnx = true;
    caps.supports_engine = true;
    caps.supports_torchscript = true;
    caps.supports_openvino_ir = true;
    caps.supports_gguf = true;
    caps.supported_precisions = {"fp32", "fp16", "bf16", "int8", "int4", "int32"};
    return caps;
}

bool FallbackBackend::canLoad(const ModelSpec& model) const {
    return model.allow_fallback;
}

InferenceResult FallbackBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
    (void)model;
    (void)input;
    InferenceResult result;
    result.status = InferenceStatus::NotExecuted;
    result.backend = BackendKind::Fallback;
    result.backend_name = "fallback";
    result.provider_name = "none";
    result.reason = "backend_not_available";
    result.evidence_json_fragment = "\"runtime_backend\":\"fallback\",\"inference_status\":\"not_executed\",\"reason\":\"backend_not_available\"";
    return result;
}

}  // namespace shorthand::ai
