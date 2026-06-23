#include "TensorRTBackend.h"

#if SHORTHAND_HAS_TENSORRT
#include <NvInfer.h>
#endif

namespace shorthand::ai {

BackendKind TensorRTBackend::kind() const { return BackendKind::TensorRT; }
std::string TensorRTBackend::name() const { return "tensorrt"; }

BackendCapabilities TensorRTBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = kind();
    caps.name = name();
    caps.supports_engine = true;
    caps.supports_onnx = true;
    caps.supported_precisions = {"fp32", "fp16", "int8"};
#if SHORTHAND_HAS_TENSORRT
    caps.available = true;
#else
    caps.available = false;
    caps.unavailable_reason = "TensorRT backend not built. Set TENSORRT_ROOT/CUDA_ROOT and enable SHORTHAND_ENABLE_TENSORRT.";
#endif
    return caps;
}

bool TensorRTBackend::canLoad(const ModelSpec& model) const {
    return backendSupportsFormat(kind(), model.format);
}

InferenceResult TensorRTBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
    (void)model;
    (void)input;
#if SHORTHAND_HAS_TENSORRT
    return unavailableResult(kind(), name(), "TensorRT SDK detected but direct execution not yet enabled");
#else
    return unavailableResult(kind(), name(), "TensorRT backend not built. Set TENSORRT_ROOT/CUDA_ROOT and enable SHORTHAND_ENABLE_TENSORRT.");
#endif
}

}  // namespace shorthand::ai
