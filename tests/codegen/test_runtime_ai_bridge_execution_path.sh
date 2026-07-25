#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat > "${WORK_DIR}/runtime_ai_bridge_execution_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"

#include <cstring>

int main() {
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 1;
    if (short_ai_register_tensor("input", "float32", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 2;
    if (short_ai_register_tensor("output", "float32", "1,2", "2", "2") != SHORTHAND_RUNTIME_OK) return 3;
    if (short_ai_register_model("classifier", "onnx", "models/classifier.onnx", "classification", "float32", "1,4", "1,2", "onnxruntime_cpu") != SHORTHAND_RUNTIME_OK) return 4;

    float input_values[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float output_values[2] = {-1.0f, -1.0f};
    int output_count = -1;

    const int status = short_ai_infer_f32("classifier", "input", input_values, 4, "output", output_values, 2, &output_count);

    if (status != SHORTHAND_RUNTIME_NOT_EXECUTED) return 5;
    if (short_runtime_last_infer_status() != SHORTHAND_RUNTIME_NOT_EXECUTED) return 6;
    if (output_count != 0) return 7;
    if (output_values[0] != -1.0f || output_values[1] != -1.0f) return 8;

    if (std::strcmp(short_runtime_last_infer_backend(), "fallback") != 0) return 9;
    if (std::strstr(short_runtime_last_infer_reason(), "backend_not_available") == nullptr) return 10;
    if (std::strstr(short_runtime_last_infer_telemetry_json(), "backend_not_available") == nullptr) return 11;

    const char *bridge_json = short_runtime_infer_bridge_request_json();
    if (std::strstr(bridge_json, "shorthand.runtime.typed_infer_buffer_bridge_request.v1") == nullptr) return 12;
    if (std::strstr(bridge_json, "ai_runtime_execution_attempted") == nullptr) return 13;
    if (std::strstr(bridge_json, "\"execution_ready\":true") == nullptr) return 14;
    if (std::strstr(bridge_json, "\"status\":\"not_executed\"") == nullptr) return 15;
    if (std::strstr(bridge_json, "\"reason\":\"backend_not_available\"") == nullptr) return 16;

    const char *obs_json = short_runtime_observability_json();
    if (std::strstr(obs_json, "\"infer_not_executed\":1") == nullptr) return 17;
    if (std::strstr(obs_json, "ai_runtime_execution_attempted") == nullptr) return 18;

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
  "${WORK_DIR}/runtime_ai_bridge_execution_probe.cpp" \
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
  -o "${WORK_DIR}/runtime_ai_bridge_execution_probe"

"${WORK_DIR}/runtime_ai_bridge_execution_probe" >/tmp/shorthand_runtime_ai_bridge_execution.out 2>/tmp/shorthand_runtime_ai_bridge_execution.err

grep -q 'ai_runtime_bridge=attempted' /tmp/shorthand_runtime_ai_bridge_execution.err
grep -q 'backend_not_available' /tmp/shorthand_runtime_ai_bridge_execution.err

printf 'PASS runtime AI bridge execution path gate\n'
