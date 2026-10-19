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
#include <iostream>
#include <string>

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

    // The existing builder stays owning. Borrowing is explicit, synchronous
    // and rejects null/count mismatches before any backend dereferences it.
    input_values[0] = 42.0f;
    if (input_buffer.f32Data() == input_values || input_buffer.f32Data()[0] != 1.0f) return 13;
    input_buffer.bindExternalF32(input_values, 4);
    if (!input_buffer.f32_data.empty() || input_buffer.f32Data() != input_values || input_buffer.f32Size() != 4) return 14;
    if (!shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, 2)) return 15;
    input_buffer.bindExternalF32(nullptr, 4);
    if (shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, 2)) return 16;
    input_buffer.bindExternalF32(input_values, 0);
    if (shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, 2)) return 17;
    input_buffer.bindExternalF32(input_values, 3);
    if (shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, 2)) return 18;
    input_buffer.bindExternalF32(input_values, 4);
    auto result = runtime.infer(model_spec, input_buffer);
    if (result.status != shorthand::ai::InferenceStatus::NotExecuted) return 11;
    if (shorthand::runtime_bridge::runtimeStatusFromInferenceStatus(result.status) != SHORTHAND_RUNTIME_NOT_EXECUTED) return 12;

    input_buffer.clearExternalF32();
    if (input_buffer.hasF32() || input_buffer.f32Size() != 0) return 19;

    // Capture a request, change registrations BEFORE the first observation,
    // and require the observation to retain the call's original descriptors.
    short_ai_infer_f32("classifier", "input", input_values, 4, "output", output_values, 2, &output_count);
    short_ai_register_model("classifier", "onnx", "changed.onnx", "other", "float32", "1,4", "1,2", "onnxruntime_cpu");
    short_ai_register_tensor("input", "int8", "99", "1", "99");
    std::cout << "REQUEST " << short_runtime_infer_bridge_request_json() << '\n';
    std::cout << "TELEMETRY " << short_runtime_last_infer_telemetry_json() << '\n';
    std::cout << "OBSERVABILITY " << short_runtime_observability_json() << '\n';
    std::cout << "OTLP " << short_runtime_otlp_spans_json() << '\n';
    const std::string first = short_runtime_infer_bridge_request_json();
    if (first != short_runtime_infer_bridge_request_json()) return 20;
    short_ai_infer_f32("missing", "input", input_values, 4, "output", output_values, 2, &output_count);
    std::cout << "REJECTED " << short_runtime_infer_bridge_request_json() << '\n';
    std::cout << "REJECTED_TELEMETRY " << short_runtime_last_infer_telemetry_json() << '\n';
    short_runtime_reset();
    std::cout << "RESET " << short_runtime_infer_bridge_request_json() << '\n';
    std::cout << "RESET_TELEMETRY " << short_runtime_last_infer_telemetry_json() << '\n';
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

"${WORK_DIR}/runtime_ai_bridge_link_probe" > "${WORK_DIR}/observations.txt"
python3 - "${WORK_DIR}/observations.txt" <<'PYTEST'
import json, pathlib, sys
observations = dict(line.split(' ', 1) for line in pathlib.Path(sys.argv[1]).read_text().splitlines())
rows = {name: json.loads(value) for name, value in observations.items()}
request = rows['REQUEST']
assert request['model']['path'] == 'models/classifier.onnx'
assert request['input']['element_type'] == 'float32' and request['input']['shape'] == '1,4'
assert request['input_element_count'] == 4 and request['output_count'] == 0
assert rows['TELEMETRY']['status'] == 'not_executed'
assert rows['OBSERVABILITY']['infer_bridge_request'] == request
assert rows['OTLP']['spans'][0]['infer_bridge_request'] == request
assert rows['OTLP']['spans'][0]['last_infer_telemetry'] == rows['TELEMETRY']
assert rows['REJECTED'] == {} and rows['REJECTED_TELEMETRY']['status'] == 'model_not_found'
assert rows['RESET'] == {} and rows['RESET_TELEMETRY'] == {}
print('PASS borrowed input validation, owned adapter compatibility and deferred observation snapshots')
PYTEST

echo "PASS runtime AI bridge link build gate"
