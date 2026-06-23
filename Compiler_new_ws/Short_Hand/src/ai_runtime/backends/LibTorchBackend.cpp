#include "LibTorchBackend.h"

#if SHORTHAND_HAS_LIBTORCH
#include <torch/script.h>
#endif

namespace shorthand::ai {

BackendKind LibTorchBackend::kind() const { return BackendKind::LibTorch; }
std::string LibTorchBackend::name() const { return "libtorch"; }

BackendCapabilities LibTorchBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = kind();
    caps.name = name();
    caps.supports_torchscript = true;
    caps.supported_precisions = {"fp32", "fp16", "int8"};
#if SHORTHAND_HAS_LIBTORCH
    caps.available = true;
#else
    caps.available = false;
    caps.unavailable_reason = "LibTorch backend not built. Set LIBTORCH_ROOT and enable SHORTHAND_ENABLE_LIBTORCH.";
#endif
    return caps;
}

bool LibTorchBackend::canLoad(const ModelSpec& model) const {
    return backendSupportsFormat(kind(), model.format);
}

InferenceResult LibTorchBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
    (void)model;
    (void)input;
#if SHORTHAND_HAS_LIBTORCH
    return unavailableResult(kind(), name(), "LibTorch SDK detected but direct execution not yet enabled");
#else
    return unavailableResult(kind(), name(), "LibTorch backend not built. Set LIBTORCH_ROOT and enable SHORTHAND_ENABLE_LIBTORCH.");
#endif
}

}  // namespace shorthand::ai
