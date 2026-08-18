#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
TEMPLATE="${ROOT_DIR}/.github/pull_request_template.md"
CI="${ROOT_DIR}/.github/workflows/ci.yml"
TOOLING_CI="${ROOT_DIR}/.github/workflows/tooling.yml"
RELEASE_CI="${ROOT_DIR}/.github/workflows/release.yml"
SECURITY_CI="${ROOT_DIR}/.github/workflows/security.yml"
DEPLOY_DOC="${ROOT_DIR}/docs/container_kubernetes_hardening.md"
TOOLING_DOC="${ROOT_DIR}/docs/formatter_linter.md"

require_file() { [[ -s "$1" ]] || { echo "error: missing or empty file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${DOC}" "${MATRIX}" "${PLAN}" "${STATUS}" "${TEMPLATE}" "${CI}" "${TOOLING_CI}" "${RELEASE_CI}" "${SECURITY_CI}" "${DEPLOY_DOC}" "${TOOLING_DOC}" \
  "${ROOT_DIR}/scripts/check_semantic_differential.sh" \
  "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" \
  "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_signed_release_contract.sh" \
  "${ROOT_DIR}/scripts/check_external_security_policy.sh" \
  "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" \
  "${ROOT_DIR}/scripts/check_container_runtime.sh" \
  "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" \
  "${ROOT_DIR}/scripts/check_formatter_linter.sh" \
  "${ROOT_DIR}/tests/deployment/test_container_kubernetes_hardening_negative.sh" \
  "${ROOT_DIR}/tests/tooling/formatter_messy.short" \
  "${ROOT_DIR}/tests/tooling/formatter_expected.short" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceTools.h" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/Makefile" \
  "${ROOT_DIR}/docs/signed_release_publication.md" \
  "${ROOT_DIR}/docs/external_security_policy.md"; do
  require_file "${file}"
done

expected_header=$'id\tarea\tstatus\texisting_evidence\tmissing_evidence\tclosure_pr\tproduction_blocker'
[[ "$(head -n 1 "${MATRIX}")" == "${expected_header}" ]] || { echo "error: compiler test matrix header changed unexpectedly" >&2; exit 1; }
row_count="$(tail -n +2 "${MATRIX}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
implemented_count="$(awk -F '\t' 'NR > 1 && $3 == "implemented" { count++ } END { print count+0 }' "${MATRIX}")"
partial_count="$(awk -F '\t' 'NR > 1 && $3 == "partial" { count++ } END { print count+0 }' "${MATRIX}")"
open_count="$(awk -F '\t' 'NR > 1 && $3 == "open" { count++ } END { print count+0 }' "${MATRIX}")"
[[ "${row_count}" == 27 ]] || { echo "error: expected 27 compiler test coverage rows, found ${row_count}" >&2; exit 1; }
[[ "${implemented_count}" == 19 ]] || { echo "error: expected 19 implemented rows in the PR79 candidate" >&2; exit 1; }
[[ "${partial_count}" == 4 ]] || { echo "error: expected 4 partial rows in the PR79 candidate" >&2; exit 1; }
[[ "${open_count}" == 4 ]] || { echo "error: expected 4 open rows in the PR79 candidate" >&2; exit 1; }

invalid_status="$(awk -F '\t' 'NR > 1 && $3 != "implemented" && $3 != "partial" && $3 != "open" { print $1 ":" $3 }' "${MATRIX}")"
[[ -z "${invalid_status}" ]] || { echo "error: invalid compiler test matrix status values: ${invalid_status}" >&2; exit 1; }
duplicate_ids="$(tail -n +2 "${MATRIX}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_ids}" ]] || { echo "error: duplicate compiler test matrix IDs: ${duplicate_ids}" >&2; exit 1; }
for number in $(seq 1 27); do require_contains "${MATRIX}" "$(printf 'TST%03d' "${number}")"; done
for pr in $(seq 68 86); do require_contains "${PLAN}" "PR${pr} -"; done

for anchor in \
  'compiler_test_strategy_version: 2026-08-18-pr79' \
  'production_claim: false' \
  '19 implemented areas' \
  '4 partial areas' \
  '4 open areas' \
  'Required test layers for every implementation PR' \
  'A test passing because a dependency, device, backend, platform, container runtime or cluster was skipped is not production success evidence.' \
  'Signing source code is not signing evidence' \
  'External security scanners are mandatory evidence' \
  'A deployment manifest is not deployment evidence.' \
  'Formatter success is not inferred from source-text checks.' \
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

require_contains "${STATUS}" 'feature_status_version: 2026-08-18-pr79'
require_contains "${STATUS}" '19 implemented, 4 partial and 4 open'
require_contains "${STATUS}" 'Signed releases | Partial'
require_contains "${STATUS}" 'External vulnerability gate | Implemented'
require_contains "${STATUS}" 'Container and Kubernetes hardening | Implemented'
require_contains "${STATUS}" 'Formatter and linter | Implemented'
require_contains "${STATUS}" 'Cross-platform reproducibility | Implemented'
require_contains "${MATRIX}" $'TST013\tplatform and compiler portability\timplemented'
require_contains "${MATRIX}" $'TST014\treproducible builds\timplemented'
require_contains "${MATRIX}" $'TST015\truntime ABI compatibility\timplemented'
require_contains "${MATRIX}" $'TST016\tpackaging and installed consumers\timplemented'
require_contains "${MATRIX}" $'TST017\tsigned protected release\tpartial'
require_contains "${MATRIX}" $'TST018\tdependency security and license policy\timplemented'
require_contains "${MATRIX}" $'TST019\tcontainer and Kubernetes deployment\timplemented'
require_contains "${MATRIX}" $'TST020\tformatter and linter correctness\timplemented'
require_contains "${MATRIX}" $'TST021\tsyntax highlighting and LSP protocol\topen'
require_contains "${MATRIX}" $'TST027\tproduction release-candidate gate\topen'

require_contains "${DEPLOY_DOC}" 'container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1'
require_contains "${DEPLOY_DOC}" 'tst019_status: implemented_after_exact_head_runtime_qualification'
require_contains "${DEPLOY_DOC}" 'No Service or Ingress is declared'
require_contains "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" 'PASS container Kubernetes production hardening contract'
require_contains "${ROOT_DIR}/scripts/check_container_runtime.sh" 'PASS hardened container runtime'
require_contains "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" 'PASS ephemeral Kubernetes production gate'
require_contains "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" 'PASS native Linux arm64 production container qualification'
require_contains "${ROOT_DIR}/tests/deployment/test_container_kubernetes_hardening_negative.sh" 'PASS container Kubernetes hardening positive and negative matrix'

require_contains "${TOOLING_DOC}" 'formatter_linter_contract_version: shorthand.tooling.format_lint.v1'
require_contains "${TOOLING_DOC}" 'shorthand.lint.v1'
require_contains "${ROOT_DIR}/scripts/check_formatter_linter.sh" 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp" 'SHL006'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp" 'fix mode requires --output'
require_contains "${TOOLING_CI}" 'formatter-linter:'
require_contains "${TOOLING_CI}" 'GCC formatter and linter gate'
require_contains "${TOOLING_CI}" 'Clang formatter and linter gate'
require_contains "${TOOLING_CI}" 'ASan UBSan formatter and linter gate'

require_contains "${RELEASE_CI}" 'environment: production-release'
require_contains "${RELEASE_CI}" 'id-token: write'
require_contains "${RELEASE_CI}" 'gh attestation verify'
require_contains "${CI}" 'security:'
require_contains "${CI}" 'queries: security-extended'
require_contains "${CI}" 'needs.security.result'
require_contains "${ROOT_DIR}/scripts/check_external_security_policy.sh" 'PASS external vulnerability SAST dependency and license policy gate'
require_contains "${ROOT_DIR}/scripts/check_signed_release_contract.sh" 'PASS signed release and protected publication contract gate'
require_contains "${ROOT_DIR}/scripts/check_semantic_differential.sh" 'PASS cross-mode semantic differential execution gate'
require_contains "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" 'PASS libFuzzer ASan LSan UBSan compiler-stage gate'
require_contains "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" 'PASS runtime ASan LSan UBSan stress gate'
require_contains "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" 'PASS mandatory ThreadSanitizer race gate'

printf 'TEST_COVERAGE implemented=%s partial=%s open=%s total=%s\n' "${implemented_count}" "${partial_count}" "${open_count}" "${row_count}"
printf 'PASS compiler test strategy and coverage audit gate\n'
