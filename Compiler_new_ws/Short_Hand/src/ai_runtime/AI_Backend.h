#pragma once
#include "AI_Types.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace shorthand::ai {
struct InferenceConfiguration { unsigned threads=1; std::size_t maximum_tensor_elements=16U*1024U*1024U; };
// Opt-in diagnostics. These clocks never authorize latency or energy claims.
struct PreparedInferenceProfile {
    std::uint64_t setup_ns=0, input_validation_ns=0, tensor_setup_ns=0;
    std::uint64_t session_run_ns=0, output_copy_ns=0, telemetry_ns=0, total_ns=0;
    bool success=false;
};
class PreparedInference {
public:
    virtual ~PreparedInference() = default;
    virtual InferenceResult run(const TensorBuffer &) = 0;
    virtual InferenceResult runProfiled(const TensorBuffer &,PreparedInferenceProfile &profile) {
        profile={};
        throw std::runtime_error("prepared_profiling_unavailable");
    }
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
