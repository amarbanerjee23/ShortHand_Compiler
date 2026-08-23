#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
WORK_DIR="$(mktemp -d)"
CURRENT_STAGE="initialization"
trap 'rm -rf "${WORK_DIR}"' EXIT

on_error() {
  local status=$?
  echo "FAIL diagnostics matrix stage=${CURRENT_STAGE} line=${BASH_LINENO[0]} command=${BASH_COMMAND}" >&2
  find "${WORK_DIR}" -maxdepth 1 -type f -name '*.out' -exec sh -c 'echo "--- $1"; cat "$1"' _ {} \; >&2 2>/dev/null || true
  exit "${status}"
}
trap on_error ERR

if [[ ! -x "${SHORT}" ]]; then
  echo "SKIP diagnostics coverage matrix: short_hand is not built"
  exit 0
fi

expect_failure() {
  local stage="$1"
  local code="$2"
  local fixture="$3"
  local mode="$4"
  local phrase="$5"
  local out="${WORK_DIR}/${stage}-${code}.out"
  CURRENT_STAGE="${stage}-${code}"

  if "${SHORT}" "${ROOT_DIR}/${fixture}" "${mode}" >"${out}" 2>&1; then
    echo "error: ${stage} fixture unexpectedly succeeded: ${fixture}" >&2
    cat "${out}" >&2 || true
    exit 1
  fi

  grep -Fq ": error: [${code}]" "${out}"
  grep -Fq "${phrase}" "${out}"
  grep -Eq '\[range [0-9]+:[0-9]+-[0-9]+:[0-9]+\]' "${out}"
}

expect_warning() {
  local stage="$1"
  local code="$2"
  local fixture="$3"
  local mode="$4"
  local phrase="$5"
  local out="${WORK_DIR}/${stage}-${code}.out"
  CURRENT_STAGE="${stage}-${code}"

  "${SHORT}" "${ROOT_DIR}/${fixture}" "${mode}" >"${out}" 2>&1
  grep -Fq ": warning: [${code}]" "${out}"
  grep -Fq "${phrase}" "${out}"
  grep -Eq '\[range [0-9]+:[0-9]+-[0-9]+:[0-9]+\]' "${out}"
  if grep -Fq ': error:' "${out}"; then
    echo "error: warning-only fixture emitted an error" >&2
    cat "${out}" >&2
    exit 1
  fi
}

expect_failure parser SHD2001 tests/parser/invalid/missing_semicolon.short run "syntax error"
expect_failure semantic SHD3001 tests/diagnostics/fixtures/break_outside_loop.short run "break outside loop"
expect_warning ai SHD4007 tests/diagnostics/fixtures/ai_incompatible_backend_warning.short run "is not compatible with format onnx"
expect_failure ai SHD4014 tests/semantic/invalid/ai_shape_mismatch.short run "infer input tensor shape"
expect_failure greenai SHD5001 tests/diagnostics/fixtures/greenai_missing_functional_unit.short run "missing functional_unit"
expect_failure semantic SHD3023 tests/semantic/functions_control/undefined_function.short compile "undefined function: missing"
expect_failure semantic SHD3024 tests/enterprise/invalid_enterprise_syntax.enterprise.short enterprise-check "first declaration must be"
expect_failure semantic SHD3025 tests/enterprise/invalid_duplicate_type.enterprise.short enterprise-check "duplicate enterprise type"

CURRENT_STAGE="complete"
echo "PASS diagnostics coverage matrix gate"
