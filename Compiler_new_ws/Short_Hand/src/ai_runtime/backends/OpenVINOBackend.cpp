#include "OpenVINOBackend.h"

#if SHORTHAND_HAS_OPENVINO
#include <openvino/openvino.hpp>
#endif

namespace shorthand::ai {

BackendKind OpenVINOBackend::kind() const { return BackendKind::OpenVINO; }
std::string OpenVINOBackend::name() const { return "openvino"; }

BackendCapabilities OpenVINOBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = kind();
    caps.name = name();
    caps.supports_openvino_ir = true;
    caps.supports_onnx = true;
    caps.supported_precisions = {"fp32", "fp16", "int8"};
#if SHORTHAND_HAS_OPENVINO
    caps.available = true;
#else
    caps.available = false;
    caps.unavailable_reason = "OpenVINO backend not built. Set OPENVINO_ROOT and enable SHORTHAND_ENABLE_OPENVINO.";
#endif
    return caps;
}

bool OpenVINOBackend::canLoad(const ModelSpec& model) const {
    return backendSupportsFormat(kind(), model.format);
}

InferenceResult OpenVINOBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
    (void)model;
    (void)input;
#if SHORTHAND_HAS_OPENVINO
    return unavailableResult(kind(), name(), "OpenVINO SDK detected but direct execution not yet enabled");
#else
    return unavailableResult(kind(), name(), "OpenVINO backend not built. Set OPENVINO_ROOT and enable SHORTHAND_ENABLE_OPENVINO.");
#endif
}

}  // namespace shorthand::ai
