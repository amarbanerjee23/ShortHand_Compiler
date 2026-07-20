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

require_not_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} contains forbidden text: ${needle}" >&2
    exit 1
  fi
}

require_file tests/codegen/test_runtime_ai_bridge_link_build.sh
require_file Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file docs/ai_runtime_execution_adapter.md
require_file docs/compiled_infer_bridge.md

require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'PASS runtime AI bridge link build gate'
require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'short_ai_infer_f32'
require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'AIRuntime runtime'
require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'runtime.infer(model_spec, input_buffer)'
require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'SHORTHAND_HAS_ONNXRUNTIME=0'
require_contains docs/ai_runtime_execution_adapter.md 'bridge_link_status: runtime_adapter_ai_core_link_checked'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge link build'

require_not_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'extern "C" int short_ai_infer'
require_not_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'extern "C" int short_greenai_emit_event'

bash tests/codegen/test_runtime_ai_bridge_link_build.sh

echo "PASS runtime AI bridge link build gate"
