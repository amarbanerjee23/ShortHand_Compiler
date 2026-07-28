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
    echo "error: ${file} missing required OpenVINO fixture text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/openvino_optional_fixture.md
require_file tests/integration/test_openvino_optional_fixture.sh
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OpenVINOBackend.cpp
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file docs/backend_live_sdk_matrix.md

require_contains docs/openvino_optional_fixture.md 'openvino_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/openvino_optional_fixture.md 'shorthand.backend_openvino_optional_fixture.v1'
require_contains docs/openvino_optional_fixture.md 'production_claim_boundary: not production-executing yet'
require_contains docs/openvino_optional_fixture.md 'must return non-success with no output copied'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OpenVINOBackend.cpp 'OpenVINO SDK detected but direct execution is not yet enabled'
require_contains tests/integration/test_openvino_optional_fixture.sh 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1'
require_contains tests/integration/test_openvino_optional_fixture.sh 'SHORTHAND_HAS_OPENVINO=${OPENVINO_MACRO}'
require_contains tests/integration/test_openvino_optional_fixture.sh 'short_ai_infer_f32("openvino_model"'
require_contains tests/integration/test_openvino_optional_fixture.sh 'unavailable_path_proved'
require_contains tests/integration/test_openvino_optional_fixture.sh 'PASS openvino optional fixture gate'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'test_openvino_optional_fixture.sh'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'openvino_unavailable_path_proved_no_false_success'
require_contains docs/backend_live_sdk_matrix.md 'OpenVINO unavailable-path proof'

bash tests/integration/test_openvino_optional_fixture.sh

echo "PASS OpenVINO optional fixture gate"
