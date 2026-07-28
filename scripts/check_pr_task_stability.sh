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
require_file docs/backend_live_sdk_matrix.md
require_file docs/hardware_capability_routing.md
require_file docs/tensorrt_optional_fixture.md
require_file docs/openvino_optional_fixture.md
require_file docs/libtorch_optional_fixture.md
require_file docs/ai_runtime_bridge_linkage.md
require_file docs/ai_runtime_execution_adapter.md
require_file docs/runtime_observability_exports.md
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h
require_file tests/conformance/manifest.txt
require_file tests/codegen/test_runtime_ai_bridge_link_build.sh
require_file tests/codegen/test_runtime_ai_bridge_execution_path.sh
require_file tests/codegen/test_runtime_observability_exports.sh
require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file tests/integration/test_hardware_capability_routing.sh
require_file tests/integration/test_tensorrt_optional_fixture.sh
require_file tests/integration/test_openvino_optional_fixture.sh
require_file tests/integration/test_libtorch_optional_fixture.sh
require_file scripts/check_compiled_hook_onnxruntime_success.sh
require_file scripts/check_backend_live_sdk_matrix.sh
require_file scripts/check_hardware_capability_routing.sh
require_file scripts/check_tensorrt_optional_fixture.sh
require_file scripts/check_openvino_optional_fixture.sh
require_file scripts/check_libtorch_optional_fixture.sh
require_file scripts/check_runtime_observability_exports.sh
require_file scripts/check_language_versioning.sh
require_file scripts/check_production_readiness_pr_plan.sh

# Preserve CI task names.
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

# Preserve feature tracker anchors.
require_contains docs/feature_implementation_status.md 'Automated SBOM'
require_contains docs/feature_implementation_status.md 'Runtime observability implementation'
require_contains docs/feature_implementation_status.md 'Module/import/package model'
require_contains docs/feature_implementation_status.md 'Production blockers'
require_contains docs/feature_implementation_status.md 'Compiled-code metadata/runtime lowering'

# Preserve enterprise hardening coverage.
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
require_contains scripts/check_backend_compatibility_matrix.sh 'check_hardware_capability_routing.sh'

for script in scripts/check_feature_plan_status.sh scripts/check_enterprise_hardening.sh scripts/check_pr_task_stability.sh scripts/check_language_correctness.sh scripts/check_language_versioning.sh scripts/check_production_readiness_pr_plan.sh scripts/check_c3eco_claims_and_schema.sh scripts/check_mlir_foundation.sh scripts/check_release_supply_chain.sh scripts/check_backend_compatibility_matrix.sh scripts/check_backend_live_sdk_matrix.sh scripts/check_hardware_capability_routing.sh scripts/check_tensorrt_optional_fixture.sh scripts/check_openvino_optional_fixture.sh scripts/check_libtorch_optional_fixture.sh scripts/check_ai_runtime_bridge_linkage.sh scripts/check_ai_runtime_execution_adapter.sh scripts/check_runtime_ai_bridge_link_build.sh scripts/check_runtime_ai_bridge_execution_path.sh scripts/check_compiled_hook_onnxruntime_success.sh scripts/check_runtime_observability_exports.sh scripts/generate_release_sbom.sh; do
  require_bash_syntax "${script}"
done

require_contains docs/pr_task_stability_strategy.md 'Old-task contract'
require_contains docs/pr_task_stability_strategy.md 'New-gate contract'

# Production roadmap anchors.
require_contains docs/production_readiness_pr_plan.md 'production_readiness_plan_version: 2026-07-28-pr56'
require_contains docs/production_readiness_pr_plan.md 'LAST_COMPLETED_PR: 56'
require_contains docs/production_readiness_pr_plan.md 'Recommended path from PR #51 onward: 29 PRs total.'
require_contains docs/production_readiness_pr_plan.md 'PR51 - Production readiness plan and tracking contract | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR52 - Backend live SDK matrix harness | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR53 - TensorRT optional live execution fixture | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR54 - OpenVINO optional live execution fixture | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR55 - LibTorch optional live execution fixture | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR56 - Hardware capability discovery and accelerator-aware routing | MERGED'
require_contains docs/production_readiness_pr_plan.md 'PR57 - Llama.cpp optional live execution fixture.'
require_contains docs/production_readiness_pr_plan.md 'remaining_planned_prs_after_pr56: 23'
require_contains docs/production_readiness_pr_plan.md 'PR59 - Runtime ABI and API version stability gate'
require_contains docs/production_readiness_pr_plan.md 'PR60 - Runtime state isolation and thread-safety policy'
require_contains docs/production_readiness_pr_plan.md 'PR67 - Parser robustness and negative corpus hardening'
require_contains docs/production_readiness_pr_plan.md 'PR79 - MLIR lowering passes and production RC gate'
require_contains scripts/check_production_readiness_pr_plan.sh 'PASS production readiness PR plan gate'

# Language contract anchors.
require_contains docs/language_grammar_ebnf.md 'Language version: beta-0.1'
require_contains docs/language_spec.md 'Language version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.language.version: beta-0.1'
require_contains docs/language_versioning_and_conformance.md 'shorthand.conformance.contract: beta-0.1'
require_contains tests/conformance/manifest.txt 'version | shorthand.language.version | beta-0.1 | Current beta language contract marker.'
require_contains scripts/check_language_versioning.sh 'PASS language versioning and conformance gate'
require_contains scripts/check_language_correctness.sh 'check_language_versioning.sh'

# Runtime bridge and backend anchors.
require_contains docs/compiled_infer_bridge.md 'input_buffer_required_for_ai_runtime_execution'
require_contains docs/compiled_infer_bridge.md 'Typed tensor-buffer bridge'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime bridge linkage'
require_contains docs/compiled_infer_bridge.md 'AI_Runtime execution adapter'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge link build'
require_contains docs/compiled_infer_bridge.md 'Runtime AI bridge execution path'
require_contains docs/compiled_infer_bridge.md 'Compiled hook ONNX Runtime success fixture'
require_contains docs/backend_compatibility_matrix.md 'Backend execution validation tiers'
require_contains docs/backend_compatibility_matrix.md 'backend_live_sdk_matrix_status: optional_matrix_harness'
require_contains docs/backend_compatibility_matrix.md 'hardware_capability_routing_status: inventory_and_execution_ready_selection'
require_contains docs/backend_compatibility_matrix.md 'trt_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'openvino_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'libtorch_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'Hardware capability discovery boundary'
require_contains docs/backend_compatibility_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/tensorrt_optional_fixture.md 'trt_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/openvino_optional_fixture.md 'openvino_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/libtorch_optional_fixture.md 'libtorch_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains scripts/check_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix gate'
require_contains scripts/check_tensorrt_optional_fixture.sh 'PASS TensorRT optional fixture gate'
require_contains scripts/check_openvino_optional_fixture.sh 'PASS OpenVINO optional fixture gate'
require_contains scripts/check_libtorch_optional_fixture.sh 'PASS LibTorch optional fixture gate'

# Hardware discovery and routing anchors.
require_contains docs/hardware_capability_routing.md 'hardware_capability_routing_status: inventory_and_execution_ready_selection'
require_contains docs/hardware_capability_routing.md 'production_claim_boundary: detection_is_not_execution_readiness'
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
require_contains tests/integration/test_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'
require_contains tests/integration/test_hardware_capability_routing.sh 'selected=gpu backend=onnxruntime_cuda status=execution_ready'

# Existing linkage, observability and success-fixture anchors.
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
