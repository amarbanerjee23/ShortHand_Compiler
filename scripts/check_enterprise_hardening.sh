#!/usr/bin/env bash
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}" || exit 1

LOG_FILE="/tmp/shorthand_enterprise_hardening.out"
: > "${LOG_FILE}"
fail=0

log() {
  printf '%s\n' "$*" | tee -a "${LOG_FILE}"
}

fail_check() {
  log "error: $*"
  fail=1
}

require_file() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    fail_check "required file is missing: ${file}"
    return 1
  fi
  return 0
}

check_contains() {
  local file="$1"
  local needle="$2"
  require_file "$file" || return 0
  if grep -Fq "$needle" "$file"; then
    log "PASS contains: ${file} :: ${needle}"
  else
    fail_check "${file} missing required text: ${needle}"
  fi
}

check_not_contains() {
  local file="$1"
  local needle="$2"
  require_file "$file" || return 0
  if grep -Fq "$needle" "$file"; then
    fail_check "${file} contains forbidden text: ${needle}"
  else
    log "PASS not contains: ${file} :: ${needle}"
  fi
}

log "Enterprise hardening check started."
log "Repository: $(git rev-parse --show-toplevel 2>/dev/null || pwd)"
log "Commit: $(git rev-parse HEAD 2>/dev/null || echo unknown)"

tracked_metadata="$(git ls-files | awk '/(^|\/)\.metadata\// { print }')"
if [[ -n "${tracked_metadata}" ]]; then
  fail_check "tracked IDE metadata must not exist:"
  log "${tracked_metadata}"
else
  log "PASS no tracked IDE metadata files"
fi

tracked_python_tools="$(git ls-files | awk '/^deprecated\/python_tools\// { print }')"
if [[ -n "${tracked_python_tools}" ]]; then
  fail_check "deprecated Python tooling must not be tracked:"
  log "${tracked_python_tools}"
else
  log "PASS no tracked deprecated Python tooling"
fi

check_contains .gitignore 'Compiler_new_ws/.metadata/'
check_contains .gitignore 'deprecated/python_tools/'
check_contains Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp 'Ort::Session'
check_contains Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp 'session.Run'
check_contains Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp 'TelemetryTimer'
check_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Telemetry.cpp 'telemetryToOtlpLikeSpanJson'
check_contains CMakeLists.txt 'AI_Telemetry.cpp'
check_contains Compiler_new_ws/Short_Hand/src/Makefile 'AI_Telemetry.cpp'
check_contains tests/integration/test_onnxruntime_sdk_gate.sh 'identity_float32_v13.onnx.b64'
check_contains docs/backend_compatibility_matrix.md 'ONNX'
check_contains docs/backend_compatibility_matrix.md 'Backend execution validation tiers'
check_contains docs/backend_compatibility_matrix.md 'full_backend_matrix_claim: false'
check_contains scripts/check_backend_compatibility_matrix.sh 'PASS backend compatibility matrix gate'
check_contains docs/telemetry_schema.md 'OTLP'
check_contains scripts/generate_certification_bundle.sh 'candidate_report.json'
check_contains scripts/generate_certification_bundle.sh 'candidate_report.schema.json'
check_contains scripts/apply_external_runtime_to_ir_source.sh 'return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);'
check_contains Compiler_new_ws/Short_Hand/src/Makefile 'runtime-source-lowering'
check_contains CMakeLists.txt 'shorthand_runtime_source_lowering'
check_contains tests/codegen/test_external_runtime_native.sh 'PASS source-level external runtime native linking'
check_not_contains Compiler_new_ws/Short_Hand/src/Makefile 'IR_Generator.default_runtime.cpp'
check_not_contains CMakeLists.txt 'IR_Generator.default_runtime.cpp'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'SHORTHAND_RUNTIME_MODEL_NOT_FOUND'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_observability_json'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_infer_bridge_request_json'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_prometheus_metrics'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_otlp_spans_json'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_ai_infer_f32'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'std::map<std::string, ModelRecord> models'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.observability.v1'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.otlp_spans.v1'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand_runtime_infer_total'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_execution_bridge_pending'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.compiled_infer_bridge_request.v1'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.typed_infer_buffer_bridge_request.v1'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'execute_typed_buffer_through_ai_runtime'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'runtime.infer(model_spec, input_buffer)'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'ai_runtime_execution_attempted'
check_contains docs/compiled_infer_bridge.md 'input_buffer_required_for_ai_runtime_execution'
check_contains docs/compiled_infer_bridge.md 'Typed tensor-buffer bridge'
check_contains docs/compiled_infer_bridge.md 'AI_Runtime bridge linkage'
check_contains docs/compiled_infer_bridge.md 'AI_Runtime execution adapter'
check_contains docs/compiled_infer_bridge.md 'Runtime AI bridge link build'
check_contains docs/compiled_infer_bridge.md 'Runtime AI bridge execution path'
check_contains docs/runtime_observability_exports.md 'Prometheus-style metrics'
check_contains docs/runtime_observability_exports.md 'OTLP-like span JSON'
check_contains scripts/check_runtime_observability_exports.sh 'PASS runtime observability export gate'
check_contains tests/codegen/test_runtime_observability_exports.sh 'PASS runtime observability export gate'
check_contains docs/prometheus_scrape_host_adapter.md 'prometheus_scrape_adapter_status: loopback_default_bounded_http_metrics_host'
check_contains docs/prometheus_scrape_host_adapter.md 'production_claim_boundary: scrape_adapter_is_not_hardened_public_ingress'
check_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'short_runtime_prometheus_metrics()'
check_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'listen_address = "127.0.0.1"'
check_contains Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp 'Request Header Fields Too Large'
check_contains CMakeLists.txt 'add_executable(shorthand_prometheus_adapter'
check_contains CMakeLists.txt 'install(TARGETS shorthand_prometheus_adapter'
check_contains tests/operations/test_prometheus_scrape_adapter.sh 'PASS Prometheus scrape endpoint host adapter gate'
check_contains scripts/check_prometheus_scrape_adapter.sh 'PASS Prometheus scrape endpoint host adapter guard'
check_contains docs/ai_runtime_bridge_linkage.md 'runtime-hook ABI ownership'
check_contains scripts/check_ai_runtime_bridge_linkage.sh 'PASS AI runtime bridge linkage gate'
check_not_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'extern "C" int short_ai_infer'
check_not_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'extern "C" int short_greenai_emit_event'
check_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h 'RuntimeBridgeModelInput'
check_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.h 'RuntimeBridgeTensorInput'
check_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'shorthand.runtime.ai_runtime_execution_adapter.v1'
check_contains Compiler_new_ws/Short_Hand/src/runtime/AIRuntimeBridgeAdapter.cpp 'bridgeRequestIsExecutionReady'
check_contains docs/ai_runtime_execution_adapter.md 'adapter_contract_status: compile_checked_mapping_only'
check_contains docs/ai_runtime_execution_adapter.md 'bridge_link_status: runtime_adapter_ai_core_link_checked'
check_contains docs/ai_runtime_execution_adapter.md 'compiled_hook_execution_status: bridge_enabled_ai_runtime_infer_attempt'
check_contains scripts/check_ai_runtime_execution_adapter.sh 'PASS AI runtime execution adapter gate'
check_contains tests/codegen/test_ai_runtime_bridge_adapter.sh 'PASS AI runtime bridge adapter compile and mapping test'
check_contains scripts/check_runtime_ai_bridge_link_build.sh 'PASS runtime AI bridge link build gate'
check_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'AIRuntime runtime'
check_contains tests/codegen/test_runtime_ai_bridge_link_build.sh 'runtime.infer(model_spec, input_buffer)'
check_contains scripts/check_runtime_ai_bridge_execution_path.sh 'PASS runtime AI bridge execution path gate'
check_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1'
check_contains tests/codegen/test_runtime_ai_bridge_execution_path.sh 'ai_runtime_execution_attempted'
check_contains tests/codegen/test_runtime_library_build.sh 'SHORTHAND_RUNTIME_NOT_EXECUTED'
check_contains tests/codegen/test_runtime_library_build.sh 'shorthand.runtime.observability.v1'
check_contains tests/codegen/test_runtime_library_build.sh 'compiled-infer bridge request'
check_contains tests/codegen/test_runtime_library_build.sh 'typed buffer bridge'
check_contains Compiler_new_ws/Short_Hand/src/semantic_ir/SemanticIR.h 'struct ProgramIR'
check_contains docs/language_grammar_ebnf.md 'infer_statement'
check_contains docs/semantic_ir_and_diagnostics_plan.md 'ShortHand semantic IR'
check_contains tests/conformance/manifest.txt 'semantic-invalid'
check_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-conformance'
check_contains schemas/c3eco/candidate_report.schema.json 'shorthand.c3eco.candidate_report.v1'
check_contains schemas/c3eco/candidate_check.schema.json 'shorthand.c3eco.check.v1'
check_contains schemas/c3eco/bundle_manifest.schema.json 'shorthand.c3eco.bundle_manifest.v1'
check_contains docs/c3eco_schema_and_claim_safety.md 'claim-safety gate'
check_contains scripts/check_c3eco_claims_and_schema.sh 'PASS C3-ECO schema and claim-safety gate'
check_contains mlir/include/ShortHand/IR/ShortHandDialect.td 'def ShortHand_Dialect : Dialect'
check_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_InferOp'
check_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.infer"'
check_contains docs/mlir_lowering_plan.md 'ShortHand MLIR dialect'
check_contains scripts/check_mlir_foundation.sh 'PASS MLIR foundation gate'
check_contains docs/pr_task_stability_strategy.md 'Old-task contract'
check_contains scripts/check_pr_task_stability.sh 'PASS PR task stability gate'
check_contains scripts/generate_release_sbom.sh 'SPDX-2.3'
check_contains scripts/check_release_supply_chain.sh 'PASS release supply-chain gate'
check_contains schemas/release/release_provenance.schema.json 'shorthand.release.provenance.v1'
check_contains docs/release_supply_chain_hardening.md 'candidate release evidence'
check_contains docs/production_truth.tsv $'current_github_pr\t83'
check_contains docs/c3eco_traceability.tsv $'G14\tmandatory_gate\tmateriality_control'
check_contains scripts/check_production_truth.sh 'PASS production truth and C3-ECO traceability gate'
check_contains tests/governance/test_production_truth_negative.sh 'PASS production truth negative contradiction, completeness, mapping and evidence cases'

if bash scripts/check_production_truth.sh >/tmp/shorthand_production_truth.out 2>&1 \
   && bash tests/governance/test_production_truth_negative.sh >>/tmp/shorthand_production_truth.out 2>&1; then
  log "PASS production truth and C3-ECO traceability gate completed"
  cat /tmp/shorthand_production_truth.out | tee -a "${LOG_FILE}"
else
  fail_check "production truth and C3-ECO traceability gate failed"
  cat /tmp/shorthand_production_truth.out | tee -a "${LOG_FILE}" || true
fi

if bash tests/integration/test_onnxruntime_sdk_gate.sh >/tmp/shorthand_onnx_sdk_gate.out 2>&1; then
  log "PASS ONNX Runtime SDK gate command completed"
  cat /tmp/shorthand_onnx_sdk_gate.out | tee -a "${LOG_FILE}"
else
  fail_check "ONNX Runtime SDK gate failed"
  cat /tmp/shorthand_onnx_sdk_gate.out | tee -a "${LOG_FILE}" || true
fi

if bash tests/codegen/test_external_runtime_native.sh >/tmp/shorthand_external_runtime_native.out 2>&1; then
  log "PASS source-level external runtime native linking gate completed"
  cat /tmp/shorthand_external_runtime_native.out | tee -a "${LOG_FILE}"
else
  fail_check "source-level external runtime native linking gate failed"
  cat /tmp/shorthand_external_runtime_native.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_language_correctness.sh >/tmp/shorthand_language_correctness.out 2>&1; then
  log "PASS language correctness gate completed"
  cat /tmp/shorthand_language_correctness.out | tee -a "${LOG_FILE}"
else
  fail_check "language correctness gate failed"
  cat /tmp/shorthand_language_correctness.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_c3eco_claims_and_schema.sh >/tmp/shorthand_c3eco_claims_schema.out 2>&1; then
  log "PASS C3-ECO schema and claim-safety gate completed"
  cat /tmp/shorthand_c3eco_claims_schema.out | tee -a "${LOG_FILE}"
else
  fail_check "C3-ECO schema and claim-safety gate failed"
  cat /tmp/shorthand_c3eco_claims_schema.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_mlir_foundation.sh >/tmp/shorthand_mlir_foundation.out 2>&1; then
  log "PASS MLIR foundation gate completed"
  cat /tmp/shorthand_mlir_foundation.out | tee -a "${LOG_FILE}"
else
  fail_check "MLIR foundation gate failed"
  cat /tmp/shorthand_mlir_foundation.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_pr_task_stability.sh >/tmp/shorthand_pr_task_stability.out 2>&1; then
  log "PASS PR task stability gate completed"
  cat /tmp/shorthand_pr_task_stability.out | tee -a "${LOG_FILE}"
else
  fail_check "PR task stability gate failed"
  cat /tmp/shorthand_pr_task_stability.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_release_supply_chain.sh >/tmp/shorthand_release_supply_chain.out 2>&1; then
  log "PASS release supply-chain gate completed"
  cat /tmp/shorthand_release_supply_chain.out | tee -a "${LOG_FILE}"
else
  fail_check "release supply-chain gate failed"
  cat /tmp/shorthand_release_supply_chain.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_backend_compatibility_matrix.sh >/tmp/shorthand_backend_matrix.out 2>&1; then
  log "PASS backend compatibility matrix gate completed"
  cat /tmp/shorthand_backend_matrix.out | tee -a "${LOG_FILE}"
else
  fail_check "backend compatibility matrix gate failed"
  cat /tmp/shorthand_backend_matrix.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_ai_runtime_bridge_linkage.sh >/tmp/shorthand_ai_runtime_bridge_linkage.out 2>&1; then
  log "PASS AI runtime bridge linkage gate completed"
  cat /tmp/shorthand_ai_runtime_bridge_linkage.out | tee -a "${LOG_FILE}"
else
  fail_check "AI runtime bridge linkage gate failed"
  cat /tmp/shorthand_ai_runtime_bridge_linkage.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_ai_runtime_execution_adapter.sh >/tmp/shorthand_ai_runtime_execution_adapter.out 2>&1; then
  log "PASS AI runtime execution adapter gate completed"
  cat /tmp/shorthand_ai_runtime_execution_adapter.out | tee -a "${LOG_FILE}"
else
  fail_check "AI runtime execution adapter gate failed"
  cat /tmp/shorthand_ai_runtime_execution_adapter.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_runtime_ai_bridge_link_build.sh >/tmp/shorthand_runtime_ai_bridge_link_build.out 2>&1; then
  log "PASS runtime AI bridge link build gate completed"
  cat /tmp/shorthand_runtime_ai_bridge_link_build.out | tee -a "${LOG_FILE}"
else
  fail_check "runtime AI bridge link build gate failed"
  cat /tmp/shorthand_runtime_ai_bridge_link_build.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_runtime_ai_bridge_execution_path.sh >/tmp/shorthand_runtime_ai_bridge_execution_path.out 2>&1; then
  log "PASS runtime AI bridge execution path gate completed"
  cat /tmp/shorthand_runtime_ai_bridge_execution_path.out | tee -a "${LOG_FILE}"
else
  fail_check "runtime AI bridge execution path gate failed"
  cat /tmp/shorthand_runtime_ai_bridge_execution_path.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_runtime_observability_exports.sh >/tmp/shorthand_runtime_observability_exports.out 2>&1; then
  log "PASS runtime observability export gate completed"
  cat /tmp/shorthand_runtime_observability_exports.out | tee -a "${LOG_FILE}"
else
  fail_check "runtime observability export gate failed"
  cat /tmp/shorthand_runtime_observability_exports.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_prometheus_scrape_adapter.sh >/tmp/shorthand_prometheus_scrape_adapter.out 2>&1; then
  log "PASS Prometheus scrape endpoint host adapter gate completed"
  cat /tmp/shorthand_prometheus_scrape_adapter.out | tee -a "${LOG_FILE}"
else
  fail_check "Prometheus scrape endpoint host adapter gate failed"
  cat /tmp/shorthand_prometheus_scrape_adapter.out | tee -a "${LOG_FILE}" || true
fi

if [[ "${fail}" -ne 0 ]]; then
  log "Enterprise hardening check failed."
  exit 1
fi

log "Enterprise hardening check passed."
