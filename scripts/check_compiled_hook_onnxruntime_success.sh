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

require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file tests/fixtures/onnx/identity_float32_v13.onnx.b64
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp
require_file docs/compiled_infer_bridge.md
require_file docs/ai_runtime_execution_adapter.md

require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'requires ONNXRUNTIME_ROOT; mandatory production qualification cannot skip'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'SHORTHAND_HAS_ONNXRUNTIME=1'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'short_ai_infer_f32("identity"'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'SHORTHAND_RUNTIME_OK'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'ai_runtime_execution_succeeded'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'Output: 42'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'PASS compiled hook ONNX Runtime success gate'
require_contains docs/compiled_infer_bridge.md 'Compiled hook ONNX Runtime success fixture'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_success_status: optional_onnxruntime_success_fixture'

bash tests/integration/test_compiled_hook_onnxruntime_success.sh

echo "PASS compiled hook ONNX Runtime success gate"
