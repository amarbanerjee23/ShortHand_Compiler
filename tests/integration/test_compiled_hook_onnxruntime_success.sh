#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
FIXTURE_B64="${ROOT_DIR}/tests/fixtures/onnx/identity_float32_v13.onnx.b64"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ -z "${ONNXRUNTIME_ROOT:-}" ]]; then
  echo "SKIP compiled_hook_onnxruntime_success: ONNXRUNTIME_ROOT is not set"
  exit 0
fi

if [[ ! -f "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" ]]; then
  echo "error: ONNXRUNTIME_ROOT does not contain include/onnxruntime_cxx_api.h" >&2
  exit 1
fi

if [[ ! -d "${ONNXRUNTIME_ROOT}/lib" ]]; then
  echo "error: ONNXRUNTIME_ROOT does not contain lib/" >&2
  exit 1
fi

if command -v base64 >/dev/null 2>&1; then
  if base64 --help 2>&1 | grep -q -- '--decode'; then
    base64 --decode "${FIXTURE_B64}" > "${WORK_DIR}/identity.onnx"
  else
    base64 -D "${FIXTURE_B64}" > "${WORK_DIR}/identity.onnx"
  fi
else
  echo "error: base64 command not found" >&2
  exit 1
fi

cat > "${WORK_DIR}/compiled_hook_onnxruntime_success_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"

#include <cmath>
#include <cstring>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    const char *model_path = argv[1];

    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 2;
    if (short_ai_register_tensor("input", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 3;
    if (short_ai_register_tensor("output", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 4;
    if (short_ai_register_model("identity", "onnx", model_path, "identity", "float32", "1", "1", "onnxruntime_cpu") != SHORTHAND_RUNTIME_OK) return 5;

    float input_values[1] = {42.0f};
    float output_values[1] = {-1.0f};
    int output_count = -1;

    const int status = short_ai_infer_f32("identity", "input", input_values, 1, "output", output_values, 1, &output_count);

    if (status != SHORTHAND_RUNTIME_OK) {
        std::cerr << "status=" << status << " reason=" << short_runtime_last_infer_reason() << " backend=" << short_runtime_last_infer_backend() << "\n";
        return 10;
    }
    if (short_runtime_last_infer_status() != SHORTHAND_RUNTIME_OK) return 11;
    if (std::strcmp(short_runtime_last_infer_backend(), "onnxruntime_cpu") != 0) return 12;
    if (std::strstr(short_runtime_last_infer_reason(), "executed") == nullptr) return 13;
    if (output_count != 1) return 14;
    if (std::fabs(output_values[0] - 42.0f) > 0.0001f) return 15;
    if (short_runtime_infer_success_count() != 1) return 16;

    const char *bridge_json = short_runtime_infer_bridge_request_json();
    if (std::strstr(bridge_json, "shorthand.runtime.typed_infer_buffer_bridge_request.v1") == nullptr) return 17;
    if (std::strstr(bridge_json, "ai_runtime_execution_succeeded") == nullptr) return 18;
    if (std::strstr(bridge_json, "\"execution_ready\":true") == nullptr) return 19;
    if (std::strstr(bridge_json, "\"status\":\"success\"") == nullptr) return 20;
    if (std::strstr(bridge_json, "\"output_count\":1") == nullptr) return 21;

    const char *telemetry_json = short_runtime_last_infer_telemetry_json();
    if (std::strstr(telemetry_json, "ai_runtime_telemetry") == nullptr) return 22;
    if (std::strstr(telemetry_json, "onnxruntime_cpu") == nullptr) return 23;

    std::cout << "Output: " << output_values[0] << "\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -DSHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1 \
  -DSHORTHAND_HAS_ONNXRUNTIME=1 \
  -DSHORTHAND_HAS_TENSORRT=0 \
  -DSHORTHAND_HAS_OPENVINO=0 \
  -DSHORTHAND_HAS_LIBTORCH=0 \
  -DSHORTHAND_HAS_LLAMACPP=0 \
  -DSHORTHAND_HAS_OPENBLAS=0 \
  -DSHORTHAND_HAS_EIGEN=0 \
  -I"${SRC_DIR}" \
  -I"${ONNXRUNTIME_ROOT}/include" \
  "${WORK_DIR}/compiled_hook_onnxruntime_success_probe.cpp" \
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
  -L"${ONNXRUNTIME_ROOT}/lib" -lonnxruntime \
  -o "${WORK_DIR}/compiled_hook_onnxruntime_success_probe"

LD_LIBRARY_PATH="${ONNXRUNTIME_ROOT}/lib:${LD_LIBRARY_PATH:-}" \
  "${WORK_DIR}/compiled_hook_onnxruntime_success_probe" "${WORK_DIR}/identity.onnx" \
  >/tmp/shorthand_compiled_hook_onnxruntime_success.out \
  2>/tmp/shorthand_compiled_hook_onnxruntime_success.err

grep -q 'Output: 42' /tmp/shorthand_compiled_hook_onnxruntime_success.out
if grep -qi 'fallback\|backend_not_available\|not_executed' /tmp/shorthand_compiled_hook_onnxruntime_success.out /tmp/shorthand_compiled_hook_onnxruntime_success.err; then
  echo "error: compiled hook ONNX Runtime success gate used fallback or did not execute" >&2
  cat /tmp/shorthand_compiled_hook_onnxruntime_success.out >&2 || true
  cat /tmp/shorthand_compiled_hook_onnxruntime_success.err >&2 || true
  exit 1
fi

echo "PASS compiled hook ONNX Runtime success gate"
