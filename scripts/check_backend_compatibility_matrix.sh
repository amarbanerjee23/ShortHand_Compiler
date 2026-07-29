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
require_file docs/hardware_capability_routing.md
require_file docs/tensorrt_optional_fixture.md
require_file docs/openvino_optional_fixture.md
require_file docs/libtorch_optional_fixture.md
require_file docs/llamacpp_optional_fixture.md
require_file docs/compiled_infer_bridge.md
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file tests/integration/test_onnxruntime_sdk_gate.sh
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file tests/integration/test_hardware_capability_routing.sh
require_file tests/integration/test_tensorrt_optional_fixture.sh
require_file tests/integration/test_openvino_optional_fixture.sh
require_file tests/integration/test_libtorch_optional_fixture.sh
require_file tests/integration/test_llamacpp_optional_fixture.sh
require_file scripts/check_backend_live_sdk_matrix.sh
require_file scripts/check_hardware_capability_routing.sh
require_file scripts/check_tensorrt_optional_fixture.sh
require_file scripts/check_openvino_optional_fixture.sh
require_file scripts/check_libtorch_optional_fixture.sh
require_file scripts/check_llamacpp_optional_fixture.sh

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
require_contains docs/backend_compatibility_matrix.md 'openvino_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'libtorch_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'Hardware capability discovery boundary'
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/backend_live_sdk_matrix.md 'unavailable_path_proved'
require_contains docs/backend_live_sdk_matrix.md 'Llama.cpp unavailable-path proof'
require_contains docs/hardware_capability_routing.md 'production_claim_boundary: detection_is_not_execution_readiness'
require_contains docs/tensorrt_optional_fixture.md 'must return non-success with no output copied'
require_contains docs/openvino_optional_fixture.md 'must return non-success with no output copied'
require_contains docs/libtorch_optional_fixture.md 'must return non-success with no output copied'
require_contains docs/llamacpp_optional_fixture.md 'must return non-success with no output copied'
require_contains docs/llamacpp_optional_fixture.md 'production_claim_boundary: not production-executing yet'

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

# Runtime selection must preserve fallback honesty and require execution-ready hardware.
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<TensorRTBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<OnnxRuntimeBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<OpenVINOBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<LibTorchBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<LlamaCppBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'registry.registerBackend(std::make_unique<FallbackBackend>());'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'selectHardwareRoute'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'result.status = InferenceStatus::NotExecuted'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'backend_not_available'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'no_execution_ready_hardware_backend'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.inventory.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.selection.v1'

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

# Shared backend live SDK matrix plus backend claim-safety and hardware routing proofs.
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix harness'
require_contains scripts/check_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix gate'
require_contains tests/integration/test_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'
require_contains scripts/check_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'
require_contains tests/integration/test_tensorrt_optional_fixture.sh 'PASS tensorrt optional fixture gate'
require_contains scripts/check_tensorrt_optional_fixture.sh 'PASS TensorRT optional fixture gate'
require_contains tests/integration/test_openvino_optional_fixture.sh 'PASS openvino optional fixture gate'
require_contains scripts/check_openvino_optional_fixture.sh 'PASS OpenVINO optional fixture gate'
require_contains tests/integration/test_libtorch_optional_fixture.sh 'PASS libtorch optional fixture gate'
require_contains scripts/check_libtorch_optional_fixture.sh 'PASS LibTorch optional fixture gate'
require_contains tests/integration/test_llamacpp_optional_fixture.sh 'PASS llamacpp optional fixture gate'
require_contains scripts/check_llamacpp_optional_fixture.sh 'PASS Llama.cpp optional fixture gate'

bash scripts/check_hardware_capability_routing.sh
bash scripts/check_backend_live_sdk_matrix.sh

printf 'PASS backend compatibility matrix gate\n'
