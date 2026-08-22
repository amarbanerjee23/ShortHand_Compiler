#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN=${SHORTHAND_BIN:-"${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand"}
EXAMPLE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short"
OUT_FILE="/tmp/shorthand_ai_runtime.out"
# Exercise the fail-closed fallback contract deterministically even when the
# mandatory ONNX Runtime CPU backend is compiled into the CI binary.
SHORTHAND_DEVICE_DENY=cpu,gpu,tpu,npu \
SHORTHAND_ALLOW_CPU_FALLBACK=0 \
  "${BIN}" "${EXAMPLE}" run >"${OUT_FILE}"
for needle in "AI inference fallback" "runtime_backend=fallback" "inference_status=not_executed" "reason=backend_not_available"; do
  grep -q "$needle" "${OUT_FILE}"
done
if grep -q "AI inference success" "${OUT_FILE}"; then
  echo "unexpected successful inference without optional backend" >&2
  exit 1
fi
