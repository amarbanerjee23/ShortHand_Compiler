#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

DOC="docs/backend_failure_mode_matrix.md"
TEST="tests/integration/test_backend_failure_mode_matrix.sh"
REPORT="/tmp/shorthand_backend_failure_mode_matrix.jsonl"

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
    echo "error: ${file} missing required backend failure-mode text: ${needle}" >&2
    exit 1
  fi
}

require_file "${DOC}"
require_file "${TEST}"
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h

require_contains "${DOC}" 'backend_failure_mode_matrix_status: finalized_v1'
require_contains "${DOC}" 'schema: shorthand.backend_failure_mode_matrix.v1'
require_contains "${DOC}" 'production_claim_boundary: failure_evidence_is_not_backend_success_evidence'
require_contains "${DOC}" 'false_success_allowed: false'
require_contains "${DOC}" '`invalid_model_format`'
require_contains "${DOC}" '`missing_sdk`'
require_contains "${DOC}" '`input_shape_mismatch`'
require_contains "${DOC}" '`unsupported_precision`'
require_contains "${DOC}" '`output_capacity_mismatch`'
require_contains "${DOC}" '`inaccessible_hardware`'
require_contains "${DOC}" '`hardware_probe_empty`'
require_contains "${DOC}" '`fallback_honesty`'
require_contains "${DOC}" 'Output count is zero on every non-success status.'
require_contains "${DOC}" 'Fallback always reports `not_executed`'
require_contains "${DOC}" 'Execution readiness does not imply successful execution'

require_contains "${TEST}" 'shorthand.backend_failure_mode_matrix.v1'
require_contains "${TEST}" 'invalid_model_format'
require_contains "${TEST}" 'missing_sdk'
require_contains "${TEST}" 'input_shape_mismatch'
require_contains "${TEST}" 'unsupported_precision'
require_contains "${TEST}" 'output_capacity_mismatch'
require_contains "${TEST}" 'inaccessible_hardware'
require_contains "${TEST}" 'hardware_probe_empty'
require_contains "${TEST}" 'fallback_honesty'
require_contains "${TEST}" 'PASS backend failure-mode matrix gate'
require_contains "${TEST}" '"false_success":true\|"observed":"success"'

require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'typed_buffer_shape_or_capacity_mismatch'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_adapter_request_not_execution_ready'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'backend_not_available'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'no_execution_ready_hardware_backend'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'executionReadyBackend'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'execution_ready'

bash -n "${TEST}"
bash "${TEST}"

require_file "${REPORT}"
if [[ "$(wc -l < "${REPORT}" | tr -d ' ')" != "8" ]]; then
  echo "error: backend failure-mode matrix report must contain exactly 8 rows" >&2
  exit 1
fi

if grep -q '"false_success":true\|"observed":"success"' "${REPORT}"; then
  echo "error: backend failure-mode matrix report contains a false success" >&2
  cat "${REPORT}" >&2 || true
  exit 1
fi

printf 'PASS backend failure-mode matrix guard\n'
