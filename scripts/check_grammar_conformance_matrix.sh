#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
PARSER="${SRC_DIR}/scanner_parser/parser.yy"
SCANNER="${SRC_DIR}/scanner_parser/scanner.ll"
CLI="${SRC_DIR}/main.cpp"
MATRIX="${ROOT_DIR}/tests/conformance/grammar_matrix_beta_0_2.tsv"
MANIFEST="${ROOT_DIR}/tests/conformance/manifest.txt"
GRAMMAR_DOC="${ROOT_DIR}/docs/language_grammar_ebnf.md"
VERSION_DOC="${ROOT_DIR}/docs/language_versioning_and_conformance.md"
SPEC_DOC="${ROOT_DIR}/docs/language_spec.md"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

require_file() {
  local file="$1"
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  grep -Fq "${needle}" "${file}" || {
    echo "error: ${file#${ROOT_DIR}/} missing required beta-0.2 text: ${needle}" >&2
    exit 1
  }
}

beta_0_2_anchor_present() {
  local id="$1"
  local source_file="$2"
  local anchor="$3"

  if grep -Fq "${anchor}" "${source_file}"; then
    return 0
  fi

  # PR69 adds an optional beta-0.3 module preamble in front of the beta-0.2
  # program body. Preserve the beta-0.2 ordering contract without requiring
  # the obsolete pre-PR69 production header to remain verbatim in the parser.
  if [[ "${id}" == "PRG001" && "${source_file}" == "${PARSER}" ]]; then
    grep -Fq \
      'PROGRAMME_RULE: MODULE_PREAMBLE_RULE DECLARATION_STATEMENT_LIST_RULE FUNCTION_LIST_RULE LOGIC_BLOCK' \
      "${PARSER}"
    return $?
  fi

  return 1
}

for file in "${PARSER}" "${SCANNER}" "${CLI}" "${MATRIX}" "${MANIFEST}" "${GRAMMAR_DOC}" "${VERSION_DOC}" "${SPEC_DOC}"; do
  require_file "${file}"
done

require_contains "${GRAMMAR_DOC}" 'Language version: beta-0.2'
require_contains "${GRAMMAR_DOC}" 'grammar_conformance_status: parser_accurate_matrix_guarded'
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.4'
require_contains "${VERSION_DOC}" 'shorthand.language.version: beta-0.2'
require_contains "${SPEC_DOC}" 'Language version: beta-0.2'
require_contains "${MANIFEST}" 'current-version | shorthand.language.version | beta-0.4 | Current executable language contract marker.'
require_contains "${MANIFEST}" 'version | shorthand.language.version | beta-0.2 | Previous executable base language contract marker.'
require_contains "${CLI}" 'mode == "parse"'

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tarea\tsource\tanchor\tfixture\texpectation\trationale") exit 10
    next
  }
  NF != 7 { print "bad matrix row: " $0 > "/dev/stderr"; exit 11 }
  $1 !~ /^[A-Z][A-Z][A-Z]?[0-9][0-9][0-9]$/ { print "bad matrix id: " $1 > "/dev/stderr"; exit 12 }
  $2 !~ /^(lexical|program|declarations|functions|statements|expressions|ai|greenai|compatibility|boundary)$/ { print "bad area: " $2 > "/dev/stderr"; exit 13 }
  $3 !~ /^(parser|scanner|cli)$/ { print "bad source: " $3 > "/dev/stderr"; exit 14 }
  $6 !~ /^(accept|reject)$/ { print "bad expectation: " $6 > "/dev/stderr"; exit 15 }
  $4 == "" || $5 == "" || $7 == "" { exit 16 }
  END { if (NR < 80) exit 17 }
' "${MATRIX}" || {
  echo "error: malformed or incomplete beta-0.2 grammar matrix" >&2
  exit 1
}

awk -F '\t' 'NR > 1 { print $1 }' "${MATRIX}" | sort >"${WORK_DIR}/ids-all.txt"
sort -u "${WORK_DIR}/ids-all.txt" >"${WORK_DIR}/ids.txt"
if [[ "$(wc -l <"${WORK_DIR}/ids-all.txt")" -ne "$(wc -l <"${WORK_DIR}/ids.txt")" ]]; then
  echo "error: duplicate grammar matrix id" >&2
  exit 1
fi

printf '%s\n' ai boundary compatibility declarations expressions functions greenai lexical program statements >"${WORK_DIR}/required-areas.txt"
awk -F '\t' 'NR > 1 { print $2 }' "${MATRIX}" | sort -u >"${WORK_DIR}/areas.txt"
diff -u "${WORK_DIR}/required-areas.txt" "${WORK_DIR}/areas.txt"

while IFS=$'\t' read -r id area source anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  case "${source}" in
    parser) source_file="${PARSER}" ;;
    scanner) source_file="${SCANNER}" ;;
    cli) source_file="${CLI}" ;;
    *) echo "error: unsupported source in ${id}: ${source}" >&2; exit 1 ;;
  esac
  if ! beta_0_2_anchor_present "${id}" "${source_file}" "${anchor}"; then
    echo "error: matrix ${id} anchor not found in ${source}: ${anchor}" >&2
    exit 1
  fi
  require_file "${ROOT_DIR}/${fixture}"
done <"${MATRIX}"

command -v bison >/dev/null 2>&1 || { echo "error: bison is required" >&2; exit 1; }
command -v flex >/dev/null 2>&1 || { echo "error: flex is required" >&2; exit 1; }
bison -Werror=conflicts-sr -Werror=conflicts-rr -d "${PARSER}" -o "${WORK_DIR}/parser.tab.cc"
flex -o "${WORK_DIR}/lex.yy.c" "${SCANNER}"

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_beta_0_2_build.out 2>&1 || {
    cat /tmp/shorthand_beta_0_2_build.out >&2 || true
    exit 1
  }
fi

declare -A executed=()
while IFS=$'\t' read -r id area source anchor fixture expectation rationale; do
  [[ "${id}" == "id" ]] && continue
  key="${fixture}|${expectation}"
  [[ -n "${executed[${key}]:-}" ]] && continue
  executed[${key}]=1
  out="${WORK_DIR}/$(echo "${key}" | tr '/|' '__').out"
  if [[ "${expectation}" == "accept" ]]; then
    if ! "${SHORT}" "${ROOT_DIR}/${fixture}" parse >"${out}" 2>&1; then
      echo "error: beta-0.2 valid grammar fixture rejected: ${fixture}" >&2
      cat "${out}" >&2 || true
      exit 1
    fi
  else
    if "${SHORT}" "${ROOT_DIR}/${fixture}" parse >"${out}" 2>&1; then
      echo "error: beta-0.2 invalid grammar fixture accepted: ${fixture}" >&2
      exit 1
    fi
    grep -Fq '[SHD2001]' "${out}" || {
      echo "error: rejected grammar fixture lacks stable parser diagnostic: ${fixture}" >&2
      cat "${out}" >&2 || true
      exit 1
    }
  fi
done <"${MATRIX}"

SEMANTIC_BOUNDARY="${ROOT_DIR}/tests/semantic/invalid/ai_shape_mismatch.short"
"${SHORT}" "${SEMANTIC_BOUNDARY}" parse >"${WORK_DIR}/parse-only.out" 2>&1
if "${SHORT}" "${SEMANTIC_BOUNDARY}" run >"${WORK_DIR}/semantic.out" 2>&1; then
  echo "error: semantic-invalid fixture unexpectedly passed normal validation" >&2
  exit 1
fi
grep -Fq '[SHD4014]' "${WORK_DIR}/semantic.out"

printf 'PASS beta-0.2 grammar and conformance matrix gate\n'
