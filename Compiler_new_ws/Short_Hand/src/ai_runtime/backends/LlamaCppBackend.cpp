#include "LlamaCppBackend.h"
namespace shorthand::ai {
BackendKind LlamaCppBackend::kind() const { return BackendKind::LlamaCpp; }
std::string LlamaCppBackend::name() const { return "llamacpp"; }
BackendCapabilities LlamaCppBackend::capabilities() const { BackendCapabilities c; c.kind=kind(); c.name=name(); c.supports_gguf=true; c.supported_precisions={"float32","float16","int8"};
#if SHORTHAND_HAS_LLAMACPP
c.available=false; c.unavailable_reason="llama.cpp SDK detected but direct execution is not yet enabled";
#else
c.available=false; c.unavailable_reason="llamacpp backend not built. Set SDK root and enable the matching SHORTHAND_ENABLE option.";
#endif
return c; }
bool LlamaCppBackend::canLoad(const ModelSpec &m) const { auto c=capabilities(); return c.available && backendSupportsFormat(c,m.format); }
InferenceResult LlamaCppBackend::infer(const ModelSpec&, const TensorBuffer&) { InferenceResult r; r.backend=kind(); r.backend_name=name(); r.status=InferenceStatus::BackendUnavailable; r.reason=capabilities().unavailable_reason; return r; }
}
