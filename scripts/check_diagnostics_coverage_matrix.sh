#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
HEADER="${SRC_DIR}/visitors/DiagnosticCodes.h"
DIAGNOSTICS_HEADER="${SRC_DIR}/visitors/Diagnostics.h"
DIAGNOSTICS_SOURCE="${SRC_DIR}/visitors/Diagnostics.cpp"
SEMANTIC="${SRC_DIR}/visitors/SemanticAnalyzer.cpp"
PARSER="${SRC_DIR}/scanner_parser/parser.yy"
MAIN="${SRC_DIR}/main.cpp"
RESOLVER="${SRC_DIR}/module/ModuleResolver.cpp"
DOC="${ROOT_DIR}/docs/diagnostics_coverage_matrix.md"
MATRIX="${ROOT_DIR}/tests/diagnostics/diagnostics_coverage_matrix.tsv"
TEST="${ROOT_DIR}/tests/diagnostics/test_diagnostics_coverage_matrix.sh"
DIFFERENTIAL="${ROOT_DIR}/scripts/check_semantic_differential.sh"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

require_contains() {
  local file="$1"
  local needle="$2"
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
  grep -Fq "${needle}" "${file}" || {
    echo "error: ${file} missing required diagnostics text: ${needle}" >&2
    exit 1
  }
}

for file in \
  "${HEADER}" "${DIAGNOSTICS_HEADER}" "${DIAGNOSTICS_SOURCE}" \
  "${SEMANTIC}" "${PARSER}" "${MAIN}" "${RESOLVER}" "${DOC}" "${MATRIX}" "${TEST}" "${DIFFERENTIAL}" \
  "${ROOT_DIR}/tests/diagnostics/fixtures/break_outside_loop.short" \
  "${ROOT_DIR}/tests/diagnostics/fixtures/ai_incompatible_backend_warning.short" \
  "${ROOT_DIR}/tests/diagnostics/fixtures/greenai_missing_functional_unit.short" \
  "${ROOT_DIR}/tests/diagnostics/fixtures/lowering_undefined_function.short" \
  "${ROOT_DIR}/tests/semantic/differential/unsupported_owned_string_array.short" \
  "${ROOT_DIR}/tests/semantic/differential/invalid_zero_array.short" \
  "${ROOT_DIR}/tests/semantic/differential/invalid_string_condition.short" \
  "${ROOT_DIR}/tests/semantic/differential/type_mismatch.short" \
  "${ROOT_DIR}/tests/semantic/differential/invalid_string_operator.short" \
  "${ROOT_DIR}/tests/types/test_production_type_memory_model.cpp" \
  "${ROOT_DIR}/scripts/check_production_type_memory_model.sh" \
  "${ROOT_DIR}/tests/semantic/differential/wrong_arity.short" \
  "${ROOT_DIR}/tests/semantic/differential/division_by_zero.short" \
  "${ROOT_DIR}/tests/semantic/differential/array_bounds.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_package.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_alias.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_path.short" \
  "${ROOT_DIR}/tests/modules/invalid/import_before_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/package_without_module.short" \
  "${ROOT_DIR}/tests/modules/resolver/valid_project/src/native_app.short" \
  "${ROOT_DIR}/scripts/check_module_resolution.sh"; do
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
done

bash -n "${TEST}"
bash -n "${ROOT_DIR}/scripts/check_module_resolution.sh"
bash -n "${DIFFERENTIAL}"

for anchor in \
  'diagnostics_coverage_contract_version: 1.3.0' \
  'diagnostics_coverage_status: stable_coded_stage_matrix_guarded' \
  'covered_stages: parser, module, semantic, ai, greenai, lowering, runtime' \
  'warning_delivery_status: printed_without_failing_successful_compilation' \
  'lowering_preflight_status: unresolved_calls_rejected_before_ir_generation' \
  'runtime_failure_status: stable_code_cross_mode_differential_guarded' \
  'runtime_abi_change: none' \
  'production_claim_boundary: matrix_is_not_parser_recovery_or_localization_completion'; do
  require_contains "${DOC}" "${anchor}"
done

require_contains "${DIAGNOSTICS_HEADER}" 'bool hasWarnings() const;'
require_contains "${DIAGNOSTICS_HEADER}" 'std::string code;'
require_contains "${DIAGNOSTICS_SOURCE}" 'printCode(record.code);'
require_contains "${PARSER}" 'ParserSyntaxError'
require_contains "${PARSER}" 'ParserExpectedAIInferBuiltin'
require_contains "${PARSER}" 'ParserDuplicatePackageDeclaration'
require_contains "${PARSER}" 'ParserModuleRequired'
require_contains "${SEMANTIC}" 'LoweringUndefinedFunction'
require_contains "${SEMANTIC}" 'SemanticUnsupportedExecutableType'
require_contains "${SEMANTIC}" 'SemanticFunctionArityMismatch'
require_contains "${SEMANTIC}" 'SemanticUndeclaredVariable'
require_contains "${SEMANTIC}" 'SemanticTypeMismatch'
require_contains "${SEMANTIC}" 'SemanticInvalidOperator'
require_contains "${SEMANTIC}" 'SemanticInvalidCondition'
require_contains "${RESOLVER}" 'ModuleLockfileMismatch'
require_contains "${MAIN}" 'resolver.verifyLockfile'
require_contains "${DIFFERENTIAL}" 'PASS cross-mode semantic differential execution gate'
require_contains "${TEST}" 'PASS diagnostics coverage matrix gate'

printf '%s\n' 'parser' 'module' 'semantic' 'ai' 'greenai' 'lowering' 'runtime' | sort >"${WORK_DIR}/required-stages.txt"
printf '%s\n' 'error' 'warning' | sort >"${WORK_DIR}/required-severities.txt"

grep -Eo 'SHD[0-9]{4}' "${HEADER}" | sort >"${WORK_DIR}/header-codes-all.txt"
sort -u "${WORK_DIR}/header-codes-all.txt" >"${WORK_DIR}/header-codes.txt"
awk -F '\t' 'NR > 1 { print $1 }' "${MATRIX}" | sort >"${WORK_DIR}/matrix-codes-all.txt"
sort -u "${WORK_DIR}/matrix-codes-all.txt" >"${WORK_DIR}/matrix-codes.txt"

if [[ "$(wc -l <"${WORK_DIR}/header-codes-all.txt")" -ne "$(wc -l <"${WORK_DIR}/header-codes.txt")" ]]; then
  echo "error: duplicate stable diagnostic code in ${HEADER}" >&2
  exit 1
fi
if [[ "$(wc -l <"${WORK_DIR}/matrix-codes-all.txt")" -ne "$(wc -l <"${WORK_DIR}/matrix-codes.txt")" ]]; then
  echo "error: duplicate diagnostic code row in ${MATRIX}" >&2
  exit 1
fi
if ! diff -u "${WORK_DIR}/header-codes.txt" "${WORK_DIR}/matrix-codes.txt"; then
  echo "error: DiagnosticCodes.h and diagnostics coverage matrix differ" >&2
  exit 1
fi

[[ "$(wc -l <"${WORK_DIR}/header-codes.txt")" -eq 78 ]] || {
  echo "error: expected 78 stable diagnostics after the PR84 type and memory expansion" >&2
  exit 1
}

awk -F '\t' '
  NR == 1 {
    if ($0 != "code\tstage\tseverity\trange\tfixture\tmode\texpectation") exit 10
    next
  }
  NF != 7 { exit 11 }
  $1 !~ /^SHD[0-9][0-9][0-9][0-9]$/ { exit 12 }
  $2 !~ /^(parser|module|semantic|ai|greenai|lowering|runtime)$/ { exit 13 }
  $3 !~ /^(error|warning)$/ { exit 14 }
  $4 != "required" { exit 15 }
  END { if (NR != 79) exit 16 }
' "${MATRIX}" || {
  echo "error: malformed diagnostics coverage matrix" >&2
  exit 1
}

awk -F '\t' 'NR > 1 { print $2 }' "${MATRIX}" | sort -u >"${WORK_DIR}/matrix-stages.txt"
awk -F '\t' 'NR > 1 { print $3 }' "${MATRIX}" | sort -u >"${WORK_DIR}/matrix-severities.txt"
diff -u "${WORK_DIR}/required-stages.txt" "${WORK_DIR}/matrix-stages.txt"
diff -u "${WORK_DIR}/required-severities.txt" "${WORK_DIR}/matrix-severities.txt"

[[ "$(awk -F '\t' 'NR > 1 && $7 == "representative" { count++ } END { print count + 0 }' "${MATRIX}")" -eq 6 ]] || {
  echo "error: expected six live representative diagnostic rows"
  exit 1
}

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_diagnostics_matrix_build.out 2>&1 || {
    cat /tmp/shorthand_diagnostics_matrix_build.out >&2 || true
    exit 1
  }
fi

SHORTHAND_BIN="${SHORT}" bash "${TEST}"
SHORTHAND_BIN="${SHORT}" bash "${ROOT_DIR}/scripts/check_module_ast_scaffold.sh"
SHORTHAND_BIN="${SHORT}" bash "${ROOT_DIR}/scripts/check_module_resolution.sh"

echo "PASS diagnostics coverage matrix guard"
