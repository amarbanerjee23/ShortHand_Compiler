#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORTHAND_BIN="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
CXX="${CXX:-g++}"
TOOL_FLAGS_TEXT="${SHORTHAND_TOOL_CXXFLAGS:--std=c++17 -O2 -Wall -Wextra -Wpedantic}"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

command -v "${CXX}" >/dev/null 2>&1 || { echo "error: C++ compiler unavailable for formatter/linter gate: ${CXX}" >&2; exit 1; }
[[ -x "${SHORTHAND_BIN}" ]] || { echo "error: ShortHand compiler unavailable for formatter parse-preservation gate: ${SHORTHAND_BIN}" >&2; exit 1; }

TOOL="${TMP}/shorthand_tool"
make -C "${SRC_DIR}/tooling" \
  BIN="${TOOL}" \
  CXX="${CXX}" \
  CXXFLAGS="${TOOL_FLAGS_TEXT}"
[[ -x "${TOOL}" ]] || { echo "error: formatter/linter build did not produce an executable" >&2; exit 1; }

MESSY="${ROOT_DIR}/tests/tooling/formatter_messy.short"
EXPECTED="${ROOT_DIR}/tests/tooling/formatter_expected.short"
FORMATTED="${TMP}/formatted.short"
FORMATTED_TWICE="${TMP}/formatted_twice.short"
FIXED="${TMP}/fixed.short"
LINT_JSON="${TMP}/lint.json"
CLEAN_JSON="${TMP}/clean.json"

# The formatter only claims preservation for parser-valid ShortHand source.
"${SHORTHAND_BIN}" "${MESSY}" parse >/dev/null
"${TOOL}" "${MESSY}" format --output "${FORMATTED}"
diff -u "${EXPECTED}" "${FORMATTED}"
"${SHORTHAND_BIN}" "${FORMATTED}" parse >/dev/null

# Determinism and idempotence are mandatory, not best-effort style properties.
"${TOOL}" "${FORMATTED}" format --output "${FORMATTED_TWICE}"
diff -u "${FORMATTED}" "${FORMATTED_TWICE}"

# Canonical input is lint-clean and the machine schema is stable.
"${TOOL}" "${FORMATTED}" lint --output "${CLEAN_JSON}"
grep -Fq '"schema":"shorthand.lint.v1"' "${CLEAN_JSON}"
grep -Fq '"diagnostics":[]' "${CLEAN_JSON}"

# Non-canonical input must fail lint with actionable, fixable diagnostics.
if "${TOOL}" "${MESSY}" lint --output "${LINT_JSON}"; then
  echo "error: non-canonical ShortHand source unexpectedly passed lint" >&2
  exit 1
fi
for code in SHL001 SHL002 SHL003 SHL005; do
  grep -Fq "\"code\":\"${code}\"" "${LINT_JSON}" || {
    echo "error: expected lint diagnostic missing: ${code}" >&2
    cat "${LINT_JSON}" >&2
    exit 1
  }
done
grep -Fq '"fixable":true' "${LINT_JSON}"

# Safe fix mode never mutates the input implicitly and must equal canonical format.
if "${TOOL}" "${MESSY}" fix >/dev/null 2>"${TMP}/fix_without_output.err"; then
  echo "error: fix mode unexpectedly allowed implicit source mutation contract" >&2
  exit 1
fi
grep -Fq 'fix mode requires --output' "${TMP}/fix_without_output.err"
"${TOOL}" "${MESSY}" fix --output "${FIXED}"
diff -u "${EXPECTED}" "${FIXED}"

# The formatter changes trivia only. Parser acceptance and executable behavior remain identical.
"${SHORTHAND_BIN}" "${MESSY}" run >"${TMP}/before.out"
"${SHORTHAND_BIN}" "${FIXED}" run >"${TMP}/after.out"
diff -u "${TMP}/before.out" "${TMP}/after.out"
grep -Fq '// keep { comment' "${FIXED}"
grep -Fq '"brace { in string"' "${FIXED}"

# Boundary diagnostics cover missing final newline and non-LF line endings.
printf 'int x;' >"${TMP}/no_final_newline.short"
if "${TOOL}" "${TMP}/no_final_newline.short" lint --output "${TMP}/newline.json"; then
  echo "error: missing-final-newline fixture unexpectedly passed lint" >&2
  exit 1
fi
grep -Fq '"code":"SHL004"' "${TMP}/newline.json"
printf 'int x;\r\nx = 0;\r\n' >"${TMP}/crlf.short"
if "${TOOL}" "${TMP}/crlf.short" lint --output "${TMP}/crlf.json"; then
  echo "error: CRLF fixture unexpectedly passed lint" >&2
  exit 1
fi
grep -Fq '"code":"SHL006"' "${TMP}/crlf.json"
"${TOOL}" "${TMP}/crlf.short" format --output "${TMP}/crlf_fixed.short"
! grep -q $'\r' "${TMP}/crlf_fixed.short"
"${SHORTHAND_BIN}" "${TMP}/crlf_fixed.short" parse >/dev/null

printf 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate compiler=%s\n' "${CXX}"
