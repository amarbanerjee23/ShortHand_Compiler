#include "AI_Backend.h"
#include <algorithm>

namespace shorthand::ai {
void BackendRegistry::registerBackend(std::unique_ptr<AIBackend> backend){ if(backend) backends.push_back(std::move(backend)); }
std::vector<BackendCapabilities> BackendRegistry::capabilities() const{ std::vector<BackendCapabilities> out; for(const auto &b:backends) out.push_back(b->capabilities()); return out; }
AIBackend *BackendRegistry::select(const ModelSpec &model){
    std::vector<BackendKind> pref = model.backend_preference.empty() ? std::vector<BackendKind>{BackendKind::OnnxRuntimeCPU, BackendKind::Fallback} : model.backend_preference;
    if(model.allow_fallback && std::find(pref.begin(), pref.end(), BackendKind::Fallback)==pref.end()) pref.push_back(BackendKind::Fallback);
    for(auto want: pref){
        for(auto &backend: backends){
            if(backend->kind()!=want) continue;
            const auto caps=backend->capabilities();
            if(!caps.available && want!=BackendKind::Fallback) continue;
            if(!backend->canLoad(model) && want!=BackendKind::Fallback) continue;
            if(want==BackendKind::Fallback && (!model.allow_fallback && std::find(model.backend_preference.begin(), model.backend_preference.end(), BackendKind::Fallback)==model.backend_preference.end())) continue;
            return backend.get();
        }
    }
    return nullptr;
}
} // namespace shorthand::ai
