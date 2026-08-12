#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"

require_file() { [[ -s "$1" ]] || { echo "error: missing required file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${PLAN}" "${PIPELINE}" \
  "${ROOT_DIR}/docs/language_objectives.md" \
  "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" \
  "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" \
  "${ROOT_DIR}/docs/fuzz_sanitizer_race_hardening.md" \
  "${ROOT_DIR}/docs/toolchain_platform_reproducibility.md" \
  "${ROOT_DIR}/docs/signed_release_publication.md" \
  "${ROOT_DIR}/docs/compiler_test_strategy.md" \
  "${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv" \
  "${ROOT_DIR}/scripts/check_signed_release_contract.sh"; do
  require_file "${file}"
done

for anchor in \
  'production_readiness_plan_version: 2026-08-12-pr75' \
  'PLAN_STATUS: active' \
  'LAST_COMPLETED_PR: 74' \
  'MERGED_OUT_OF_BAND_PR: 71' \
  'CURRENT_IMPLEMENTATION_PR: 75' \
  'GITHUB_IMPLEMENTATION_PR: 76' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR75: 76' \
  'BASELINE_LANGUAGE_VERSION: beta-0.3' \
  'TARGET: enterprise production usage ready language' \
  'Roadmap PR75 - Signed release and protected publication workflow is IN PROGRESS as GitHub PR76.' \
  'remaining_planned_implementation_prs_pr75_through_pr86: 12' \
  'remaining_planned_implementation_prs_after_pr75: 11' \
  'Mandatory rule for every remaining PR' \
  'Robust pipeline architecture'; do
  require_contains "${PLAN}" "${anchor}"
done

for pr in $(seq 68 86); do
  require_contains "${PLAN}" "PR${pr} -"
  require_contains "${PLAN}" "| PR${pr} -"
done
require_contains "${PLAN}" '| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | MERGED'
require_contains "${PLAN}" '| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | MERGED as GitHub PR75'
require_contains "${PLAN}" '| PR75 - Signed release and protected publication workflow | IN PROGRESS as GitHub PR76'
require_contains "${PLAN}" '| PR76 - External vulnerability, SAST, dependency and license policy gate | PLANNED'

for anchor in \
  'ci_pipeline_architecture_version: 2026-08-09-v1' \
  'Tier 0 - CI policy and repository invariants' \
  'Tier 3 - memory, undefined behavior and concurrency safety' \
  'Tier 5 - runtime/backend/hardware qualification' \
  'CPU, GPU, TPU and NPU' \
  'Release-candidate profile' \
  'PR74: multi-job DAG, GCC/Clang/platform matrix and reproducibility.' \
  'PR86: performance, energy and zero-skip production RC aggregation.'; do
  require_contains "${PIPELINE}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" 'resolution_status: deterministic_manifest_locked_multi_file_codegen'
require_contains "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" 'execution_semantics_contract: beta-0.3-pr72-v1'
require_contains "${ROOT_DIR}/docs/fuzz_sanitizer_race_hardening.md" 'fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1'
require_contains "${ROOT_DIR}/docs/toolchain_platform_reproducibility.md" 'toolchain_platform_contract_version: shorthand.portability.reproducibility.v1'
require_contains "${ROOT_DIR}/docs/signed_release_publication.md" 'signed_release_contract_version: shorthand.release.protected.v1'
require_contains "${ROOT_DIR}/scripts/check_signed_release_contract.sh" 'PASS signed release and protected publication contract gate'
require_contains "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh" 'PASS compiler test strategy and coverage audit gate'

# Historical milestones remain auditable without being mistaken for active state.
for anchor in \
  'production_readiness_plan_version: 2026-08-11-pr72' \
  'CURRENT_IMPLEMENTATION_PR: 72' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73' \
  'remaining_planned_implementation_prs_pr73_through_pr86: 14' \
  '| PR72 - Cross-mode semantic correctness and differential execution suite | IN PROGRESS' \
  'production_readiness_plan_version: 2026-08-02-pr62' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'PR79 - MLIR lowering passes and production RC gate'; do
  require_contains "${PLAN}" "${anchor}"
done

printf 'PASS production readiness PR plan gate\n'
