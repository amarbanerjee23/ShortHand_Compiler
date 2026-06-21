#include "TensorRTBackend.h"
namespace shorthand::ai { std::string TensorRTBackend::name() const { return "tensorrt"; } bool TensorRTBackend::available() const { return false; } InferenceResult TensorRTBackend::infer(const ModelSpec&){ return {}; } }
