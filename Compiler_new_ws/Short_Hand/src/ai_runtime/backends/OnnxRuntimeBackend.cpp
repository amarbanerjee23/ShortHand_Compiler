#include "OnnxRuntimeBackend.h"

#include <sstream>
#include <string>

#if SHORTHAND_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace shorthand::ai {
namespace {

std::string stripQuotes(std::string value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string shapeToString(const std::vector<int64_t> &shape) {
    std::ostringstream out;
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i) out << ",";
        out << shape[i];
    }
    return out.str();
}

#if SHORTHAND_HAS_ONNXRUNTIME
Ort::Env &ortEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "shorthand_onnxruntime_cpu");
    return env;
}

std::string firstInputName(Ort::Session &session, Ort::AllocatorWithDefaultOptions &allocator) {
#if ORT_API_VERSION >= 12
    auto allocated = session.GetInputNameAllocated(0, allocator);
    return allocated ? allocated.get() : "";
#else
    char *name = session.GetInputName(0, allocator);
    std::string out = name ? name : "";
    allocator.Free(name);
    return out;
#endif
}

std::string firstOutputName(Ort::Session &session, Ort::AllocatorWithDefaultOptions &allocator) {
#if ORT_API_VERSION >= 12
    auto allocated = session.GetOutputNameAllocated(0, allocator);
    return allocated ? allocated.get() : "";
#else
    char *name = session.GetOutputName(0, allocator);
    std::string out = name ? name : "";
    allocator.Free(name);
    return out;
#endif
}
#endif

} // namespace

BackendKind OnnxRuntimeBackend::kind() const { return BackendKind::OnnxRuntimeCPU; }

std::string OnnxRuntimeBackend::name() const { return "onnxruntime_cpu"; }

BackendCapabilities OnnxRuntimeBackend::capabilities() const {
    BackendCapabilities c;
    c.kind = kind();
    c.name = name();
    c.supports_onnx = true;
    c.supported_precisions = {"float32", "float16", "int8", "int32"};
#if SHORTHAND_HAS_ONNXRUNTIME
    c.available = true;
#else
    c.available = false;
    c.unavailable_reason = "ONNX Runtime backend not built. Set ONNXRUNTIME_ROOT and enable SHORTHAND_ENABLE_ONNXRUNTIME.";
#endif
    return c;
}

bool OnnxRuntimeBackend::canLoad(const ModelSpec &m) const {
    auto c = capabilities();
    return c.available && backendSupportsFormat(c, m.format);
}

InferenceResult OnnxRuntimeBackend::infer(const ModelSpec &model, const TensorBuffer &input) {
    InferenceResult r;
    r.backend = kind();
    r.backend_name = name();
    r.provider_name = "onnxruntime_cpu";

    if (!validateInputMatchesShape(input)) {
        r.status = InferenceStatus::InvalidInput;
        r.reason = "input_shape_mismatch";
        return r;
    }

    if (model.format != ModelFormat::Onnx) {
        r.status = InferenceStatus::InvalidInput;
        r.reason = "onnxruntime_cpu_requires_onnx_model";
        return r;
    }

    if (input.f32_data.empty()) {
        r.status = InferenceStatus::InvalidInput;
        r.reason = "onnxruntime_cpu_currently_requires_float32_input";
        return r;
    }

#if SHORTHAND_HAS_ONNXRUNTIME
    try {
        const std::string model_path = stripQuotes(model.path);
        if (model_path.empty()) {
            r.status = InferenceStatus::InvalidInput;
            r.reason = "missing_model_path";
            return r;
        }

        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        Ort::Session session(ortEnv(), model_path.c_str(), session_options);
        Ort::AllocatorWithDefaultOptions allocator;

        if (session.GetInputCount() == 0 || session.GetOutputCount() == 0) {
            r.status = InferenceStatus::RuntimeError;
            r.reason = "onnx_model_missing_input_or_output";
            return r;
        }

        std::string input_name = firstInputName(session, allocator);
        std::string output_name = firstOutputName(session, allocator);
        if (input_name.empty() || output_name.empty()) {
            r.status = InferenceStatus::RuntimeError;
            r.reason = "onnxruntime_failed_to_read_io_names";
            return r;
        }

        std::vector<int64_t> input_shape = input.spec.shape;
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info,
            const_cast<float *>(input.f32_data.data()),
            input.f32_data.size(),
            input_shape.data(),
            input_shape.size());

        const char *input_names[] = {input_name.c_str()};
        const char *output_names[] = {output_name.c_str()};
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            input_names,
            &input_tensor,
            1,
            output_names,
            1);

        if (output_tensors.empty() || !output_tensors.front().IsTensor()) {
            r.status = InferenceStatus::RuntimeError;
            r.reason = "onnxruntime_output_not_tensor";
            return r;
        }

        auto type_info = output_tensors.front().GetTensorTypeAndShapeInfo();
        if (type_info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            r.status = InferenceStatus::RuntimeError;
            r.reason = "onnxruntime_output_not_float32";
            return r;
        }

        const size_t count = type_info.GetElementCount();
        float *output_data = output_tensors.front().GetTensorMutableData<float>();
        r.output_f32.assign(output_data, output_data + count);
        r.status = InferenceStatus::Success;
        r.reason = "executed";
        r.evidence_json_fragment = "{\"runtime_backend\":\"onnxruntime_cpu\",\"provider\":\"CPUExecutionProvider\",\"inference_status\":\"success\",\"input_shape\":\"" + shapeToString(input_shape) + "\",\"output_elements\":" + std::to_string(count) + "}";
        return r;
    } catch (const Ort::Exception &ex) {
        r.status = InferenceStatus::RuntimeError;
        r.reason = std::string("onnxruntime_exception:") + ex.what();
        return r;
    } catch (const std::exception &ex) {
        r.status = InferenceStatus::RuntimeError;
        r.reason = std::string("std_exception:") + ex.what();
        return r;
    }
#else
    r.status = InferenceStatus::BackendUnavailable;
    r.reason = capabilities().unavailable_reason;
    return r;
#endif
}

} // namespace shorthand::ai
