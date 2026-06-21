#include "LlamaCppBackend.h"
namespace shorthand::ai { std::string LlamaCppBackend::name() const { return "llamacpp"; } bool LlamaCppBackend::available() const { return false; } InferenceResult LlamaCppBackend::infer(const ModelSpec&){ return {}; } }
