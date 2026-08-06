#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
PARSER="${SRC_DIR}/scanner_parser/parser.yy"
SCANNER="${SRC_DIR}/scanner_parser/scanner.ll"
CLI="${SRC_DIR}/main.cpp"
MATRIX="${ROOT_DIR}/tests/conformance/module_matrix_beta_0_3.tsv"
DOC="${ROOT_DIR}/docs/module_import_package_syntax.md"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
VALID="${ROOT_DIR}/tests/modules/valid/module_preamble.short"
LEGACY="${ROOT_DIR}/tests/conformance/beta_0_2/core_declarations.short"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

for file in "${PARSER}" "${SCANNER}" "${CLI}" "${MATRIX}" "${DOC}"; do
  [[ -f "${file}" ]] || { echo "error: missing module contract file: ${file}" >&2; exit 1; }
done

grep -Fq 'module_syntax_contract_version: beta-0.3' "${DOC}"
grep -Fq 'module_resolution_status: parser_and_ast_scaffold_only' "${DOC}"
grep -Fq 'multi_file_execution_status: deferred_to_pr70' "${DOC}"
grep -Fq 'production_claim: false' "${DOC}"

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tarea\tsource\tanchor\tfixture\texpectation\trationale") exit 10
    next
  }
  NF != 7 { exit 11 }
  $1 !~ /^MOD[0-9][0-9][0-9]$/ { exit 12 }
  $2 !~ /^(module|boundary)$/ { exit 13 }
  $3 !~ /^(parser|scanner|cli)$/ { exit 14 }
  $6 !~ /^(accept|reject)$/ { exit 15 }
  $4 == "" || $5 == "" || $7 == "" { exit 16 }
  END { if (NR != 18) exit 17 }
' "${MATRIX}" || {
  echo "error: malformed beta-0.3 module conformance matrix" >&2
  exit 1
}

if [[ "$(tail -n +2 "${MATRIX}" | cut -f1 | sort | uniq -d | wc -l | tr -d ' ')" != "0" ]]; then
  echo "error: duplicate beta-0.3 module matrix ID" >&2
  exit 1
fi

while IFS=$'\t' read -r id area source anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  case "${source}" in
    parser) source_file="${PARSER}" ;;
    scanner) source_file="${SCANNER}" ;;
    cli) source_file="${CLI}" ;;
  esac
  grep -Fq "${anchor}" "${source_file}" || {
    echo "error: matrix ${id} anchor not found: ${anchor}" >&2
    exit 1
  }
  [[ -f "${ROOT_DIR}/${fixture}" ]] || {
    echo "error: matrix ${id} fixture missing: ${fixture}" >&2
    exit 1
  }
done <"${MATRIX}"

command -v bison >/dev/null 2>&1 || { echo "error: bison is required" >&2; exit 1; }
command -v flex >/dev/null 2>&1 || { echo "error: flex is required" >&2; exit 1; }
bison -Werror=conflicts-sr -Werror=conflicts-rr -d "${PARSER}" -o "${WORK_DIR}/parser.tab.cc"
flex -o "${WORK_DIR}/lex.yy.c" "${SCANNER}"

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_module_ast_build.out 2>&1 || {
    cat /tmp/shorthand_module_ast_build.out >&2 || true
    exit 1
  }
fi

for file in \
  "${VALID}" \
  "${LEGACY}" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_package.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_alias.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_path.short" \
  "${ROOT_DIR}/tests/modules/invalid/import_before_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/package_without_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/malformed_module_path.short"; do
  [[ -f "${file}" ]] || { echo "error: missing module fixture: ${file}" >&2; exit 1; }
done

"${SHORT}" "${VALID}" parse >"${WORK_DIR}/valid-parse.out" 2>&1
"${SHORT}" "${VALID}" module-info >"${WORK_DIR}/module-info.json" 2>"${WORK_DIR}/module-info.err"

grep -Fq '"schema":"shorthand.module.ast.v1"' "${WORK_DIR}/module-info.json"
grep -Fq '"language_version":"beta-0.3"' "${WORK_DIR}/module-info.json"
grep -Fq '"resolver_status":"not_resolved"' "${WORK_DIR}/module-info.json"
grep -Fq '"package":{"name":"acme.ai"' "${WORK_DIR}/module-info.json"
grep -Fq '"module":{"name":"acme.recommendation"' "${WORK_DIR}/module-info.json"
grep -Fq '"path":"acme.models","alias":"models"' "${WORK_DIR}/module-info.json"
grep -Fq '"path":"acme.telemetry","alias":null' "${WORK_DIR}/module-info.json"
grep -Fq '"range":{"begin":{"line":1,"column":1}' "${WORK_DIR}/module-info.json"
grep -Fq '"range":{"begin":{"line":2,"column":1}' "${WORK_DIR}/module-info.json"

"${SHORT}" "${LEGACY}" module-info >"${WORK_DIR}/legacy-info.json" 2>"${WORK_DIR}/legacy-info.err"
grep -Fq '"package":null' "${WORK_DIR}/legacy-info.json"
grep -Fq '"module":null' "${WORK_DIR}/legacy-info.json"
grep -Fq '"imports":[]' "${WORK_DIR}/legacy-info.json"

expect_failure() {
  local fixture="$1"
  local code="$2"
  local output="${WORK_DIR}/$(basename "${fixture}").out"
  if "${SHORT}" "${fixture}" parse >"${output}" 2>&1; then
    echo "error: invalid module fixture unexpectedly parsed: ${fixture}" >&2
    exit 1
  fi
  grep -Fq "[${code}]" "${output}" || {
    echo "error: ${fixture} did not emit ${code}" >&2
    cat "${output}" >&2 || true
    exit 1
  }
  grep -Fq '[range ' "${output}" || {
    echo "error: ${fixture} did not emit a source range" >&2
    cat "${output}" >&2 || true
    exit 1
  }
  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' "${output}"; then
    echo "error: sanitizer failure while rejecting ${fixture}" >&2
    cat "${output}" >&2 || true
    exit 1
  fi
}

expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_package.short" SHD2011
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_module.short" SHD2012
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_import_alias.short" SHD2013
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_import_path.short" SHD2014
expect_failure "${ROOT_DIR}/tests/modules/invalid/import_before_module.short" SHD2015
expect_failure "${ROOT_DIR}/tests/modules/invalid/package_without_module.short" SHD2016
expect_failure "${ROOT_DIR}/tests/modules/invalid/malformed_module_path.short" SHD2001

{
  printf 'module stress.generated;\n'
  for index in $(seq 1 512); do
    printf 'import stress.dependency_%s as dep_%s;\n' "${index}" "${index}"
  done
  printf 'int value;\nvalue = 1;\n'
} >"${WORK_DIR}/large-import-set.short"

timeout --signal=TERM --kill-after=2 10 \
  "${SHORT}" "${WORK_DIR}/large-import-set.short" parse \
  >"${WORK_DIR}/large-import-set.out" 2>&1

if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' \
  "${WORK_DIR}/large-import-set.out"; then
  echo "error: sanitizer failure in import-set stress fixture" >&2
  cat "${WORK_DIR}/large-import-set.out" >&2 || true
  exit 1
fi

printf 'PASS module import package syntax and AST scaffold gate\n'
