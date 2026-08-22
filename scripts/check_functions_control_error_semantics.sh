#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
INVOCATION_DIR="${PWD}"

anchor_invocation_path() {
  case "$1" in
    /*) printf '%s\n' "$1" ;;
    *) printf '%s/%s\n' "${INVOCATION_DIR}" "$1" ;;
  esac
}

SHORT="$(anchor_invocation_path "${SHORTHAND_BIN:-${BUILD_DIR}/short_hand}")"
RUNTIME_LIB="$(anchor_invocation_path "${SHORTHAND_RUNTIME_LIB:-${BUILD_DIR}/libshorthand_runtime.a}")"
FIXTURE_DIR="${ROOT_DIR}/tests/semantic/functions_control"
MATRIX="${ROOT_DIR}/tests/conformance/functions_control_matrix_beta_0_5.tsv"
ARTIFACT="${SHORTHAND_CONTROL_FLOW_ARTIFACT:-/tmp/shorthand_control_flow_differential.json}"
CXX_BIN="${CXX:-c++}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

write_failure() {
  local case_name="$1"
  local mode="$2"
  local reason="$3"
  printf '{"schema":"shorthand.control_flow.differential.v1","status":"fail","case":"%s","mode":"%s","reason":"%s"}\n' \
    "${case_name}" "${mode}" "${reason//\"/\\\"}" >"${ARTIFACT}"
}

fail_case() {
  write_failure "$1" "$2" "$3"
  echo "error: $1 $2: $3" >&2
  return 1
}

for command in "${CXX_BIN}" timeout lli llc clang++ bison flex; do
  command -v "${command}" >/dev/null 2>&1 || {
    write_failure infrastructure "${command}" "required tool missing"
    echo "error: beta-0.5 control-flow gate requires ${command}" >&2
    exit 1
  }
done

for file in \
  "${MATRIX}" \
  "${ROOT_DIR}/docs/functions_control_error_semantics.md" \
  "${FIXTURE_DIR}/functions_control.short" \
  "${FIXTURE_DIR}/functions_control.expected" \
  "${FIXTURE_DIR}/undefined_label.short" \
  "${FIXTURE_DIR}/duplicate_label.short" \
  "${FIXTURE_DIR}/cross_scope_goto.short" \
  "${FIXTURE_DIR}/declaration_crossing_goto.short" \
  "${FIXTURE_DIR}/missing_return.short" \
  "${FIXTURE_DIR}/duplicate_local.short" \
  "${FIXTURE_DIR}/void_value.short" \
  "${FIXTURE_DIR}/undefined_function.short" \
  "${FIXTURE_DIR}/argument_type.short"; do
  [[ -s "${file}" ]] || { write_failure infrastructure fixture "missing ${file#${ROOT_DIR}/}"; exit 1; }
done

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tarea\tsource\tanchor\tfixture\texpectation\trationale") exit 10
    next
  }
  NF != 7 { exit 11 }
  $1 !~ /^CTL[0-9][0-9][0-9]$/ { exit 12 }
  $2 !~ /^(functions|scopes|control|errors|compatibility)$/ { exit 13 }
  $3 !~ /^(parser|ast|semantic|interpreter|llvm)$/ { exit 14 }
  $6 !~ /^(accept|reject)$/ { exit 15 }
  $4 == "" || $5 == "" || $7 == "" { exit 16 }
  END { if (NR != 26) exit 17 }
' "${MATRIX}" || { write_failure matrix schema "malformed beta-0.5 matrix"; exit 1; }

cut -f1 "${MATRIX}" | tail -n +2 | sort >"${WORK_DIR}/ids"
sort -u "${WORK_DIR}/ids" >"${WORK_DIR}/unique-ids"
cmp -s "${WORK_DIR}/ids" "${WORK_DIR}/unique-ids" || {
  write_failure matrix ids "duplicate beta-0.5 matrix id"
  exit 1
}

while IFS=$'\t' read -r id area source anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  case "${source}" in
    parser) source_file="${SRC_DIR}/scanner_parser/parser.yy" ;;
    ast) source_file="${SRC_DIR}/ast/AST.h" ;;
    semantic) source_file="${SRC_DIR}/visitors/SemanticAnalyzer.cpp" ;;
    interpreter) source_file="${SRC_DIR}/visitors/Interpreter.cpp" ;;
    llvm) source_file="${SRC_DIR}/visitors/IR_Generator.cpp" ;;
  esac
  grep -Fq "${anchor}" "${source_file}" || {
    fail_case "${id}" matrix "missing ${source} anchor: ${anchor}"
    exit 1
  }
  [[ -s "${ROOT_DIR}/${fixture}" ]] || {
    fail_case "${id}" matrix "missing fixture: ${fixture}"
    exit 1
  }
done <"${MATRIX}"

for anchor in \
  'language_version: beta-0.5' \
  'control_flow_contract: shorthand.control_flow.v1' \
  'call_evaluation_order: left_to_right' \
  'goto_resolution: same_lexical_block_only' \
  'local_cleanup: lexical_block_exit_and_function_frame_exit' \
  'resource_limit_equivalence: terminating_programs_within_interpreter_budgets' \
  'runtime_abi_change: none' \
  'c3eco_alignment: repeatability_safeguards_evidence_integrity_no_quality_degradation' \
  'production_claim: false'; do
  grep -Fq "${anchor}" "${ROOT_DIR}/docs/functions_control_error_semantics.md" || {
    fail_case contract documentation "missing anchor: ${anchor}"
    exit 1
  }
done

"${CXX_BIN}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -I"${SRC_DIR}" \
  -c "${SRC_DIR}/ast/AST.cpp" -o "${WORK_DIR}/AST.o"
"${CXX_BIN}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -I"${SRC_DIR}" \
  -c "${SRC_DIR}/visitors/SemanticAnalyzer.cpp" -o "${WORK_DIR}/SemanticAnalyzer.o"
"${CXX_BIN}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -I"${SRC_DIR}" \
  -c "${SRC_DIR}/visitors/Interpreter.cpp" -o "${WORK_DIR}/Interpreter.o"

bison -Werror=conflicts-sr -Werror=conflicts-rr -d "${SRC_DIR}/scanner_parser/parser.yy" \
  -o "${WORK_DIR}/parser.tab.cc"
flex -o "${WORK_DIR}/lex.yy.c" "${SRC_DIR}/scanner_parser/scanner.ll"

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_pr85_build.out 2>&1 || {
    cat /tmp/shorthand_pr85_build.out >&2 || true
    write_failure infrastructure compiler "compiler build failed"
    exit 1
  }
fi
if [[ ! -f "${RUNTIME_LIB}" ]]; then
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_pr85_runtime_build.out 2>&1 || {
    cat /tmp/shorthand_pr85_runtime_build.out >&2 || true
    write_failure infrastructure runtime "runtime library build failed"
    exit 1
  }
fi

run_interpreter() {
  timeout --signal=TERM --kill-after=2 20 "${SHORT}" "$1" run >"$2" 2>"$2.err" || {
    cat "$2.err" >&2 || true
    fail_case functions_control interpreter "expected successful execution"
  }
}

run_lli() {
  local fixture="$1"
  local output="$2"
  local run_dir="${WORK_DIR}/lli"
  mkdir -p "${run_dir}"
  (
    cd "${run_dir}"
    timeout --signal=TERM --kill-after=2 25 "${SHORT}" "${fixture}" compile-bc >compile.out 2>compile.err
    timeout --signal=TERM --kill-after=2 20 lli functions_control.bc >"${output}" 2>"${output}.err"
  ) || {
    cat "${run_dir}/compile.err" >&2 2>/dev/null || true
    cat "${output}.err" >&2 2>/dev/null || true
    fail_case functions_control lli "compile or execution failed"
  }
}

run_native() {
  local fixture="$1"
  local output="$2"
  local run_dir="${WORK_DIR}/native"
  mkdir -p "${run_dir}"
  (
    cd "${run_dir}"
    SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout --signal=TERM --kill-after=2 35 \
      "${SHORT}" "${fixture}" compile-native >compile.out 2>compile.err
    timeout --signal=TERM --kill-after=2 20 ./functions_control >"${output}" 2>"${output}.err"
  ) || {
    cat "${run_dir}/compile.err" >&2 2>/dev/null || true
    cat "${output}.err" >&2 2>/dev/null || true
    fail_case functions_control native "compile or execution failed"
  }
}

POSITIVE="${FIXTURE_DIR}/functions_control.short"
run_interpreter "${POSITIVE}" "${WORK_DIR}/interpreter.out"
run_lli "${POSITIVE}" "${WORK_DIR}/lli.out"
run_native "${POSITIVE}" "${WORK_DIR}/native.out"

for mode in interpreter lli native; do
  diff -u "${FIXTURE_DIR}/functions_control.expected" "${WORK_DIR}/${mode}.out" || {
    fail_case functions_control "${mode}" "stdout differs from beta-0.5 golden contract"
    exit 1
  }
done
cmp -s "${WORK_DIR}/interpreter.out" "${WORK_DIR}/lli.out"
cmp -s "${WORK_DIR}/interpreter.out" "${WORK_DIR}/native.out"

expect_semantic_failure() {
  local fixture="$1"
  local expected="$2"
  local case_name
  case_name="$(basename "${fixture}" .short)"
  for mode in run compile-bc compile-native; do
    local prefix="${WORK_DIR}/${case_name}.${mode}"
    if SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout --signal=TERM --kill-after=2 20 \
         "${SHORT}" "${fixture}" "${mode}" >"${prefix}.out" 2>"${prefix}.err"; then
      fail_case "${case_name}" "${mode}" "semantic-negative program unexpectedly succeeded"
      return 1
    fi
    grep -Fq "[${expected}]" "${prefix}.err" || {
      cat "${prefix}.err" >&2 || true
      fail_case "${case_name}" "${mode}" "expected ${expected}"
      return 1
    }
    grep -Fq '[range ' "${prefix}.err" || {
      fail_case "${case_name}" "${mode}" "diagnostic missing source range"
      return 1
    }
  done
}

expect_semantic_failure "${FIXTURE_DIR}/duplicate_label.short" SHD3017
expect_semantic_failure "${FIXTURE_DIR}/undefined_label.short" SHD3018
expect_semantic_failure "${FIXTURE_DIR}/cross_scope_goto.short" SHD3019
expect_semantic_failure "${FIXTURE_DIR}/declaration_crossing_goto.short" SHD3019
expect_semantic_failure "${FIXTURE_DIR}/missing_return.short" SHD3020
expect_semantic_failure "${FIXTURE_DIR}/duplicate_local.short" SHD3021
expect_semantic_failure "${FIXTURE_DIR}/void_value.short" SHD3022
expect_semantic_failure "${FIXTURE_DIR}/undefined_function.short" SHD3023
expect_semantic_failure "${FIXTURE_DIR}/argument_type.short" SHD3013

printf '{"schema":"shorthand.control_flow.differential.v1","status":"pass","engines":["interpreter","lli","native"],"positive_cases":1,"semantic_negative_cases":9,"mandatory_skips":0}\n' >"${ARTIFACT}"
printf 'PASS beta-0.5 functions scopes control flow deterministic errors and cleanup gate\n'
