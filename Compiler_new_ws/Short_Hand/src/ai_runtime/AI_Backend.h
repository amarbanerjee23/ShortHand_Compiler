#pragma once
#include "AI_Types.h"
#include <memory>
#include <string>
#include <vector>

namespace shorthand::ai {
class AIBackend { public: virtual ~AIBackend() = default; virtual BackendKind kind() const = 0; virtual std::string name() const = 0; virtual BackendCapabilities capabilities() const = 0; virtual bool canLoad(const ModelSpec &model) const = 0; virtual InferenceResult infer(const ModelSpec &model, const TensorBuffer &input) = 0; };
class BackendRegistry { public: void registerBackend(std::unique_ptr<AIBackend> backend); AIBackend *select(const ModelSpec &model); std::vector<BackendCapabilities> capabilities() const; private: std::vector<std::unique_ptr<AIBackend>> backends; };
} // namespace shorthand::ai
