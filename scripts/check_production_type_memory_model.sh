#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CXX_BIN="${CXX:-c++}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
MATRIX="${ROOT_DIR}/tests/conformance/type_matrix_beta_0_4.tsv"

for file in \
  Compiler_new_ws/Short_Hand/src/type_system/ProductionTypeSystem.h \
  Compiler_new_ws/Short_Hand/src/type_system/ProductionTypeSystem.cpp \
  tests/types/test_production_type_memory_model.cpp \
  docs/production_type_memory_model.md \
  docs/execution_semantics_beta_0_4.md \
  tests/conformance/type_matrix_beta_0_4.tsv; do
  [[ -s "${ROOT_DIR}/${file}" ]] || {
    echo "error: missing PR84 type/memory evidence: ${file}" >&2
    exit 1
  }
done

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tarea\tsource\tanchor\tfixture\texpectation\trationale") exit 10
    next
  }
  NF != 7 { exit 11 }
  $1 !~ /^TYP[0-9][0-9][0-9]$/ { exit 12 }
  $3 !~ /^(parser|semantic|interpreter|llvm|type-system)$/ { exit 13 }
  $6 !~ /^(accept|reject)$/ { exit 14 }
  $4 == "" || $5 == "" || $7 == "" { exit 15 }
  END { if (NR != 20) exit 16 }
' "${MATRIX}" || {
  echo "error: malformed beta-0.4 type conformance matrix" >&2
  exit 1
}

cut -f1 "${MATRIX}" | tail -n +2 | sort >"${WORK_DIR}/type-ids.txt"
sort -u "${WORK_DIR}/type-ids.txt" >"${WORK_DIR}/type-ids-unique.txt"
cmp -s "${WORK_DIR}/type-ids.txt" "${WORK_DIR}/type-ids-unique.txt" || {
  echo "error: duplicate beta-0.4 type matrix ID" >&2
  exit 1
}

while IFS=$'\t' read -r id area source anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  case "${source}" in
    parser) source_file="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/scanner_parser/parser.yy" ;;
    semantic) source_file="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/SemanticAnalyzer.cpp" ;;
    interpreter) source_file="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/Interpreter.cpp" ;;
    llvm) source_file="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/IR_Generator.cpp" ;;
    type-system) source_file="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/type_system/ProductionTypeSystem.cpp" ;;
    *) echo "error: unknown matrix source: ${source}" >&2; exit 1 ;;
  esac
  grep -Fq "${anchor}" "${source_file}" || {
    echo "error: ${id} anchor not found in ${source}: ${anchor}" >&2
    exit 1
  }
  [[ -s "${ROOT_DIR}/${fixture}" ]] || {
    echo "error: ${id} fixture is missing: ${fixture}" >&2
    exit 1
  }
done <"${MATRIX}"

"${CXX_BIN}" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -I"${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/type_system/ProductionTypeSystem.cpp" \
  "${ROOT_DIR}/tests/types/test_production_type_memory_model.cpp" \
  -o "${WORK_DIR}/production_type_memory_model_test"

"${WORK_DIR}/production_type_memory_model_test"

for anchor in \
  'type_system_contract: shorthand.type_memory.v1' \
  'implicit_numeric_narrowing: forbidden' \
  'use_after_move: SHD3016' \
  'slice_bounds_failure: SHD7002' \
  'c3eco_alignment: evidence_integrity_and_no_quality_degradation' \
  'production_claim: false'; do
  grep -Fq "${anchor}" "${ROOT_DIR}/docs/production_type_memory_model.md" || {
    echo "error: production type/memory contract missing anchor: ${anchor}" >&2
    exit 1
  }
done

echo 'PASS production type and memory model gate'
