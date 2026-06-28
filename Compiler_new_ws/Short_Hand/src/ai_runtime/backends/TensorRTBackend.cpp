#include "TensorRTBackend.h"
namespace shorthand::ai {
BackendKind TensorRTBackend::kind() const { return BackendKind::TensorRT; }
std::string TensorRTBackend::name() const { return "tensorrt"; }
BackendCapabilities TensorRTBackend::capabilities() const { BackendCapabilities c; c.kind=kind(); c.name=name(); c.supports_engine=true; c.supported_precisions={"float32","float16","int8"};
#if SHORTHAND_HAS_TENSORRT
c.available=false; c.unavailable_reason="TensorRT SDK detected but direct execution is not yet enabled";
#else
c.available=false; c.unavailable_reason="tensorrt backend not built. Set SDK root and enable the matching SHORTHAND_ENABLE option.";
#endif
return c; }
bool TensorRTBackend::canLoad(const ModelSpec &m) const { auto c=capabilities(); return c.available && backendSupportsFormat(c,m.format); }
InferenceResult TensorRTBackend::infer(const ModelSpec&, const TensorBuffer&) { InferenceResult r; r.backend=kind(); r.backend_name=name(); r.status=InferenceStatus::BackendUnavailable; r.reason=capabilities().unavailable_reason; return r; }
}
