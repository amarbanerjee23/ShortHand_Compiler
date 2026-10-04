#ifndef SHORT_AI_RUNTIME_H
#define SHORT_AI_RUNTIME_H

#include "AI_Backend.h"
#include "HardwareDiscovery.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct TensorData { std::vector<int64_t> shape; std::vector<float> data; };

namespace shorthand::ai {
// Caller-owned, single-entry cache. Access must be serialized by the caller.
// Only self-contained ONNX FP32 models <=16 MiB are cached. Each call rereads
// and compares the complete model bytes, including in-place file replacements.
class PreparedInferenceCache {
public:
    void clear() { session_.reset(); snapshot_.clear(); candidate_snapshot_.clear(); model_=ModelSpec{}; }
    std::uint64_t preparations() const { return preparations_; }
private:
    friend class AIRuntime;
    ModelSpec model_;
    std::vector<unsigned char> snapshot_;
    // Reuse comparison storage, but reread all bytes on every request. Together
    // with snapshot_ this retains at most two 16 MiB model buffers.
    std::vector<unsigned char> candidate_snapshot_;
    std::unique_ptr<PreparedInference> session_;
    std::uint64_t preparations_=0;
};
class AIRuntime {
public:
    AIRuntime();
    AIRuntime(std::shared_ptr<HardwareProbe> hardware_probe, HardwareRoutingPolicy hardware_policy);
    InferenceResult infer(const ModelSpec &model, const TensorBuffer &input);
    // Preserves request-time routing qualification while reusing an unchanged
    // inventory/route generation. Unsupported models use the existing uncached
    // backend, with its original failure semantics.
    InferenceResult inferCached(const ModelSpec &,const TensorBuffer &,PreparedInferenceCache &);
    // Same public validation and model-content checks as inferCached, but can
    // bind backend output into caller-supplied scratch. Transactional callers
    // must use private/runtime-owned scratch and commit only after success.
    InferenceResult inferCachedInto(const ModelSpec &,const TensorBuffer &,PreparedInferenceCache &,
                                    float *output,std::size_t output_elements);
    // The serialized C bridge retains this runtime, while environment policy
    // remains request-scoped. Explicit-policy application runtimes opt in only.
    void refreshPolicyFromEnvironment();
    std::unique_ptr<PreparedInference> prepare(const ModelSpec &,const InferenceConfiguration &,std::string &error);
    std::vector<BackendCapabilities> capabilities() const;

private:
    struct ResolvedRoute {
        std::shared_ptr<const HardwareRoute> route;
        bool cache_hit=false;
        std::uint64_t hardware_probes=0;
    };
    InferenceResult inferImpl(const ModelSpec &,const TensorBuffer &,PreparedInferenceCache *,
                              float *output,std::size_t output_elements);
    ResolvedRoute routeForModel(const ModelSpec &);
    BackendRegistry registry;
    std::shared_ptr<HardwareProbe> hardware_probe_;
    HardwareRoutingPolicy hardware_policy_;
    HardwareRoutingPolicyEnvironmentState policy_environment_state_{};
    bool policy_environment_state_valid_=false;
    std::uint64_t hardware_generation_token_=0;
    std::vector<HardwareDeviceCapability> hardware_devices_;
    bool hardware_inventory_valid_=false;
    ModelSpec route_model_;
    std::shared_ptr<const HardwareRoute> route_cache_;
    std::uint64_t hardware_probe_count_=0;
    mutable std::mutex state_mutex_;
};
}

class AI_Runtime {
public:
    AI_Runtime(){}
    bool loadModel(const std::string &model_path);
    bool run(const TensorData &input_tensor, std::vector<float> &output);
    std::string getLastError() const;

private:
    std::string model_path_;
    std::string last_error_;
};
#endif
