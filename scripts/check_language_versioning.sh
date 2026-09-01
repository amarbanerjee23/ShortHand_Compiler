#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${ROOT_DIR}/tests/conformance/manifest.txt"
BASE_MATRIX="${ROOT_DIR}/tests/conformance/grammar_matrix_beta_0_2.tsv"
MODULE_MATRIX="${ROOT_DIR}/tests/conformance/module_matrix_beta_0_3.tsv"
TYPE_MATRIX="${ROOT_DIR}/tests/conformance/type_matrix_beta_0_4.tsv"
CONTROL_MATRIX="${ROOT_DIR}/tests/conformance/functions_control_matrix_beta_0_5.tsv"
ENTERPRISE_MATRIX="${ROOT_DIR}/tests/conformance/enterprise_matrix_beta_0_6.tsv"
PROFILE_MATRIX="${ROOT_DIR}/tests/conformance/c3eco_profile_matrix_beta_0_7.tsv"
VERSION_DOC="${ROOT_DIR}/docs/language_versioning_and_conformance.md"
GRAMMAR_DOC="${ROOT_DIR}/docs/language_grammar_ebnf.md"
SPEC_DOC="${ROOT_DIR}/docs/language_spec.md"
MODULE_DOC="${ROOT_DIR}/docs/module_import_package_syntax.md"
RESOLVER_DOC="${ROOT_DIR}/docs/module_resolution_and_lockfile.md"
TRACKER="${ROOT_DIR}/docs/feature_implementation_status.md"
BASE_GATE="${ROOT_DIR}/scripts/check_grammar_conformance_matrix.sh"
MODULE_GATE="${ROOT_DIR}/scripts/check_module_ast_scaffold.sh"
RESOLVER_GATE="${ROOT_DIR}/scripts/check_module_resolution.sh"
TYPE_GATE="${ROOT_DIR}/scripts/check_production_type_memory_model.sh"
CONTROL_DOC="${ROOT_DIR}/docs/functions_control_error_semantics.md"
CONTROL_GATE="${ROOT_DIR}/scripts/check_functions_control_error_semantics.sh"
ENTERPRISE_DOC="${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md"
ENTERPRISE_GATE="${ROOT_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh"
PROFILE_DOC="${ROOT_DIR}/docs/c3eco_certification_profile.md"
PROFILE_GATE="${ROOT_DIR}/scripts/check_c3eco_certification_profile.sh"

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
  "${MANIFEST}" "${BASE_MATRIX}" "${MODULE_MATRIX}" "${TYPE_MATRIX}" "${CONTROL_MATRIX}" "${ENTERPRISE_MATRIX}" "${PROFILE_MATRIX}" "${VERSION_DOC}" \
  "${GRAMMAR_DOC}" "${SPEC_DOC}" "${MODULE_DOC}" "${RESOLVER_DOC}" "${TRACKER}" \
  "${CONTROL_DOC}" "${ENTERPRISE_DOC}" "${PROFILE_DOC}" "${BASE_GATE}" "${MODULE_GATE}" "${RESOLVER_GATE}" "${TYPE_GATE}" "${CONTROL_GATE}" "${ENTERPRISE_GATE}" "${PROFILE_GATE}"; do
  require_file "${file}"
done

require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.7'
require_contains "${VERSION_DOC}" 'shorthand.conformance.contract: beta-0.7'
require_contains "${VERSION_DOC}" 'shorthand.grammar.matrix: beta-0.2-base+beta-0.3-modules+beta-0.4-types+beta-0.5-control+beta-0.6-enterprise+beta-0.7-c3eco-profile'
require_contains "${VERSION_DOC}" 'shorthand.module.resolution.contract: shorthand.package.v1+shorthand.lock.v1+shorthand.package.v2+shorthand.lock.v2'
require_contains "${VERSION_DOC}" 'production_claim: false'
require_contains "${MODULE_DOC}" 'module_syntax_contract_version: beta-0.3'
require_contains "${MODULE_DOC}" 'module_resolution_status: deterministic_resolver_available_via_pr70'
require_contains "${RESOLVER_DOC}" 'package_manifest_schema: shorthand.package.v1'
require_contains "${RESOLVER_DOC}" 'package_lock_schema: shorthand.lock.v1'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.2'
require_contains "${GRAMMAR_DOC}" 'grammar_conformance_status: parser_accurate_matrix_guarded'
require_contains "${GRAMMAR_DOC}" 'Beta-0.5 function, scope and control-flow extension'
require_contains "${GRAMMAR_DOC}" 'Beta-0.7 typed C3-ECO profile grammar'
require_contains "${SPEC_DOC}" 'Language version: beta-0.7'
require_contains "${SPEC_DOC}" 'Base grammar version: beta-0.2'
require_contains "${SPEC_DOC}" 'language_contract_status: parser_accurate_executable_matrix'
require_contains "${MANIFEST}" 'current-version | shorthand.language.version | beta-0.7 | Current layered language contract marker.'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.2 | Previous executable base language contract marker.'
require_contains "${MANIFEST}" 'grammar-matrix | tests/conformance/grammar_matrix_beta_0_2.tsv | accept'
require_contains "${MANIFEST}" 'grammar-extension | tests/conformance/module_matrix_beta_0_3.tsv | accept'
require_contains "${MANIFEST}" 'type-matrix | tests/conformance/type_matrix_beta_0_4.tsv | accept'
require_contains "${MANIFEST}" 'control-flow-matrix | tests/conformance/functions_control_matrix_beta_0_5.tsv | accept'
require_contains "${MANIFEST}" 'enterprise-matrix | tests/conformance/enterprise_matrix_beta_0_6.tsv | accept'
require_contains "${MANIFEST}" 'c3eco-profile-matrix | tests/conformance/c3eco_profile_matrix_beta_0_7.tsv | accept'
require_contains "${MANIFEST}" 'module-resolution | scripts/check_module_resolution.sh | accept'
require_contains "${TRACKER}" 'language versioning and conformance policy gate'
require_contains "${BASE_GATE}" 'PASS beta-0.2 grammar and conformance matrix gate'
require_contains "${MODULE_GATE}" 'PASS module import package syntax and AST scaffold gate'
require_contains "${RESOLVER_GATE}" 'PASS deterministic module resolver, package lock and multi-file codegen gate'
require_contains "${TYPE_GATE}" 'PASS production type and memory model gate'
require_contains "${CONTROL_GATE}" 'PASS beta-0.5 functions scopes control flow deterministic errors and cleanup gate'
require_contains "${CONTROL_DOC}" 'control_flow_contract: shorthand.control_flow.v1'
require_contains "${ENTERPRISE_DOC}" 'enterprise_contract: shorthand.enterprise_language.v1'
require_contains "${ENTERPRISE_GATE}" 'PASS enterprise packages standard library and safe FFI gate'
require_contains "${PROFILE_DOC}" 'c3eco_profile_contract: shorthand.c3eco.profile.v2'
require_contains "${PROFILE_GATE}" 'PASS typed C3-ECO profile identity units links boundary materiality lifecycle validity migration and claim-safety gate'

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tarea\tstage\tcontract_anchor\tfixture\texpectation\trationale") exit 10
    next
  }
  NF != 7 { exit 11 }
  $1 !~ /^C3P[0-9][0-9][0-9]$/ { exit 12 }
  $3 !~ /^(parser|ast|semantic|evidence)$/ { exit 13 }
  $6 !~ /^(accept|reject|accept_and_review)$/ { exit 14 }
  $2 == "" || $4 == "" || $5 == "" || $7 == "" { exit 15 }
  END { if (NR != 22) exit 16 }
' "${PROFILE_MATRIX}" || {
  echo "error: malformed or incomplete beta-0.7 typed-profile matrix" >&2
  exit 1
}
while IFS=$'\t' read -r id area stage anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  require_file "${ROOT_DIR}/${fixture}"
done <"${PROFILE_MATRIX}"

# Historical markers stay visible for compatibility and old-task gates.
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.conformance.contract: beta-0.2'
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.1'
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.3'
require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.1'
require_contains "${SPEC_DOC}" 'Historical base marker retained for compatibility gates: Language version: beta-0.2'
require_contains "${SPEC_DOC}" 'Language version: beta-0.1'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'

require_manifest_rows_have_four_fields
for category in current-version version grammar-matrix grammar-extension type-matrix control-flow-matrix enterprise-matrix c3eco-profile-matrix module-resolution parser-valid parser-invalid semantic-invalid diagnostics codegen runtime typed-runtime evidence; do
  require_manifest_category "${category}"
done

bash -n "${BASE_GATE}"
bash -n "${MODULE_GATE}"
bash -n "${RESOLVER_GATE}"
bash -n "${TYPE_GATE}"
bash -n "${CONTROL_GATE}"
bash -n "${ENTERPRISE_GATE}"
bash -n "${PROFILE_GATE}"
bash "${BASE_GATE}"
bash "${MODULE_GATE}"
bash "${RESOLVER_GATE}"
bash "${TYPE_GATE}"
bash "${CONTROL_GATE}"
bash "${ENTERPRISE_GATE}"
bash "${PROFILE_GATE}"

printf 'PASS language versioning and conformance gate beta-0.7\n'
