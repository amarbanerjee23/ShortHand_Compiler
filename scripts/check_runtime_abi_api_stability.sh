#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

DOC="docs/runtime_abi_api_stability.md"
HEADER="Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h"
SNAPSHOT="abi/shorthand_runtime_abi_v1.h"
MANIFEST="abi/runtime_public_symbols_v1.txt"
TEST="tests/abi/test_runtime_abi_api_stability.sh"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local text="$2"
  require_file "${file}"
  if ! grep -Fq "${text}" "${file}"; then
    echo "error: ${file} missing required ABI/API text: ${text}" >&2
    exit 1
  fi
}

require_file "${DOC}"
require_file "${HEADER}"
require_file "${SNAPSHOT}"
require_file "${MANIFEST}"
require_file "${TEST}"

require_contains "${DOC}" 'runtime_abi_contract_status: frozen_v1_symbol_manifest'
require_contains "${DOC}" 'runtime_abi_version: 1.0.0'
require_contains "${DOC}" 'runtime_api_version: 1.0.0'
require_contains "${DOC}" 'runtime_external_symbol_count: 25'
require_contains "${DOC}" 'production_claim_boundary: abi_stability_gate_is_not_full_production_readiness'
require_contains "${DOC}" 'Removing, renaming or changing the parameter or return type'
require_contains "${DOC}" 'No ABI v1 symbol is deprecated in this release.'
require_contains "${DOC}" 'retain the deprecated external symbol throughout the current ABI major'
require_contains "${DOC}" 'ABI v1 does not claim that global runtime state is thread-safe'

require_contains "${HEADER}" '#define SHORTHAND_RUNTIME_ABI_VERSION_MAJOR 1'
require_contains "${HEADER}" '#define SHORTHAND_RUNTIME_ABI_VERSION_MINOR 0'
require_contains "${HEADER}" '#define SHORTHAND_RUNTIME_ABI_VERSION_PATCH 0'
require_contains "${HEADER}" '#define SHORTHAND_RUNTIME_ABI_VERSION_STRING "1.0.0"'
require_contains "${HEADER}" '#define SHORTHAND_RUNTIME_API_VERSION_STRING "1.0.0"'
require_contains "${HEADER}" 'short_runtime_abi_version(void)'
require_contains "${HEADER}" 'short_runtime_api_version(void)'
require_contains "${HEADER}" 'short_runtime_is_abi_compatible'
require_contains "${HEADER}" 'short_runtime_is_api_compatible'
require_contains "${HEADER}" 'SHORTHAND_RUNTIME_DEPRECATED(message)'
require_contains "${HEADER}" 'SHORTHAND_RUNTIME_API int short_runtime_reset(void);'

require_contains "${SNAPSHOT}" 'Frozen consumer snapshot for ShortHand runtime ABI 1.0.0.'
require_contains "${SNAPSHOT}" 'SHORTHAND_RUNTIME_RUNTIME_ERROR = 8'
require_contains "${SNAPSHOT}" 'int short_ai_infer_f32'

manifest_count="$(grep -c '^short_' "${MANIFEST}")"
if [[ "${manifest_count}" != "25" ]]; then
  echo "error: ABI v1 symbol manifest must contain exactly 25 symbols; observed ${manifest_count}" >&2
  exit 1
fi

if [[ "$(LC_ALL=C sort -u "${MANIFEST}" | wc -l | tr -d ' ')" != "25" ]]; then
  echo "error: ABI v1 symbol manifest contains duplicates" >&2
  exit 1
fi

for symbol in \
  short_runtime_reset \
  short_runtime_observability_json \
  short_runtime_otlp_spans_json \
  short_ai_register_model \
  short_ai_infer \
  short_ai_infer_f32 \
  short_ai_infer_legacy \
  short_greenai_register_contract; do
  require_contains "${MANIFEST}" "${symbol}"
done

require_contains "${TEST}" 'ABI_SYMBOL_COUNT'
require_contains "${TEST}" 'PASS frozen runtime ABI v1 consumer'
require_contains "${TEST}" 'PASS runtime ABI and API stability test'
require_contains "${TEST}" 'diff -u'
require_contains "${TEST}" 'SHORTHAND_RUNTIME_RUNTIME_ERROR != 8'
require_contains "${TEST}" 'short_runtime_is_abi_compatible(1, 0)'
require_contains "${TEST}" 'short_runtime_is_abi_compatible(2, 0)'

bash -n "${TEST}"
bash "${TEST}"

printf 'PASS runtime ABI and API stability gate\n'
