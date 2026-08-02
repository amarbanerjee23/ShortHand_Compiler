#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/ast_source_ranges.md"
HEADER="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.h"
IMPLEMENTATION="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.cpp"
PARSER="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/scanner_parser/parser.yy"
SCANNER="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/scanner_parser/scanner.ll"
DIAGNOSTICS="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/Diagnostics.cpp"
SEMANTIC="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/SemanticAnalyzer.cpp"
TEST="${ROOT_DIR}/tests/diagnostics/test_source_diagnostics.sh"

require_contains() {
  local file="$1"
  local needle="$2"
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
  grep -Fq "${needle}" "${file}" || {
    echo "error: ${file} missing required source range text: ${needle}" >&2
    exit 1
  }
}

require_contains "${DOC}" 'ast_source_range_status: parser_propagated_line_column_ranges'
require_contains "${DOC}" 'runtime_abi_change: none'
require_contains "${HEADER}" 'struct SourceRange'
require_contains "${HEADER}" 'shorthand_set_ast_source_range'
require_contains "${IMPLEMENTATION}" 'std::unordered_map<const void *, SourceRange>'
require_contains "${PARSER}" '%locations'
require_contains "${PARSER}" 'static T *located'
require_contains "${PARSER}" 'new AST_BINARY_EXPRESSION_RULE'
require_contains "${PARSER}" 'new AST_MODEL_DECLARATION'
require_contains "${SCANNER}" 'YY_USER_ACTION'
require_contains "${SCANNER}" 'yylloc.first_column'
require_contains "${DIAGNOSTICS}" 'errorAtNode'
require_contains "${DIAGNOSTICS}" '[range '
require_contains "${SEMANTIC}" 'diagnostics.errorAtNode(n'
require_contains "${TEST}" 'break outside loop [range 2:1-2:6]'

bash -n "${TEST}"
if [[ -x "${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand" ]]; then
  SHORTHAND_BIN="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand" bash "${TEST}"
fi

echo "PASS AST source range guard"
