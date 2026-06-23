#include "OnnxRuntimeBackend.h"
#if SHORTHAND_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif
namespace shorthand::ai {
BackendKind OnnxRuntimeBackend::kind() const { return BackendKind::OnnxRuntimeCPU; }
std::string OnnxRuntimeBackend::name() const { return "onnxruntime_cpu"; }
BackendCapabilities OnnxRuntimeBackend::capabilities() const { BackendCapabilities c; c.kind=kind(); c.name=name(); c.supports_onnx=true; c.supported_precisions={"float32","float16","int8","int32"};
#if SHORTHAND_HAS_ONNXRUNTIME
c.available=true;
#else
c.available=false; c.unavailable_reason="ONNX Runtime backend not built. Set ONNXRUNTIME_ROOT and enable SHORTHAND_ENABLE_ONNXRUNTIME.";
#endif
return c; }
bool OnnxRuntimeBackend::canLoad(const ModelSpec &m) const { auto c=capabilities(); return c.available && backendSupportsFormat(c,m.format); }
InferenceResult OnnxRuntimeBackend::infer(const ModelSpec&, const TensorBuffer &input) { InferenceResult r; r.backend=kind(); r.backend_name=name(); r.provider_name="onnxruntime_cpu"; if(!validateInputMatchesShape(input)){ r.status=InferenceStatus::InvalidInput; r.reason="input_shape_mismatch"; return r; }
#if SHORTHAND_HAS_ONNXRUNTIME
r.status=InferenceStatus::RuntimeError; r.reason="ONNX Runtime execution hook is built but model execution is not configured in this minimal build"; return r;
#else
r.status=InferenceStatus::BackendUnavailable; r.reason=capabilities().unavailable_reason; return r;
#endif
}
}
