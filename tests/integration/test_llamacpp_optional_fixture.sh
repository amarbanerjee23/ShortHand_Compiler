#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

LLAMACPP_MACRO=0
LLAMACPP_REASON="LLAMACPP_ROOT_not_set"
if [[ -n "${LLAMACPP_ROOT:-}" ]]; then
  LLAMACPP_MACRO=1
  LLAMACPP_REASON="LLAMACPP_ROOT_set_but_direct_execution_not_enabled"
fi

cat > "${WORK_DIR}/llamacpp_optional_fixture_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"

#include <cstring>
#include <iostream>

int main() {
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 1;
    if (short_ai_register_tensor("input", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 2;
    if (short_ai_register_tensor("output", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 3;
    if (short_ai_register_model("llama_model", "gguf", "models/not_a_real_fixture.gguf", "text_generation", "float32", "1", "1", "llamacpp") != SHORTHAND_RUNTIME_OK) return 4;

    float input_values[1] = {42.0f};
    float output_values[1] = {-999.0f};
    int output_count = -1;

    const int status = short_ai_infer_f32("llama_model", "input", input_values, 1, "output", output_values, 1, &output_count);

    if (status == SHORTHAND_RUNTIME_OK) {
        std::cerr << "error: Llama.cpp unavailable-path fixture returned success\n";
        return 10;
    }
    if (status != SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE && status != SHORTHAND_RUNTIME_NOT_EXECUTED) {
        std::cerr << "error: unexpected Llama.cpp unavailable-path status=" << status << "\n";
        return 11;
    }
    if (short_runtime_infer_success_count() != 0) {
        std::cerr << "error: Llama.cpp unavailable-path fixture incremented success counter\n";
        return 12;
    }
    if (output_count > 0) {
        std::cerr << "error: Llama.cpp unavailable-path fixture copied outputs\n";
        return 13;
    }
    if (output_values[0] != -999.0f) {
        std::cerr << "error: Llama.cpp unavailable-path fixture mutated output buffer\n";
        return 14;
    }

    const char *bridge_json = short_runtime_infer_bridge_request_json();
    if (std::strstr(bridge_json, "shorthand.runtime.typed_infer_buffer_bridge_request.v1") == nullptr) return 15;
    if (std::strstr(bridge_json, "\"status\":\"success\"") != nullptr) return 16;
    if (std::strstr(bridge_json, "\"output_count\":0") == nullptr) return 17;

    const char *telemetry_json = short_runtime_last_infer_telemetry_json();
    if (std::strstr(telemetry_json, "shorthand.ai_runtime.telemetry.v2") == nullptr) return 18;
    if (std::strstr(telemetry_json, "shorthand.hardware.inventory.v1") == nullptr) return 19;
    if (std::strstr(telemetry_json, "shorthand.hardware.selection.v1") == nullptr) return 20;
    if (std::strstr(telemetry_json, "\"selected\":false") == nullptr) return 21;
    if (std::strstr(telemetry_json, "\"execution_ready\":true") != nullptr) return 22;

    const char *observability_json = short_runtime_observability_json();
    if (std::strstr(observability_json, "\"infer_success\":0") == nullptr) return 23;

    std::cout << "LLAMACPP_FIXTURE backend=llamacpp status=unavailable_path_proved reason="
              << short_runtime_last_infer_reason() << " runtime_status=" << status << "\n";
    std::cout << "PASS llamacpp optional fixture gate\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -DSHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1 \
  -DSHORTHAND_HAS_ONNXRUNTIME=0 \
  -DSHORTHAND_HAS_TENSORRT=0 \
  -DSHORTHAND_HAS_OPENVINO=0 \
  -DSHORTHAND_HAS_LIBTORCH=0 \
  -DSHORTHAND_HAS_LLAMACPP=${LLAMACPP_MACRO} \
  -DSHORTHAND_HAS_OPENBLAS=0 \
  -DSHORTHAND_HAS_EIGEN=0 \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/llamacpp_optional_fixture_probe.cpp" \
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
  -o "${WORK_DIR}/llamacpp_optional_fixture_probe"

"${WORK_DIR}/llamacpp_optional_fixture_probe" \
  >/tmp/shorthand_llamacpp_optional_fixture.out \
  2>/tmp/shorthand_llamacpp_optional_fixture.err

grep -q 'PASS llamacpp optional fixture gate' /tmp/shorthand_llamacpp_optional_fixture.out
if grep -qi 'status=success\|SHORTHAND_RUNTIME_OK\|live_success' /tmp/shorthand_llamacpp_optional_fixture.out /tmp/shorthand_llamacpp_optional_fixture.err; then
  echo "error: Llama.cpp optional fixture made a false success claim" >&2
  cat /tmp/shorthand_llamacpp_optional_fixture.out >&2 || true
  cat /tmp/shorthand_llamacpp_optional_fixture.err >&2 || true
  exit 1
fi

cat /tmp/shorthand_llamacpp_optional_fixture.out
printf 'LLAMACPP_FIXTURE_ENV %s\n' "${LLAMACPP_REASON}"
