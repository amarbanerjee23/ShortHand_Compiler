#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"
TRUTH="${ROOT_DIR}/docs/production_truth.tsv"
TRACE="${ROOT_DIR}/docs/c3eco_traceability.tsv"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
STRATEGY="${ROOT_DIR}/docs/compiler_test_strategy.md"
ASSESSMENT_DOC="${ROOT_DIR}/docs/c3eco_assessment.md"
ASSESSMENT_GATE="${ROOT_DIR}/scripts/check_c3eco_assessment.sh"

require_file() { [[ -s "$1" ]] || { echo "error: missing required file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${PLAN}" "${PIPELINE}" "${TRUTH}" "${TRACE}" "${STATUS}" "${STRATEGY}" \
  "${ASSESSMENT_DOC}" "${ASSESSMENT_GATE}" \
  "${ROOT_DIR}/docs/language_objectives.md" \
  "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" \
  "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" \
  "${ROOT_DIR}/docs/execution_semantics_beta_0_4.md" \
  "${ROOT_DIR}/docs/production_type_memory_model.md" \
  "${ROOT_DIR}/docs/functions_control_error_semantics.md" \
  "${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md" \
  "${ROOT_DIR}/docs/concurrent_serving_runtime.md" \
  "${ROOT_DIR}/docs/c3eco_certification_profile.md" \
  "${ROOT_DIR}/docs/c3eco_measurement_workbook.md" \
  "${ROOT_DIR}/scripts/check_production_truth.sh" \
  "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh" \
  "${ROOT_DIR}/tests/governance/test_production_truth_negative.sh"; do
  require_file "${file}"
done

for anchor in \
  'production_readiness_plan_version: 2026-09-08-pr90' \
  'PLAN_STATUS: active' \
  'LAST_MERGED_GITHUB_PR: 89' \
  'CURRENT_GITHUB_PR: 90' \
  'LAST_PLANNED_GITHUB_PR: 96' \
  'CURRENT_IMPLEMENTATION_SCOPE: c3eco_eligibility_scoring_claims_eco_regression' \
  'BASELINE_LANGUAGE_VERSION: beta-0.7' \
  'TARGET: enterprise production usage ready language' \
  'PR90 - Eligibility, scoring, claims and eco-regression is IN PROGRESS.' \
  'remaining_planned_implementation_prs_pr90_through_pr96: 7' \
  'remaining_planned_implementation_prs_after_pr90: 6' \
  'Mandatory rule for every remaining PR' \
  'Robust pipeline architecture' \
  'Assessment is not certification.' \
  'complete 76-criterion A-K scorecard' \
  'greater than 10 percent' \
  'official_certification_granted:false' \
  'deferred_pr95'; do
  require_contains "${PLAN}" "${anchor}"
done

for pr in $(seq 68 80); do require_contains "${PLAN}" "PR${pr} -"; done
require_contains "${PLAN}" '| Roadmap PR81 / GitHub PR82 - C3-ECO language blocks and zero-skip CI | MERGED'
for pr in $(seq 83 96); do require_contains "${PLAN}" "PR${pr} -"; done
for anchor in \
  '| PR83 - Production truth baseline and C3-ECO traceability | MERGED' \
  '| PR84 - Production type system and memory model | MERGED' \
  '| PR85 - Functions, structured control flow and error semantics | MERGED' \
  '| PR86 - Enterprise packages, standard library and FFI | MERGED' \
  '| PR87 - Concurrent serving and operational runtime | MERGED' \
  '| PR88 - Typed C3-ECO certification profile | MERGED' \
  '| PR89 - Measurement, carbon accounting and cost workbook | MERGED' \
  '| PR90 - Eligibility, scoring, claims and eco-regression | IN PROGRESS' \
  '| PR91 - Auditor bundle, retention, surveillance and reporting | PLANNED' \
  '| PR96 - Enterprise pilot and production RC | PLANNED'; do
  require_contains "${PLAN}" "${anchor}"
done

# Historical milestones must remain visible without masquerading as current state.
for anchor in \
  'production_readiness_plan_version: 2026-09-02-pr89' \
  'PR89 - Measurement, carbon accounting and cost workbook is IN PROGRESS.' \
  'remaining_planned_implementation_prs_pr89_through_pr96: 8' \
  'production_readiness_plan_version: 2026-09-01-pr88' \
  'PR88 - Typed C3-ECO certification profile is IN PROGRESS.' \
  'production_readiness_plan_version: 2026-08-18-pr79' \
  'production_readiness_plan_version: 2026-08-12-pr77' \
  'production_readiness_plan_version: 2026-08-02-pr62'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${TRUTH}" $'current_github_pr\t90'
require_contains "${TRUTH}" $'last_merged_github_pr\t89'
require_contains "${TRUTH}" $'c3eco_assessment_contract\tshorthand.c3eco.assessment.v1'
require_contains "${TRUTH}" $'production_claim\tfalse'
require_contains "${TRUTH}" $'comparative_energy_claim\tfalse'
require_contains "${STATUS}" 'feature_status_version: 2026-09-08-pr90'
require_contains "${STRATEGY}" 'compiler_test_strategy_version: 2026-09-08-pr90'
require_contains "${ASSESSMENT_DOC}" 'Assessment is not certification'
require_contains "${ASSESSMENT_GATE}" 'PASS: PR90 C3-ECO eligibility, A-K scoring, evidence caps, claims and eco-regression gate'

# The pipeline architecture remains inherited; PR90 cannot erase its mandatory tiers.
for anchor in \
  'Tier 0 - CI policy and repository invariants' \
  'Tier 3 - memory, undefined behavior and concurrency safety' \
  'Tier 5 - runtime/backend/hardware qualification' \
  'CPU, GPU, TPU and NPU' \
  'Release-candidate profile' \
  'PR74: multi-job DAG, GCC/Clang/platform matrix and reproducibility.' \
  'PR76: security/SAST/dependency/license policy.' \
  'PR77: hardened multi-architecture containers and ephemeral Kubernetes enforcement.' \
  'PR83: production truth and C3-ECO traceability.' \
  'PR96: enterprise pilot and zero-skip production RC aggregation.'; do
  require_contains "${PIPELINE}" "${anchor}"
done

bash "${ROOT_DIR}/scripts/check_production_truth.sh"
bash "${ROOT_DIR}/tests/governance/test_production_truth_negative.sh"
bash "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh"

printf 'PASS production readiness PR plan gate\n'
