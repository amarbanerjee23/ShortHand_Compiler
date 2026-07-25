#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat > "${WORK_DIR}/runtime_ai_bridge_link_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"
#include "runtime/AIRuntimeBridgeAdapter.h"
#include "ai_runtime/AI_Runtime.h"

#include <cstring>

int main() {
    if (std::strcmp(shorthand::runtime_bridge::bridgeAdapterContractVersion(),
                    "shorthand.runtime.ai_runtime_execution_adapter.v1") != 0) return 1;

    shorthand::ai::AIRuntime runtime;
    if (runtime.capabilities().empty()) return 2;

    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 3;
    if (short_ai_register_tensor("input", "float32", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 4;
    if (short_ai_register_tensor("output", "float32", "1,2", "2", "2") != SHORTHAND_RUNTIME_OK) return 5;
    if (short_ai_register_model("classifier", "onnx", "models/classifier.onnx", "classification", "float32", "1,4", "1,2", "onnxruntime_cpu") != SHORTHAND_RUNTIME_OK) return 6;

    float input_values[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float output_values[2] = {0.0f, 0.0f};
    int output_count = -1;
    const int hook_status = short_ai_infer_f32("classifier", "input", input_values, 4, "output", output_values, 2, &output_count);
    if (hook_status != SHORTHAND_RUNTIME_NOT_EXECUTED) return 7;
    if (output_count != 0) return 8;
    if (std::strstr(short_runtime_infer_bridge_request_json(), "shorthand.runtime.typed_infer_buffer_bridge_request.v1") == nullptr) return 9;

    shorthand::runtime_bridge::RuntimeBridgeModelInput model{
        "classifier", "onnx", "models/classifier.onnx", "classification", "float32", "1,4", "1,2", "onnxruntime_cpu", true
    };
    shorthand::runtime_bridge::RuntimeBridgeTensorInput input{
        "input", "float32", "1,4", "2", "4"
    };
    shorthand::runtime_bridge::RuntimeBridgeTensorInput output{
        "output", "float32", "1,2", "2", "2"
    };

    auto model_spec = shorthand::runtime_bridge::buildModelSpec(model, input, output);
    auto input_buffer = shorthand::runtime_bridge::buildInputTensorBuffer(input, input_values, 4);
    if (!shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, 2)) return 10;

    auto result = runtime.infer(model_spec, input_buffer);
    if (result.status != shorthand::ai::InferenceStatus::NotExecuted) return 11;
    if (shorthand::runtime_bridge::runtimeStatusFromInferenceStatus(result.status) != SHORTHAND_RUNTIME_NOT_EXECUTED) return 12;

    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -DSHORTHAND_HAS_ONNXRUNTIME=0 \
  -DSHORTHAND_HAS_TENSORRT=0 \
  -DSHORTHAND_HAS_OPENVINO=0 \
  -DSHORTHAND_HAS_LIBTORCH=0 \
  -DSHORTHAND_HAS_LLAMACPP=0 \
  -DSHORTHAND_HAS_OPENBLAS=0 \
  -DSHORTHAND_HAS_EIGEN=0 \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/runtime_ai_bridge_link_probe.cpp" \
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
  -o "${WORK_DIR}/runtime_ai_bridge_link_probe"

"${WORK_DIR}/runtime_ai_bridge_link_probe"

echo "PASS runtime AI bridge link build gate"
