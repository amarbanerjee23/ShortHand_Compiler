#include "AI_Types.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace shorthand::ai {
namespace {
std::string normalized(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

ElementType parseElementType(const std::string& value) {
    const std::string v = normalized(value);
    if (v == "float" || v == "fp32" || v == "float32") return ElementType::Float32;
    if (v == "fp16" || v == "float16") return ElementType::Float16;
    if (v == "bf16" || v == "bfloat16") return ElementType::BFloat16;
    if (v == "int8") return ElementType::Int8;
    if (v == "int4") return ElementType::Int4;
    if (v == "int" || v == "int32") return ElementType::Int32;
    return ElementType::Unknown;
}

ModelFormat parseModelFormat(const std::string& value) {
    const std::string v = normalized(value);
    if (v == "onnx") return ModelFormat::Onnx;
    if (v == "engine" || v == "tensorrt_engine") return ModelFormat::TensorRTEngine;
    if (v == "torchscript") return ModelFormat::TorchScript;
    if (v == "openvino_ir") return ModelFormat::OpenVINOIR;
    if (v == "gguf") return ModelFormat::GGUF;
    return ModelFormat::Unknown;
}

BackendKind parseBackendKind(const std::string& value) {
    const std::string v = normalized(value);
    if (v == "tensorrt") return BackendKind::TensorRT;
    if (v == "onnxruntime_tensorrt") return BackendKind::OnnxRuntimeTensorRT;
    if (v == "onnxruntime_cuda") return BackendKind::OnnxRuntimeCUDA;
    if (v == "onnxruntime_cpu" || v == "onnxruntime" || v == "ort_cpu") return BackendKind::OnnxRuntimeCPU;
    if (v == "openvino") return BackendKind::OpenVINO;
    if (v == "libtorch") return BackendKind::LibTorch;
    if (v == "llamacpp" || v == "llama.cpp") return BackendKind::LlamaCpp;
    return BackendKind::Fallback;
}

std::string elementTypeToString(ElementType value) {
    switch (value) {
        case ElementType::Float32: return "fp32";
        case ElementType::Float16: return "fp16";
        case ElementType::BFloat16: return "bf16";
        case ElementType::Int8: return "int8";
        case ElementType::Int4: return "int4";
        case ElementType::Int32: return "int32";
        case ElementType::Unknown: return "unknown";
    }
    return "unknown";
}

std::string modelFormatToString(ModelFormat value) {
    switch (value) {
        case ModelFormat::Onnx: return "onnx";
        case ModelFormat::TensorRTEngine: return "engine";
        case ModelFormat::TorchScript: return "torchscript";
        case ModelFormat::OpenVINOIR: return "openvino_ir";
        case ModelFormat::GGUF: return "gguf";
        case ModelFormat::Unknown: return "unknown";
    }
    return "unknown";
}

std::string backendKindToString(BackendKind value) {
    switch (value) {
        case BackendKind::TensorRT: return "tensorrt";
        case BackendKind::OnnxRuntimeTensorRT: return "onnxruntime_tensorrt";
        case BackendKind::OnnxRuntimeCUDA: return "onnxruntime_cuda";
        case BackendKind::OnnxRuntimeCPU: return "onnxruntime_cpu";
        case BackendKind::OpenVINO: return "openvino";
        case BackendKind::LibTorch: return "libtorch";
        case BackendKind::LlamaCpp: return "llamacpp";
        case BackendKind::Fallback: return "fallback";
    }
    return "fallback";
}

std::string inferenceStatusToString(InferenceStatus value) {
    switch (value) {
        case InferenceStatus::Success: return "success";
        case InferenceStatus::NotExecuted: return "not_executed";
        case InferenceStatus::BackendUnavailable: return "backend_unavailable";
        case InferenceStatus::InvalidInput: return "invalid_input";
        case InferenceStatus::RuntimeError: return "runtime_error";
    }
    return "runtime_error";
}

std::vector<int64_t> parseShapeCsv(const std::string& csv, bool* dynamic, std::string* error) {
    if (dynamic) *dynamic = false;
    std::vector<int64_t> shape;
    if (csv == "dynamic") {
        if (dynamic) *dynamic = true;
        return shape;
    }
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            if (error) *error = "shape contains an empty dimension";
            return {};
        }
        char* end = nullptr;
        const long long value = std::strtoll(token.c_str(), &end, 10);
        if (end == token.c_str() || *end != '\0' || value <= 0) {
            if (error) *error = "shape dimension must be a positive integer: " + token;
            return {};
        }
        shape.push_back(static_cast<int64_t>(value));
    }
    if (shape.empty() && error) *error = "shape must not be empty";
    return shape;
}

std::vector<float> parseFloatCsv(const std::string& csv, std::string* error) {
    std::vector<float> values;
    if (csv.empty()) return values;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        char* end = nullptr;
        const float value = std::strtof(token.c_str(), &end);
        if (end == token.c_str() || *end != '\0') {
            if (error) *error = "input value is not a float: " + token;
            return {};
        }
        values.push_back(value);
    }
    return values;
}

size_t productOfShape(const std::vector<int64_t>& shape) {
    size_t product = 1;
    for (int64_t dim : shape) {
        if (dim <= 0) return 0;
        if (product > std::numeric_limits<size_t>::max() / static_cast<size_t>(dim)) return 0;
        product *= static_cast<size_t>(dim);
    }
    return shape.empty() ? 0 : product;
}

bool validateShape(const std::vector<int64_t>& shape, bool dynamic, std::string* error) {
    if (dynamic) return true;
    if (shape.empty()) {
        if (error) *error = "static shape must contain at least one dimension";
        return false;
    }
    for (int64_t dim : shape) {
        if (dim <= 0) {
            if (error) *error = "shape dimensions must be positive";
            return false;
        }
    }
    return true;
}

bool validateInputMatchesShape(const TensorSpec& spec, const TensorBuffer& input, std::string* error) {
    if (!spec.dynamic && input.spec.shape != spec.shape) {
        if (error) *error = "input tensor shape does not match model input shape";
        return false;
    }
    const size_t expected = spec.dynamic ? input.f32_data.size() : productOfShape(spec.shape);
    if (!input.f32_data.empty() && expected != 0 && input.f32_data.size() != expected) {
        if (error) *error = "input tensor element count does not match shape";
        return false;
    }
    return true;
}

bool backendSupportsFormat(BackendKind backend, ModelFormat format) {
    if (backend == BackendKind::Fallback) return true;
    switch (format) {
        case ModelFormat::Onnx:
            return backend == BackendKind::OnnxRuntimeCPU || backend == BackendKind::OnnxRuntimeCUDA ||
                   backend == BackendKind::OnnxRuntimeTensorRT || backend == BackendKind::OpenVINO ||
                   backend == BackendKind::TensorRT;
        case ModelFormat::TensorRTEngine:
            return backend == BackendKind::TensorRT;
        case ModelFormat::TorchScript:
            return backend == BackendKind::LibTorch;
        case ModelFormat::OpenVINOIR:
            return backend == BackendKind::OpenVINO;
        case ModelFormat::GGUF:
            return backend == BackendKind::LlamaCpp;
        case ModelFormat::Unknown:
            return false;
    }
    return false;
}

bool backendIsFallback(BackendKind backend) {
    return backend == BackendKind::Fallback;
}

}  // namespace shorthand::ai
