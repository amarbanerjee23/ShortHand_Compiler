#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
TEMPLATE="${ROOT_DIR}/.github/pull_request_template.md"
CI="${ROOT_DIR}/.github/workflows/ci.yml"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"
MODULE_MATRIX="${ROOT_DIR}/tests/conformance/module_matrix_beta_0_3.tsv"
MODULE_GATE="${ROOT_DIR}/scripts/check_module_ast_scaffold.sh"
RESOLVER_GATE="${ROOT_DIR}/scripts/check_module_resolution.sh"
DIFFERENTIAL_GATE="${ROOT_DIR}/scripts/check_semantic_differential.sh"
FUZZ_GATE="${ROOT_DIR}/scripts/check_fuzz_safety.sh"
TSAN_GATE="${ROOT_DIR}/scripts/check_tsan_concurrency.sh"
FUZZ_DOC="${ROOT_DIR}/docs/fuzz_sanitizer_concurrency.md"
EXECUTION_CONTRACT="${ROOT_DIR}/docs/execution_semantics_beta_0_3.md"

require_file() {
  local file="$1"
  [[ -s "${file}" ]] || { echo "error: missing or empty file: ${file}" >&2; exit 1; }
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  grep -Fq "${needle}" "${file}" || {
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  }
}

for file in "${DOC}" "${MATRIX}" "${PLAN}" "${STATUS}" "${TEMPLATE}" "${CI}" "${PIPELINE}" \
  "${MODULE_MATRIX}" "${MODULE_GATE}" "${RESOLVER_GATE}" "${DIFFERENTIAL_GATE}" \
  "${FUZZ_GATE}" "${TSAN_GATE}" "${FUZZ_DOC}" "${EXECUTION_CONTRACT}" \
  "${ROOT_DIR}/tests/fuzz/FuzzSubprocess.cpp" \
  "${ROOT_DIR}/tests/runtime/runtime_tsan_stress.cpp" \
  "${ROOT_DIR}/.github/workflows/fuzz-nightly.yml"; do
  require_file "${file}"
done

header="$(head -n 1 "${MATRIX}")"
expected_header=$'id\tarea\tstatus\texisting_evidence\tmissing_evidence\tclosure_pr\tproduction_blocker'
[[ "${header}" == "${expected_header}" ]] || {
  echo "error: compiler test matrix header changed unexpectedly" >&2
  exit 1
}

row_count="$(tail -n +2 "${MATRIX}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
[[ "${row_count}" == "27" ]] || {
  echo "error: expected 27 compiler test coverage rows, found ${row_count}" >&2
  exit 1
}

implemented_count="$(awk -F '\t' 'NR > 1 && $3 == "implemented" { count++ } END { print count+0 }' "${MATRIX}")"
partial_count="$(awk -F '\t' 'NR > 1 && $3 == "partial" { count++ } END { print count+0 }' "${MATRIX}")"
open_count="$(awk -F '\t' 'NR > 1 && $3 == "open" { count++ } END { print count+0 }' "${MATRIX}")"
[[ "${implemented_count}" == "12" ]] || { echo "error: expected 12 implemented rows in the PR73 candidate" >&2; exit 1; }
[[ "${partial_count}" == "6" ]] || { echo "error: expected 6 partial rows in the PR73 candidate" >&2; exit 1; }
[[ "${open_count}" == "9" ]] || { echo "error: expected 9 open rows in the PR73 candidate" >&2; exit 1; }

invalid_status="$(awk -F '\t' 'NR > 1 && $3 != "implemented" && $3 != "partial" && $3 != "open" { print $1 ":" $3 }' "${MATRIX}")"
[[ -z "${invalid_status}" ]] || {
  echo "error: invalid compiler test matrix status values: ${invalid_status}" >&2
  exit 1
}

duplicate_ids="$(tail -n +2 "${MATRIX}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_ids}" ]] || {
  echo "error: duplicate compiler test matrix IDs: ${duplicate_ids}" >&2
  exit 1
}

for number in $(seq 1 27); do
  id="$(printf 'TST%03d' "${number}")"
  require_contains "${MATRIX}" "${id}"
done

for pr in $(seq 68 86); do
  require_contains "${PLAN}" "PR${pr} -"
done

for anchor in \
  'compiler_test_strategy_version: 2026-08-11-pr73' \
  'production_claim: false' \
  'Required test layers for every implementation PR' \
  'A test passing because a dependency, device, backend or platform was skipped is not production success evidence.' \
  'measured energy and performance comparison with equivalent Python workloads' \
  'PR70 and all PR72 through PR86 implementation completion gates are merged' \
  'CPU/GPU/TPU/NPU' \
  'Tsan' ; do
  if [[ "${anchor}" == 'Tsan' ]]; then continue; fi
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'Tests added or updated' \
  'Sanitizer coverage' \
  'Security or misuse tests' \
  'Performance or energy regression evidence' \
  'No mandatory production test is converted to an unconditional skip'; do
  require_contains "${TEMPLATE}" "${anchor}"
done

require_contains "${PLAN}" 'PR72 - Cross-mode semantic correctness and differential execution suite | MERGED'
require_contains "${PLAN}" 'PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | IN PROGRESS'
require_contains "${PLAN}" 'PR86 - Measured energy, performance and zero-skip production RC gate'
require_contains "${STATUS}" 'Cross-mode semantic equivalence'
require_contains "${STATUS}" 'Coverage-guided fuzzing'
require_contains "${STATUS}" 'ThreadSanitizer'
require_contains "${CI}" 'Coverage-guided sanitizer fuzz smoke'
require_contains "${CI}" 'ThreadSanitizer concurrency race gate'
require_contains "${PIPELINE}" 'Tier 5 - runtime/backend/hardware qualification'
require_contains "${PIPELINE}" 'CPU, GPU, TPU and NPU'
require_contains "${MODULE_GATE}" 'PASS module import package syntax and AST scaffold gate'
require_contains "${RESOLVER_GATE}" 'PASS deterministic module resolver, package lock and multi-file codegen gate'
require_contains "${DIFFERENTIAL_GATE}" 'PASS cross-mode semantic differential execution gate'
require_contains "${FUZZ_GATE}" 'PASS coverage-guided sanitizer fuzz gate'
require_contains "${TSAN_GATE}" 'PASS ThreadSanitizer concurrency gate'
require_contains "${FUZZ_DOC}" 'fuzz_safety_contract_version: 1.0.0'
require_contains "${EXECUTION_CONTRACT}" 'execution_semantics_contract: beta-0.3-pr72-v1'
require_contains "${MATRIX}" $'TST008\tfull sanitizer coverage\timplemented'
require_contains "${MATRIX}" $'TST009\tcontinuous fuzzing\timplemented'
require_contains "${MATRIX}" $'TST010\tconcurrency and race detection\timplemented'
require_contains "${MATRIX}" $'TST027\tproduction release-candidate gate\topen'

printf 'TEST_COVERAGE implemented=%s partial=%s open=%s total=%s\n' \
  "${implemented_count}" "${partial_count}" "${open_count}" "${row_count}"
printf 'PASS compiler test strategy and coverage audit gate\n'
