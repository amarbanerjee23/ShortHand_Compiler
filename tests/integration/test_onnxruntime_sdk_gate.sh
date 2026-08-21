#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
FIXTURE_B64="${ROOT_DIR}/tests/fixtures/onnx/identity_float32_v13.onnx.b64"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ -z "${ONNXRUNTIME_ROOT:-}" ]]; then
  echo "error: onnxruntime_sdk_gate requires ONNXRUNTIME_ROOT; mandatory production qualification cannot skip" >&2
  exit 1
fi

if [[ ! -f "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" ]]; then
  echo "error: ONNXRUNTIME_ROOT does not contain include/onnxruntime_cxx_api.h" >&2
  exit 1
fi

if [[ ! -d "${ONNXRUNTIME_ROOT}/lib" ]]; then
  echo "error: ONNXRUNTIME_ROOT does not contain lib/" >&2
  exit 1
fi

if command -v base64 >/dev/null 2>&1; then
  if base64 --help 2>&1 | grep -q -- '--decode'; then
    base64 --decode "${FIXTURE_B64}" > "${WORK_DIR}/identity.onnx"
  else
    base64 -D "${FIXTURE_B64}" > "${WORK_DIR}/identity.onnx"
  fi
else
  echo "error: base64 command not found" >&2
  exit 1
fi

make -C "${SRC_DIR}" clean >/tmp/shorthand_onnx_gate_clean.out 2>&1 || true
make -C "${SRC_DIR}" ai_app ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" >/tmp/shorthand_onnx_gate_build.out 2>&1

LD_LIBRARY_PATH="${ONNXRUNTIME_ROOT}/lib:${LD_LIBRARY_PATH:-}" \
  "${BUILD_DIR}/short_ai_app" "${WORK_DIR}/identity.onnx" "1" "42" \
  >/tmp/shorthand_onnx_gate_run.out 2>/tmp/shorthand_onnx_gate_run.err

if ! grep -q "Output: 42" /tmp/shorthand_onnx_gate_run.out; then
  echo "error: ONNX Runtime identity fixture did not round-trip the input" >&2
  cat /tmp/shorthand_onnx_gate_run.out >&2 || true
  cat /tmp/shorthand_onnx_gate_run.err >&2 || true
  exit 1
fi

if grep -qi "fallback\|backend_unavailable\|not_executed" /tmp/shorthand_onnx_gate_run.out /tmp/shorthand_onnx_gate_run.err; then
  echo "error: ONNX Runtime gate used fallback instead of real execution" >&2
  cat /tmp/shorthand_onnx_gate_run.out >&2 || true
  cat /tmp/shorthand_onnx_gate_run.err >&2 || true
  exit 1
fi

echo "PASS onnxruntime_sdk_gate: real ONNX Runtime CPU execution succeeded"
