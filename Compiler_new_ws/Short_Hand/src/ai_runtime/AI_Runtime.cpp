#include "AI_Runtime.h"

#ifdef USE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

bool AI_Runtime::loadModel(const std::string &model_path) {
    this->model_path_ = model_path;
#ifndef USE_ONNXRUNTIME
    this->last_error_ = "ONNX Runtime support not enabled. Rebuild with ONNXRUNTIME_ROOT set.";
    return false;
#else
    this->last_error_.clear();
    return true;
#endif
}

bool AI_Runtime::run(const TensorData &input_tensor, std::vector<float> &output) {
#ifndef USE_ONNXRUNTIME
    (void)input_tensor;
    (void)output;
    this->last_error_ = "ONNX Runtime support not enabled. Rebuild with ONNXRUNTIME_ROOT set.";
    return false;
#else
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "short_ai_runtime");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        Ort::Session session(env, this->model_path_.c_str(), options);
        Ort::AllocatorWithDefaultOptions allocator;

        const char *input_name = session.GetInputName(0, allocator);
        const char *output_name = session.GetOutputName(0, allocator);

        std::vector<int64_t> dims = input_tensor.shape;
        std::vector<float> input_values = input_tensor.data;
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input = Ort::Value::CreateTensor<float>(
            memory_info,
            input_values.data(),
            input_values.size(),
            dims.data(),
            dims.size());

        std::vector<const char*> input_names{input_name};
        std::vector<const char*> output_names{output_name};

        auto outputs = session.Run(
            Ort::RunOptions{nullptr},
            input_names.data(),
            &input,
            1,
            output_names.data(),
            1);

        float *out = outputs[0].GetTensorMutableData<float>();
        auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
        size_t out_count = type_info.GetElementCount();

        output.assign(out, out + out_count);
        allocator.Free((void*)input_name);
        allocator.Free((void*)output_name);
        this->last_error_.clear();
        return true;
    } catch (const std::exception &e) {
        this->last_error_ = e.what();
        return false;
    }
#endif
}

std::string AI_Runtime::getLastError() const {
    return this->last_error_;
}
