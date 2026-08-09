#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${ROOT_DIR}/tests/conformance/manifest.txt"
BASE_MATRIX="${ROOT_DIR}/tests/conformance/grammar_matrix_beta_0_2.tsv"
MODULE_MATRIX="${ROOT_DIR}/tests/conformance/module_matrix_beta_0_3.tsv"
VERSION_DOC="${ROOT_DIR}/docs/language_versioning_and_conformance.md"
GRAMMAR_DOC="${ROOT_DIR}/docs/language_grammar_ebnf.md"
SPEC_DOC="${ROOT_DIR}/docs/language_spec.md"
MODULE_DOC="${ROOT_DIR}/docs/module_import_package_syntax.md"
RESOLVER_DOC="${ROOT_DIR}/docs/module_resolution_and_lockfile.md"
TRACKER="${ROOT_DIR}/docs/feature_implementation_status.md"
BASE_GATE="${ROOT_DIR}/scripts/check_grammar_conformance_matrix.sh"
MODULE_GATE="${ROOT_DIR}/scripts/check_module_ast_scaffold.sh"
RESOLVER_GATE="${ROOT_DIR}/scripts/check_module_resolution.sh"

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
  awk -F '|' '
    /^[[:space:]]*#/ { next }
    /^[[:space:]]*$/ { next }
    NF != 4 { print "bad manifest row: " $0; bad = 1 }
    END { exit bad ? 1 : 0 }
  ' "${MANIFEST}"
}

for file in \
  "${MANIFEST}" "${BASE_MATRIX}" "${MODULE_MATRIX}" "${VERSION_DOC}" \
  "${GRAMMAR_DOC}" "${SPEC_DOC}" "${MODULE_DOC}" "${RESOLVER_DOC}" "${TRACKER}" \
  "${BASE_GATE}" "${MODULE_GATE}" "${RESOLVER_GATE}"; do
  require_file "${file}"
done

require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.3'
require_contains "${VERSION_DOC}" 'shorthand.conformance.contract: beta-0.3'
require_contains "${VERSION_DOC}" 'shorthand.grammar.matrix: beta-0.2-base+beta-0.3-modules'
require_contains "${VERSION_DOC}" 'shorthand.module.resolution.contract: shorthand.package.v1+shorthand.lock.v1'
require_contains "${VERSION_DOC}" 'production_claim: false'
require_contains "${MODULE_DOC}" 'module_syntax_contract_version: beta-0.3'
require_contains "${MODULE_DOC}" 'module_resolution_status: deterministic_resolver_available_via_pr70'
require_contains "${RESOLVER_DOC}" 'package_manifest_schema: shorthand.package.v1'
require_contains "${RESOLVER_DOC}" 'package_lock_schema: shorthand.lock.v1'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.2'
require_contains "${GRAMMAR_DOC}" 'grammar_conformance_status: parser_accurate_matrix_guarded'
require_contains "${SPEC_DOC}" 'Language version: beta-0.2'
require_contains "${SPEC_DOC}" 'language_contract_status: parser_accurate_executable_matrix'
require_contains "${MANIFEST}" 'current-version | shorthand.language.version | beta-0.3 | Current executable language contract marker.'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.2 | Previous executable base language contract marker.'
require_contains "${MANIFEST}" 'grammar-matrix | tests/conformance/grammar_matrix_beta_0_2.tsv | accept'
require_contains "${MANIFEST}" 'grammar-extension | tests/conformance/module_matrix_beta_0_3.tsv | accept'
require_contains "${MANIFEST}" 'module-resolution | scripts/check_module_resolution.sh | accept'
require_contains "${TRACKER}" 'language versioning and conformance policy gate'
require_contains "${BASE_GATE}" 'PASS beta-0.2 grammar and conformance matrix gate'
require_contains "${MODULE_GATE}" 'PASS module import package syntax and AST scaffold gate'
require_contains "${RESOLVER_GATE}" 'PASS deterministic module resolver, package lock and multi-file codegen gate'

# Historical markers stay visible for compatibility and old-task gates.
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.conformance.contract: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.1'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.1'
require_contains "${SPEC_DOC}" 'Language version: beta-0.1'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'

require_manifest_rows_have_four_fields
for category in current-version version grammar-matrix grammar-extension module-resolution parser-valid parser-invalid semantic-invalid diagnostics codegen runtime evidence; do
  require_manifest_category "${category}"
done

bash -n "${BASE_GATE}"
bash -n "${MODULE_GATE}"
bash -n "${RESOLVER_GATE}"
bash "${BASE_GATE}"
bash "${MODULE_GATE}"
bash "${RESOLVER_GATE}"

printf 'PASS language versioning and conformance gate beta-0.3\n'
