#include "OpenVINOBackend.h"
namespace shorthand::ai { std::string OpenVINOBackend::name() const { return "openvino"; } bool OpenVINOBackend::available() const { return false; } InferenceResult OpenVINOBackend::infer(const ModelSpec&){ return {}; } }
