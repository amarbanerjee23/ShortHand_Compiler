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
    echo "error: ${file} missing required backend live SDK matrix text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/backend_live_sdk_matrix.md
require_file docs/backend_compatibility_matrix.md
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file scripts/check_compiled_hook_onnxruntime_success.sh

require_contains docs/backend_live_sdk_matrix.md 'backend_live_sdk_matrix_status: optional_matrix_harness'
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/backend_live_sdk_matrix.md 'live_success'
require_contains docs/backend_live_sdk_matrix.md 'skip_safe'
require_contains docs/backend_live_sdk_matrix.md 'policy_compatible_only'
require_contains docs/backend_live_sdk_matrix.md 'dedicated_fixture_planned'
require_contains docs/backend_live_sdk_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/backend_compatibility_matrix.md 'Backend live SDK matrix harness'
require_contains docs/backend_compatibility_matrix.md 'backend_live_sdk_matrix_status: optional_matrix_harness'

require_contains tests/integration/test_backend_live_sdk_matrix.sh 'BACKEND_MATRIX backend=onnxruntime_cpu'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'BACKEND_MATRIX backend=tensorrt'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'BACKEND_MATRIX backend=openvino'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'BACKEND_MATRIX backend=libtorch'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'BACKEND_MATRIX backend=llamacpp'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'compiled_hook_success_fixture_passed'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'dedicated_fixture_planned'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'policy_compatible_only'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix harness'

bash tests/integration/test_backend_live_sdk_matrix.sh

echo "PASS backend live SDK matrix gate"
