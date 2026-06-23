#pragma once

#include "../AI_Backend.h"

namespace shorthand::ai {

class TensorRTBackend final : public AIBackend {
public:
    BackendKind kind() const override;
    std::string name() const override;
    BackendCapabilities capabilities() const override;
    bool canLoad(const ModelSpec& model) const override;
    InferenceResult infer(const ModelSpec& model, const TensorBuffer& input) override;
};

}  // namespace shorthand::ai
