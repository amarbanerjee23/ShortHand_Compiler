#ifndef SHORT_AI_RUNTIME_H
#define SHORT_AI_RUNTIME_H

#include <string>
#include <vector>

struct TensorData {
    std::vector<int64_t> shape;
    std::vector<float> data;
};

class AI_Runtime {
public:
    AI_Runtime() {}
    ~AI_Runtime() {}

    bool loadModel(const std::string &model_path);
    bool run(const TensorData &input_tensor, std::vector<float> &output);
    std::string getLastError() const;

private:
    std::string model_path_;
    std::string last_error_;
};

#endif
