#include "OpenVINOBackend.h"
namespace shorthand::ai {
BackendKind OpenVINOBackend::kind() const { return BackendKind::OpenVINO; }
std::string OpenVINOBackend::name() const { return "openvino"; }
BackendCapabilities OpenVINOBackend::capabilities() const { BackendCapabilities c; c.kind=kind(); c.name=name(); c.supports_openvino_ir=true; c.supported_precisions={"float32","float16","int8"};
#if SHORTHAND_HAS_OPENVINO
c.available=false; c.unavailable_reason="OpenVINO SDK detected but direct execution is not yet enabled";
#else
c.available=false; c.unavailable_reason="openvino backend not built. Set SDK root and enable the matching SHORTHAND_ENABLE option.";
#endif
return c; }
bool OpenVINOBackend::canLoad(const ModelSpec &m) const { auto c=capabilities(); return c.available && backendSupportsFormat(c,m.format); }
InferenceResult OpenVINOBackend::infer(const ModelSpec&, const TensorBuffer&) { InferenceResult r; r.backend=kind(); r.backend_name=name(); r.status=InferenceStatus::BackendUnavailable; r.reason=capabilities().unavailable_reason; return r; }
}
