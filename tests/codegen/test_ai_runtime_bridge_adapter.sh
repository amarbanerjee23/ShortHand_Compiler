#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat > "${WORK_DIR}/bridge_adapter_probe.cpp" <<'CPP'
#include "runtime/AIRuntimeBridgeAdapter.h"
#include <cstring>
#include <vector>

int main() {
    using namespace shorthand::runtime_bridge;
    using namespace shorthand::ai;

    if (std::strcmp(bridgeAdapterContractVersion(), "shorthand.runtime.ai_runtime_execution_adapter.v1") != 0) return 10;

    RuntimeBridgeModelInput model{
        "classifier",
        "onnx",
        "models/classifier.onnx",
        "classification",
        "float32",
        "1,4",
        "1,2",
        "onnxruntime_cpu",
        true
    };
    RuntimeBridgeTensorInput input{"input", "float32", "1,4", "2", "4"};
    RuntimeBridgeTensorInput output{"output", "float32", "1,2", "2", "2"};

    ModelSpec model_spec = buildModelSpec(model, input, output);
    if (model_spec.name != "classifier") return 20;
    if (model_spec.format != ModelFormat::Onnx) return 21;
    if (model_spec.input.element_count != 4) return 22;
    if (model_spec.output.element_count != 2) return 23;
    if (model_spec.backend_preference.empty()) return 24;
    if (model_spec.backend_preference[0] != BackendKind::OnnxRuntimeCPU) return 25;

    float values[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    TensorBuffer buffer = buildInputTensorBuffer(input, values, 4);
    if (buffer.spec.name != "input") return 30;
    if (buffer.f32_data.size() != 4) return 31;
    if (!validateInputMatchesShape(buffer)) return 32;
    if (!bridgeRequestIsExecutionReady(model_spec, buffer, 2)) return 33;
    if (bridgeRequestIsExecutionReady(model_spec, buffer, 1)) return 34;

    if (runtimeStatusFromInferenceStatus(InferenceStatus::Success) != SHORTHAND_RUNTIME_OK) return 40;
    if (runtimeStatusFromInferenceStatus(InferenceStatus::NotExecuted) != SHORTHAND_RUNTIME_NOT_EXECUTED) return 41;
    if (runtimeStatusFromInferenceStatus(InferenceStatus::BackendUnavailable) != SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE) return 42;
    if (runtimeStatusFromInferenceStatus(InferenceStatus::InvalidInput) != SHORTHAND_RUNTIME_INVALID_INPUT) return 43;
    if (runtimeStatusFromInferenceStatus(InferenceStatus::RuntimeError) != SHORTHAND_RUNTIME_RUNTIME_ERROR) return 44;

    RuntimeBridgeModelInput fallback_model{
        "fallback_model", "onnx", "models/fallback.onnx", "classification", "float32", "1,4", "1,2", "", true
    };
    ModelSpec fallback_spec = buildModelSpec(fallback_model, input, output);
    if (fallback_spec.backend_preference.empty()) return 50;
    if (fallback_spec.backend_preference[0] != BackendKind::Fallback) return 51;

    return 0;
}
CPP

${CXX:-g++} -std=c++17 -I"${SRC_DIR}" \
  "${SRC_DIR}/runtime/AIRuntimeBridgeAdapter.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Types.cpp" \
  "${WORK_DIR}/bridge_adapter_probe.cpp" \
  -o "${WORK_DIR}/bridge_adapter_probe"

"${WORK_DIR}/bridge_adapter_probe"
echo "PASS AI runtime bridge adapter compile and mapping test"
