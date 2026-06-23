#include "AI_Backend.h"

namespace shorthand::ai {

void BackendRegistry::registerBackend(std::unique_ptr<AIBackend> backend) {
    if (backend) backends_.push_back(std::move(backend));
}

AIBackend* BackendRegistry::select(const ModelSpec& model) {
    selection_diagnostics_.clear();
    for (BackendKind preferred : model.backend_preference) {
        for (const auto& backend : backends_) {
            if (backend->kind() != preferred) continue;
            const BackendCapabilities caps = backend->capabilities();
            if (!caps.available) {
                selection_diagnostics_.push_back(caps.name + ": " + caps.unavailable_reason);
                continue;
            }
            if (!backendSupportsFormat(preferred, model.format) || !backend->canLoad(model)) {
                selection_diagnostics_.push_back(caps.name + ": incompatible model format " + modelFormatToString(model.format));
                continue;
            }
            return backend.get();
        }
    }

    if (model.allow_fallback) {
        for (const auto& backend : backends_) {
            if (backend->kind() == BackendKind::Fallback) return backend.get();
        }
    }
    return nullptr;
}

std::vector<BackendCapabilities> BackendRegistry::capabilities() const {
    std::vector<BackendCapabilities> result;
    for (const auto& backend : backends_) result.push_back(backend->capabilities());
    return result;
}

const std::vector<std::string>& BackendRegistry::selectionDiagnostics() const {
    return selection_diagnostics_;
}

InferenceResult unavailableResult(BackendKind backend, const std::string& backend_name, const std::string& reason) {
    InferenceResult result;
    result.status = InferenceStatus::BackendUnavailable;
    result.backend = backend;
    result.backend_name = backend_name;
    result.provider_name = "none";
    result.reason = reason;
    result.evidence_json_fragment = "\"runtime_backend\":\"" + backend_name +
                                    "\",\"inference_status\":\"" + inferenceStatusToString(result.status) +
                                    "\",\"reason\":\"" + reason + "\"";
    return result;
}

}  // namespace shorthand::ai
