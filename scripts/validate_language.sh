#!/usr/bin/env bash
set -euo pipefail

STRICT=0
if [[ "${1:-}" == "--strict" ]]; then STRICT=1; fi
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
EXAMPLE_GREENAI="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short"

require() {
  if ! command -v "$1" >/dev/null 2>&1; then
    if [[ "${STRICT}" -eq 1 ]]; then
      printf 'error: required tool %s missing\n' "$1" >&2
      exit 1
    fi
    printf 'warning: %s missing; related validation environment-limited\n' "$1"
    return 1
  fi
}

printf '[1/7] Checking parser grammar with Bison conflicts as errors...\n'
if require bison; then
  bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d "${SRC_DIR}/scanner_parser/parser.yy" -o /tmp/short_hand_parser_validate.cc
fi

printf '[2/7] Checking Flex availability...\n'
if ! require flex; then :; fi

printf '[3/7] Checking LLVM native toolchain availability...\n'
if ! require llvm-config; then :; fi
if ! require llc; then :; fi
if ! require clang; then :; fi

printf '[4/7] Building compiler and Green AI tool...\n'
if command -v llvm-config >/dev/null 2>&1; then
  make -C "${SRC_DIR}" compiler green_ai_tool
else
  [[ "${STRICT}" -eq 0 ]] || exit 1
  make -C "${SRC_DIR}" green_ai_tool
fi

printf '[5/7] Running interpreter GreenAI example...\n'
if [[ -x "${BUILD_DIR}/short_hand" ]]; then
  "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" run >/tmp/shorthand_validate_run.out
  grep -q "GreenAI workload" /tmp/shorthand_validate_run.out
else
  [[ "${STRICT}" -eq 0 ]] || { printf 'error: short_hand missing after build\n' >&2; exit 1; }
  printf 'warning: short_hand unavailable; compiler examples environment-limited\n'
fi

printf '[6/7] Validating bitcode and native compilation...\n'
if [[ -x "${BUILD_DIR}/short_hand" ]]; then
  (cd "${ROOT_DIR}" && "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-bc >/tmp/shorthand_validate_bc.out 2>&1)
  test -f "${ROOT_DIR}/greenai_report.bc"
  if command -v llc >/dev/null 2>&1 && command -v clang >/dev/null 2>&1; then
    (cd "${ROOT_DIR}" && "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-native >/tmp/shorthand_validate_native.out 2>&1)
    test -x "${ROOT_DIR}/greenai_report"
  elif [[ "${STRICT}" -eq 1 ]]; then
    printf 'error: llc/clang unavailable for strict native validation\n' >&2
    exit 1
  else
    printf 'warning: llc/clang unavailable; native validation environment-limited\n'
  fi
fi

printf '[7/7] Running Green AI regression tests...\n'
bash "${ROOT_DIR}/tests/green_ai/test_green_ai_tool.sh"
printf 'Language validation completed. Strict=%d\n' "${STRICT}"
