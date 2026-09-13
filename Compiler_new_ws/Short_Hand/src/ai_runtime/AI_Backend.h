#pragma once
#include "AI_Types.h"
#include <memory>
#include <string>
#include <vector>

namespace shorthand::ai {
struct InferenceConfiguration { unsigned threads=1; std::size_t maximum_tensor_elements=16U*1024U*1024U; };
class PreparedInference {
public:
    virtual ~PreparedInference() = default;
    virtual InferenceResult run(const TensorBuffer &) = 0;
    virtual TensorSpec inputSpec() const = 0;
    virtual TensorSpec outputSpec() const = 0;
    virtual std::string runtimeVersion() const = 0;
};
class AIBackend { public: virtual ~AIBackend() = default; virtual BackendKind kind() const = 0; virtual std::string name() const = 0; virtual BackendCapabilities capabilities() const = 0; virtual bool canLoad(const ModelSpec &model) const = 0; virtual InferenceResult infer(const ModelSpec &model, const TensorBuffer &input) = 0;
    virtual std::unique_ptr<PreparedInference> prepare(const ModelSpec &,const InferenceConfiguration &,std::string &error) {
        error="prepared_execution_unavailable"; return nullptr;
    }
};
class BackendRegistry { public: void registerBackend(std::unique_ptr<AIBackend> backend); AIBackend *select(const ModelSpec &model); std::vector<BackendCapabilities> capabilities() const; private: std::vector<std::unique_ptr<AIBackend>> backends; };
} // namespace shorthand::ai
