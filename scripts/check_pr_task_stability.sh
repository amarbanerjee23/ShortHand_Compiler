#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required stability text: ${needle}" >&2
    exit 1
  fi
}

require_bash_syntax() {
  local file="$1"
  require_file "${file}"
  bash -n "${file}"
}

require_file .github/workflows/ci.yml
require_file scripts/check_feature_plan_status.sh
require_file scripts/check_enterprise_hardening.sh
require_file docs/feature_implementation_status.md
require_file docs/pr_task_stability_strategy.md
require_file docs/production_readiness_pr_plan.md
require_file docs/language_grammar_ebnf.md
require_file docs/language_spec.md
require_file docs/language_versioning_and_conformance.md
require_file docs/compiled_infer_bridge.md
require_file docs/backend_compatibility_matrix.md
require_file docs/ai_runtime_bridge_linkage.md
require_file docs/ai_runtime_execution_adapter.md
require_file docs/runtime_observability_exports.md
require_file tests/conformance/manifest.txt
require_file tests/codegen/test_runtime_ai_bridge_link_build.sh
require_file tests/codegen/test_runtime_ai_bridge_execution_path.sh
require_file tests/codegen/test_runtime_observability_exports.sh
require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file scripts/check_compiled_hook_onnxruntime_success.sh
require_file scripts/check_runtime_observability_exports.sh
require_file scripts/check_language_versioning.sh
require_file scripts/check_production_readiness_pr_plan.sh

# Preserve old task names and ordering anchors from the CI workflow.
require_contains .github/workflows/ci.yml 'Strict language validation'
require_contains .github/workflows/ci.yml 'Smoke tests'
require_contains .github/workflows/ci.yml 'Feature plan status check'
require_contains .github/workflows/ci.yml 'Enterprise hardening check'
require_contains .github/workflows/ci.yml 'Makefile test suite'
require_contains .github/workflows/ci.yml 'Certification bundle smoke'
require_contains .github/workflows/ci.yml 'Sanitizer tests'
require_contains .github/workflows/ci.yml 'Configure CMake'
require_contains .github/workflows/ci.yml 'Build with CMake'
require_contains .github/workflows/ci.yml 'Run CTest'

# Preserve feature tracker anchor phrases that old gates depend on.
require_contains docs/feature_implementation_status.md 'Automated SBOM'
require_contains docs/feature_implementation_status.md 'Runtime observability implementation'
require_contains docs/feature_implementation_status.md 'Module/import/package model'
require_contains docs/feature_implementation_status.md 'Production blockers'
require_contains docs/feature_implementation_status.md 'Compiled-code metadata/runtime lowering'

# Preserve existing enterprise hardening gate coverage before new gates are added.
require_contains scripts/check_enterprise_hardening.sh 'check_language_correctness.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_c3eco_claims_and_schema.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_mlir_foundation.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_pr_task_stability.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_release_supply_chain.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_backend_compatibility_matrix.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_ai_runtime_bridge_linkage.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_ai_runtime_execution_adapter.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_runtime_ai_bridge_link_build.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_runtime_ai_bridge_execution_path.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_runtime_observability_exports.sh'
require_contains scripts/check_enterprise_hardening.sh 'shorthand.runtime.compiled_infer_bridge_request.v1'
require_contains scripts/check_enterprise_hardening.sh 'shorthand.runtime.typed_infer_buffer_bridge_request.v1'

# Keep guardrail scripts syntactically valid before they can block CI.
for script in scripts/check_feature_plan_status.sh scripts/check_enterprise_hardening.sh scripts/check_pr_task_stability.sh scripts/check_language_correctness.sh scripts/check_language_versioning.sh scripts/check_production_readiness_pr_plan.sh scripts/check_c3eco_claims_and_schema.sh scripts/check_mlir_foundation.sh scripts/check_release_supply_chain.sh scripts/check_backend_compatibility_matrix.sh scripts/check_ai_runtime_bridge_linkage.sh scripts/check_ai_runtime_execution_adapter.sh scripts/check_runtime_ai_bridge_link_build.sh scripts/check_runtime_ai_bridge_execution_path.sh scripts/check_compiled_hook_onnxruntime_success.sh scripts/check_runtime_observability_exports.sh scripts/generate_release_sbom.sh; do
  require_bash_syntax "${script}"
done

# The strategy itself must explain the old-task contract so future changes do not silently rename CI anchors.
require_contains docs/pr_task_stability_strategy.md 'Old-task contract'
require_contains docs/pr_task_stability_strategy.md 'New-gate contract'
require_contains docs/production_readiness_pr_plan.md 'production_readiness_plan_version: 2026-07-25-pr51-r2'
require_contains docs/production_readiness_pr_plan.md 'PLAN_STATUS: active'
require_contains docs/production_readiness_pr_plan.md 'Desired outcome definition'
require_contains docs/production_readiness_pr_plan.md 'Recommended path from PR #51 onward: 28 PRs total.'
require_contains docs/production_readiness_pr_plan.md 'PR52 - Backend live SDK matrix harness'
require_contains docs/production_readiness_pr_plan.md 'PR58 - Runtime ABI and API version stability gate'
require_contains docs/production_readiness_pr_plan.md 'PR59 - Runtime state isolation and thread-safety policy'
require_contains docs/production_readiness_pr_plan.md 'PR66 - Parser robustness and negative corpus hardening'
require_contains docs/production_readiness_pr_plan.md 'PR78 - MLIR lowering passes and production RC gate'
require_contains scripts/check_production_readiness_pr_plan.sh 'PASS production readiness PR plan gate'
require_contains docs/language_grammar_ebnf.md 'Language version: beta-0.1'
require_contains docs/language_spec.md 'Language version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.language.version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.conformance.contract: beta-0.1'
require_contains tests/conformance/manifest.txt 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'
require_contains scripts/check_language_versioning.sh 'PASS language versioning and conformance gate'
require_contains scripts/check_language_correctness.sh 'check_language_versioning.sh'
require_contains docs/compiled_infer_bridge.md 'input_buffer_required_for_ai_runtime_execution'
require_contains docs/compiled_infer_bridge.md 'Typed tensor-buffer bridge'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime bridge linkage'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime execution adapter'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge link build'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge execution path'
require_contains docs/compiled_infer_bridge.md 'Compiled hook ONNX Runtime success fixture'
require_contains docs/backend_compatibility_matrix.md 'Backend execution validation tiers'
require_contains docs/backend_compatibility_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/ai_runtime_bridge_linkage.md 'runtime-hook ABI ownership'
require_contains docs/ai_runtime_execution_adapter.md 'adapter_contract_status: compile_checked_mapping_only'
require_contains docs/ai_runtime_execution_adapter.md 'bridge_link_status: runtime_adapter_ai_core_link_checked'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_execution_status: bridge_enabled_ai_runtime_infer_attempt'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_success_status: optional_onnxruntime_success_fixture'
require_contains docs/runtime_observability_exports.md 'runtime_observability_export_status: dependency_free_prometheus_and_otlp_like_exports'
require_contains scripts/check_ai_runtime_bridge_linkage.sh 'PASS AI runtime bridge linkage gate'
require_contains scripts/check_ai_runtime_execution_adapter.sh 'PASS AI runtime execution adapter gate'
require_contains scripts/check_runtime_ai_bridge_link_build.sh 'PASS runtime AI bridge link build gate'
require_contains scripts/check_runtime_ai_bridge_execution_path.sh 'PASS runtime AI bridge execution path gate'
require_contains scripts/check_runtime_ai_bridge_execution_path.sh 'check_compiled_hook_onnxruntime_success.sh'
require_contains scripts/check_compiled_hook_onnxruntime_success.sh 'PASS compiled hook ONNX Runtime success gate'
require_contains scripts/check_runtime_observability_exports.sh 'PASS runtime observability export gate'
require_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'runtime.infer(model_spec, input_buffer)'
require_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1'
require_contains tests/codegen/test_runtime_observability_exports.sh 'short_runtime_prometheus_metrics'
require_contains tests/codegen/test_runtime_observability_exports.sh 'short_runtime_otlp_spans_json'
require_contains tests/integration/test_compiled_hook_onnxruntime_success.sh 'Output: 42'

printf 'PASS PR task stability gate\n'
