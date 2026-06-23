#include "LibTorchBackend.h"
namespace shorthand::ai {
BackendKind LibTorchBackend::kind() const { return BackendKind::LibTorch; }
std::string LibTorchBackend::name() const { return "libtorch"; }
BackendCapabilities LibTorchBackend::capabilities() const { BackendCapabilities c; c.kind=kind(); c.name=name(); c.supports_torchscript=true; c.supported_precisions={"float32","float16","int8"};
#if SHORTHAND_HAS_LIBTORCH
c.available=false; c.unavailable_reason="LibTorch SDK detected but direct execution is not yet enabled";
#else
c.available=false; c.unavailable_reason="libtorch backend not built. Set SDK root and enable the matching SHORTHAND_ENABLE option.";
#endif
return c; }
bool LibTorchBackend::canLoad(const ModelSpec &m) const { auto c=capabilities(); return c.available && backendSupportsFormat(c,m.format); }
InferenceResult LibTorchBackend::infer(const ModelSpec&, const TensorBuffer&) { InferenceResult r; r.backend=kind(); r.backend_name=name(); r.status=InferenceStatus::BackendUnavailable; r.reason=capabilities().unavailable_reason; return r; }
}
