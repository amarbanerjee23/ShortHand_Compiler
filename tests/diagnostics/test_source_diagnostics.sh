#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
CASE_FILE="${ROOT_DIR}/tests/semantic/invalid/ai_shape_mismatch.short"
OUT="/tmp/shorthand_source_diagnostics.out"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

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
grep -q '\[range [0-9][0-9]*:[0-9][0-9]*-[0-9][0-9]*:[0-9][0-9]*\]' "${OUT}"
grep -q '^  \^' "${OUT}"

cat >"${WORK_DIR}/exact_range.short" <<'SHORT'
int seed;
break;
SHORT

if "${SHORT}" "${WORK_DIR}/exact_range.short" run >"${WORK_DIR}/exact.out" 2>&1; then
  echo "error: break outside loop unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'exact_range.short:2:1: error: break outside loop [range 2:1-2:6]' "${WORK_DIR}/exact.out"
grep -Fq '  ^^^^^^' "${WORK_DIR}/exact.out"

cat >"${WORK_DIR}/syntax_range.short" <<'SHORT'
int seed;
print 1
SHORT

if "${SHORT}" "${WORK_DIR}/syntax_range.short" run >"${WORK_DIR}/syntax.out" 2>&1; then
  echo "error: malformed source unexpectedly parsed" >&2
  exit 1
fi
grep -Eq '[0-9]+:[0-9]+-[0-9]+:[0-9]+: syntax error' "${WORK_DIR}/syntax.out"

echo "PASS source-aware diagnostics use exact AST and parser ranges"
