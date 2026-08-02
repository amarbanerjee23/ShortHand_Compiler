#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
REPORT="/tmp/shorthand_backend_failure_mode_matrix.jsonl"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat > "${WORK_DIR}/backend_failure_mode_matrix.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"
#include "ai_runtime/AI_Runtime.h"
#include "ai_runtime/HardwareDiscovery.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace shorthand::ai;

namespace {
[[noreturn]] void fail(int code, const std::string &message) {
    std::cerr << "error: " << message << "\n";
    std::exit(code);
}

void require(bool condition, int code, const std::string &message) {
    if (!condition) fail(code, message);
}

bool contains(const std::string &value, const std::string &needle) {
    return value.find(needle) != std::string::npos;
}

void emit(const std::string &name,
          const std::string &layer,
          const std::string &expected,
          const std::string &observed,
          const std::string &reason) {
    std::cout << "{\"schema\":\"shorthand.backend_failure_mode_matrix.v1\""
              << ",\"case\":\"" << name << "\""
              << ",\"layer\":\"" << layer << "\""
              << ",\"expected\":\"" << expected << "\""
              << ",\"observed\":\"" << observed << "\""
              << ",\"reason\":\"" << reason << "\""
              << ",\"false_success\":false}\n";
}

void registerTensorPair(const char *input_shape, const char *input_elements,
                        const char *output_shape, const char *output_elements) {
    require(short_ai_register_tensor("input", "float32", input_shape, "1", input_elements) == SHORTHAND_RUNTIME_OK,
            2, "input tensor registration failed");
    require(short_ai_register_tensor("output", "float32", output_shape, "1", output_elements) == SHORTHAND_RUNTIME_OK,
            3, "output tensor registration failed");
}

void registerModel(const char *format, const char *precision, const char *backend,
                   const char *input_shape = "1", const char *output_shape = "1") {
    require(short_ai_register_model("model", format, "models/not_a_real_fixture", "inference",
                                    precision, input_shape, output_shape, backend) == SHORTHAND_RUNTIME_OK,
            4, "model registration failed");
}

BackendCapabilities syntheticBackend(BackendKind kind,
                                     const std::string &name,
                                     bool available,
                                     ModelFormat format,
                                     std::vector<std::string> precisions) {
    BackendCapabilities capability;
    capability.kind = kind;
    capability.name = name;
    capability.available = available;
    capability.supported_precisions = std::move(precisions);
    capability.supports_onnx = format == ModelFormat::Onnx;
    capability.supports_engine = format == ModelFormat::TensorRTEngine;
    capability.supports_torchscript = format == ModelFormat::TorchScript;
    capability.supports_openvino_ir = format == ModelFormat::OpenVINOIR;
    capability.supports_gguf = format == ModelFormat::GGUF;
    return capability;
}

HardwareDeviceCapability syntheticDevice(DeviceClass kind,
                                         const std::string &id,
                                         bool detected,
                                         bool accessible,
                                         std::size_t memory_mb = 16384) {
    HardwareDeviceCapability device;
    device.device_class = kind;
    device.device_id = id;
    device.provider = "failure_matrix";
    device.detected = detected;
    device.accessible = accessible;
    device.memory_mb = memory_mb;
    device.reason = accessible ? "injected_accessible" : "injected_inaccessible";
    return device;
}

ModelSpec onnxModel(const std::string &precision = "float32", bool allow_fallback = true) {
    ModelSpec model;
    model.name = "matrix_model";
    model.path = "models/not_a_real_fixture.onnx";
    model.format = ModelFormat::Onnx;
    model.precision = precision;
    model.backend_preference = {BackendKind::OnnxRuntimeCPU};
    model.allow_fallback = allow_fallback;
    model.input.name = "input";
    model.input.element_type = ElementType::Float32;
    model.input.shape = {1};
    model.input.element_count = 1;
    model.output.name = "output";
    model.output.element_type = ElementType::Float32;
    model.output.shape = {1};
    model.output.element_count = 1;
    return model;
}

TensorBuffer scalarInput(float value = 42.0f) {
    TensorBuffer input;
    input.spec.name = "input";
    input.spec.element_type = ElementType::Float32;
    input.spec.shape = {1};
    input.spec.element_count = 1;
    input.f32_data = {value};
    return input;
}
} // namespace

int main() {
    float input_values[2] = {42.0f, 7.0f};

    // 1. Invalid model format must be rejected by the public typed-buffer bridge.
    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 10, "reset failed");
    registerTensorPair("1", "1", "1", "1");
    registerModel("invalid_format", "float32", "onnxruntime_cpu");
    float invalid_format_output[1] = {-901.0f};
    int invalid_format_count = -1;
    const int invalid_format_status = short_ai_infer_f32(
        "model", "input", input_values, 1, "output", invalid_format_output, 1, &invalid_format_count);
    require(invalid_format_status == SHORTHAND_RUNTIME_INVALID_INPUT, 11, "invalid model format was not rejected");
    require(invalid_format_count == 0 && invalid_format_output[0] == -901.0f, 12,
            "invalid model format copied output");
    require(std::string(short_runtime_last_infer_reason()) == "ai_runtime_adapter_request_not_execution_ready", 13,
            "invalid model format reason mismatch");
    require(short_runtime_infer_success_count() == 0, 14, "invalid model format incremented success counter");
    emit("invalid_model_format", "compiled_hook", "invalid_input", "invalid_input",
         short_runtime_last_infer_reason());

    // 2. Missing optional SDK must use honest fallback and leave output untouched.
    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 20, "reset failed");
    registerTensorPair("1", "1", "1", "1");
    registerModel("onnx", "float32", "onnxruntime_cpu");
    float missing_sdk_output[1] = {-902.0f};
    int missing_sdk_count = -1;
    const int missing_sdk_status = short_ai_infer_f32(
        "model", "input", input_values, 1, "output", missing_sdk_output, 1, &missing_sdk_count);
    require(missing_sdk_status == SHORTHAND_RUNTIME_NOT_EXECUTED, 21, "missing SDK did not return not_executed");
    require(missing_sdk_count == 0 && missing_sdk_output[0] == -902.0f, 22, "missing SDK copied output");
    require(std::string(short_runtime_last_infer_backend()) == "fallback", 23, "missing SDK did not identify fallback");
    require(std::string(short_runtime_last_infer_reason()) == "backend_not_available", 24, "missing SDK reason mismatch");
    require(short_runtime_infer_success_count() == 0, 25, "missing SDK incremented success counter");
    require(std::strstr(short_runtime_last_infer_telemetry_json(), "\"selected\":false") != nullptr, 26,
            "missing SDK telemetry selected hardware");
    emit("missing_sdk", "compiled_hook", "not_executed", "not_executed",
         short_runtime_last_infer_reason());

    // 3. Input shape/count mismatch must be rejected before backend execution.
    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 30, "reset failed");
    registerTensorPair("2", "2", "1", "1");
    registerModel("onnx", "float32", "onnxruntime_cpu", "2", "1");
    float shape_output[1] = {-903.0f};
    int shape_count = -1;
    const int shape_status = short_ai_infer_f32(
        "model", "input", input_values, 1, "output", shape_output, 1, &shape_count);
    require(shape_status == SHORTHAND_RUNTIME_INVALID_INPUT, 31, "shape mismatch was not rejected");
    require(shape_count == 0 && shape_output[0] == -903.0f, 32, "shape mismatch copied output");
    require(std::string(short_runtime_last_infer_reason()) == "typed_buffer_shape_or_capacity_mismatch", 33,
            "shape mismatch reason mismatch");
    emit("input_shape_mismatch", "compiled_hook", "invalid_input", "invalid_input",
         short_runtime_last_infer_reason());

    // 4. Unsupported precision must not produce an execution-ready route.
    const std::vector<HardwareDeviceCapability> cpu_devices = {
        syntheticDevice(DeviceClass::CPU, "cpu:0", true, true)
    };
    const std::vector<BackendCapabilities> cpu_backends = {
        syntheticBackend(BackendKind::OnnxRuntimeCPU, "onnxruntime_cpu", true,
                         ModelFormat::Onnx, {"float32"})
    };
    HardwareRoutingPolicy cpu_only;
    cpu_only.preference = {DeviceClass::CPU};
    cpu_only.allow_cpu_fallback = false;
    const auto precision_route = selectHardwareRoute(cpu_devices, cpu_backends, onnxModel("float64", false), cpu_only);
    require(!precision_route.selected, 40, "unsupported precision selected a route");
    require(contains(precision_route.inventory_json, "\"backend_compatible\":false"), 41,
            "unsupported precision was marked backend-compatible");
    require(contains(precision_route.selection_json, "\"selected\":false"), 42,
            "unsupported precision selection evidence mismatch");
    emit("unsupported_precision", "hardware_router", "no_execution_ready_route", "no_execution_ready_route",
         "precision_not_supported_by_backend");

    // 5. Output capacity mismatch must fail before execution and preserve the caller buffer.
    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 50, "reset failed");
    registerTensorPair("1", "1", "2", "2");
    registerModel("onnx", "float32", "onnxruntime_cpu", "1", "2");
    float capacity_output[1] = {-904.0f};
    int capacity_count = -1;
    const int capacity_status = short_ai_infer_f32(
        "model", "input", input_values, 1, "output", capacity_output, 1, &capacity_count);
    require(capacity_status == SHORTHAND_RUNTIME_INVALID_INPUT, 51, "capacity mismatch was not rejected");
    require(capacity_count == 0 && capacity_output[0] == -904.0f, 52, "capacity mismatch copied output");
    require(std::string(short_runtime_last_infer_reason()) == "typed_buffer_shape_or_capacity_mismatch", 53,
            "capacity mismatch reason mismatch");
    emit("output_capacity_mismatch", "compiled_hook", "invalid_input", "invalid_input",
         short_runtime_last_infer_reason());

    // 6. Detected but inaccessible hardware must never become execution-ready.
    const std::vector<HardwareDeviceCapability> inaccessible_gpu = {
        syntheticDevice(DeviceClass::GPU, "gpu:0", true, false)
    };
    const std::vector<BackendCapabilities> cuda_backends = {
        syntheticBackend(BackendKind::OnnxRuntimeCUDA, "onnxruntime_cuda", true,
                         ModelFormat::Onnx, {"float32"})
    };
    ModelSpec cuda_model = onnxModel();
    cuda_model.backend_preference = {BackendKind::OnnxRuntimeCUDA};
    cuda_model.allow_fallback = false;
    HardwareRoutingPolicy gpu_only;
    gpu_only.preference = {DeviceClass::GPU};
    gpu_only.allow_cpu_fallback = false;
    const auto inaccessible_route = selectHardwareRoute(inaccessible_gpu, cuda_backends, cuda_model, gpu_only);
    require(!inaccessible_route.selected, 60, "inaccessible GPU selected a route");
    require(contains(inaccessible_route.inventory_json, "\"detected\":true"), 61,
            "inaccessible GPU was not recorded as detected");
    require(contains(inaccessible_route.inventory_json, "\"accessible\":false"), 62,
            "inaccessible GPU evidence mismatch");
    require(contains(inaccessible_route.inventory_json, "\"execution_ready\":false"), 63,
            "inaccessible GPU was marked execution-ready");
    emit("inaccessible_hardware", "hardware_router", "no_execution_ready_route", "no_execution_ready_route",
         "device_detected_but_inaccessible");

    // 7. A failed/empty hardware probe must remain claim-safe and fall back without execution.
    auto empty_probe = std::make_shared<StaticHardwareProbe>(std::vector<HardwareDeviceCapability>{});
    HardwareRoutingPolicy default_policy;
    AIRuntime empty_probe_runtime(empty_probe, default_policy);
    const auto empty_probe_result = empty_probe_runtime.infer(onnxModel(), scalarInput());
    require(empty_probe_result.status == InferenceStatus::NotExecuted, 70,
            "empty hardware probe did not return not_executed fallback");
    require(empty_probe_result.backend == BackendKind::Fallback && empty_probe_result.backend_name == "fallback", 71,
            "empty hardware probe did not use honest fallback");
    require(empty_probe_result.output_f32.empty(), 72, "empty hardware probe produced output");
    require(contains(empty_probe_result.hardware_inventory_json, "\"devices\":[]"), 73,
            "empty hardware probe inventory mismatch");
    require(contains(empty_probe_result.hardware_selection_json, "\"selected\":false"), 74,
            "empty hardware probe selected hardware");
    emit("hardware_probe_empty", "ai_runtime", "not_executed", "not_executed",
         empty_probe_result.reason);

    // 8. Fallback itself must never report success or carry output data.
    auto cpu_probe = std::make_shared<StaticHardwareProbe>(cpu_devices);
    AIRuntime fallback_runtime(cpu_probe, default_policy);
    const auto fallback_result = fallback_runtime.infer(onnxModel(), scalarInput());
    require(fallback_result.status == InferenceStatus::NotExecuted, 80,
            "fallback reported an unexpected status");
    require(fallback_result.backend == BackendKind::Fallback && fallback_result.backend_name == "fallback", 81,
            "fallback identity mismatch");
    require(fallback_result.reason == "backend_not_available", 82, "fallback reason mismatch");
    require(fallback_result.output_f32.empty(), 83, "fallback returned output values");
    require(!contains(fallback_result.telemetry_json_fragment, "\"status\":\"success\""), 84,
            "fallback telemetry claimed success");
    emit("fallback_honesty", "ai_runtime", "not_executed", "not_executed",
         fallback_result.reason);

    std::cout << "PASS backend failure-mode matrix gate\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -DSHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1 \
  -DSHORTHAND_HAS_ONNXRUNTIME=0 \
  -DSHORTHAND_HAS_TENSORRT=0 \
  -DSHORTHAND_HAS_OPENVINO=0 \
  -DSHORTHAND_HAS_LIBTORCH=0 \
  -DSHORTHAND_HAS_LLAMACPP=0 \
  -DSHORTHAND_HAS_OPENBLAS=0 \
  -DSHORTHAND_HAS_EIGEN=0 \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/backend_failure_mode_matrix.cpp" \
  "${SRC_DIR}/runtime/ShorthandRuntime.cpp" \
  "${SRC_DIR}/runtime/AIRuntimeBridgeAdapter.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Types.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Telemetry.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Backend.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" \
  "${SRC_DIR}/ai_runtime/backends/FallbackBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/TensorRTBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/OpenVINOBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/LibTorchBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/LlamaCppBackend.cpp" \
  -o "${WORK_DIR}/backend_failure_mode_matrix"

"${WORK_DIR}/backend_failure_mode_matrix" \
  >/tmp/shorthand_backend_failure_mode_matrix.out \
  2>/tmp/shorthand_backend_failure_mode_matrix.err

grep '^{' /tmp/shorthand_backend_failure_mode_matrix.out > "${REPORT}"
grep -q 'PASS backend failure-mode matrix gate' /tmp/shorthand_backend_failure_mode_matrix.out

if [[ "$(wc -l < "${REPORT}" | tr -d ' ')" != "8" ]]; then
  echo "error: backend failure-mode report must contain exactly 8 rows" >&2
  cat "${REPORT}" >&2 || true
  exit 1
fi

for required_case in \
  invalid_model_format \
  missing_sdk \
  input_shape_mismatch \
  unsupported_precision \
  output_capacity_mismatch \
  inaccessible_hardware \
  hardware_probe_empty \
  fallback_honesty; do
  grep -q "\"case\":\"${required_case}\"" "${REPORT}"
done

if grep -q '"false_success":true\|"observed":"success"' "${REPORT}"; then
  echo "error: backend failure-mode matrix contains a false success" >&2
  cat "${REPORT}" >&2 || true
  exit 1
fi

cat "${REPORT}"
grep 'PASS backend failure-mode matrix gate' /tmp/shorthand_backend_failure_mode_matrix.out
printf 'Backend failure-mode matrix report: %s\n' "${REPORT}"
