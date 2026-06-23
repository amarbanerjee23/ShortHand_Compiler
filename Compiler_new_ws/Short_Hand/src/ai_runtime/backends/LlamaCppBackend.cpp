#include "LlamaCppBackend.h"

#if SHORTHAND_HAS_LLAMACPP
#include <llama.h>
#endif

namespace shorthand::ai {

BackendKind LlamaCppBackend::kind() const { return BackendKind::LlamaCpp; }
std::string LlamaCppBackend::name() const { return "llamacpp"; }

BackendCapabilities LlamaCppBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = kind();
    caps.name = name();
    caps.supports_gguf = true;
    caps.supported_precisions = {"fp32", "fp16", "int8"};
#if SHORTHAND_HAS_LLAMACPP
    caps.available = true;
#else
    caps.available = false;
    caps.unavailable_reason = "llama.cpp backend not built. Set LLAMACPP_ROOT and enable SHORTHAND_ENABLE_LLAMACPP.";
#endif
    return caps;
}

bool LlamaCppBackend::canLoad(const ModelSpec& model) const {
    return backendSupportsFormat(kind(), model.format);
}

InferenceResult LlamaCppBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
    (void)model;
    (void)input;
#if SHORTHAND_HAS_LLAMACPP
    return unavailableResult(kind(), name(), "llama.cpp SDK detected but direct execution not yet enabled");
#else
    return unavailableResult(kind(), name(), "llama.cpp backend not built. Set LLAMACPP_ROOT and enable SHORTHAND_ENABLE_LLAMACPP.");
#endif
}

}  // namespace shorthand::ai
