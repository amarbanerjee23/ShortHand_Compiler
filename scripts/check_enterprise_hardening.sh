#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

fail=0

require_file() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    echo "error: required file is missing: ${file}" >&2
    fail=1
    return 1
  fi
  return 0
}

check_contains() {
  local file="$1"
  local needle="$2"
  require_file "$file" || return 0
  if ! grep -Fq "$needle" "$file"; then
    echo "error: ${file} missing required text: ${needle}" >&2
    fail=1
  fi
}

tracked_metadata="$(git ls-files | grep -E '(^|/)\.metadata/' || true)"
if [[ -n "${tracked_metadata}" ]]; then
  echo "error: tracked IDE metadata must not exist:" >&2
  echo "${tracked_metadata}" >&2
  fail=1
fi

tracked_python_tools="$(git ls-files | grep -E '^deprecated/python_tools/' || true)"
if [[ -n "${tracked_python_tools}" ]]; then
  echo "error: deprecated Python tooling must not be tracked:" >&2
  echo "${tracked_python_tools}" >&2
  fail=1
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

if ! bash tests/integration/test_onnxruntime_sdk_gate.sh >/tmp/shorthand_onnx_sdk_gate.out 2>&1; then
  cat /tmp/shorthand_onnx_sdk_gate.out >&2 || true
  fail=1
else
  cat /tmp/shorthand_onnx_sdk_gate.out
fi

if [[ "${fail}" -ne 0 ]]; then
  echo "Enterprise hardening check failed." >&2
  exit 1
fi

echo "Enterprise hardening check passed."
