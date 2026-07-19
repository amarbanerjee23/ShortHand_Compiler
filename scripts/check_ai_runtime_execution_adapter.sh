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

require_file Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h
require_file Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp
require_file tests/codegen/test_ai_runtime_bridge_adapter.sh
require_file docs/ai_runtime_execution_adapter.md
require_file docs/ai_runtime_bridge_linkage.md

require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h 'RuntimeBridgeModelInput'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h 'RuntimeBridgeTensorInput'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h 'runtimeStatusFromInferenceStatus'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'shorthand.runtime.ai_runtime_execution_adapter.v1'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'buildModelSpec'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'buildInputTensorBuffer'
require_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'bridgeRequestIsExecutionReady'
require_contains tests/codegen/test_ai_runtime_bridge_adapter.sh 'PASS AI runtime bridge adapter compile and mapping test'
require_contains docs/ai_runtime_execution_adapter.md 'adapter_contract_status: compile_checked_mapping_only'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime execution adapter'

require_not_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'AIRuntime runtime;'
require_not_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp '.infer('

bash tests/codegen/test_ai_runtime_bridge_adapter.sh

echo "PASS AI runtime execution adapter gate"
