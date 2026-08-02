#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${ROOT_DIR}/tests/conformance/manifest.txt"
MATRIX="${ROOT_DIR}/tests/conformance/grammar_matrix_beta_0_2.tsv"
VERSION_DOC="${ROOT_DIR}/docs/language_versioning_and_conformance.md"
GRAMMAR_DOC="${ROOT_DIR}/docs/language_grammar_ebnf.md"
SPEC_DOC="${ROOT_DIR}/docs/language_spec.md"
TRACKER="${ROOT_DIR}/docs/feature_implementation_status.md"
GRAMMAR_GATE="${ROOT_DIR}/scripts/check_grammar_conformance_matrix.sh"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file#${ROOT_DIR}/}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file#${ROOT_DIR}/} missing required text: ${needle}" >&2
    exit 1
  fi
}

require_manifest_category() {
  local category="$1"
  if ! awk -F '|' -v category="${category}" '
    /^[[:space:]]*#/ { next }
    NF >= 4 {
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", $1)
      if ($1 == category) found = 1
    }
    END { exit found ? 0 : 1 }
  ' "${MANIFEST}"; then
    echo "error: conformance manifest missing category: ${category}" >&2
    exit 1
  fi
}

require_manifest_rows_have_four_fields() {
  if ! awk -F '|' '
    /^[[:space:]]*#/ { next }
    /^[[:space:]]*$/ { next }
    NF != 4 { print "bad manifest row: " $0; bad = 1 }
    END { exit bad ? 1 : 0 }
  ' "${MANIFEST}"; then
    exit 1
  fi
}

for file in "${MANIFEST}" "${MATRIX}" "${VERSION_DOC}" "${GRAMMAR_DOC}" "${SPEC_DOC}" "${TRACKER}" "${GRAMMAR_GATE}"; do
  require_file "${file}"
done

require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.conformance.contract: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.grammar.matrix: beta-0.2'
require_contains "${VERSION_DOC}" 'production_claim: false'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.2'
require_contains "${GRAMMAR_DOC}" 'Conformance contract: beta-0.2'
require_contains "${GRAMMAR_DOC}" 'grammar_conformance_status: parser_accurate_matrix_guarded'
require_contains "${SPEC_DOC}" 'Language version: beta-0.2'
require_contains "${SPEC_DOC}" 'Conformance contract: beta-0.2'
require_contains "${SPEC_DOC}" 'language_contract_status: parser_accurate_executable_matrix'
require_contains "${MANIFEST}" 'current-version | shorthand.language.version | beta-0.2 | Current executable language contract marker.'
require_contains "${MANIFEST}" 'grammar-matrix | tests/conformance/grammar_matrix_beta_0_2.tsv | accept'
require_contains "${TRACKER}" 'language versioning and conformance policy gate'
require_contains "${GRAMMAR_GATE}" 'PASS beta-0.2 grammar and conformance matrix gate'

# Earlier version evidence remains visible for compatibility and old-task gates.
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.1'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.1'
require_contains "${SPEC_DOC}" 'Language version: beta-0.1'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'

require_manifest_rows_have_four_fields
for category in current-version version grammar-matrix parser-valid parser-invalid semantic-invalid diagnostics codegen runtime evidence; do
  require_manifest_category "${category}"
done

bash -n "${GRAMMAR_GATE}"
bash "${GRAMMAR_GATE}"

printf 'PASS language versioning and conformance gate beta-0.2\n'
