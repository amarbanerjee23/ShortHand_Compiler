#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace shorthand::ai {

enum class ElementType { Float32, Float16, BFloat16, Int8, Int4, Int32, Unknown };
enum class ModelFormat { Onnx, TensorRTEngine, TorchScript, OpenVINOIR, GGUF, Unknown };
enum class BackendKind { TensorRT, OnnxRuntimeTensorRT, OnnxRuntimeCUDA, OnnxRuntimeCPU, OpenVINO, LibTorch, LlamaCpp, Fallback };
enum class InferenceStatus { Success, NotExecuted, BackendUnavailable, InvalidInput, RuntimeError };

struct TensorSpec { std::string name; ElementType element_type = ElementType::Unknown; std::vector<int64_t> shape; bool dynamic = false; size_t element_count = 0; };
struct TensorBuffer { TensorSpec spec; std::vector<float> f32_data; std::vector<int8_t> i8_data; };
struct ModelSpec { std::string name; std::string path; ModelFormat format = ModelFormat::Unknown; std::string task; std::string precision; TensorSpec input; TensorSpec output; std::vector<BackendKind> backend_preference; bool compact = false; bool allow_fallback = true; std::string quality_metric; std::string quality_op; double quality_threshold = 0.0; };
struct BackendCapabilities { BackendKind kind = BackendKind::Fallback; std::string name; bool available = false; bool supports_onnx = false; bool supports_engine = false; bool supports_torchscript = false; bool supports_openvino_ir = false; bool supports_gguf = false; std::vector<std::string> supported_precisions; std::string unavailable_reason; };
struct InferenceResult { InferenceStatus status = InferenceStatus::NotExecuted; BackendKind backend = BackendKind::Fallback; std::string backend_name = "fallback"; std::string provider_name = "none"; std::string reason = "backend_not_available"; std::vector<float> output_f32; std::string evidence_json_fragment; };

ElementType parseElementType(const std::string &value);
ModelFormat parseModelFormat(const std::string &value);
BackendKind parseBackendKind(const std::string &value);
std::string backendKindToString(BackendKind kind);
std::string modelFormatToString(ModelFormat format);
std::string inferenceStatusToString(InferenceStatus status);
std::vector<int64_t> parseShapeCsv(const std::string &csv);
std::vector<float> parseFloatCsv(const std::string &csv);
size_t productOfShape(const std::vector<int64_t> &shape);
bool validateShape(const std::vector<int64_t> &shape);
bool validateInputMatchesShape(const TensorBuffer &input);
bool backendSupportsFormat(const BackendCapabilities &caps, ModelFormat format);

} // namespace shorthand::ai
