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
  scripts/check_production_readiness_pr_plan.sh
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
  docs/language_grammar_ebnf.md
  docs/language_spec.md
  docs/language_versioning_and_conformance.md
  docs/language_objectives.md
  docs/runtime_abi_api_stability.md
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
  Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h
  Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h
  tests/conformance/manifest.txt
  tests/abi/test_runtime_abi_api_stability.sh
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
)
for file in "${required_files[@]}"; do require_file "${file}"; done

# Preserve CI task names.
for task in \
  'Strict language validation' \
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

# Preserve feature tracking and enterprise hardening coverage.
for anchor in \
  'Automated SBOM' \
  'Runtime observability implementation' \
  'Module/import/package model' \
  'Production blockers' \
  'Compiled-code metadata/runtime lowering'; do
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
  check_runtime_observability_exports.sh; do
  require_contains scripts/check_enterprise_hardening.sh "${gate}"
done
require_contains scripts/check_enterprise_hardening.sh 'shorthand.runtime.compiled_infer_bridge_request.v1'
require_contains scripts/check_enterprise_hardening.sh 'shorthand.runtime.typed_infer_buffer_bridge_request.v1'
require_contains scripts/check_backend_compatibility_matrix.sh 'check_hardware_capability_routing.sh'
require_contains scripts/check_backend_compatibility_matrix.sh 'check_backend_failure_mode_matrix.sh'
require_contains scripts/check_language_correctness.sh 'check_runtime_abi_api_stability.sh'

# Keep every shell guard syntactically valid.
for script in \
  scripts/check_feature_plan_status.sh \
  scripts/check_enterprise_hardening.sh \
  scripts/check_pr_task_stability.sh \
  scripts/check_language_correctness.sh \
  scripts/check_language_versioning.sh \
  scripts/check_language_objectives.sh \
  scripts/check_runtime_abi_api_stability.sh \
  scripts/check_production_readiness_pr_plan.sh \
  scripts/check_c3eco_claims_and_schema.sh \
  scripts/check_mlir_foundation.sh \
  scripts/check_release_supply_chain.sh \
  scripts/check_backend_compatibility_matrix.sh \
  scripts/check_backend_live_sdk_matrix.sh \
  scripts/check_backend_failure_mode_matrix.sh \
  scripts/check_hardware_capability_routing.sh \
  scripts/check_tensorrt_optional_fixture.sh \
  scripts/check_openvino_optional_fixture.sh \
  scripts/check_libtorch_optional_fixture.sh \
  scripts/check_llamacpp_optional_fixture.sh \
  scripts/check_ai_runtime_bridge_linkage.sh \
  scripts/check_ai_runtime_execution_adapter.sh \
  scripts/check_runtime_ai_bridge_link_build.sh \
  scripts/check_runtime_ai_bridge_execution_path.sh \
  scripts/check_compiled_hook_onnxruntime_success.sh \
  scripts/check_runtime_observability_exports.sh \
  scripts/generate_release_sbom.sh \
  tests/abi/test_runtime_abi_api_stability.sh \
  tests/integration/test_backend_failure_mode_matrix.sh; do
  require_bash_syntax "${script}"
done

require_contains docs/pr_task_stability_strategy.md 'Old-task contract'
require_contains docs/pr_task_stability_strategy.md 'New-gate contract'

# Production roadmap anchors.
for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr59' \
  'LAST_COMPLETED_PR: 59' \
  'Language objectives consolidation applied in PR #57' \
  'Backend failure-mode finalization applied in PR #58' \
  'Runtime ABI and API stability applied in PR #59' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'PR51 - Production readiness plan and tracking contract | MERGED' \
  'PR52 - Backend live SDK matrix harness | MERGED' \
  'PR53 - TensorRT optional live execution fixture | MERGED' \
  'PR54 - OpenVINO optional live execution fixture | MERGED' \
  'PR55 - LibTorch optional live execution fixture | MERGED' \
  'PR56 - Hardware capability discovery and accelerator-aware routing | MERGED' \
  'PR57 - Llama.cpp optional live execution fixture | MERGED' \
  'PR58 - Backend failure-mode matrix finalization | MERGED' \
  'PR59 - Runtime ABI and API version stability gate | MERGED' \
  'Next recommended PR after PR #59:' \
  'PR60 - Runtime state isolation and thread-safety policy.' \
  'remaining_planned_prs_after_pr59: 20' \
  'PR61 - Production build packaging for runtime and AI bridge' \
  'PR67 - Parser robustness and negative corpus hardening' \
  'PR79 - MLIR lowering passes and production RC gate'; do
  require_contains docs/production_readiness_pr_plan.md "${anchor}"
done
require_contains scripts/check_production_readiness_pr_plan.sh 'PASS production readiness PR plan gate'

# Language contract and objective anchors.
require_contains docs/language_grammar_ebnf.md 'Language version: beta-0.1'
require_contains docs/language_spec.md 'Language version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.language.version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.conformance.contract: beta-0.1'
for anchor in \
  'shorthand.language.objectives.version: 2026-07-29-v1' \
  'production_claim: false' \
  'Honest execution and fallback' \
  'First-class Green AI evidence' \
  '## Explicit non-goals'; do
  require_contains docs/language_objectives.md "${anchor}"
done
require_contains tests/conformance/manifest.txt 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'
require_contains scripts/check_language_versioning.sh 'PASS language versioning and conformance gate'
require_contains scripts/check_language_objectives.sh 'PASS language objectives gate'
require_contains scripts/check_language_correctness.sh 'check_language_objectives.sh'
require_contains scripts/check_language_correctness.sh 'check_language_versioning.sh'

# Runtime ABI and API anchors.
for anchor in \
  'runtime_abi_contract_status: frozen_v1_symbol_manifest' \
  'runtime_abi_version: 1.0.0' \
  'runtime_api_version: 1.0.0' \
  'runtime_external_symbol_count: 25' \
  'production_claim_boundary: abi_stability_gate_is_not_full_production_readiness' \
  'No ABI v1 symbol is deprecated in this release.' \
  'ABI v1 does not claim that global runtime state is thread-safe'; do
  require_contains docs/runtime_abi_api_stability.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h '#define SHORTHAND_RUNTIME_ABI_VERSION_STRING "1.0.0"'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h '#define SHORTHAND_RUNTIME_API_VERSION_STRING "1.0.0"'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_is_abi_compatible'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'SHORTHAND_RUNTIME_API int short_runtime_reset(void);'
require_contains abi/shorthand_runtime_abi_v1.h 'Frozen consumer snapshot for ShortHand runtime ABI 1.0.0.'
require_contains abi/shorthand_runtime_abi_v1.h 'SHORTHAND_RUNTIME_RUNTIME_ERROR = 8'
require_contains abi/runtime_public_symbols_v1.txt 'short_runtime_reset'
require_contains abi/runtime_public_symbols_v1.txt 'short_ai_infer_f32'
require_contains tests/abi/test_runtime_abi_api_stability.sh 'ABI_SYMBOL_COUNT'
require_contains tests/abi/test_runtime_abi_api_stability.sh 'PASS frozen runtime ABI v1 consumer'
require_contains scripts/check_runtime_abi_api_stability.sh 'PASS runtime ABI and API stability gate'

# Runtime bridge, backend, failure-mode, and hardware anchors.
for anchor in \
  'input_buffer_required_for_ai_runtime_execution' \
  'Typed tensor-buffer bridge' \
  'AI_Runtime bridge linkage' \
  'AI_Runtime execution adapter' \
  'Runtime AI bridge link build' \
  'Runtime AI bridge execution path' \
  'Compiled hook ONNX Runtime success fixture'; do
  require_contains docs/compiled_infer_bridge.md "${anchor}"
done

for anchor in \
  'Backend execution validation tiers' \
  'backend_live_sdk_matrix_status: optional_matrix_harness' \
  'backend_failure_mode_matrix_status: finalized_v1' \
  'hardware_capability_routing_status: inventory_and_execution_ready_selection' \
  'trt_optional_fixture_status: unavailable_path_proof_no_false_success' \
  'openvino_optional_fixture_status: unavailable_path_proof_no_false_success' \
  'libtorch_optional_fixture_status: unavailable_path_proof_no_false_success' \
  'llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success' \
  'Finalized backend failure-mode matrix' \
  'full_backend_matrix_claim: false'; do
  require_contains docs/backend_compatibility_matrix.md "${anchor}"
done
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/backend_live_sdk_matrix.md 'Llama.cpp unavailable-path proof'
require_contains docs/backend_failure_mode_matrix.md 'shorthand.backend_failure_mode_matrix.v1'
require_contains docs/backend_failure_mode_matrix.md 'false_success_allowed: false'
require_contains docs/backend_failure_mode_matrix.md 'production_claim_boundary: failure_evidence_is_not_backend_success_evidence'
require_contains tests/integration/test_backend_failure_mode_matrix.sh 'PASS backend failure-mode matrix gate'
require_contains tests/integration/test_backend_failure_mode_matrix.sh 'fallback_honesty'
require_contains scripts/check_backend_failure_mode_matrix.sh 'PASS backend failure-mode matrix guard'
require_contains scripts/check_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix gate'
require_contains scripts/check_tensorrt_optional_fixture.sh 'PASS TensorRT optional fixture gate'
require_contains scripts/check_openvino_optional_fixture.sh 'PASS OpenVINO optional fixture gate'
require_contains scripts/check_libtorch_optional_fixture.sh 'PASS LibTorch optional fixture gate'
require_contains scripts/check_llamacpp_optional_fixture.sh 'PASS Llama.cpp optional fixture gate'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'llamacpp_unavailable_path_proved_no_false_success'

for anchor in \
  'hardware_capability_routing_status: inventory_and_execution_ready_selection' \
  'production_claim_boundary: detection_is_not_execution_readiness'; do
  require_contains docs/hardware_capability_routing.md "${anchor}"
done
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'enum class DeviceClass { CPU, GPU, TPU, NPU, Unknown }'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'class SystemHardwareProbe'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'class StaticHardwareProbe'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.inventory.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.selection.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'selectHardwareRoute'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'shorthand.ai_runtime.telemetry.v2'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h 'hardware_inventory_json'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h 'selected_device_class'
require_contains scripts/check_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'
require_contains tests/integration/test_hardware_capability_routing.sh 'selected=gpu backend=onnxruntime_cuda status=execution_ready'

# Existing linkage, observability, and success-fixture anchors.
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
