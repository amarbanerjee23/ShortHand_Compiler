#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

DOC="docs/runtime_state_and_thread_safety.md"
FACADE="Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp"
HEADER="Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h"
MAKEFILE="Compiler_new_ws/Short_Hand/src/Makefile"
TEST="tests/runtime/test_runtime_state_thread_safety.sh"
MANIFEST="abi/runtime_public_symbols_v1.txt"

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
    echo "error: ${file} missing required runtime state text: ${needle}" >&2
    exit 1
  fi
}

for file in "${DOC}" "${FACADE}" "${HEADER}" "${MAKEFILE}" CMakeLists.txt "${TEST}" "${MANIFEST}"; do
  require_file "${file}"
done

require_contains "${DOC}" 'runtime_state_contract_version: 1.0.0'
require_contains "${DOC}" 'runtime_state_model: single_process_wide_default_context'
require_contains "${DOC}" 'runtime_thread_safety_status: serialized_public_abi'
require_contains "${DOC}" 'runtime_multi_tenant_isolation: process_boundary_required'
require_contains "${DOC}" 'production_claim_boundary: thread_safe_does_not_mean_multi_tenant_isolated'
require_contains "${DOC}" 'one process-wide recursive mutex'
require_contains "${DOC}" 'thread-local snapshot storage'
require_contains "${DOC}" 'independent tenants must use separate processes'
require_contains "${DOC}" 'It does not mean:'

require_contains "${FACADE}" 'std::recursive_mutex &runtimeMutex()'
require_contains "${FACADE}" 'std::lock_guard<std::recursive_mutex>'
require_contains "${FACADE}" 'thread_local std::string snapshot'
require_contains "${FACADE}" 'shimpl_runtime_reset'
require_contains "${FACADE}" 'extern "C" int short_runtime_reset(void)'
require_contains "${FACADE}" 'extern "C" int short_ai_infer_f32'
require_contains "${FACADE}" 'extern "C" const char *short_runtime_observability_json'

require_contains "${MAKEFILE}" 'RUNTIME_ABI_RENAME_DEFS'
require_contains "${MAKEFILE}" '-Dshort_runtime_reset=shimpl_runtime_reset'
require_contains "${MAKEFILE}" 'runtime/RuntimeThreadSafeFacade.cpp'
require_contains "${MAKEFILE}" 'RuntimeThreadSafeFacade.o'
require_contains CMakeLists.txt 'SHORTHAND_RUNTIME_ABI_RENAMES'
require_contains CMakeLists.txt 'short_runtime_reset=shimpl_runtime_reset'
require_contains CMakeLists.txt 'RuntimeThreadSafeFacade.cpp'
require_contains CMakeLists.txt 'Threads::Threads'

require_contains "${HEADER}" 'SHORTHAND_RUNTIME_ABI_VERSION_STRING "1.0.0"'
require_contains "${TEST}" 'PASS runtime state isolation and thread-safety gate'
require_contains "${TEST}" 'context=process_wide synchronization=serialized_public_abi'
require_contains "${TEST}" 'string_snapshots=thread_local tenant_isolation=process_boundary'
require_contains "${TEST}" 'public_symbol_count'

if [[ "$(grep -c '^short_' "${MANIFEST}")" != "25" ]]; then
  echo "error: ABI v1 manifest must continue to contain exactly 25 public symbols" >&2
  exit 1
fi

bash -n "${TEST}"
bash "${TEST}"
bash scripts/check_runtime_abi_api_stability.sh

printf 'PASS runtime state isolation and thread-safety guard\n'
