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
    echo "error: ${file} missing required Llama.cpp fixture text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/llamacpp_optional_fixture.md
require_file tests/integration/test_llamacpp_optional_fixture.sh
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/backends/LlamaCppBackend.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h

require_contains docs/llamacpp_optional_fixture.md 'llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/llamacpp_optional_fixture.md 'production_claim_boundary: not production-executing yet'
require_contains docs/llamacpp_optional_fixture.md 'must return non-success with no output copied'
require_contains docs/llamacpp_optional_fixture.md 'Hardware inventory may detect CPU, GPU, TPU, or NPU'
require_contains tests/integration/test_llamacpp_optional_fixture.sh 'LLAMACPP_FIXTURE backend=llamacpp status=unavailable_path_proved'
require_contains tests/integration/test_llamacpp_optional_fixture.sh 'PASS llamacpp optional fixture gate'
require_contains tests/integration/test_llamacpp_optional_fixture.sh 'shorthand.hardware.inventory.v1'
require_contains tests/integration/test_llamacpp_optional_fixture.sh '\"selected\":false'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/backends/LlamaCppBackend.cpp 'direct execution is not yet enabled'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'case BackendKind::LlamaCpp:'

bash tests/integration/test_llamacpp_optional_fixture.sh

echo 'PASS Llama.cpp optional fixture gate'
