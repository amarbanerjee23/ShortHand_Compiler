#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
TEMPLATE="${ROOT_DIR}/.github/pull_request_template.md"
CI="${ROOT_DIR}/.github/workflows/ci.yml"
RELEASE_CI="${ROOT_DIR}/.github/workflows/release.yml"

require_file() { [[ -s "$1" ]] || { echo "error: missing or empty file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${DOC}" "${MATRIX}" "${PLAN}" "${STATUS}" "${TEMPLATE}" "${CI}" "${RELEASE_CI}" \
  "${ROOT_DIR}/scripts/check_semantic_differential.sh" \
  "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" \
  "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_signed_release_contract.sh" \
  "${ROOT_DIR}/docs/signed_release_publication.md"; do
  require_file "${file}"
done

expected_header=$'id\tarea\tstatus\texisting_evidence\tmissing_evidence\tclosure_pr\tproduction_blocker'
[[ "$(head -n 1 "${MATRIX}")" == "${expected_header}" ]] || { echo "error: compiler test matrix header changed unexpectedly" >&2; exit 1; }
row_count="$(tail -n +2 "${MATRIX}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
implemented_count="$(awk -F '\t' 'NR > 1 && $3 == "implemented" { count++ } END { print count+0 }' "${MATRIX}")"
partial_count="$(awk -F '\t' 'NR > 1 && $3 == "partial" { count++ } END { print count+0 }' "${MATRIX}")"
open_count="$(awk -F '\t' 'NR > 1 && $3 == "open" { count++ } END { print count+0 }' "${MATRIX}")"
[[ "${row_count}" == 27 ]] || { echo "error: expected 27 compiler test coverage rows, found ${row_count}" >&2; exit 1; }
[[ "${implemented_count}" == 16 ]] || { echo "error: expected 16 implemented rows in the PR76 candidate" >&2; exit 1; }
[[ "${partial_count}" == 5 ]] || { echo "error: expected 5 partial rows in the PR76 candidate" >&2; exit 1; }
[[ "${open_count}" == 6 ]] || { echo "error: expected 6 open rows in the PR76 candidate" >&2; exit 1; }

invalid_status="$(awk -F '\t' 'NR > 1 && $3 != "implemented" && $3 != "partial" && $3 != "open" { print $1 ":" $3 }' "${MATRIX}")"
[[ -z "${invalid_status}" ]] || { echo "error: invalid compiler test matrix status values: ${invalid_status}" >&2; exit 1; }
duplicate_ids="$(tail -n +2 "${MATRIX}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_ids}" ]] || { echo "error: duplicate compiler test matrix IDs: ${duplicate_ids}" >&2; exit 1; }
for number in $(seq 1 27); do require_contains "${MATRIX}" "$(printf 'TST%03d' "${number}")"; done
for pr in $(seq 68 86); do require_contains "${PLAN}" "PR${pr} -"; done

for anchor in \
  'compiler_test_strategy_version: 2026-08-12-pr76' \
  'production_claim: false' \
  '16 implemented areas' \
  '5 partial areas' \
  '6 open areas' \
  'Required test layers for every implementation PR' \
  'A test passing because a dependency, device, backend or platform was skipped is not production success evidence.' \
  'Signing source code is not signing evidence' \
  'CPU/GPU/TPU/NPU'; do
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

require_contains "${STATUS}" 'feature_status_version: 2026-08-12-pr76'
require_contains "${STATUS}" '16 implemented, 5 partial and 6 open'
require_contains "${STATUS}" 'Signed releases | Partial'
require_contains "${STATUS}" 'Cross-platform reproducibility | Implemented'
require_contains "${MATRIX}" $'TST013\tplatform and compiler portability\timplemented'
require_contains "${MATRIX}" $'TST014\treproducible builds\timplemented'
require_contains "${MATRIX}" $'TST015\truntime ABI compatibility\timplemented'
require_contains "${MATRIX}" $'TST016\tpackaging and installed consumers\timplemented'
require_contains "${MATRIX}" $'TST017\tsigned protected release\tpartial'
require_contains "${MATRIX}" $'TST027\tproduction release-candidate gate\topen'
require_contains "${RELEASE_CI}" 'environment: production-release'
require_contains "${RELEASE_CI}" 'id-token: write'
require_contains "${RELEASE_CI}" 'gh attestation verify'
require_contains "${ROOT_DIR}/scripts/check_signed_release_contract.sh" 'PASS signed release and protected publication contract gate'
require_contains "${ROOT_DIR}/scripts/check_semantic_differential.sh" 'PASS cross-mode semantic differential execution gate'
require_contains "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" 'PASS libFuzzer ASan LSan UBSan compiler-stage gate'
require_contains "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" 'PASS runtime ASan LSan UBSan stress gate'
require_contains "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" 'PASS mandatory ThreadSanitizer race gate'

printf 'TEST_COVERAGE implemented=%s partial=%s open=%s total=%s\n' "${implemented_count}" "${partial_count}" "${open_count}" "${row_count}"
printf 'PASS compiler test strategy and coverage audit gate\n'
