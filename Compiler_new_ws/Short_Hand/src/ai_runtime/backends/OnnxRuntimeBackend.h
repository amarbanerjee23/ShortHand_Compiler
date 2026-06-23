#pragma once

#include "../AI_Backend.h"

namespace shorthand::ai {

class OnnxRuntimeBackend final : public AIBackend {
public:
    explicit OnnxRuntimeBackend(BackendKind kind = BackendKind::OnnxRuntimeCPU);
    BackendKind kind() const override;
    std::string name() const override;
    BackendCapabilities capabilities() const override;
    bool canLoad(const ModelSpec& model) const override;
    InferenceResult infer(const ModelSpec& model, const TensorBuffer& input) override;

private:
    BackendKind kind_;
};

}  // namespace shorthand::ai
