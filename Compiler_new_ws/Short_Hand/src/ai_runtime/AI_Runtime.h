#ifndef SHORT_AI_RUNTIME_H
#define SHORT_AI_RUNTIME_H

#include "AI_Backend.h"

#include <string>
#include <vector>

struct TensorData {
    std::vector<int64_t> shape;
    std::vector<float> data;
};

class AI_Runtime {
public:
    AI_Runtime() = default;
    ~AI_Runtime() = default;

    bool loadModel(const std::string& model_path);
    bool run(const TensorData& input_tensor, std::vector<float>& output);
    std::string getLastError() const;

private:
    std::string model_path_;
    std::string last_error_;
};

namespace shorthand::ai {

class AIRuntime {
public:
    AIRuntime();
    InferenceResult infer(const ModelSpec& model, const TensorBuffer& input);
    std::vector<BackendCapabilities> capabilities() const;

private:
    BackendRegistry registry_;
};

}  // namespace shorthand::ai

#endif
