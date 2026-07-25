#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
SHORT="${BUILD_DIR}/short_hand"
MANIFEST="${ROOT_DIR}/tests/conformance/manifest.txt"
WORK_DIR="$(mktemp -d)"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${BUILD_DIR}/libshorthand_runtime.a}"
if [[ "${RUNTIME_LIB}" != /* ]]; then
  RUNTIME_LIB="${SRC_DIR}/${RUNTIME_LIB}"
fi
RUNTIME_LIB="$(cd "$(dirname "${RUNTIME_LIB}")" 2>/dev/null && pwd)/$(basename "${RUNTIME_LIB}")"
trap 'rm -rf "${WORK_DIR}"' EXIT

pass=0
fail=0

log_pass() {
  printf 'PASS %s\n' "$*"
  pass=$((pass + 1))
}

log_fail() {
  printf 'FAIL %s\n' "$*" >&2
  fail=$((fail + 1))
}

require_file() {
  local file="$1"
  if [[ -f "${file}" ]]; then
    log_pass "file exists: ${file#${ROOT_DIR}/}"
  else
    log_fail "missing required file: ${file#${ROOT_DIR}/}"
  fi
}

require_contains() {
  local file="$1"
  local text="$2"
  if grep -Fq "${text}" "${file}"; then
    log_pass "${file#${ROOT_DIR}/} contains ${text}"
  else
    log_fail "${file#${ROOT_DIR}/} missing ${text}"
  fi
}

ensure_compiler() {
  if [[ ! -x "${SHORT}" ]]; then
    make -C "${SRC_DIR}" short_hand >/tmp/shorthand_language_correctness_make.out 2>&1 || {
      cat /tmp/shorthand_language_correctness_make.out >&2 || true
      log_fail "compiler build failed"
      return 1
    }
  fi
  log_pass "compiler available"
}

ensure_runtime_lib() {
  if [[ ! -f "${RUNTIME_LIB}" ]]; then
    make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_language_correctness_runtime_make.out 2>&1 || {
      cat /tmp/shorthand_language_correctness_runtime_make.out >&2 || true
      log_fail "runtime library build failed"
      return 1
    }
  fi
  if [[ -f "${RUNTIME_LIB}" ]]; then
    log_pass "runtime library available: ${RUNTIME_LIB}"
  else
    log_fail "runtime library missing after build: ${RUNTIME_LIB}"
    return 1
  fi
}

compile_semantic_ir_probe() {
  cat > "${WORK_DIR}/semantic_ir_probe.cpp" <<'CPP'
#include "semantic_ir/SemanticIR.h"

int main() {
    shorthand::semantic_ir::TensorShape shape{{1, 4}};
    shorthand::semantic_ir::TensorOp input("input", shorthand::semantic_ir::ElementType::Float32, shape);
    shorthand::semantic_ir::ModelOp model("classifier");
    shorthand::semantic_ir::InferOp infer("classifier", "input", "output");
    shorthand::semantic_ir::ProgramIR program;
    program.tensors.push_back(input);
    program.models.push_back(model);
    program.inferences.push_back(infer);
    return program.empty() ? 1 : 0;
}
CPP
  ${CXX:-g++} -std=c++17 -I"${SRC_DIR}" "${WORK_DIR}/semantic_ir_probe.cpp" -o "${WORK_DIR}/semantic_ir_probe" \
    && "${WORK_DIR}/semantic_ir_probe" \
    && log_pass "semantic IR header compiles and runs" \
    || log_fail "semantic IR header probe failed"
}

run_valid_compile() {
  local rel="$1"
  local file="${ROOT_DIR}/${rel}"
  require_file "${file}"
  (cd "${WORK_DIR}" && "${SHORT}" "${file}" compile >/tmp/shorthand_conformance_valid.out 2>&1) \
    && log_pass "valid fixture accepted: ${rel}" \
    || { cat /tmp/shorthand_conformance_valid.out >&2 || true; log_fail "valid fixture rejected: ${rel}"; }
}

run_invalid_reject() {
  local rel="$1"
  local file="${ROOT_DIR}/${rel}"
  require_file "${file}"
  if "${SHORT}" "${file}" run >/tmp/shorthand_conformance_invalid.out 2>&1; then
    cat /tmp/shorthand_conformance_invalid.out >&2 || true
    log_fail "invalid fixture unexpectedly accepted: ${rel}"
  else
    log_pass "invalid fixture rejected: ${rel}"
  fi
}

run_existing_gate() {
  local name="$1"
  shift
  if "$@" >/tmp/shorthand_conformance_gate.out 2>&1; then
    log_pass "gate passed: ${name}"
  else
    cat /tmp/shorthand_conformance_gate.out >&2 || true
    log_fail "gate failed: ${name}"
  fi
}

require_file "${MANIFEST}"
require_file "${ROOT_DIR}/docs/language_grammar_ebnf.md"
require_file "${ROOT_DIR}/docs/language_spec.md"
require_file "${ROOT_DIR}/docs/language_versioning_and_conformance.md"
require_file "${ROOT_DIR}/docs/semantic_ir_and_diagnostics_plan.md"
require_file "${ROOT_DIR}/scripts/check_language_versioning.sh"
require_file "${SRC_DIR}/semantic_ir/SemanticIR.h"

require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'Language version: beta-0.1'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'model_declaration'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'tensor_declaration'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'infer_statement'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'greenai_contract'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'greenai_measurement'
require_contains "${ROOT_DIR}/docs/language_spec.md" 'Language version: beta-0.1'
require_contains "${ROOT_DIR}/docs/language_versioning_and_conformance.md" 'shorthand.conformance.contract: beta-0.1'
require_contains "${ROOT_DIR}/docs/semantic_ir_and_diagnostics_plan.md" 'ShortHand semantic IR'
require_contains "${SRC_DIR}/semantic_ir/SemanticIR.h" 'struct ProgramIR'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.1'
require_contains "${MANIFEST}" 'parser-valid'
require_contains "${MANIFEST}" 'semantic-invalid'
require_contains "${MANIFEST}" 'runtime | tests/codegen/test_external_runtime_native.sh'

run_existing_gate 'language versioning and conformance' bash "${ROOT_DIR}/scripts/check_language_versioning.sh"
compile_semantic_ir_probe
ensure_compiler
ensure_runtime_lib

run_valid_compile 'tests/fixtures/external_runtime_ai.short'
run_valid_compile 'Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short'
run_valid_compile 'Compiler_new_ws/Short_Hand/examples/greenai_report.short'

run_invalid_reject 'tests/parser/invalid/missing_semicolon.short'
run_invalid_reject 'tests/semantic/invalid/ai_shape_mismatch.short'
run_invalid_reject 'tests/semantic/invalid/ai_backend_mismatch.short'
run_invalid_reject 'tests/semantic/invalid/ai_output_shape_mismatch.short'

run_existing_gate 'source diagnostics' env SHORTHAND_BIN="${SHORT}" bash "${ROOT_DIR}/tests/diagnostics/test_source_diagnostics.sh"
run_existing_gate 'AI metadata IR' env SHORTHAND_BIN="${SHORT}" bash "${ROOT_DIR}/tests/codegen/test_ai_metadata_ir.sh"
run_existing_gate 'AI evidence backend fields' env SHORTHAND_BIN="${SHORT}" bash "${ROOT_DIR}/tests/evidence/test_ai_evidence.sh"
run_existing_gate 'external runtime native linking' env SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" bash "${ROOT_DIR}/tests/codegen/test_external_runtime_native.sh"

printf 'Language correctness summary: pass=%d fail=%d\n' "${pass}" "${fail}"
[[ "${fail}" -eq 0 ]]
