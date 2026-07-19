#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
CASE_FILE="${ROOT_DIR}/tests/semantic/invalid/ai_shape_mismatch.short"
OUT="/tmp/shorthand_source_diagnostics.out"

if [[ ! -x "${SHORT}" ]]; then
  echo "SKIP source diagnostics: short_hand is not built"
  exit 0
fi

if "${SHORT}" "${CASE_FILE}" run >"${OUT}" 2>&1; then
  echo "error: expected semantic diagnostics for invalid AI shape mismatch" >&2
  cat "${OUT}" >&2 || true
  exit 1
fi

grep -E 'ai_shape_mismatch\.short:[0-9]+:[0-9]+: error:' "${OUT}" >/dev/null
grep -q 'infer input tensor shape' "${OUT}"
grep -q '^  \^' "${OUT}"

echo "PASS source-aware diagnostics include file, line, column, message and caret"
