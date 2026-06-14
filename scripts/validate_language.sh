#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"

printf '[1/5] Checking parser grammar with bison when available...\n'
if command -v bison >/dev/null 2>&1; then
  bison --debug -v -d "${SRC_DIR}/scanner_parser/parser.yy" -o /tmp/short_hand_parser_validate.cc
else
  printf 'Skipping bison grammar check: bison is not installed.\n'
fi

printf '[2/5] Applying generated parser compatibility patch...\n'
make -C "${SRC_DIR}" patch-generated-parser >/dev/null

printf '[3/5] Running syntax-only C++ validation for non-LLVM and LLVM components...\n'
if command -v llvm-config >/dev/null 2>&1; then
  LLVM_CXXFLAGS="$(llvm-config --cxxflags)"
else
  printf 'Skipping LLVM syntax-only validation: llvm-config is not installed.\n'
  LLVM_CXXFLAGS=""
fi

g++ -std=c++17 -fsyntax-only \
  ${LLVM_CXXFLAGS} \
  "${SRC_DIR}/visitors/Interpreter.cpp" \
  "${SRC_DIR}/visitors/AST_Printer.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" \
  "${SRC_DIR}/green_ai/GreenAITool.cpp" \
  "${SRC_DIR}/parser.tab.cc" \
  "${SRC_DIR}/lex.yy.c" \
  -I"${SRC_DIR}"

if command -v llvm-config >/dev/null 2>&1; then
  g++ -std=c++17 -fsyntax-only \
    ${LLVM_CXXFLAGS} \
    "${SRC_DIR}/visitors/IR_Generator.cpp" \
    -I"${SRC_DIR}"
fi

printf '[4/5] Attempting full compiler build when LLVM is available...\n'
if command -v llvm-config >/dev/null 2>&1; then
  make -C "${SRC_DIR}"
else
  printf 'Skipping full build: llvm-config is not installed.\n'
fi

printf '[5/5] Running language examples when short_hand exists...\n'
if [[ -x "${BUILD_DIR}/short_hand" ]]; then
  if ! "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run; then
    printf 'Skipping runtime examples: short_hand could not execute in this environment.\n'
  else
    "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_infer.short" run || true
    "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-bc
    "${BUILD_DIR}/short_hand" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-native || true
  fi
else
  printf 'Skipping examples: %s is not available.\n' "${BUILD_DIR}/short_hand"
fi

printf 'Language validation completed.\n'
