#include "OnnxRuntimeBackend.h"

#if SHORTHAND_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

#include <exception>

namespace shorthand::ai {

OnnxRuntimeBackend::OnnxRuntimeBackend(BackendKind kind) : kind_(kind) {}
BackendKind OnnxRuntimeBackend::kind() const { return kind_; }
std::string OnnxRuntimeBackend::name() const { return backendKindToString(kind_); }

BackendCapabilities OnnxRuntimeBackend::capabilities() const {
    BackendCapabilities caps;
    caps.kind = kind_;
    caps.name = name();
    caps.supports_onnx = true;
    caps.supported_precisions = {"fp32", "fp16", "int8"};
#if SHORTHAND_HAS_ONNXRUNTIME
    caps.available = true;
#else
    caps.available = false;
    caps.unavailable_reason = "ONNX Runtime backend not built. Set ONNXRUNTIME_ROOT and enable SHORTHAND_ENABLE_ONNXRUNTIME.";
#endif
    return caps;
}

bool OnnxRuntimeBackend::canLoad(const ModelSpec& model) const {
    return model.format == ModelFormat::Onnx;
}

InferenceResult OnnxRuntimeBackend::infer(const ModelSpec& model, const TensorBuffer& input) {
#if !SHORTHAND_HAS_ONNXRUNTIME
    (void)model;
    (void)input;
    return unavailableResult(kind_, name(), "ONNX Runtime backend not built. Set ONNXRUNTIME_ROOT and enable SHORTHAND_ENABLE_ONNXRUNTIME.");
#else
    try {
        if (!canLoad(model)) return unavailableResult(kind_, name(), "ONNX Runtime supports ONNX models only");
        std::string validation_error;
        if (!validateInputMatchesShape(model.input, input, &validation_error)) {
            InferenceResult invalid;
            invalid.status = InferenceStatus::InvalidInput;
            invalid.backend = kind_;
            invalid.backend_name = name();
            invalid.provider_name = "onnxruntime";
            invalid.reason = validation_error;
            return invalid;
        }

        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "shorthand_onnxruntime");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        Ort::Session session(env, model.path.c_str(), options);
        Ort::AllocatorWithDefaultOptions allocator;
        auto input_name_alloc = session.GetInputNameAllocated(0, allocator);
        auto output_name_alloc = session.GetOutputNameAllocated(0, allocator);
        const char* input_name = input_name_alloc.get();
        const char* output_name = output_name_alloc.get();

        std::vector<float> input_values = input.f32_data;
        if (input_values.empty() && model.input.element_count > 0) input_values.assign(model.input.element_count, 0.0f);
        std::vector<int64_t> dims = input.spec.shape.empty() ? model.input.shape : input.spec.shape;
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_values.data(), input_values.size(), dims.data(), dims.size());
        std::vector<const char*> input_names{input_name};
        std::vector<const char*> output_names{output_name};
        auto outputs = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1, output_names.data(), 1);
        float* output_data = outputs.front().GetTensorMutableData<float>();
        const size_t output_count = outputs.front().GetTensorTypeAndShapeInfo().GetElementCount();

        InferenceResult result;
        result.status = InferenceStatus::Success;
        result.backend = kind_;
        result.backend_name = name();
        result.provider_name = kind_ == BackendKind::OnnxRuntimeCPU ? "CPUExecutionProvider" : backendKindToString(kind_);
        result.output_f32.assign(output_data, output_data + output_count);
        result.reason = "ok";
        result.evidence_json_fragment = "\"runtime_backend\":\"" + result.backend_name + "\",\"inference_status\":\"success\"";
        return result;
    } catch (const std::exception& ex) {
        InferenceResult result;
        result.status = InferenceStatus::RuntimeError;
        result.backend = kind_;
        result.backend_name = name();
        result.provider_name = "onnxruntime";
        result.reason = ex.what();
        return result;
    }
#endif
}

}  // namespace shorthand::ai
