#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "$file"
  if ! grep -Fq "$needle" "$file"; then
    echo "error: ${file} missing required backend-matrix text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/backend_compatibility_matrix.md
require_file docs/backend_live_sdk_matrix.md
require_file docs/tensorrt_optional_fixture.md
require_file docs/compiled_infer_bridge.md
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file tests/integration/test_onnxruntime_sdk_gate.sh
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file tests/integration/test_tensorrt_optional_fixture.sh
require_file scripts/check_backend_live_sdk_matrix.sh
require_file scripts/check_tensorrt_optional_fixture.sh

# Documentation must explicitly distinguish compatibility policy from real execution status.
require_contains docs/backend_compatibility_matrix.md 'Backend execution validation tiers'
require_contains docs/backend_compatibility_matrix.md 'policy_compatible'
require_contains docs/backend_compatibility_matrix.md 'sdk_execution_optional'
require_contains docs/backend_compatibility_matrix.md 'backend_live_sdk_matrix_harness'
require_contains docs/backend_compatibility_matrix.md 'compiled_hook_bridge_pending'
require_contains docs/backend_compatibility_matrix.md 'fallback must report `not_executed`'
require_contains docs/backend_compatibility_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/backend_compatibility_matrix.md 'typed buffer bridge'
require_contains docs/backend_compatibility_matrix.md 'trt_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/backend_live_sdk_matrix.md 'unavailable_path_proved'
require_contains docs/tensorrt_optional_fixture.md 'must return non-success with no output copied'

# All advertised formats and backend aliases must still be present in the parser/matrix contract.
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'ModelFormat::Onnx'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'ModelFormat::TensorRTEngine'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'ModelFormat::TorchScript'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'ModelFormat::OpenVINOIR'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'ModelFormat::GGUF'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'BackendKind::OnnxRuntimeCPU'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'BackendKind::TensorRT'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'BackendKind::OpenVINO'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'BackendKind::LibTorch'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'BackendKind::LlamaCpp'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp 'backendSupportsFormat'

# Runtime selection must preserve fallback honesty.
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<TensorRTBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<OnnxRuntimeBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<FallbackBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'r.status=InferenceStatus::NotExecuted'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'backend_not_available'

# The compiled hook bridge may validate metadata and typed buffers, but must remain honest unless SDK execution succeeds.
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.typed_infer_buffer_bridge_request.v1'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_typed_buffer_bridge_pending'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'SHORTHAND_RUNTIME_NOT_EXECUTED'
require_contains docs/compiled_infer_bridge.md 'typed buffer bridge'
require_contains docs/compiled_infer_bridge.md 'must not claim that compiled inference executed through ONNX Runtime'

# Optional SDK gates must skip safely without SDKs and must reject fallback on real execution runs.
require_contains tests/integration/test_onnxruntime_sdk_gate.sh 'SKIP onnxruntime_sdk_gate: ONNXRUNTIME_ROOT is not set'
require_contains tests/integration/test_onnxruntime_sdk_gate.sh 'PASS onnxruntime_sdk_gate: real ONNX Runtime CPU execution succeeded'
require_contains tests/integration/test_onnxruntime_sdk_gate.sh 'fallback\|backend_unavailable\|not_executed'

# Shared backend live SDK matrix and PR #53 TensorRT proof.
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix harness'
require_contains scripts/check_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix gate'
require_contains tests/integration/test_tensorrt_optional_fixture.sh 'PASS tensorrt optional fixture gate'
require_contains scripts/check_tensorrt_optional_fixture.sh 'PASS TensorRT optional fixture gate'
bash scripts/check_backend_live_sdk_matrix.sh

printf 'PASS backend compatibility matrix gate\n'
