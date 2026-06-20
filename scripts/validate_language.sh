#!/usr/bin/env bash
set -euo pipefail
STRICT=0
if [[ "${1:-}" == "--strict" ]]; then STRICT=1; fi
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
require(){ if ! command -v "$1" >/dev/null 2>&1; then if [[ "$STRICT" -eq 1 ]]; then echo "error: required tool '$1' missing" >&2; exit 1; else echo "warning: '$1' missing; related validation environment-limited"; return 1; fi; fi; }
printf '[1/6] Checking parser grammar with Bison conflicts as errors...\n'
if require bison; then bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d "${SRC_DIR}/scanner_parser/parser.yy" -o /tmp/short_hand_parser_validate.cc; fi
printf '[2/6] Checking Flex availability...\n'
if ! require flex; then :; fi
printf '[3/6] Checking LLVM availability...\n'
if ! require llvm-config; then :; fi
printf '[4/6] Building mandatory targets...\n'
if command -v llvm-config >/dev/null 2>&1; then make -C "${SRC_DIR}" compiler green_ai_tool; else [[ "$STRICT" -eq 0 ]] || exit 1; make -C "${SRC_DIR}" green_ai_tool; fi
printf '[5/6] Running compiler-backed validation...\n'
if [[ -x "${BUILD_DIR}/short_hand" ]]; then
  "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run >/tmp/shorthand_validate_run.out
  "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-bc >/tmp/shorthand_validate_bc.out 2>&1
else
  [[ "$STRICT" -eq 0 ]] || { echo "error: short_hand missing after build" >&2; exit 1; }
  echo "warning: short_hand unavailable; compiler examples environment-limited"
fi
printf '[6/6] Running Green AI regression tests...\n'
bash "${ROOT_DIR}/tests/green_ai/test_green_ai_tool.sh"
printf 'Language validation completed. Strict=%d\n' "$STRICT"
