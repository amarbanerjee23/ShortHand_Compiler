#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
CXX_BIN="${CXX:-c++}"
LOG="${SHORTHAND_PARSER_LIFETIME_LOG:-/tmp/shorthand_parser_lifetime.out}"
for tool in "${CXX_BIN}" bison flex; do command -v "${tool}" >/dev/null; done
bison -Werror=conflicts-sr -Werror=conflicts-rr -d "${SRC_DIR}/scanner_parser/parser.yy" -o "${WORK_DIR}/parser.tab.cc"
flex -o "${WORK_DIR}/lex.yy.cpp" "${SRC_DIR}/scanner_parser/scanner.ll"
"${CXX_BIN}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -I"${SRC_DIR}" -I"${WORK_DIR}" \
  "${ROOT_DIR}/tests/parser/ParserLifetime.cpp" "${WORK_DIR}/parser.tab.cc" "${WORK_DIR}/lex.yy.cpp" \
  "${SRC_DIR}/ast/AST.cpp" "${SRC_DIR}/visitors/Diagnostics.cpp" \
  "${SRC_DIR}/visitors/SemanticAnalyzer.cpp" "${SRC_DIR}/type_system/ProductionTypeSystem.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Types.cpp" -o "${WORK_DIR}/parser-lifetime"
if ! "${WORK_DIR}/parser-lifetime" >"${LOG}" 2>&1; then
  tail -n 100 "${LOG}" >&2
  exit 1
fi
tail -n 2 "${LOG}"
