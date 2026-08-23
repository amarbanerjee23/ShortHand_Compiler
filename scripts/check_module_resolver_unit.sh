#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
TEST="${ROOT_DIR}/tests/modules/resolver/test_module_resolver_unit.cpp"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

[[ -f "${TEST}" ]] || { echo "error: missing module resolver unit source" >&2; exit 1; }
${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -I"${SRC_DIR}" \
  "${TEST}" "${SRC_DIR}/module/ModuleResolver.cpp" "${SRC_DIR}/module/Sha256.cpp" \
  -o "${WORK_DIR}/module_resolver_unit"
"${WORK_DIR}/module_resolver_unit"

printf 'PASS module resolver unit gate\n'
