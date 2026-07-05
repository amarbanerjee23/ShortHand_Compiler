#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN=${SHORTHAND_BIN:-"${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand"}
EXAMPLE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short"
OUT="$(${BIN} "${EXAMPLE}" run)"
for needle in "AI inference fallback" "runtime_backend=fallback" "inference_status=not_executed" "reason=backend_not_available"; do
  grep -q "$needle" <<<"$OUT"
done
if grep -q "AI inference success" <<<"$OUT"; then
  echo "unexpected successful inference without optional backend" >&2
  exit 1
fi
