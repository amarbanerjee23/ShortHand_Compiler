#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
SHORT="${SHORTHAND_BIN:-${BUILD_DIR}/short_hand}"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${BUILD_DIR}/libshorthand_runtime.a}"
FIXTURE_DIR="${ROOT_DIR}/tests/semantic/differential"
WORK_DIR="$(mktemp -d)"
ARTIFACT="${SHORTHAND_DIFFERENTIAL_ARTIFACT:-/tmp/shorthand_semantic_differential.json}"
trap 'rm -rf "${WORK_DIR}"' EXIT

canonicalize_existing_parent() {
  local value="$1"
  if [[ "${value}" == /* ]]; then
    printf '%s\n' "${value}"
    return 0
  fi
  local parent
  parent="$(cd "$(dirname "${value}")" 2>/dev/null && pwd)" || return 1
  printf '%s/%s\n' "${parent}" "$(basename "${value}")"
}

SHORT="$(canonicalize_existing_parent "${SHORT}")" || {
  echo "error: unable to canonicalize SHORTHAND_BIN: ${SHORT}" >&2
  exit 1
}
RUNTIME_LIB="$(canonicalize_existing_parent "${RUNTIME_LIB}")" || {
  echo "error: unable to canonicalize SHORTHAND_RUNTIME_LIB: ${RUNTIME_LIB}" >&2
  exit 1
}

write_failure() {
  local case_name="$1"
  local mode="$2"
  local reason="$3"
  printf '{"schema":"shorthand.semantic.differential.v2","status":"fail","case":"%s","mode":"%s","reason":"%s"}\n' \
    "${case_name}" "${mode}" "${reason//\"/\\\"}" >"${ARTIFACT}"
}

fail_case() {
  local case_name="$1"
  local mode="$2"
  local reason="$3"
  write_failure "${case_name}" "${mode}" "${reason}"
  echo "error: ${case_name} ${mode}: ${reason}" >&2
  return 1
}

for command in timeout lli llc clang++; do
  command -v "${command}" >/dev/null 2>&1 || {
    write_failure infrastructure "${command}" "required tool missing"
    echo "error: semantic differential gate requires ${command}" >&2
    exit 1
  }
done

for file in \
  "${FIXTURE_DIR}/core_control.short" \
  "${FIXTURE_DIR}/core_control.expected" \
  "${FIXTURE_DIR}/production_types.short" \
  "${FIXTURE_DIR}/production_types.expected" \
  "${FIXTURE_DIR}/type_mismatch.short" \
  "${FIXTURE_DIR}/invalid_string_operator.short" \
  "${FIXTURE_DIR}/invalid_array_index_type.short" \
  "${FIXTURE_DIR}/invalid_zero_array.short" \
  "${FIXTURE_DIR}/invalid_string_condition.short" \
  "${FIXTURE_DIR}/greenai_float_rejected.short" \
  "${FIXTURE_DIR}/unsupported_owned_string_array.short" \
  "${FIXTURE_DIR}/float_division_by_zero.short" \
  "${FIXTURE_DIR}/float_array_bounds.short" \
  "${FIXTURE_DIR}/wrong_arity.short" \
  "${FIXTURE_DIR}/return_outside_function.short" \
  "${FIXTURE_DIR}/goto_rejected.short" \
  "${FIXTURE_DIR}/undeclared_variable.short" \
  "${FIXTURE_DIR}/return_type_mismatch.short" \
  "${FIXTURE_DIR}/division_by_zero.short" \
  "${FIXTURE_DIR}/array_bounds.short"; do
  [[ -f "${file}" ]] || {
    write_failure infrastructure fixture "missing ${file#${ROOT_DIR}/}"
    echo "error: missing semantic differential fixture: ${file}" >&2
    exit 1
  }
done

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_semantic_differential_make.out 2>&1 || {
    cat /tmp/shorthand_semantic_differential_make.out >&2 || true
    write_failure infrastructure compiler "compiler build failed"
    exit 1
  }
fi
if [[ ! -f "${RUNTIME_LIB}" ]]; then
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_semantic_differential_runtime_make.out 2>&1 || {
    cat /tmp/shorthand_semantic_differential_runtime_make.out >&2 || true
    write_failure infrastructure runtime "runtime library build failed"
    exit 1
  }
fi
[[ -x "${SHORT}" ]] || { write_failure infrastructure compiler "compiler unavailable"; exit 1; }
[[ -f "${RUNTIME_LIB}" ]] || { write_failure infrastructure runtime "runtime library unavailable"; exit 1; }

run_valid_interpreter() {
  local fixture="$1"
  local output="$2"
  timeout --signal=TERM --kill-after=2 15 "${SHORT}" "${fixture}" run >"${output}" 2>"${output}.err" || {
    cat "${output}.err" >&2 || true
    fail_case "$(basename "${fixture}")" interpreter "expected successful execution"
  }
}

run_valid_lli() {
  local fixture="$1"
  local output="$2"
  local case_dir="${WORK_DIR}/valid-lli"
  mkdir -p "${case_dir}"
  (
    cd "${case_dir}"
    timeout --signal=TERM --kill-after=2 20 "${SHORT}" "${fixture}" compile-bc >compile.out 2>compile.err
    [[ -s "$(basename "${fixture}" .short).bc" ]]
    timeout --signal=TERM --kill-after=2 15 lli "$(basename "${fixture}" .short).bc" >"${output}" 2>"${output}.err"
  ) || {
    cat "${case_dir}/compile.err" >&2 2>/dev/null || true
    cat "${output}.err" >&2 2>/dev/null || true
    fail_case "$(basename "${fixture}")" lli "compile or execution failed"
  }
}

run_valid_native() {
  local fixture="$1"
  local output="$2"
  local case_dir="${WORK_DIR}/valid-native"
  mkdir -p "${case_dir}"
  (
    cd "${case_dir}"
    SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout --signal=TERM --kill-after=2 30 \
      "${SHORT}" "${fixture}" compile-native >compile.out 2>compile.err
    timeout --signal=TERM --kill-after=2 15 "./$(basename "${fixture}" .short)" >"${output}" 2>"${output}.err"
  ) || {
    cat "${case_dir}/compile.err" >&2 2>/dev/null || true
    cat "${output}.err" >&2 2>/dev/null || true
    fail_case "$(basename "${fixture}")" native "compile or execution failed"
  }
}

CORE="${FIXTURE_DIR}/core_control.short"
run_valid_interpreter "${CORE}" "${WORK_DIR}/core.interpreter.out"
run_valid_lli "${CORE}" "${WORK_DIR}/core.lli.out"
run_valid_native "${CORE}" "${WORK_DIR}/core.native.out"

for mode in interpreter lli native; do
  if ! cmp -s "${FIXTURE_DIR}/core_control.expected" "${WORK_DIR}/core.${mode}.out"; then
    diff -u "${FIXTURE_DIR}/core_control.expected" "${WORK_DIR}/core.${mode}.out" >&2 || true
    fail_case core_control "${mode}" "observable stdout differs from golden contract"
    exit 1
  fi
done

if ! cmp -s "${WORK_DIR}/core.interpreter.out" "${WORK_DIR}/core.lli.out" ||
   ! cmp -s "${WORK_DIR}/core.interpreter.out" "${WORK_DIR}/core.native.out"; then
  fail_case core_control differential "interpreter, lli and native stdout differ"
  exit 1
fi

TYPES="${FIXTURE_DIR}/production_types.short"
run_valid_interpreter "${TYPES}" "${WORK_DIR}/types.interpreter.out"
run_valid_lli "${TYPES}" "${WORK_DIR}/types.lli.out"
run_valid_native "${TYPES}" "${WORK_DIR}/types.native.out"

for mode in interpreter lli native; do
  if ! cmp -s "${FIXTURE_DIR}/production_types.expected" "${WORK_DIR}/types.${mode}.out"; then
    diff -u "${FIXTURE_DIR}/production_types.expected" "${WORK_DIR}/types.${mode}.out" >&2 || true
    fail_case production_types "${mode}" "observable typed stdout differs from golden contract"
    exit 1
  fi
done

if ! cmp -s "${WORK_DIR}/types.interpreter.out" "${WORK_DIR}/types.lli.out" ||
   ! cmp -s "${WORK_DIR}/types.interpreter.out" "${WORK_DIR}/types.native.out"; then
  fail_case production_types differential "interpreter, lli and native typed stdout differ"
  exit 1
fi

expect_semantic_failure() {
  local fixture="$1"
  local expected="$2"
  local case_name
  case_name="$(basename "${fixture}" .short)"
  for mode in run compile-bc compile-native; do
    local output="${WORK_DIR}/${case_name}.${mode}.semantic"
    if SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout --signal=TERM --kill-after=2 20 \
         "${SHORT}" "${fixture}" "${mode}" >"${output}.out" 2>"${output}.err"; then
      fail_case "${case_name}" "${mode}" "semantic-negative program unexpectedly succeeded"
      return 1
    fi
    if ! grep -Fq "[${expected}]" "${output}.err"; then
      cat "${output}.err" >&2 || true
      fail_case "${case_name}" "${mode}" "expected ${expected}"
      return 1
    fi
    if ! grep -Fq '[range ' "${output}.err"; then
      cat "${output}.err" >&2 || true
      fail_case "${case_name}" "${mode}" "semantic diagnostic missing source range"
      return 1
    fi
  done
}

expect_semantic_failure "${FIXTURE_DIR}/unsupported_owned_string_array.short" SHD3003
expect_semantic_failure "${FIXTURE_DIR}/type_mismatch.short" SHD3013
expect_semantic_failure "${FIXTURE_DIR}/invalid_string_operator.short" SHD3014
expect_semantic_failure "${FIXTURE_DIR}/invalid_array_index_type.short" SHD3013
expect_semantic_failure "${FIXTURE_DIR}/invalid_zero_array.short" SHD3010
expect_semantic_failure "${FIXTURE_DIR}/invalid_string_condition.short" SHD3015
expect_semantic_failure "${FIXTURE_DIR}/greenai_float_rejected.short" SHD3013
expect_semantic_failure "${FIXTURE_DIR}/wrong_arity.short" SHD3004
expect_semantic_failure "${FIXTURE_DIR}/return_outside_function.short" SHD3005
expect_semantic_failure "${FIXTURE_DIR}/goto_rejected.short" SHD3006
expect_semantic_failure "${FIXTURE_DIR}/return_type_mismatch.short" SHD3007
expect_semantic_failure "${FIXTURE_DIR}/undeclared_variable.short" SHD3008

cat >"${WORK_DIR}/loop_step_zero.short" <<'EOF'
int i,step,total;
i = 0;
step = 0;
total = 0;
loop i = 0, step, 3 {
    total = total + i;
}
print "total", total;
EOF

expect_runtime_failure() {
  local fixture="$1"
  local expected="$2"
  local case_name
  case_name="$(basename "${fixture}" .short)"

  local interpreter="${WORK_DIR}/${case_name}.runtime.interpreter"
  if timeout --signal=TERM --kill-after=2 15 "${SHORT}" "${fixture}" run >"${interpreter}.out" 2>"${interpreter}.err"; then
    fail_case "${case_name}" interpreter "runtime-negative program unexpectedly succeeded"
    return 1
  fi
  grep -Fq "[${expected}]" "${interpreter}.err" || {
    cat "${interpreter}.err" >&2 || true
    fail_case "${case_name}" interpreter "expected runtime code ${expected}"
    return 1
  }

  local lli_dir="${WORK_DIR}/${case_name}.lli"
  mkdir -p "${lli_dir}"
  (
    cd "${lli_dir}"
    timeout --signal=TERM --kill-after=2 20 "${SHORT}" "${fixture}" compile-bc >compile.out 2>compile.err
  ) || {
    cat "${lli_dir}/compile.err" >&2 || true
    fail_case "${case_name}" lli "runtime-negative fixture failed to compile"
    return 1
  }
  if timeout --signal=TERM --kill-after=2 15 lli "${lli_dir}/${case_name}.bc" >"${lli_dir}/run.out" 2>"${lli_dir}/run.err"; then
    fail_case "${case_name}" lli "runtime-negative bitcode unexpectedly succeeded"
    return 1
  fi
  grep -Fq "[${expected}]" "${lli_dir}/run.err" || {
    cat "${lli_dir}/run.err" >&2 || true
    fail_case "${case_name}" lli "expected runtime code ${expected}"
    return 1
  }

  local native_dir="${WORK_DIR}/${case_name}.native"
  mkdir -p "${native_dir}"
  (
    cd "${native_dir}"
    SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" timeout --signal=TERM --kill-after=2 30 \
      "${SHORT}" "${fixture}" compile-native >compile.out 2>compile.err
  ) || {
    cat "${native_dir}/compile.err" >&2 || true
    fail_case "${case_name}" native "runtime-negative fixture failed to compile"
    return 1
  }
  if timeout --signal=TERM --kill-after=2 15 "${native_dir}/${case_name}" >"${native_dir}/run.out" 2>"${native_dir}/run.err"; then
    fail_case "${case_name}" native "runtime-negative binary unexpectedly succeeded"
    return 1
  fi
  grep -Fq "[${expected}]" "${native_dir}/run.err" || {
    cat "${native_dir}/run.err" >&2 || true
    fail_case "${case_name}" native "expected runtime code ${expected}"
    return 1
  }
}

expect_runtime_failure "${FIXTURE_DIR}/division_by_zero.short" SHD7001
expect_runtime_failure "${FIXTURE_DIR}/array_bounds.short" SHD7002
expect_runtime_failure "${FIXTURE_DIR}/float_division_by_zero.short" SHD7001
expect_runtime_failure "${FIXTURE_DIR}/float_array_bounds.short" SHD7002
expect_runtime_failure "${WORK_DIR}/loop_step_zero.short" SHD7003

for output in "${WORK_DIR}"/*.out "${WORK_DIR}"/*.err "${WORK_DIR}"/*/*.out "${WORK_DIR}"/*/*.err; do
  [[ -f "${output}" ]] || continue
  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' "${output}"; then
    cat "${output}" >&2 || true
    fail_case sanitizer corpus "sanitizer or crash marker detected"
    exit 1
  fi
done

printf '{"schema":"shorthand.semantic.differential.v2","status":"pass","engines":["interpreter","lli","native"],"valid_cases":2,"semantic_negative_cases":12,"runtime_negative_cases":5}\n' >"${ARTIFACT}"
printf 'PASS cross-mode semantic differential execution gate\n'
