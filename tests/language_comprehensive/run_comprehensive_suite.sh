#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SUITE_DIR="${ROOT_DIR}/tests/language_comprehensive"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/build/short_hand}"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${ROOT_DIR}/build/libshorthand_runtime.a}"

[[ -x "${SHORT}" ]] || { echo "error: short_hand binary not found at ${SHORT}" >&2; exit 1; }
for tool in lli llc clang++ timeout; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: missing required tool ${tool}" >&2; exit 1; }
done

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

pass=0
fail=0

has_crash_or_sanitizer() {
  grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|SUMMARY:.*Sanitizer|Segmentation fault|core dumped' "$1" 2>/dev/null
}

record_pass() {
  printf 'PASS %s\n' "$1"
  pass=$((pass + 1))
}

record_fail() {
  printf 'FAIL %s: %s\n' "$1" "$2" >&2
  fail=$((fail + 1))
}

# 1. Positive cross-mode parity tests (Interpreter == LLI == Native)
for src in "${SUITE_DIR}"/positive/*.short; do
  name="$(basename "${src}" .short)"
  stdin_file="${SUITE_DIR}/positive/${name}.stdin"
  [[ -f "${stdin_file}" ]] || stdin_file="/dev/null"
  case_dir="${WORK}/pos/${name}"
  mkdir -p "${case_dir}"

  if ! timeout 15 "${SHORT}" "${src}" run <"${stdin_file}" >"${case_dir}/interp.out" 2>"${case_dir}/interp.err"; then
    record_fail "${name}" "interpreter failed ($(cat "${case_dir}/interp.err"))"
    continue
  fi

  if ! (cd "${case_dir}" && timeout 20 "${SHORT}" "${src}" compile-bc >bc.out 2>bc.err && timeout 15 lli "${name}.bc" <"${stdin_file}" >lli.out 2>lli.err); then
    record_fail "${name}" "lli failed"
    continue
  fi

  if ! (cd "${case_dir}" && SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout 30 "${SHORT}" "${src}" compile-native >nat.out 2>nat.err && timeout 15 "./${name}" <"${stdin_file}" >native.out 2>native.err); then
    record_fail "${name}" "native failed"
    continue
  fi

  if ! diff -u "${case_dir}/interp.out" "${case_dir}/lli.out" >/dev/null; then
    record_fail "${name}" "interpreter vs lli output mismatch"
    diff -u "${case_dir}/interp.out" "${case_dir}/lli.out" >&2 || true
    continue
  fi
  if ! diff -u "${case_dir}/interp.out" "${case_dir}/native.out" >/dev/null; then
    record_fail "${name}" "interpreter vs native output mismatch"
    diff -u "${case_dir}/interp.out" "${case_dir}/native.out" >&2 || true
    continue
  fi

  record_pass "positive/${name} (3-way parity)"
done

# 2. Runtime domain/bounds error tests (must fail cleanly in all 3 modes)
for src in "${SUITE_DIR}"/runtime_errors/*.short; do
  name="$(basename "${src}" .short)"
  case_dir="${WORK}/rt/${name}"
  mkdir -p "${case_dir}"
  ok_rt=1

  if timeout 15 "${SHORT}" "${src}" run >"${case_dir}/interp.out" 2>"${case_dir}/interp.err"; then
    record_fail "runtime/${name}" "interpreter unexpectedly succeeded"
    ok_rt=0
  elif has_crash_or_sanitizer "${case_dir}/interp.err"; then
    record_fail "runtime/${name}" "interpreter crashed/sanitizer"
    ok_rt=0
  fi

  if [[ "${ok_rt}" -eq 1 ]]; then
    if (cd "${case_dir}" && timeout 20 "${SHORT}" "${src}" compile-bc >bc.out 2>bc.err && timeout 15 lli "${name}.bc" >lli.out 2>lli.err); then
      record_fail "runtime/${name}" "lli unexpectedly succeeded"
      ok_rt=0
    elif has_crash_or_sanitizer "${case_dir}/lli.err"; then
      record_fail "runtime/${name}" "lli crashed/sanitizer"
      ok_rt=0
    fi
  fi

  if [[ "${ok_rt}" -eq 1 ]]; then
    if (cd "${case_dir}" && SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout 30 "${SHORT}" "${src}" compile-native >nat.out 2>nat.err && timeout 15 "./${name}" >native.out 2>native.err); then
      record_fail "runtime/${name}" "native unexpectedly succeeded"
      ok_rt=0
    elif has_crash_or_sanitizer "${case_dir}/native.err"; then
      record_fail "runtime/${name}" "native crashed/sanitizer"
      ok_rt=0
    fi
  fi

  if [[ "${ok_rt}" -eq 1 ]]; then
    record_pass "runtime/${name} (3-way clean failure)"
  fi
done

# 3. Negative parser/semantic tests (must reject cleanly without crash/sanitizer)
for src in "${SUITE_DIR}"/negative/*.short; do
  name="$(basename "${src}" .short)"
  case_dir="${WORK}/neg/${name}"
  mkdir -p "${case_dir}"

  if timeout 15 "${SHORT}" "${src}" run >"${case_dir}/run.out" 2>"${case_dir}/run.err"; then
    record_fail "negative/${name}" "compiler unexpectedly accepted invalid program"
  elif has_crash_or_sanitizer "${case_dir}/run.err"; then
    record_fail "negative/${name}" "sanitizer or segfault on invalid input"
  else
    record_pass "negative/${name} (rejected cleanly)"
  fi
done

printf 'Comprehensive suite summary: pass=%d fail=%d\n' "${pass}" "${fail}"
[[ "${fail}" -eq 0 ]]
