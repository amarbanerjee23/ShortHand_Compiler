#ifndef SHORT_AI_RUNTIME_H
#define SHORT_AI_RUNTIME_H

#include "AI_Backend.h"
#include "HardwareDiscovery.h"

#include <memory>
#include <string>
#include <vector>

struct TensorData { std::vector<int64_t> shape; std::vector<float> data; };

namespace shorthand::ai {
class AIRuntime {
public:
    AIRuntime();
    AIRuntime(std::shared_ptr<HardwareProbe> hardware_probe, HardwareRoutingPolicy hardware_policy);
    InferenceResult infer(const ModelSpec &model, const TensorBuffer &input);
    std::unique_ptr<PreparedInference> prepare(const ModelSpec &,const InferenceConfiguration &,std::string &error);
    std::vector<BackendCapabilities> capabilities() const;

private:
    BackendRegistry registry;
    std::shared_ptr<HardwareProbe> hardware_probe_;
    HardwareRoutingPolicy hardware_policy_;
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
