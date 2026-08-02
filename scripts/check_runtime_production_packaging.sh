#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/runtime_production_packaging.md"
TEST="${ROOT_DIR}/tests/packaging/test_runtime_production_packaging.sh"
CMAKE_FILE="${ROOT_DIR}/CMakeLists.txt"

require_contains() {
  local file="$1"
  local needle="$2"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required packaging text: ${needle}" >&2
    exit 1
  fi
}

for file in \
  "${DOC}" \
  "${TEST}" \
  "${ROOT_DIR}/cmake/ShortHandConfig.cmake.in" \
  "${ROOT_DIR}/cmake/shorthand-runtime.pc.in" \
  "${ROOT_DIR}/cmake/shorthand-ai-bridge.pc.in"; do
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
done

bash -n "${TEST}"

for anchor in \
  'runtime_packaging_contract_version: 1.0.0' \
  'runtime_packaging_status: installable_static_shared_and_consumer_checked' \
  'runtime_shared_soversion: 1' \
  'ai_bridge_packaging_status: adapter_static_shared_and_consumer_checked' \
  'production_claim_boundary: packaging_gate_is_not_full_production_readiness'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'project(ShortHand VERSION 1.0.0 LANGUAGES CXX)' \
  'add_library(shorthand_runtime_shared SHARED' \
  'add_library(shorthand_ai_bridge STATIC' \
  'add_library(shorthand_ai_bridge_shared SHARED' \
  'SOVERSION ${SHORTHAND_RUNTIME_ABI_VERSION_MAJOR}' \
  'install(EXPORT ShortHandTargets' \
  'configure_package_config_file(' \
  'write_basic_package_version_file(' \
  'shorthand-runtime.pc' \
  'shorthand-ai-bridge.pc'; do
  require_contains "${CMAKE_FILE}" "${anchor}"
done

require_contains "${TEST}" 'find_package(ShortHand 1 CONFIG REQUIRED)'
require_contains "${TEST}" 'ShortHand::runtime_shared'
require_contains "${TEST}" 'ShortHand::ai_bridge_shared'
require_contains "${TEST}" 'SONAME.*libshorthand_runtime'
require_contains "${TEST}" 'pkg-config --modversion shorthand-runtime'
require_contains "${TEST}" 'PASS production runtime and AI bridge packaging consumer gate'

bash "${TEST}"

bash "${ROOT_DIR}/scripts/check_runtime_abi_api_stability.sh"

echo "PASS runtime production packaging guard"
