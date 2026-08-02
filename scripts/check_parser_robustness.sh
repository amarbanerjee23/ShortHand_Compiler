#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
MATRIX="${ROOT_DIR}/tests/parser/robustness/malformed_cases.tsv"
WORK_DIR="$(mktemp -d)"
TIMEOUT_SECONDS="${SHORTHAND_PARSER_TIMEOUT_SECONDS:-3}"
trap 'rm -rf "${WORK_DIR}"' EXIT

pass=0
fail=0

ok() {
  printf 'PASS %s\n' "$*"
  pass=$((pass + 1))
}

bad() {
  printf 'FAIL %s\n' "$*" >&2
  fail=$((fail + 1))
}

require_file() {
  [[ -f "$1" ]] || { bad "missing required file: ${1#${ROOT_DIR}/}"; return 1; }
}

for tool in timeout awk sed cmp head; do
  command -v "${tool}" >/dev/null 2>&1 || {
    echo "error: required parser robustness tool is missing: ${tool}" >&2
    exit 1
  }
done

require_file "${MATRIX}"
if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_parser_robustness_build.out 2>&1 || {
    cat /tmp/shorthand_parser_robustness_build.out >&2 || true
    exit 1
  }
fi

run_bounded() {
  local output="$1"
  shift
  local -a environment=("$@")
  local status

  set +e
  if [[ "${SHORTHAND_PARSER_SANITIZER_MODE:-0}" == "1" ]]; then
    env "${environment[@]}" timeout --signal=TERM --kill-after=1s \
      "${TIMEOUT_SECONDS}s" "${SHORT}" "${CASE_FILE}" parse >"${output}" 2>&1
    status=$?
  else
    env "${environment[@]}" timeout --signal=TERM --kill-after=1s \
      "${TIMEOUT_SECONDS}s" bash -c \
      'ulimit -v 524288; exec "$@"' _ "${SHORT}" "${CASE_FILE}" parse \
      >"${output}" 2>&1
    status=$?
  fi
  set -e
  RUN_STATUS=${status}
}

check_process_safety() {
  local name="$1"
  local status="$2"
  local output="$3"

  if [[ "${status}" -eq 124 || "${status}" -eq 137 || "${status}" -eq 143 ]]; then
    bad "${name} exceeded the bounded execution deadline"
    return 1
  fi
  if [[ "${status}" -ge 128 ]]; then
    bad "${name} terminated by signal with status ${status}"
    return 1
  fi
  if grep -Eq 'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault|Aborted|terminate called' "${output}"; then
    cat "${output}" >&2 || true
    bad "${name} produced a crash or sanitizer marker"
    return 1
  fi
  return 0
}

run_reject_case() {
  local name="$1"
  local file="$2"
  local expected_code="$3"
  shift 3
  local out1="${WORK_DIR}/${name}.first.out"
  local out2="${WORK_DIR}/${name}.second.out"
  local status1 status2

  CASE_FILE="${file}"
  run_bounded "${out1}" "$@"
  status1=${RUN_STATUS}
  check_process_safety "${name}" "${status1}" "${out1}" || return
  if [[ "${status1}" -eq 0 ]]; then
    bad "${name} was unexpectedly accepted"
    return
  fi
  if ! grep -Fq "[${expected_code}]" "${out1}"; then
    cat "${out1}" >&2 || true
    bad "${name} did not emit ${expected_code}"
    return
  fi

  CASE_FILE="${file}"
  run_bounded "${out2}" "$@"
  status2=${RUN_STATUS}
  check_process_safety "${name} repeat" "${status2}" "${out2}" || return
  if [[ "${status1}" -ne "${status2}" ]] || ! cmp -s "${out1}" "${out2}"; then
    diff -u "${out1}" "${out2}" >&2 || true
    bad "${name} rejection was not deterministic"
    return
  fi
  ok "${name} rejected deterministically with ${expected_code}"
}

awk -F '\t' '
  NR == 1 {
    if ($0 != "id\tfixture\texpected_code\tcategory") exit 10
    next
  }
  NF != 4 { exit 11 }
  $1 !~ /^ROB[0-9][0-9][0-9]$/ { exit 12 }
  $3 !~ /^SHD20[0-9][0-9]$/ { exit 13 }
  END { if (NR < 11) exit 14 }
' "${MATRIX}" || {
  echo "error: malformed parser robustness corpus manifest" >&2
  exit 1
}

while IFS=$'\t' read -r id fixture expected_code category; do
  [[ "${id}" == "id" ]] && continue
  file="${ROOT_DIR}/${fixture}"
  require_file "${file}" || continue
  run_reject_case "${id}_${category}" "${file}" "${expected_code}"
done <"${MATRIX}"

# Source-size guard. Test overrides may only lower the production ceiling.
printf 'int x;\n' >"${WORK_DIR}/source_limit.short"
awk 'BEGIN { for (i = 0; i < 256; ++i) printf ";"; printf "\n" }' \
  >>"${WORK_DIR}/source_limit.short"
run_reject_case source_limit "${WORK_DIR}/source_limit.short" SHD2004 \
  SHORTHAND_MAX_SOURCE_BYTES=128

# Individual token guard.
long_name="$(awk 'BEGIN { for (i = 0; i < 96; ++i) printf "a" }')"
printf 'int %s;\n%s = 1;\n' "${long_name}" "${long_name}" \
  >"${WORK_DIR}/token_limit.short"
run_reject_case token_limit "${WORK_DIR}/token_limit.short" SHD2005 \
  SHORTHAND_MAX_TOKEN_BYTES=32

# Scanner-work guard.
printf 'int x;\n' >"${WORK_DIR}/scanner_budget.short"
awk 'BEGIN { for (i = 0; i < 24; ++i) print "x = x + 1;" }' \
  >>"${WORK_DIR}/scanner_budget.short"
run_reject_case scanner_budget "${WORK_DIR}/scanner_budget.short" SHD2006 \
  SHORTHAND_MAX_SCANNER_MATCHES=16

# Lexical delimiter-depth guard.
printf 'int x;\nx = ' >"${WORK_DIR}/nesting_limit.short"
awk 'BEGIN { for (i = 0; i < 12; ++i) printf "("; printf "1"; for (i = 0; i < 12; ++i) printf ")"; print ";" }' \
  >>"${WORK_DIR}/nesting_limit.short"
run_reject_case nesting_limit "${WORK_DIR}/nesting_limit.short" SHD2007 \
  SHORTHAND_MAX_NESTING_DEPTH=8

# Deterministic mutation smoke. Mutations may be accepted or rejected, but they
# must finish within the deadline, avoid signals, and use a stable parser code
# whenever they reject.
printf 'int x;\nx = 1;\n' >"${WORK_DIR}/seed.short"
mutation_count=0
for cut in 0 1 2 4 7 10 13; do
  mutation="${WORK_DIR}/mutation_truncate_${cut}.short"
  head -c "${cut}" "${WORK_DIR}/seed.short" >"${mutation}"
  mutation_count=$((mutation_count + 1))
done
for suffix in '@' '"' '/*' '}' ')' ']' 'infer' 'model {' '&&'; do
  mutation="${WORK_DIR}/mutation_suffix_${mutation_count}.short"
  cat "${WORK_DIR}/seed.short" >"${mutation}"
  printf '%s\n' "${suffix}" >>"${mutation}"
  mutation_count=$((mutation_count + 1))
done
sed 's/;//' "${WORK_DIR}/seed.short" >"${WORK_DIR}/mutation_removed_semicolon.short"
mutation_count=$((mutation_count + 1))

for mutation in "${WORK_DIR}"/mutation_*.short; do
  name="$(basename "${mutation}" .short)"
  CASE_FILE="${mutation}"
  run_bounded "${WORK_DIR}/${name}.out"
  status=${RUN_STATUS}
  check_process_safety "${name}" "${status}" "${WORK_DIR}/${name}.out" || continue
  if [[ "${status}" -ne 0 ]] && ! grep -Eq '\[SHD20[0-9][0-9]\]' "${WORK_DIR}/${name}.out"; then
    cat "${WORK_DIR}/${name}.out" >&2 || true
    bad "${name} rejected without a stable parser or scanner code"
    continue
  fi
  ok "${name} completed safely"
done

if [[ "${mutation_count}" -lt 16 ]]; then
  bad "deterministic mutation corpus is unexpectedly small"
fi

printf 'Parser robustness summary: pass=%d fail=%d mutations=%d\n' \
  "${pass}" "${fail}" "${mutation_count}"
[[ "${fail}" -eq 0 ]]
printf 'PASS parser robustness and negative corpus gate\n'
