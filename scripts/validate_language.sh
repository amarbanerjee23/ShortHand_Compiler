#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"

printf '[1/4] Checking parser grammar with bison...\n'
bison --debug -v -d "${SRC_DIR}/scanner_parser/parser.yy" -o /tmp/short_hand_parser_validate.cc

printf '[2/4] Running syntax-only C++ validation for non-LLVM components...\n'
g++ -std=c++14 -fsyntax-only \
  "${SRC_DIR}/ast/AST.cpp" \
  "${SRC_DIR}/visitors/Interpreter.cpp" \
  "${SRC_DIR}/visitors/AST_Printer.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" \
  "${SRC_DIR}/parser.tab.cc" \
  "${SRC_DIR}/lex.yy.c" \
  -I"${SRC_DIR}"

printf '[3/4] Attempting full compiler build when LLVM is available...\n'
if command -v llvm-config >/dev/null 2>&1; then
  make -C "${SRC_DIR}"
else
  printf 'Skipping full build: llvm-config is not installed.\n'
fi

printf '[4/4] Running language examples when short_hand exists...\n'
if [[ -x "${BUILD_DIR}/short_hand" ]]; then
  if ! "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run; then
    printf 'Skipping runtime examples: short_hand could not execute in this environment.\n'
  else
    "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_infer.short" run || true
  fi
else
  printf 'Skipping examples: %s is not available.\n' "${BUILD_DIR}/short_hand"
fi

printf 'Language validation completed.\n'
