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
check_contains docs/telemetry_schema.md 'OTLP'
check_contains scripts/generate_certification_bundle.sh 'candidate_report.json'
check_contains scripts/generate_external_runtime_ir_generator.sh 'return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);'
check_contains tests/codegen/test_external_runtime_native.sh 'PASS default external runtime native linking'
check_contains Compiler_new_ws/Short_Hand/src/Makefile 'IR_Generator.default_runtime.cpp'
check_contains CMakeLists.txt 'IR_Generator.default_runtime.cpp'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'SHORTHAND_RUNTIME_MODEL_NOT_FOUND'
check_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'std::map<std::string, ModelRecord> models'
check_contains tests/codegen/test_runtime_library_build.sh 'SHORTHAND_RUNTIME_NOT_EXECUTED'
check_contains Compiler_new_ws/Short_Hand/src/semantic_ir/SemanticIR.h 'struct ProgramIR'
check_contains docs/language_grammar_ebnf.md 'infer_statement'
check_contains docs/semantic_ir_and_diagnostics_plan.md 'ShortHand semantic IR'
check_contains tests/conformance/manifest.txt 'semantic-invalid'
check_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-conformance'

if bash tests/integration/test_onnxruntime_sdk_gate.sh >/tmp/shorthand_onnx_sdk_gate.out 2>&1; then
  log "PASS ONNX Runtime SDK gate command completed"
  cat /tmp/shorthand_onnx_sdk_gate.out | tee -a "${LOG_FILE}"
else
  fail_check "ONNX Runtime SDK gate failed"
  cat /tmp/shorthand_onnx_sdk_gate.out | tee -a "${LOG_FILE}" || true
fi

if bash tests/codegen/test_external_runtime_native.sh >/tmp/shorthand_external_runtime_native.out 2>&1; then
  log "PASS default external runtime native linking gate completed"
  cat /tmp/shorthand_external_runtime_native.out | tee -a "${LOG_FILE}"
else
  fail_check "default external runtime native linking gate failed"
  cat /tmp/shorthand_external_runtime_native.out | tee -a "${LOG_FILE}" || true
fi

if bash scripts/check_language_correctness.sh >/tmp/shorthand_language_correctness.out 2>&1; then
  log "PASS language correctness gate completed"
  cat /tmp/shorthand_language_correctness.out | tee -a "${LOG_FILE}"
else
  fail_check "language correctness gate failed"
  cat /tmp/shorthand_language_correctness.out | tee -a "${LOG_FILE}" || true
fi

if [[ "${fail}" -ne 0 ]]; then
  log "Enterprise hardening check failed."
  exit 1
fi

log "Enterprise hardening check passed."
