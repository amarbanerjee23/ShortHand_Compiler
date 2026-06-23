#!/usr/bin/env bash
set -euo pipefail
BIN=${SHORTHAND_BIN:-Compiler_new_ws/Short_Hand/build/short_hand}
OUT=$($BIN Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short run)
for needle in "AI inference fallback" "runtime_backend=fallback" "inference_status=not_executed" "reason=backend_not_available"; do
  grep -q "$needle" <<<"$OUT"
done
! grep -q "AI inference success" <<<"$OUT"
