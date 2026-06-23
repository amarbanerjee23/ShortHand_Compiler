#!/usr/bin/env bash
set -euo pipefail
BIN=${SHORTHAND_BIN:-Compiler_new_ws/Short_Hand/build/short_hand}
OUT=$($BIN Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short evidence)
for needle in backend_preference runtime_backend inference_status disclaimer classifier int8 "1,3,224,224"; do
  grep -q "$needle" <<<"$OUT"
done
