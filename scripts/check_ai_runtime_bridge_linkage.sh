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

forbid_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} contains forbidden linkage text: ${needle}" >&2
    exit 1
  fi
}

AI_RUNTIME="Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp"
RUNTIME_HEADER="Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h"
RUNTIME_IMPL="Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp"
MAKEFILE="Compiler_new_ws/Short_Hand/src/Makefile"

require_file "${AI_RUNTIME}"
require_file "${RUNTIME_HEADER}"
require_file "${RUNTIME_IMPL}"
require_file "${MAKEFILE}"
require_file CMakeLists.txt
require_file docs/ai_runtime_bridge_linkage.md
require_file docs/compiled_infer_bridge.md

# AI_Runtime must not own public compiled-hook C symbols. Those are exported by
# runtime/ShorthandRuntime.* and later bridge into AI_Runtime through one owner.
forbid_contains "${AI_RUNTIME}" 'extern "C" int short_ai_infer'
forbid_contains "${AI_RUNTIME}" 'extern "C" int short_greenai_emit_event'
require_contains "${AI_RUNTIME}" 'shorthand_runtime_hook_integration_ready'
require_contains "${AI_RUNTIME}" 'AIRuntime::infer'
require_contains "${AI_RUNTIME}" 'AI_Runtime::run'

# The runtime hook library remains the public ABI owner.
require_contains "${RUNTIME_HEADER}" 'short_ai_infer_f32'
require_contains "${RUNTIME_HEADER}" 'short_runtime_infer_bridge_request_json'
require_contains "${RUNTIME_IMPL}" 'shorthand.runtime.typed_infer_buffer_bridge_request.v1'
require_contains "${RUNTIME_IMPL}" 'SHORTHAND_RUNTIME_NOT_EXECUTED'

# Build graph guardrails: the standalone runtime hook library should not pull in
# the SDK-backed runtime until a deliberate bridge target is added.
require_contains "${MAKEFILE}" 'SHORTHAND_RUNTIME_SRC := runtime/ShorthandRuntime.cpp'
require_contains CMakeLists.txt 'add_library(shorthand_runtime STATIC'
require_contains CMakeLists.txt '${SRC_DIR}/runtime/ShorthandRuntime.cpp'

# Documentation must state the ownership and current execution boundary.
require_contains docs/ai_runtime_bridge_linkage.md 'runtime-hook ABI ownership'
require_contains docs/ai_runtime_bridge_linkage.md 'duplicate C symbol'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime bridge linkage'
require_contains docs/compiled_infer_bridge.md 'runtime-hook ABI owner'

printf 'PASS AI runtime bridge linkage gate\n'
