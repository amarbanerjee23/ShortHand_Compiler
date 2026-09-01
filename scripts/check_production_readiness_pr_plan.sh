#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"
LSP_DOC="${ROOT_DIR}/docs/syntax_highlighting_lsp.md"
BACKEND_DOC="${ROOT_DIR}/docs/production_backend_hardware_qualification.md"
C3ECO_DOC="${ROOT_DIR}/docs/c3eco_language_contract.md"
PROFILE_DOC="${ROOT_DIR}/docs/c3eco_certification_profile.md"
TRUTH_DOC="${ROOT_DIR}/docs/production_truth.md"
TRUTH="${ROOT_DIR}/docs/production_truth.tsv"
TRACE="${ROOT_DIR}/docs/c3eco_traceability.tsv"

require_file() { [[ -s "$1" ]] || { echo "error: missing required file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${PLAN}" "${PIPELINE}" "${LSP_DOC}" "${BACKEND_DOC}" "${C3ECO_DOC}" "${PROFILE_DOC}" "${TRUTH_DOC}" "${TRUTH}" "${TRACE}" \
  "${ROOT_DIR}/docs/language_objectives.md" \
  "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" \
  "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" \
  "${ROOT_DIR}/docs/execution_semantics_beta_0_4.md" \
  "${ROOT_DIR}/docs/production_type_memory_model.md" \
  "${ROOT_DIR}/docs/functions_control_error_semantics.md" \
  "${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md" \
  "${ROOT_DIR}/docs/concurrent_serving_runtime.md" \
  "${ROOT_DIR}/docs/fuzz_sanitizer_race_hardening.md" \
  "${ROOT_DIR}/docs/toolchain_platform_reproducibility.md" \
  "${ROOT_DIR}/docs/signed_release_publication.md" \
  "${ROOT_DIR}/docs/external_security_policy.md" \
  "${ROOT_DIR}/docs/container_kubernetes_hardening.md" \
  "${ROOT_DIR}/docs/formatter_linter.md" \
  "${ROOT_DIR}/docs/compiler_test_strategy.md" \
  "${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv" \
  "${ROOT_DIR}/scripts/check_signed_release_contract.sh" \
  "${ROOT_DIR}/scripts/check_external_security_policy.sh" \
  "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" \
  "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" \
  "${ROOT_DIR}/scripts/check_formatter_linter.sh" \
  "${ROOT_DIR}/scripts/check_lsp_editor.sh" \
  "${ROOT_DIR}/scripts/check_production_backend_hardware_qualification.sh" \
  "${ROOT_DIR}/scripts/check_production_truth.sh" \
  "${ROOT_DIR}/scripts/check_production_type_memory_model.sh" \
  "${ROOT_DIR}/scripts/check_functions_control_error_semantics.sh" \
  "${ROOT_DIR}/tests/conformance/functions_control_matrix_beta_0_5.tsv" \
  "${ROOT_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh" \
  "${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh" \
  "${ROOT_DIR}/scripts/check_c3eco_certification_profile.sh" \
  "${ROOT_DIR}/tests/conformance/c3eco_profile_matrix_beta_0_7.tsv" \
  "${ROOT_DIR}/tests/conformance/enterprise_matrix_beta_0_6.tsv" \
  "${ROOT_DIR}/tests/governance/test_production_truth_negative.sh"; do
  require_file "${file}"
done

for anchor in \
  'production_readiness_plan_version: 2026-09-01-pr88' \
  'PLAN_STATUS: active' \
  'LAST_MERGED_GITHUB_PR: 87' \
  'CURRENT_GITHUB_PR: 88' \
  'LAST_PLANNED_GITHUB_PR: 96' \
  'CURRENT_IMPLEMENTATION_SCOPE: typed_c3eco_certification_profile' \
  'BASELINE_LANGUAGE_VERSION: beta-0.7' \
  'TARGET: enterprise production usage ready language' \
  'PR88 - Typed C3-ECO certification profile is IN PROGRESS.' \
  'remaining_planned_implementation_prs_pr88_through_pr96: 9' \
  'remaining_planned_implementation_prs_after_pr88: 8' \
  'Mandatory rule for every remaining PR' \
  'Robust pipeline architecture'; do
  require_contains "${PLAN}" "${anchor}"
done

for pr in $(seq 68 80); do
  require_contains "${PLAN}" "PR${pr} -"
  require_contains "${PLAN}" "| PR${pr} -"
done
require_contains "${PLAN}" '| Roadmap PR81 / GitHub PR82 - C3-ECO language blocks and zero-skip CI | MERGED'
for pr in $(seq 83 96); do
  require_contains "${PLAN}" "PR${pr} -"
  require_contains "${PLAN}" "| PR${pr} -"
done
require_contains "${PLAN}" '| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | MERGED'
require_contains "${PLAN}" '| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | MERGED as GitHub PR75'
require_contains "${PLAN}" '| PR75 - Signed release and protected publication workflow | MERGED as GitHub PR76'
require_contains "${PLAN}" '| PR76 - External vulnerability, SAST, dependency and license policy gate | MERGED as GitHub PR77'
require_contains "${PLAN}" '| PR77 - Container and Kubernetes production hardening | MERGED as GitHub PR78'
require_contains "${PLAN}" '| PR78 - Formatter and linter baseline | MERGED as GitHub PR79'
require_contains "${PLAN}" '| PR79 - Syntax highlighting and LSP implementation | MERGED as GitHub PR80'
require_contains "${PLAN}" '| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | MERGED as GitHub PR81'
require_contains "${PLAN}" '| PR83 - Production truth baseline and C3-ECO traceability | MERGED'
require_contains "${PLAN}" '| PR84 - Production type system and memory model | MERGED'
require_contains "${PLAN}" '| PR85 - Functions, structured control flow and error semantics | MERGED'
require_contains "${PLAN}" '| PR86 - Enterprise packages, standard library and FFI | MERGED'
require_contains "${PLAN}" '| PR87 - Concurrent serving and operational runtime | MERGED'
require_contains "${PLAN}" '| PR88 - Typed C3-ECO certification profile | IN PROGRESS'

for anchor in \
  'ci_pipeline_architecture_version: 2026-09-01-pr88' \
  'Tier 0 - CI policy and repository invariants' \
  'Tier 3 - memory, undefined behavior and concurrency safety' \
  'Tier 5 - runtime/backend/hardware qualification' \
  'CPU, GPU, TPU and NPU' \
  'Release-candidate profile' \
  'PR74: multi-job DAG, GCC/Clang/platform matrix and reproducibility.' \
  'PR76: security/SAST/dependency/license policy.' \
  'PR77: hardened multi-architecture containers and ephemeral Kubernetes enforcement.' \
  'PR83: production truth and C3-ECO traceability.' \
  'PR84: production type and memory model plus beta-0.4 typed execution.' \
  'PR85: functions, lexical scopes, structured control flow and deterministic errors under beta-0.5.' \
  'PR86: enterprise ABI schemas/ownership plans, cryptographic offline packages, core library and safe FFI under beta-0.6.' \
  'PR87: concurrent serving and operational runtime.' \
  'PR88: typed C3-ECO profile and deterministic migration review.' \
  'PR96: enterprise pilot and zero-skip production RC aggregation.'; do
  require_contains "${PIPELINE}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/module_resolution_and_lockfile.md" 'resolution_status: deterministic_manifest_locked_multi_file_codegen'
require_contains "${ROOT_DIR}/docs/execution_semantics_beta_0_3.md" 'execution_semantics_contract: beta-0.3-pr72-v1'
require_contains "${ROOT_DIR}/docs/execution_semantics_beta_0_4.md" 'execution_semantics_contract: beta-0.4-pr84-v1'
require_contains "${ROOT_DIR}/docs/production_type_memory_model.md" 'type_system_contract: shorthand.type_memory.v1'
require_contains "${ROOT_DIR}/docs/fuzz_sanitizer_race_hardening.md" 'fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1'
require_contains "${ROOT_DIR}/docs/toolchain_platform_reproducibility.md" 'toolchain_platform_contract_version: shorthand.portability.reproducibility.v1'
require_contains "${ROOT_DIR}/docs/signed_release_publication.md" 'signed_release_contract_version: shorthand.release.protected.v1'
require_contains "${ROOT_DIR}/docs/external_security_policy.md" 'external_security_policy_version: shorthand.security.external.v1'
require_contains "${ROOT_DIR}/docs/container_kubernetes_hardening.md" 'container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1'
require_contains "${ROOT_DIR}/docs/formatter_linter.md" 'formatter_linter_contract_version: shorthand.tooling.format_lint.v1'
require_contains "${LSP_DOC}" 'lsp_editor_contract_version: shorthand.tooling.lsp.v1'
require_contains "${LSP_DOC}" '1 MiB'
require_contains "${BACKEND_DOC}" 'backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1'
require_contains "${BACKEND_DOC}" 'production_scope: linux-x64-cpu-v1'
require_contains "${ROOT_DIR}/scripts/check_production_backend_hardware_qualification.sh" 'PASS production backend and hardware qualification gate'
require_contains "${ROOT_DIR}/scripts/check_signed_release_contract.sh" 'PASS signed release and protected publication contract gate'
require_contains "${ROOT_DIR}/scripts/check_external_security_policy.sh" 'PASS external vulnerability SAST dependency and license policy gate'
require_contains "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" 'PASS container Kubernetes production hardening contract'
require_contains "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" 'PASS ephemeral Kubernetes production gate'
require_contains "${ROOT_DIR}/scripts/check_formatter_linter.sh" 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate'
require_contains "${ROOT_DIR}/scripts/check_lsp_editor.sh" 'PASS syntax highlighting LSP protocol compiler-diagnostics navigation cancellation UTF16 bounded-framing gate'
require_contains "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh" 'PASS compiler test strategy and coverage audit gate'
require_contains "${ROOT_DIR}/scripts/check_production_truth.sh" 'PASS production truth and C3-ECO traceability gate'
require_contains "${ROOT_DIR}/scripts/check_production_type_memory_model.sh" 'PASS production type and memory model gate'
require_contains "${ROOT_DIR}/docs/functions_control_error_semantics.md" 'control_flow_contract: shorthand.control_flow.v1'
require_contains "${ROOT_DIR}/scripts/check_functions_control_error_semantics.sh" 'PASS beta-0.5 functions scopes control flow deterministic errors and cleanup gate'
require_contains "${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md" 'enterprise_contract: shorthand.enterprise_language.v1'
require_contains "${ROOT_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh" 'PASS enterprise packages standard library and safe FFI gate'
require_contains "${ROOT_DIR}/docs/concurrent_serving_runtime.md" 'serving_runtime_contract: shorthand.serving.runtime.v1'
require_contains "${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh" 'PASS concurrent serving cancellation deadline backpressure quota isolation health load soak restart and graceful shutdown gate'
require_contains "${PROFILE_DOC}" 'c3eco_profile_contract: shorthand.c3eco.profile.v2'
require_contains "${ROOT_DIR}/scripts/check_c3eco_certification_profile.sh" 'PASS typed C3-ECO profile identity units links boundary materiality lifecycle validity migration and claim-safety gate'
require_contains "${ROOT_DIR}/tests/governance/test_production_truth_negative.sh" 'PASS production truth negative contradiction, contract, completeness, mapping and evidence cases'

# Historical milestones remain auditable without being mistaken for active state.
for anchor in \
  'production_readiness_plan_version: 2026-08-18-pr79' \
  'CURRENT_IMPLEMENTATION_PR: 79' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR79: 80' \
  'remaining_planned_implementation_prs_pr79_through_pr86: 8' \
  '| PR79 - Syntax highlighting and LSP implementation | IN PROGRESS as GitHub PR80' \
  'production_readiness_plan_version: 2026-08-18-pr78' \
  'CURRENT_IMPLEMENTATION_PR: 78' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR78: 79' \
  'remaining_planned_implementation_prs_pr78_through_pr86: 9' \
  '| PR78 - Formatter and linter baseline | IN PROGRESS as GitHub PR79' \
  'production_readiness_plan_version: 2026-08-12-pr77' \
  'CURRENT_IMPLEMENTATION_PR: 77' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR77: 78' \
  'remaining_planned_implementation_prs_pr77_through_pr86: 10' \
  '| PR77 - Container and Kubernetes production hardening | IN PROGRESS as GitHub PR78' \
  'production_readiness_plan_version: 2026-08-12-pr76' \
  'CURRENT_IMPLEMENTATION_PR: 76' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR76: 77' \
  'remaining_planned_implementation_prs_pr76_through_pr86: 11' \
  '| PR76 - External vulnerability, SAST, dependency and license policy gate | IN PROGRESS as GitHub PR77' \
  'production_readiness_plan_version: 2026-08-12-pr75' \
  'CURRENT_IMPLEMENTATION_PR: 75' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR75: 76' \
  'remaining_planned_implementation_prs_pr75_through_pr86: 12' \
  '| PR75 - Signed release and protected publication workflow | IN PROGRESS as GitHub PR76' \
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

require_contains "${C3ECO_DOC}" 'c3eco_language_contract_version: shorthand.c3eco.language.v1'
require_contains "${ROOT_DIR}/scripts/check_c3eco_language_blocks.sh" 'PASS C3-ECO first-class language blocks grammar AST semantics evidence and claim-safety gate'

bash "${ROOT_DIR}/scripts/check_production_truth.sh"
bash "${ROOT_DIR}/tests/governance/test_production_truth_negative.sh"

printf 'PASS production readiness PR plan gate\n'
