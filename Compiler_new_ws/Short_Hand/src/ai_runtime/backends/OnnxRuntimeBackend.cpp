#include "OnnxRuntimeBackend.h"
namespace shorthand::ai { std::string OnnxRuntimeBackend::name() const { return "onnxruntime"; } bool OnnxRuntimeBackend::available() const { return false; } InferenceResult OnnxRuntimeBackend::infer(const ModelSpec&){ return {}; } }
