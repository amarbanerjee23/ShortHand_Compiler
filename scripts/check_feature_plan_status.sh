#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"
STATUS_FILE=docs/feature_implementation_status.md

required_files=(
  "${STATUS_FILE}"
  docs/compiler_test_strategy.md
  docs/production_readiness_pr_plan.md
  docs/production_truth.md
  docs/production_truth.tsv
  docs/c3eco_traceability.tsv
  docs/production_backend_hardware_qualification.md
  docs/c3eco_language_contract.md
  docs/c3eco_certification_profile.md
  docs/c3eco_measurement_workbook.md
  docs/c3eco_assessment.md
  schemas/c3eco_measurement_workbook_v1.schema.json
  schemas/c3eco_assessment_v1.schema.json
  Compiler_new_ws/Short_Hand/src/evidence/MeasurementWorkbook.cpp
  Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessment.h
  Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessmentIO.cpp
  Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessmentScoring.cpp
  Compiler_new_ws/Short_Hand/src/evidence/AssessmentEngine.cpp
  tests/c3eco/assessment/test_c3eco_assessment_scoring.cpp
  scripts/check_c3eco_measurement_workbook.sh
  scripts/check_c3eco_assessment.sh
  docs/execution_semantics_beta_0_3.md
  docs/execution_semantics_beta_0_4.md
  docs/production_type_memory_model.md
  docs/functions_control_error_semantics.md
  docs/enterprise_packages_stdlib_ffi.md
  docs/concurrent_serving_runtime.md
  docs/fuzz_sanitizer_race_hardening.md
  docs/signed_release_publication.md
  docs/external_security_policy.md
  docs/toolchain_platform_reproducibility.md
  docs/container_kubernetes_hardening.md
  docs/formatter_linter.md
  docs/syntax_highlighting_lsp.md
  tests/coverage/compiler_test_coverage_matrix.tsv
  tests/conformance/functions_control_matrix_beta_0_5.tsv
  tests/conformance/enterprise_matrix_beta_0_6.tsv
  tests/conformance/c3eco_profile_matrix_beta_0_7.tsv
  tests/integration/test_production_backend_hardware_qualification.sh
  tests/integration/test_compiled_hook_onnxruntime_success.sh
  scripts/check_module_resolution.sh
  scripts/check_semantic_differential.sh
  scripts/check_fuzz_sanitizers.sh
  scripts/check_runtime_memory_sanitizer.sh
  scripts/check_thread_sanitizer.sh
  scripts/check_ci_status_hygiene.sh
  scripts/check_signed_release_contract.sh
  scripts/check_external_security_policy.sh
  scripts/check_third_party_license_policy.sh
  scripts/check_security_exceptions.sh
  scripts/check_action_pinning.sh
  scripts/check_container_kubernetes_hardening.sh
  scripts/check_container_runtime.sh
  scripts/check_kubernetes_ephemeral_cluster.sh
  scripts/check_formatter_linter.sh
  scripts/check_lsp_editor.sh
  scripts/install_ci_onnxruntime_cpu.sh
  scripts/check_production_backend_hardware_qualification.sh
  scripts/check_c3eco_language_blocks.sh
  scripts/check_no_mandatory_test_skips.sh
  scripts/check_production_truth.sh
  scripts/check_production_type_memory_model.sh
  scripts/check_functions_control_error_semantics.sh
  scripts/check_enterprise_packages_stdlib_ffi.sh
  scripts/check_concurrent_serving_runtime.sh
  scripts/check_c3eco_certification_profile.sh
  tests/governance/test_production_truth_negative.sh
  Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h
  Compiler_new_ws/Short_Hand/src/serving/ServingRuntime.cpp
  Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp
  tests/deployment/test_container_kubernetes_hardening_negative.sh
  deploy/k8s/production.yaml
  security/third_party_inventory.tsv
  .github/workflows/tooling.yml
  .github/workflows/release.yml
  .github/workflows/security.yml
  .github/dependency-review-config.yml
)
for file in "${required_files[@]}"; do
  [[ -s "${file}" ]] || { echo "error: required feature/status evidence missing: ${file}" >&2; exit 1; }
done

required_status_terms=(
  "Implemented" "Partial" "Open" "Production blockers"
  "Real ONNX Runtime CPU backend execution"
  "Full backend compatibility"
  "Cross-mode semantic equivalence"
  "Full sanitizer coverage"
  "Continuous fuzzing" "Concurrency and race detection"
  "Cross-platform reproducibility"
  "Measured ShortHand versus Python energy evidence"
  "Zero-skip production RC gate" "CPU/GPU/TPU/NPU"
  "Signed releases" "Protected publication"
  "External vulnerability gate" "Container and Kubernetes hardening"
  "Formatter and linter" "Syntax highlighting and LSP"
  "Production type and memory model"
  "Functions, structured control flow and deterministic errors"
  "Concurrent serving and operational runtime"
  "Typed C3-ECO certification profile"
  "Instrumented C3-ECO measurement/accounting"
  "C3-ECO assessment, scoring and controlled claims"
)
for term in "${required_status_terms[@]}"; do
  grep -Fiq "${term}" "${STATUS_FILE}" || { echo "error: feature implementation status missing required tracking term: ${term}" >&2; exit 1; }
done

for anchor in \
  'feature_status_version: 2026-09-08-pr90' \
  'language_version: beta-0.7' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  'current_github_pr: 90' \
  'current_roadmap_scope: c3eco_eligibility_scoring_claims_eco_regression' \
  '29 implemented, 3 partial and 3 open' \
  'GitHub PR90 now implements the `shorthand.c3eco.assessment.v1` candidate' \
  'C3-ECO assessment, scoring and controlled claims | Implemented for `shorthand.c3eco.assessment.v1` candidate' \
  'c3eco_assessment_contract: shorthand.c3eco.assessment.v1' \
  'assessment_status: certification_readiness_candidate' \
  'comparative_energy_claim: false' \
  'official_certification_granted: false' \
  'production_claim: false'; do
  grep -Fiq "${anchor}" "${STATUS_FILE}" || { echo "error: feature implementation status missing PR90 active anchor: ${anchor}" >&2; exit 1; }
done

for anchor in \
  'feature_status_version: 2026-09-02-pr89' \
  'current_github_pr: 89' \
  'current_roadmap_scope: measurement_carbon_accounting_cost_workbook' \
  '28 implemented, 3 partial and 3 open' \
  'feature_status_version: 2026-09-01-pr88' \
  'current_github_pr: 88' \
  'current_roadmap_scope: typed_c3eco_certification_profile' \
  '27 implemented, 3 partial and 3 open' \
  'GitHub PR85 implemented beta-0.5 functions/control flow' \
  'GitHub PR86 implemented the bounded beta-0.6 enterprise schema' \
  'GitHub PR87 implemented the process-scoped concurrent serving and operational runtime' \
  'GitHub PR88 now implements the beta-0.7 typed C3-ECO certification-preparation profile'; do
  grep -Fiq "${anchor}" "${STATUS_FILE}" || { echo "error: feature implementation status missing historical audit anchor: ${anchor}" >&2; exit 1; }
done

grep -Fq 'resolution_status: deterministic_manifest_locked_multi_file_codegen' docs/module_resolution_and_lockfile.md
grep -Fq 'execution_semantics_contract: beta-0.3-pr72-v1' docs/execution_semantics_beta_0_3.md
grep -Fq 'execution_semantics_contract: beta-0.4-pr84-v1' docs/execution_semantics_beta_0_4.md
grep -Fq 'type_system_contract: shorthand.type_memory.v1' docs/production_type_memory_model.md
grep -Fq 'control_flow_contract: shorthand.control_flow.v1' docs/functions_control_error_semantics.md
grep -Fq 'enterprise_contract: shorthand.enterprise_language.v1' docs/enterprise_packages_stdlib_ffi.md
grep -Fq 'serving_runtime_contract: shorthand.serving.runtime.v1' docs/concurrent_serving_runtime.md
grep -Fq 'c3eco_profile_contract: shorthand.c3eco.profile.v2' docs/c3eco_certification_profile.md
grep -Fq 'shorthand.c3eco.measurement_workbook.v1' docs/c3eco_measurement_workbook.md
grep -Fq 'PASS: PR89 C3-ECO measurement, carbon accounting and cost workbook gate' scripts/check_c3eco_measurement_workbook.sh
grep -Fq 'shorthand.c3eco.assessment.v1' docs/c3eco_assessment.md
grep -Fq 'Assessment is not certification' docs/c3eco_assessment.md
grep -Fq 'shorthand.c3eco.assessment.v1' schemas/c3eco_assessment_v1.schema.json
grep -Fq 'complete 76-criterion A-K catalog' Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessmentIO.cpp
grep -Fq 'deferred_pr95' Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessmentScoring.cpp
grep -Fq 'PASS: PR90 C3-ECO eligibility, A-K scoring, evidence caps, claims and eco-regression gate' scripts/check_c3eco_assessment.sh
grep -Fq 'PASS: PR90 C3-ECO scoring unit' tests/c3eco/assessment/test_c3eco_assessment_scoring.cpp
grep -Fq 'backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1' docs/production_backend_hardware_qualification.md
grep -Fq 'production_scope: linux-x64-cpu-v1' docs/production_backend_hardware_qualification.md
grep -Fq 'backend_device_not_production_qualified' Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h
grep -Fq 'formatter_linter_contract_version: shorthand.tooling.format_lint.v1' docs/formatter_linter.md
grep -Fq 'lsp_editor_contract_version: shorthand.tooling.lsp.v1' docs/syntax_highlighting_lsp.md
grep -Fq 'SHLSP900' Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp
grep -Fq 'add_executable(shorthand_c3eco_assess' CMakeLists.txt

for gate in \
  'PASS CI status hygiene guard|scripts/check_ci_status_hygiene.sh' \
  'PASS signed release and protected publication contract gate|scripts/check_signed_release_contract.sh' \
  'PASS external vulnerability SAST dependency and license policy gate|scripts/check_external_security_policy.sh' \
  'PASS container Kubernetes production hardening contract|scripts/check_container_kubernetes_hardening.sh' \
  'PASS hardened container runtime|scripts/check_container_runtime.sh' \
  'PASS ephemeral Kubernetes production gate|scripts/check_kubernetes_ephemeral_cluster.sh' \
  'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate|scripts/check_formatter_linter.sh' \
  'PASS syntax highlighting LSP protocol compiler-diagnostics navigation cancellation UTF16 bounded-framing gate|scripts/check_lsp_editor.sh' \
  'PASS production backend and hardware qualification gate|scripts/check_production_backend_hardware_qualification.sh' \
  'PASS production type and memory model gate|scripts/check_production_type_memory_model.sh' \
  'PASS beta-0.5 functions scopes control flow deterministic errors and cleanup gate|scripts/check_functions_control_error_semantics.sh' \
  'PASS enterprise packages standard library and safe FFI gate|scripts/check_enterprise_packages_stdlib_ffi.sh' \
  'PASS concurrent serving cancellation deadline backpressure quota isolation health load soak restart and graceful shutdown gate|scripts/check_concurrent_serving_runtime.sh' \
  'PASS typed C3-ECO profile identity units links boundary materiality lifecycle validity migration and claim-safety gate|scripts/check_c3eco_certification_profile.sh'; do
  text="${gate%%|*}"; file="${gate#*|}"
  grep -Fq "${text}" "${file}" || { echo "error: inherited gate anchor missing: ${file}" >&2; exit 1; }
done

bash scripts/check_container_kubernetes_hardening.sh
bash tests/deployment/test_container_kubernetes_hardening_negative.sh
bash scripts/check_formatter_linter.sh
bash scripts/check_lsp_editor.sh
bash scripts/check_no_mandatory_test_skips.sh
bash scripts/check_c3eco_language_blocks.sh
bash scripts/check_production_truth.sh
bash scripts/check_production_type_memory_model.sh
bash scripts/check_functions_control_error_semantics.sh
bash scripts/check_enterprise_packages_stdlib_ffi.sh
bash scripts/check_concurrent_serving_runtime.sh
bash scripts/check_c3eco_certification_profile.sh
bash scripts/check_c3eco_measurement_workbook.sh
bash scripts/check_c3eco_assessment.sh
bash tests/governance/test_production_truth_negative.sh
bash tests/integration/test_production_backend_hardware_qualification.sh

if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" && "$(uname -m)" == "x86_64" ]]; then
  ORT_ROOT="${ONNXRUNTIME_ROOT:-${RUNNER_TEMP:-/tmp}/shorthand-onnxruntime-1.20.1}"
  if [[ ! -s "${ORT_ROOT}/include/onnxruntime_cxx_api.h" ]]; then
    bash scripts/install_ci_onnxruntime_cpu.sh "${ORT_ROOT}"
  fi
  ONNXRUNTIME_ROOT="${ORT_ROOT}" bash scripts/check_production_backend_hardware_qualification.sh
  bash scripts/check_kubernetes_ephemeral_cluster.sh
fi

unsupported_claim_patterns=(
  "Current status: fully production-ready"
  "ShortHand is fully production-ready"
  "all production blockers are complete"
  "ShortHand uses less energy than Python"
  "C3-ECO Certified"
  "official_certification_granted: true"
  "GPU production support is implemented"
  "TPU production support is implemented"
  "NPU production support is implemented"
)
for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -Fiq "${pattern}" "${STATUS_FILE}"; then
    echo "error: unsupported production/certification claim in feature status: ${pattern}" >&2
    exit 1
  fi
done

echo "Feature plan status check passed. GitHub PR90 adds deterministic C3-ECO eligibility, scoring, evidence caps, controlled claims and eco-regression while preserving all inherited zero-skip qualification gates; PR91-PR96 and the protected release exercise remain fail-closed."
