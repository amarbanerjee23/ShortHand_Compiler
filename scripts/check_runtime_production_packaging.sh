#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/runtime_production_packaging.md"
TEST="${ROOT_DIR}/tests/packaging/test_runtime_production_packaging.sh"
CMAKE_FILE="${ROOT_DIR}/CMakeLists.txt"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"

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
  "${ROOT_DIR}/cmake/shorthand-ai-bridge.pc.in" \
  "${ROOT_DIR}/cmake/shorthand-core.pc.in"; do
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
done

bash -n "${TEST}"

for anchor in \
  'runtime_packaging_contract_version: 1.0.0' \
  'runtime_packaging_status: installable_static_shared_and_consumer_checked' \
  'runtime_shared_soversion: 1' \
  'ai_bridge_packaging_status: adapter_static_shared_and_consumer_checked' \
  'core_packaging_status: core_ffi_static_shared_and_consumer_checked' \
  'serving_packaging_status: bounded_scheduler_worker_and_consumer_checked' \
  'production_claim_boundary: packaging_gate_is_not_full_production_readiness'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'project(ShortHand VERSION 1.0.0 LANGUAGES CXX)' \
  'add_library(shorthand_runtime_shared SHARED' \
  'add_library(shorthand_ai_bridge STATIC' \
  'add_library(shorthand_ai_bridge_shared SHARED' \
  'add_library(shorthand_core STATIC' \
  'add_library(shorthand_core_shared SHARED' \
  'add_library(shorthand_serving STATIC' \
  'add_executable(shorthand_serving_worker' \
  'add_executable(shorthand_c3eco_measure' \
  'SOVERSION ${SHORTHAND_RUNTIME_ABI_VERSION_MAJOR}' \
  'install(EXPORT ShortHandTargets' \
  'configure_package_config_file(' \
  'write_basic_package_version_file(' \
  'shorthand-runtime.pc' \
  'shorthand-ai-bridge.pc' \
  'shorthand-core.pc'; do
  require_contains "${CMAKE_FILE}" "${anchor}"
done

require_contains "${TEST}" 'find_package(ShortHand 1 CONFIG REQUIRED)'
require_contains "${TEST}" 'ShortHand::runtime_shared'
require_contains "${TEST}" 'ShortHand::ai_bridge_shared'
require_contains "${TEST}" 'ShortHand::core_shared'
require_contains "${TEST}" 'ShortHand::serving'
require_contains "${TEST}" 'shorthand_c3eco_measure'
require_contains "${TEST}" "require_installed '*/bin/shorthand_c3eco_measure'"
require_contains "${TEST}" 'SONAME.*libshorthand_runtime'
require_contains "${TEST}" 'pkg-config --modversion shorthand-runtime'
require_contains "${TEST}" 'PASS production runtime AI bridge core FFI and serving packaging consumer gate'

bash "${TEST}"

# CI runs sanitizer builds before CTest. The Makefile sanitizer target leaves an
# ASan/UBSan-instrumented runtime archive in the shared workspace. Force a clean
# non-sanitized ABI artifact before the ABI consumer gate so validation never
# depends on stale objects produced by an earlier build mode.
make -C "${SRC_DIR}" -B runtime_lib \
  >/tmp/shorthand_runtime_packaging_abi_rebuild.out 2>&1 || {
    cat /tmp/shorthand_runtime_packaging_abi_rebuild.out >&2 || true
    exit 1
  }

bash "${ROOT_DIR}/scripts/check_runtime_abi_api_stability.sh"

echo "PASS runtime production packaging guard"
