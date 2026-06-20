#!/usr/bin/env bash
set -euo pipefail

STRICT=0
if [[ "${1:-}" == "--strict" ]]; then
  STRICT=1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
EXAMPLE_GREENAI="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short"
NATIVE_OUT="${ROOT_DIR}/greenai_report"

require() {
  local tool="$1"
  if command -v "${tool}" >/dev/null 2>&1; then
    return 0
  fi
  if [[ "${STRICT}" -eq 1 ]]; then
    echo "error: required tool '${tool}' missing" >&2
    exit 1
  fi
  echo "warning: '${tool}' missing; full validation was not performed" >&2
  return 1
}

run_or_warn() {
  if [[ "${STRICT}" -eq 1 ]]; then
    "$@"
  else
    "$@" || echo "warning: command failed in non-strict mode: $*" >&2
  fi
}

printf '[1/7] Checking mandatory tool availability...\n'
for tool in bison flex llvm-config llc clang; do
  require "${tool}" || true
done

printf '[2/7] Checking parser grammar with Bison conflicts as errors...\n'
if require bison; then
  run_or_warn bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d "${SRC_DIR}/scanner_parser/parser.yy" -o /tmp/short_hand_parser_validate.cc
fi

printf '[3/7] Checking Flex scanner generation...\n'
if require flex; then
  run_or_warn flex -o /tmp/short_hand_lex_validate.c "${SRC_DIR}/scanner_parser/scanner.ll"
fi

printf '[4/7] Checking LLVM toolchain availability...\n'
if require llvm-config && require llc && require clang; then
  llvm-config --version
  llc --version | head -n 1
  clang --version | head -n 1
fi

printf '[5/7] Building mandatory targets...\n'
run_or_warn make -C "${SRC_DIR}" compiler green_ai_tool

printf '[6/7] Running compiler-backed validation...\n'
if [[ ! -x "${BUILD_DIR}/short_hand" ]]; then
  if [[ "${STRICT}" -eq 1 ]]; then
    echo "error: short_hand missing after build" >&2
    exit 1
  fi
  echo "warning: short_hand unavailable; compiler examples skipped in non-strict mode" >&2
else
  "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" run >/tmp/shorthand_validate_run.out 2>/tmp/shorthand_validate_run.err
  grep -q "GreenAI workload" /tmp/shorthand_validate_run.out
  grep -q "energy_j=500" /tmp/shorthand_validate_run.out

  rm -f "${ROOT_DIR}/greenai_report.bc" "${ROOT_DIR}/greenai_report.o" "${NATIVE_OUT}"
  (cd "${ROOT_DIR}" && "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-bc >/tmp/shorthand_validate_bc.out 2>/tmp/shorthand_validate_bc.err)
  test -f "${ROOT_DIR}/greenai_report.bc"

  (cd "${ROOT_DIR}" && "${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-native >/tmp/shorthand_validate_native_compile.out 2>/tmp/shorthand_validate_native_compile.err)
  test -x "${NATIVE_OUT}"
  "${NATIVE_OUT}" >/tmp/shorthand_validate_native_run.out 2>/tmp/shorthand_validate_native_run.err
  grep -q "GreenAI workload" /tmp/shorthand_validate_native_run.out
fi

printf '[7/7] Running Green AI regression tests...\n'
run_or_warn bash "${ROOT_DIR}/tests/green_ai/test_green_ai_tool.sh"

if [[ "${STRICT}" -eq 1 ]]; then
  printf 'Strict language validation completed successfully.\n'
else
  printf 'Non-strict language validation completed; review warnings for skipped or environment-limited checks.\n'
fi
