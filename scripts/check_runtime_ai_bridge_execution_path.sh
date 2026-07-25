#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  fi
}

require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file tests/codegen/test_runtime_ai_bridge_execution_path.sh
require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file scripts/check_compiled_hook_onnxruntime_success.sh
require_file docs/compiled_infer_bridge.md
require_file docs/ai_runtime_execution_adapter.md

require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'execute_typed_buffer_through_ai_runtime'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'AIRuntime runtime;'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'runtime.infer(model_spec, input_buffer)'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_execution_attempted'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_execution_succeeded'
require_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1'
require_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'backend_not_available'
require_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'PASS runtime AI bridge execution path gate'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'SHORTHAND_HAS_ONNXRUNTIME=1'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'short_ai_infer_f32("identity"'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'Output: 42'
require_contains scripts/check_compiled_hook_onnxruntime_success.sh 'PASS compiled hook ONNX Runtime success gate'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge execution path'
require_contains docs/compiled_infer_bridge.md 'Compiled hook ONNX Runtime success fixture'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_execution_status: bridge_enabled_ai_runtime_infer_attempt'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_success_status: optional_onnxruntime_success_fixture'

bash tests/codegen/test_runtime_ai_bridge_execution_path.sh
bash scripts/check_compiled_hook_onnxruntime_success.sh

echo "PASS runtime AI bridge execution path gate"
