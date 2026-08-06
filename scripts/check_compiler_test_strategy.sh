#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
TEMPLATE="${ROOT_DIR}/.github/pull_request_template.md"
CI="${ROOT_DIR}/.github/workflows/ci.yml"

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

for file in "${DOC}" "${MATRIX}" "${PLAN}" "${STATUS}" "${TEMPLATE}" "${CI}"; do
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
[[ "${implemented_count}" == "3" ]] || { echo "error: expected 3 implemented rows" >&2; exit 1; }
[[ "${partial_count}" == "11" ]] || { echo "error: expected 11 partial rows" >&2; exit 1; }
[[ "${open_count}" == "13" ]] || { echo "error: expected 13 open rows" >&2; exit 1; }

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

for id in $(seq -w 1 27); do
  require_contains "${MATRIX}" "TST${id}"
done

for pr in $(seq 68 85); do
  require_contains "${PLAN}" "PR${pr} -"
done

for anchor in \
  'compiler_test_strategy_version: 2026-08-06-pr68' \
  'production_claim: false' \
  'Required test layers for every implementation PR' \
  'A test passing because a dependency, device, backend, or platform was skipped is not production success evidence.' \
  'measured energy and performance comparison with equivalent Python workloads' \
  'all PR69 through PR85 completion gates are merged'; do
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

require_contains "${PLAN}" 'After PR #67, 18 production-readiness PRs are required.'
require_contains "${PLAN}" 'After PR #68 is merged, 17 implementation PRs remain.'
require_contains "${PLAN}" 'PR85 - Measured energy, performance and production RC gate'
require_contains "${STATUS}" 'Compiler test strategy and coverage matrix'
require_contains "${STATUS}" 'Measured ShortHand versus Python energy evidence'
require_contains "${CI}" 'Compiler test strategy and coverage audit'

printf 'TEST_COVERAGE implemented=%s partial=%s open=%s total=%s\n' \
  "${implemented_count}" "${partial_count}" "${open_count}" "${row_count}"
printf 'PASS compiler test strategy and coverage audit gate\n'
