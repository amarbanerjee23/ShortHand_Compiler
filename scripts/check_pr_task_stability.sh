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

required_files=(
  .github/workflows/ci.yml
  scripts/check_feature_plan_status.sh
  scripts/check_enterprise_hardening.sh
  scripts/check_pr_task_stability.sh
  scripts/check_language_correctness.sh
  scripts/check_language_versioning.sh
  scripts/check_language_objectives.sh
  scripts/check_runtime_abi_api_stability.sh
  scripts/check_runtime_state_thread_safety.sh
  scripts/check_runtime_production_packaging.sh
  scripts/check_prometheus_scrape_adapter.sh
  scripts/check_production_readiness_pr_plan.sh
  scripts/check_production_truth.sh
  scripts/check_concurrent_serving_runtime.sh
  scripts/check_c3eco_certification_profile.sh
  scripts/check_semantic_differential.sh
  scripts/check_backend_compatibility_matrix.sh
  scripts/check_backend_live_sdk_matrix.sh
  scripts/check_backend_failure_mode_matrix.sh
  scripts/check_hardware_capability_routing.sh
  scripts/check_tensorrt_optional_fixture.sh
  scripts/check_openvino_optional_fixture.sh
  scripts/check_libtorch_optional_fixture.sh
  scripts/check_llamacpp_optional_fixture.sh
  scripts/check_ai_runtime_bridge_linkage.sh
  scripts/check_ai_runtime_execution_adapter.sh
  scripts/check_runtime_ai_bridge_link_build.sh
  scripts/check_runtime_ai_bridge_execution_path.sh
  scripts/check_compiled_hook_onnxruntime_success.sh
  scripts/check_runtime_observability_exports.sh
  scripts/check_c3eco_claims_and_schema.sh
  scripts/check_mlir_foundation.sh
  scripts/check_release_supply_chain.sh
  scripts/generate_release_sbom.sh
  docs/feature_implementation_status.md
  docs/pr_task_stability_strategy.md
  docs/production_readiness_pr_plan.md
  docs/production_truth.md
  docs/production_truth.tsv
  docs/c3eco_traceability.tsv
  docs/production_readiness_history.md
  docs/execution_semantics_beta_0_3.md
  docs/language_grammar_ebnf.md
  docs/language_spec.md
  docs/language_versioning_and_conformance.md
  docs/language_objectives.md
  docs/runtime_abi_api_stability.md
  docs/runtime_state_and_thread_safety.md
  docs/runtime_production_packaging.md
  docs/concurrent_serving_runtime.md
  docs/c3eco_certification_profile.md
  docs/prometheus_scrape_host_adapter.md
  docs/compiled_infer_bridge.md
  docs/backend_compatibility_matrix.md
  docs/backend_live_sdk_matrix.md
  docs/backend_failure_mode_matrix.md
  docs/hardware_capability_routing.md
  docs/tensorrt_optional_fixture.md
  docs/openvino_optional_fixture.md
  docs/libtorch_optional_fixture.md
  docs/llamacpp_optional_fixture.md
  docs/ai_runtime_bridge_linkage.md
  docs/ai_runtime_execution_adapter.md
  docs/runtime_observability_exports.md
  abi/runtime_public_symbols_v1.txt
  abi/shorthand_runtime_abi_v1.h
  cmake/ShortHandConfig.cmake.in
  cmake/shorthand-runtime.pc.in
  cmake/shorthand-ai-bridge.pc.in
  Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h
  Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
  Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp
  Compiler_new_ws/Short_Hand/src/serving/ServingRuntime.h
  Compiler_new_ws/Short_Hand/src/serving/ServingRuntime.cpp
  Compiler_new_ws/Short_Hand/src/serving/ServingWorkerMain.cpp
  Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp
  Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h
  Compiler_new_ws/Short_Hand/src/Makefile
  CMakeLists.txt
  tests/conformance/manifest.txt
  tests/abi/test_runtime_abi_api_stability.sh
  tests/runtime/test_runtime_state_thread_safety.sh
  tests/runtime/test_serving_runtime.cpp
  tests/runtime/serving_runtime_stress.cpp
  tests/c3eco/profile/c3eco_profile_v2.short
  tests/c3eco/profile/c3eco_profile_v2_negative_matrix.short
  tests/packaging/test_runtime_production_packaging.sh
  tests/operations/test_prometheus_scrape_adapter.sh
  tests/codegen/test_runtime_ai_bridge_link_build.sh
  tests/codegen/test_runtime_ai_bridge_execution_path.sh
  tests/codegen/test_runtime_observability_exports.sh
  tests/integration/test_compiled_hook_onnxruntime_success.sh
  tests/integration/test_backend_live_sdk_matrix.sh
  tests/integration/test_backend_failure_mode_matrix.sh
  tests/integration/test_hardware_capability_routing.sh
  tests/integration/test_tensorrt_optional_fixture.sh
  tests/integration/test_openvino_optional_fixture.sh
  tests/integration/test_libtorch_optional_fixture.sh
  tests/integration/test_llamacpp_optional_fixture.sh
  tests/governance/test_production_truth_negative.sh
)
for file in "${required_files[@]}"; do require_file "${file}"; done

for task in \
  'Strict language validation' \
  'Semantic differential execution' \
  'Production truth and C3-ECO traceability' \
  'Concurrent serving and operational runtime' \
  'Typed C3-ECO certification profile beta-0.7' \
  'Smoke tests' \
  'Feature plan status check' \
  'Enterprise hardening check' \
  'Makefile test suite' \
  'Certification bundle smoke' \
  'Sanitizer tests' \
  'Configure CMake' \
  'Build with CMake' \
  'Run CTest'; do
  require_contains .github/workflows/ci.yml "${task}"
done

for anchor in \
  'Automated SBOM' \
  'Runtime observability implementation' \
  'Module/import/package model' \
  'Production blockers' \
  'Compiled-code metadata/runtime lowering' \
  'Cross-mode semantic equivalence'; do
  require_contains docs/feature_implementation_status.md "${anchor}"
done

for gate in \
  check_language_correctness.sh \
  check_c3eco_claims_and_schema.sh \
  check_mlir_foundation.sh \
  check_pr_task_stability.sh \
  check_release_supply_chain.sh \
  check_backend_compatibility_matrix.sh \
  check_ai_runtime_bridge_linkage.sh \
  check_ai_runtime_execution_adapter.sh \
  check_runtime_ai_bridge_link_build.sh \
  check_runtime_ai_bridge_execution_path.sh \
  check_runtime_observability_exports.sh \
  check_prometheus_scrape_adapter.sh; do
  require_contains scripts/check_enterprise_hardening.sh "${gate}"
done

require_contains scripts/check_enterprise_hardening.sh 'check_production_truth.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_concurrent_serving_runtime.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_c3eco_certification_profile.sh'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-governance'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-serving'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-c3eco-profile'
require_contains CMakeLists.txt 'NAME production_truth_traceability'
require_contains CMakeLists.txt 'NAME production_truth_negative'
require_contains CMakeLists.txt 'NAME concurrent_serving_runtime'
require_contains CMakeLists.txt 'NAME c3eco_certification_profile'

require_contains scripts/check_language_correctness.sh 'check_runtime_abi_api_stability.sh'
require_contains scripts/check_language_correctness.sh 'check_runtime_state_thread_safety.sh'
require_contains scripts/check_backend_compatibility_matrix.sh 'check_hardware_capability_routing.sh'
require_contains scripts/check_backend_compatibility_matrix.sh 'check_backend_failure_mode_matrix.sh'
require_contains scripts/check_semantic_differential.sh 'PASS cross-mode semantic differential execution gate'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-semantic-differential'

shell_files=(
  scripts/check_feature_plan_status.sh
  scripts/check_enterprise_hardening.sh
  scripts/check_pr_task_stability.sh
  scripts/check_language_correctness.sh
  scripts/check_language_versioning.sh
  scripts/check_language_objectives.sh
  scripts/check_runtime_abi_api_stability.sh
  scripts/check_runtime_state_thread_safety.sh
  scripts/check_runtime_production_packaging.sh
  scripts/check_prometheus_scrape_adapter.sh
  scripts/check_production_readiness_pr_plan.sh
  scripts/check_production_truth.sh
  scripts/check_concurrent_serving_runtime.sh
  scripts/check_c3eco_certification_profile.sh
  scripts/check_semantic_differential.sh
  scripts/check_c3eco_claims_and_schema.sh
  scripts/check_mlir_foundation.sh
  scripts/check_release_supply_chain.sh
  scripts/check_backend_compatibility_matrix.sh
  scripts/check_backend_live_sdk_matrix.sh
  scripts/check_backend_failure_mode_matrix.sh
  scripts/check_hardware_capability_routing.sh
  scripts/check_tensorrt_optional_fixture.sh
  scripts/check_openvino_optional_fixture.sh
  scripts/check_libtorch_optional_fixture.sh
  scripts/check_llamacpp_optional_fixture.sh
  scripts/check_ai_runtime_bridge_linkage.sh
  scripts/check_ai_runtime_execution_adapter.sh
  scripts/check_runtime_ai_bridge_link_build.sh
  scripts/check_runtime_ai_bridge_execution_path.sh
  scripts/check_compiled_hook_onnxruntime_success.sh
  scripts/check_runtime_observability_exports.sh
  scripts/generate_release_sbom.sh
  tests/abi/test_runtime_abi_api_stability.sh
  tests/runtime/test_runtime_state_thread_safety.sh
  tests/packaging/test_runtime_production_packaging.sh
  tests/operations/test_prometheus_scrape_adapter.sh
  tests/integration/test_backend_failure_mode_matrix.sh
  tests/governance/test_production_truth_negative.sh
)
for script in "${shell_files[@]}"; do require_bash_syntax "${script}"; done

require_contains docs/pr_task_stability_strategy.md 'Old-task contract'
require_contains docs/pr_task_stability_strategy.md 'New-gate contract'

# Old roadmap/task anchors are immutable history, not active plan state. Moving
# them into a dedicated history document keeps the old-task contract strict
# without forcing stale CURRENT_IMPLEMENTATION_PR text into today's roadmap.
require_contains docs/production_readiness_history.md 'production_readiness_history_contract: immutable-milestone-anchors-v1'
for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr62' \
  'LAST_COMPLETED_PR: 62' \
  'Language objectives consolidation applied in PR #57' \
  'Backend failure-mode finalization applied in PR #58' \
  'Runtime ABI and API stability applied in PR #59' \
  'Runtime state and thread-safety applied in PR #60' \
  'Production build packaging applied in PR #61' \
  'Prometheus scrape endpoint host adapter applied in PR #62' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'PR59 - Runtime ABI and API version stability gate | MERGED' \
  'PR60 - Runtime state isolation and thread-safety policy | MERGED' \
  'PR61 - Production build packaging for runtime and AI bridge | MERGED' \
  'PR62 - Prometheus scrape endpoint host adapter | MERGED' \
  'Next recommended PR after PR #62:' \
  'PR63 - OTLP exporter adapter.' \
  'remaining_planned_prs_after_pr61: 18' \
  'remaining_planned_prs_after_pr62: 17' \
  'PR67 - Parser robustness and negative corpus hardening' \
  'PR79 - MLIR lowering passes and production RC gate'; do
  require_contains docs/production_readiness_history.md "${anchor}"
done

for anchor in \
  'production_readiness_plan_version: 2026-08-11-pr72' \
  'CURRENT_IMPLEMENTATION_PR: 72' \
  'NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73' \
  '| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED' \
  '| PR71 - CI status publication hygiene | MERGED' \
  '| PR72 - Cross-mode semantic correctness and differential execution suite | IN PROGRESS' \
  'remaining_planned_implementation_prs_pr73_through_pr86: 14'; do
  require_contains docs/production_readiness_pr_plan.md "${anchor}"
done
require_contains scripts/check_production_readiness_pr_plan.sh 'PASS production readiness PR plan gate'

require_contains docs/language_grammar_ebnf.md 'Language version: beta-0.1'
require_contains docs/language_spec.md 'Language version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.language.version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.conformance.contract: beta-0.1'
require_contains docs/language_objectives.md 'shorthand.language.objectives.version: 2026-07-29-v1'
require_contains docs/language_objectives.md 'production_claim: false'
require_contains docs/language_objectives.md 'Honest execution and fallback'
require_contains docs/language_objectives.md 'First-class Green AI evidence'
require_contains docs/execution_semantics_beta_0_3.md 'execution_semantics_contract: beta-0.3-pr72-v1'
require_contains docs/execution_semantics_beta_0_3.md 'production_claim: false'
require_contains tests/conformance/manifest.txt 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'

for anchor in \
  'runtime_abi_contract_status: frozen_v1_symbol_manifest' \
  'runtime_abi_version: 1.0.0' \
  'runtime_api_version: 1.0.0' \
  'runtime_external_symbol_count: 25' \
  'production_claim_boundary: abi_stability_gate_is_not_full_production_readiness'; do
  require_contains docs/runtime_abi_api_stability.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h '#define SHORTHAND_RUNTIME_ABI_VERSION_STRING "1.0.0"'
require_contains abi/shorthand_runtime_abi_v1.h 'Frozen consumer snapshot for ShortHand runtime ABI 1.0.0.'
require_contains abi/runtime_public_symbols_v1.txt 'short_runtime_reset'
require_contains abi/runtime_public_symbols_v1.txt 'short_ai_infer_f32'
require_contains tests/abi/test_runtime_abi_api_stability.sh 'ABI_SYMBOL_COUNT'
require_contains scripts/check_runtime_abi_api_stability.sh 'PASS runtime ABI and API stability gate'

for anchor in \
  'runtime_state_contract_version: 1.0.0' \
  'runtime_state_model: single_process_wide_default_context' \
  'runtime_thread_safety_status: serialized_public_abi' \
  'runtime_multi_tenant_isolation: process_boundary_required' \
  'production_claim_boundary: thread_safe_does_not_mean_multi_tenant_isolated'; do
  require_contains docs/runtime_state_and_thread_safety.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp 'std::recursive_mutex &runtimeMutex()'
require_contains Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp 'thread_local std::string snapshot'
require_contains Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp 'shimpl_ai_infer_f32'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'RUNTIME_ABI_RENAME_DEFS'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'RuntimeThreadSafeFacade.o'
require_contains CMakeLists.txt 'SHORTHAND_RUNTIME_ABI_RENAMES'
require_contains CMakeLists.txt 'RuntimeThreadSafeFacade.cpp'
require_contains CMakeLists.txt 'Threads::Threads'
require_contains tests/runtime/test_runtime_state_thread_safety.sh 'PASS runtime state isolation and thread-safety gate'
require_contains scripts/check_runtime_state_thread_safety.sh 'PASS runtime state isolation and thread-safety guard'

for anchor in \
  'runtime_packaging_contract_version: 1.0.0' \
  'runtime_packaging_status: installable_static_shared_and_consumer_checked' \
  'runtime_shared_soversion: 1' \
  'ai_bridge_packaging_status: adapter_static_shared_and_consumer_checked' \
  'production_claim_boundary: packaging_gate_is_not_full_production_readiness'; do
  require_contains docs/runtime_production_packaging.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'defined(short_runtime_reset)'
require_contains CMakeLists.txt 'add_library(shorthand_runtime_shared SHARED'
require_contains CMakeLists.txt 'CXX_VISIBILITY_PRESET hidden'
require_contains CMakeLists.txt 'add_library(shorthand_ai_bridge_shared SHARED'
require_contains CMakeLists.txt 'install(EXPORT ShortHandTargets'
require_contains CMakeLists.txt 'configure_package_config_file('
require_contains CMakeLists.txt 'write_basic_package_version_file('
require_contains tests/packaging/test_runtime_production_packaging.sh 'find_package(ShortHand 1 CONFIG REQUIRED)'
require_contains tests/packaging/test_runtime_production_packaging.sh 'PASS production runtime AI bridge core FFI and serving packaging consumer gate'
require_contains scripts/check_runtime_production_packaging.sh 'PASS runtime production packaging guard'

for anchor in \
  'prometheus_scrape_adapter_contract_version: 1.0.0' \
  'prometheus_scrape_adapter_status: loopback_default_bounded_http_metrics_host' \
  'prometheus_metrics_source: frozen_runtime_short_runtime_prometheus_metrics' \
  'runtime_abi_change: none' \
  'runtime_external_symbol_count: 25' \
  'production_claim_boundary: scrape_adapter_is_not_hardened_public_ingress'; do
  require_contains docs/prometheus_scrape_host_adapter.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'listen_address = "127.0.0.1"'
require_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'short_runtime_prometheus_metrics()'
require_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'PROMETHEUS_ADAPTER_LISTENING'
require_contains CMakeLists.txt 'add_executable(shorthand_prometheus_adapter'
require_contains CMakeLists.txt 'target_link_libraries(shorthand_prometheus_adapter PRIVATE shorthand_runtime)'
require_contains CMakeLists.txt 'install(TARGETS shorthand_prometheus_adapter'
require_contains CMakeLists.txt 'NAME prometheus_scrape_adapter'
require_contains tests/operations/test_prometheus_scrape_adapter.sh 'PASS Prometheus scrape endpoint host adapter gate'
require_contains scripts/check_prometheus_scrape_adapter.sh 'PASS Prometheus scrape endpoint host adapter guard'

require_contains docs/backend_compatibility_matrix.md 'Backend execution validation tiers'
require_contains docs/backend_compatibility_matrix.md 'backend_failure_mode_matrix_status: finalized_v1'
require_contains docs/backend_compatibility_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/backend_failure_mode_matrix.md 'shorthand.backend_failure_mode_matrix.v1'
require_contains docs/backend_failure_mode_matrix.md 'false_success_allowed: false'
require_contains tests/integration/test_backend_failure_mode_matrix.sh 'PASS backend failure-mode matrix gate'
require_contains scripts/check_backend_failure_mode_matrix.sh 'PASS backend failure-mode matrix guard'
require_contains docs/hardware_capability_routing.md 'production_claim_boundary: detection_is_not_execution_readiness'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'enum class DeviceClass { CPU, GPU, TPU, NPU, Unknown }'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.inventory.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.selection.v1'
require_contains scripts/check_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'

require_contains docs/compiled_infer_bridge.md 'Typed tensor-buffer bridge'
require_contains docs/ai_runtime_bridge_linkage.md 'runtime-hook ABI ownership'
require_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_execution_status: bridge_enabled_ai_runtime_infer_attempt'
require_contains docs/runtime_observability_exports.md 'runtime_observability_export_status: dependency_free_prometheus_and_otlp_like_exports'
require_contains scripts/check_ai_runtime_bridge_linkage.sh 'PASS AI runtime bridge linkage gate'
require_contains scripts/check_ai_runtime_execution_adapter.sh 'PASS AI runtime execution adapter gate'
require_contains scripts/check_runtime_ai_bridge_link_build.sh 'PASS runtime AI bridge link build gate'
require_contains scripts/check_runtime_ai_bridge_execution_path.sh 'PASS runtime AI bridge execution path gate'
require_contains scripts/check_runtime_observability_exports.sh 'PASS runtime observability export gate'

require_contains scripts/check_c3eco_claims_and_schema.sh 'PASS C3-ECO schema and claim-safety gate'
require_contains scripts/check_mlir_foundation.sh 'PASS MLIR foundation gate'
require_contains scripts/check_release_supply_chain.sh 'PASS release supply-chain gate'
require_contains scripts/generate_release_sbom.sh 'SPDX-2.3'

printf 'PASS PR task stability gate\n'
