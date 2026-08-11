#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"

require_file() {
  local file="$1"
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
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

required_files=(
  "${PLAN}"
  "${PIPELINE}"
  "${ROOT_DIR}/docs/ci_status_hygiene.md"
  "${ROOT_DIR}/scripts/check_ci_status_hygiene.sh"
  "${ROOT_DIR}/docs/language_objectives.md"
  "${ROOT_DIR}/docs/language_grammar_ebnf.md"
  "${ROOT_DIR}/docs/language_spec.md"
  "${ROOT_DIR}/docs/language_versioning_and_conformance.md"
  "${ROOT_DIR}/docs/module_import_package_syntax.md"
  "${ROOT_DIR}/docs/module_resolution_and_lockfile.md"
  "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md"
  "${ROOT_DIR}/docs/fuzz_sanitizer_concurrency.md"
  "${ROOT_DIR}/docs/backend_failure_mode_matrix.md"
  "${ROOT_DIR}/docs/runtime_abi_api_stability.md"
  "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md"
  "${ROOT_DIR}/docs/runtime_production_packaging.md"
  "${ROOT_DIR}/docs/compiler_test_strategy.md"
  "${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
  "${ROOT_DIR}/tests/fuzz/FuzzSubprocess.cpp"
  "${ROOT_DIR}/tests/runtime/runtime_tsan_stress.cpp"
  "${ROOT_DIR}/scripts/check_fuzz_safety.sh"
  "${ROOT_DIR}/scripts/check_tsan_concurrency.sh"
  "${ROOT_DIR}/scripts/check_semantic_differential.sh"
  "${ROOT_DIR}/.github/workflows/fuzz-nightly.yml"
  "${ROOT_DIR}/.github/workflows/ci.yml"
  "${ROOT_DIR}/abi/runtime_public_symbols_v1.txt"
)
for file in "${required_files[@]}"; do require_file "${file}"; done

for anchor in \
  'production_readiness_plan_version: 2026-08-11-pr73' \
  'PLAN_STATUS: active' \
  'LAST_COMPLETED_PR: 72' \
  'MERGED_OUT_OF_BAND_PR: 71' \
  'CURRENT_IMPLEMENTATION_PR: 73' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR73: 74' \
  'BASELINE_LANGUAGE_VERSION: beta-0.3' \
  'TARGET: enterprise production usage ready language' \
  'PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening is IN PROGRESS.' \
  'after PR73 is successfully merged, 13 implementation PRs remain.' \
  'Mandatory rule for every remaining PR' \
  'Robust pipeline architecture' \
  'remaining_planned_prs_after_pr73: 13' \
  'remaining_planned_implementation_prs_pr74_through_pr86: 13' \
  'Next recommended PR after PR #73:' \
  'PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds.'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${PLAN}" '| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED'
require_contains "${PLAN}" '| PR69 - Module, import and package syntax with AST scaffold | MERGED'
require_contains "${PLAN}" '| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED'
require_contains "${PLAN}" '| PR71 - CI status publication hygiene | MERGED'
require_contains "${PLAN}" '| PR72 - Cross-mode semantic correctness and differential execution suite | MERGED'
require_contains "${PLAN}" '| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | IN PROGRESS'
for pr in $(seq 74 86); do
  require_contains "${PLAN}" "PR${pr} -"
  require_contains "${PLAN}" "| PR${pr} -"
done

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

# Preserve guarded history while explicitly superseding earlier estimates.
for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr62' \
  'LAST_COMPLETED_PR: 62' \
  'production_readiness_plan_version: 2026-08-02-pr63' \
  'LAST_COMPLETED_PR: 63' \
  'production_readiness_plan_version: 2026-08-02-pr66' \
  'LAST_COMPLETED_PR: 66' \
  'production_readiness_plan_version: 2026-08-02-pr67' \
  'LAST_COMPLETED_PR: 67' \
  'production_readiness_plan_version: 2026-08-06-pr68' \
  'LAST_COMPLETED_PR: 68' \
  'production_readiness_plan_version: 2026-08-06-pr69' \
  'LAST_COMPLETED_PR: 69' \
  'production_readiness_plan_version: 2026-08-09-pr70' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'After PR #62 is merged, approximately 17 implementation PRs remain.' \
  'PR63 - OTLP exporter adapter.' \
  'After PR #65 is merged, approximately 14 implementation PRs remain.' \
  'PR66 - Full grammar and conformance matrix beta-0.2.' \
  'After PR #66 is merged, approximately 13 implementation PRs remain.' \
  'PR67 - Parser robustness and negative corpus hardening.' \
  'After PR #67 is merged, approximately 12 implementation PRs remain.' \
  'PR68 - Module/import/package design and parser scaffold.' \
  'PR79 - MLIR lowering passes and production RC gate' \
  'The historical PR67 recommendation is superseded by the test re-audit.' \
  'production_readiness_plan_version: 2026-08-11-pr72' \
  'CURRENT_IMPLEMENTATION_PR: 72' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73' \
  'PR72 - Cross-mode semantic correctness and differential execution suite is IN PROGRESS.' \
  'after PR72 is successfully merged, 14 implementation PRs remain.' \
  'remaining_planned_prs_after_pr72: 14' \
  'remaining_planned_implementation_prs_pr73_through_pr86: 14' \
  'Next recommended PR after PR #72:' \
  'PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening.'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/backend_failure_mode_matrix.md" 'backend_failure_mode_matrix_status: finalized_v1'
require_contains "${ROOT_DIR}/docs/runtime_abi_api_stability.md" 'runtime_external_symbol_count: 25'
require_contains "${ROOT_DIR}/docs/language_versioning_and_conformance.md" 'shorthand.language.version: beta-0.3'
require_contains "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" 'resolution_status: deterministic_manifest_locked_multi_file_codegen'
require_contains "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" 'execution_semantics_contract: beta-0.3-pr72-v1'
require_contains "${ROOT_DIR}/docs/fuzz_sanitizer_concurrency.md" 'fuzz_safety_contract_version: 1.0.0'
require_contains "${ROOT_DIR}/docs/compiler_test_strategy.md" 'compiler_test_strategy_version: 2026-08-11-pr73'
require_contains "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh" 'PASS compiler test strategy and coverage audit gate'
require_contains "${ROOT_DIR}/scripts/check_semantic_differential.sh" 'PASS cross-mode semantic differential execution gate'
require_contains "${ROOT_DIR}/scripts/check_fuzz_safety.sh" 'PASS coverage-guided sanitizer fuzz gate'
require_contains "${ROOT_DIR}/scripts/check_tsan_concurrency.sh" 'PASS ThreadSanitizer concurrency gate'
require_contains "${ROOT_DIR}/scripts/check_ci_status_hygiene.sh" 'PASS CI status hygiene guard'
require_contains "${ROOT_DIR}/.github/workflows/ci.yml" 'Coverage-guided sanitizer fuzz smoke'
require_contains "${ROOT_DIR}/.github/workflows/ci.yml" 'ThreadSanitizer concurrency race gate'
require_contains "${ROOT_DIR}/.github/workflows/fuzz-nightly.yml" 'Extended coverage-guided sanitizer fuzzing'
require_contains "${ROOT_DIR}/.github/workflows/fuzz-nightly.yml" 'Extended ThreadSanitizer concurrency stress'

printf 'PASS production readiness PR plan gate\n'
