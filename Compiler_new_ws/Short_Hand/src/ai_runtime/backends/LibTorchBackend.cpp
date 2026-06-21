#include "LibTorchBackend.h"
namespace shorthand::ai { std::string LibTorchBackend::name() const { return "libtorch"; } bool LibTorchBackend::available() const { return false; } InferenceResult LibTorchBackend::infer(const ModelSpec&){ return {}; } }
