#include "AI_Runtime.h"
#include "backends/FallbackBackend.h"
#include "backends/LibTorchBackend.h"
#include "backends/LlamaCppBackend.h"
#include "backends/OnnxRuntimeBackend.h"
#include "backends/OpenVINOBackend.h"
#include "backends/TensorRTBackend.h"

namespace shorthand::ai {
AIRuntime::AIRuntime(){ registry.registerBackend(std::make_unique<TensorRTBackend>()); registry.registerBackend(std::make_unique<OnnxRuntimeBackend>()); registry.registerBackend(std::make_unique<OpenVINOBackend>()); registry.registerBackend(std::make_unique<LibTorchBackend>()); registry.registerBackend(std::make_unique<LlamaCppBackend>()); registry.registerBackend(std::make_unique<FallbackBackend>()); }
std::vector<BackendCapabilities> AIRuntime::capabilities() const { return registry.capabilities(); }
InferenceResult AIRuntime::infer(const ModelSpec &model, const TensorBuffer &input){ if(!validateInputMatchesShape(input)){ InferenceResult r; r.status=InferenceStatus::InvalidInput; r.reason="input_shape_mismatch"; return r; } auto *b=registry.select(model); if(!b){ InferenceResult r; r.status=InferenceStatus::BackendUnavailable; r.reason="no_compatible_backend"; return r; } auto r=b->infer(model,input); if(b->kind()==BackendKind::Fallback){ r.status=InferenceStatus::NotExecuted; r.backend=BackendKind::Fallback; r.backend_name="fallback"; r.provider_name="none"; r.reason="backend_not_available"; r.output_f32.clear(); } return r; }
}

bool AI_Runtime::loadModel(const std::string &model_path){ model_path_=model_path; last_error_.clear(); return true; }
bool AI_Runtime::run(const TensorData &input_tensor, std::vector<float> &output){ (void)input_tensor; output.clear(); last_error_="backend_not_available"; return false; }
std::string AI_Runtime::getLastError() const { return last_error_; }
extern "C" int short_ai_infer(const char*, const char*, const char*, const char*, const char*) { return 2; }
extern "C" int short_greenai_emit_event(const char*, const char*) { return 0; }
