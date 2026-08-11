#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
SHORT="${BUILD_DIR}/short_hand"
FUZZ_SRC="${ROOT_DIR}/tests/fuzz/FuzzSubprocess.cpp"
FUZZ_BUILD="${BUILD_DIR}/fuzz"
ARTIFACT_ROOT="/tmp/shorthand_fuzz_artifacts"
RUNS="${SHORTHAND_FUZZ_RUNS:-64}"
SEED="${SHORTHAND_FUZZ_SEED:-73073}"
CXX="${SHORTHAND_FUZZ_CXX:-clang++}"

command -v "${CXX}" >/dev/null 2>&1 || { echo "error: ${CXX} is required for libFuzzer" >&2; exit 1; }
command -v timeout >/dev/null 2>&1 || { echo "error: timeout is required for bounded fuzz execution" >&2; exit 1; }
[[ -f "${FUZZ_SRC}" ]] || { echo "error: missing fuzz driver: ${FUZZ_SRC}" >&2; exit 1; }
[[ "${RUNS}" =~ ^[0-9]+$ ]] && [[ "${RUNS}" -gt 0 ]] || { echo "error: SHORTHAND_FUZZ_RUNS must be a positive integer" >&2; exit 1; }

restore_normal_build() {
  rm -f "${BUILD_DIR}/short_hand"
  make -C "${SRC_DIR}" compiler >/tmp/shorthand_fuzz_restore.out 2>/tmp/shorthand_fuzz_restore.err || {
    cat /tmp/shorthand_fuzz_restore.out >&2 || true
    cat /tmp/shorthand_fuzz_restore.err >&2 || true
    return 1
  }
}
trap restore_normal_build EXIT

rm -rf "${FUZZ_BUILD}" "${ARTIFACT_ROOT}"
mkdir -p "${FUZZ_BUILD}" "${ARTIFACT_ROOT}"

# Instrument the real compiler. On Linux, ASan's leak detector provides the LSan
# contract while UBSan is configured to halt on the first undefined operation.
rm -f "${SHORT}"
make -C "${SRC_DIR}" \
  CXX="${CXX}" \
  CXXFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -std=c++17" \
  short_hand >/tmp/shorthand_fuzz_compiler_build.out 2>/tmp/shorthand_fuzz_compiler_build.err || {
    cat /tmp/shorthand_fuzz_compiler_build.out >&2 || true
    cat /tmp/shorthand_fuzz_compiler_build.err >&2 || true
    exit 1
  }

[[ -x "${SHORT}" ]] || { echo "error: sanitized compiler was not produced" >&2; exit 1; }

stages=(scanner parser semantic module lowering)
for index in "${!stages[@]}"; do
  stage="${stages[$index]}"
  corpus="${ROOT_DIR}/tests/fuzz/corpus/${stage}"
  binary="${FUZZ_BUILD}/fuzz_${stage}"
  artifacts="${ARTIFACT_ROOT}/${stage}"
  out="/tmp/shorthand_fuzz_${stage}.out"
  err="/tmp/shorthand_fuzz_${stage}.err"
  mkdir -p "${artifacts}"
  [[ -d "${corpus}" ]] || { echo "error: missing ${stage} fuzz corpus" >&2; exit 1; }

  "${CXX}" -std=c++17 -O1 -g -Wall -Wextra -Wpedantic \
    -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer \
    -DSHORTHAND_FUZZ_STAGE="${index}" \
    "${FUZZ_SRC}" -o "${binary}"

  ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:allocator_may_return_null=1" \
  UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
  SHORTHAND_FUZZ_BIN="${SHORT}" \
  timeout --signal=TERM --kill-after=5 90 \
    "${binary}" "${corpus}" \
      -runs="${RUNS}" \
      -seed="${SEED}" \
      -max_len=4096 \
      -timeout=4 \
      -rss_limit_mb=2048 \
      -artifact_prefix="${artifacts}/" \
      >"${out}" 2>"${err}" || {
        status=$?
        echo "error: ${stage} fuzz target failed with status ${status}" >&2
        cat "${out}" >&2 || true
        cat "${err}" >&2 || true
        find "${artifacts}" -maxdepth 1 -type f -print >&2 || true
        exit "${status}"
      }

  if grep -Eqi 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault|core dumped|ERROR: libFuzzer' "${out}" "${err}"; then
    echo "error: sanitizer/fuzzer marker found in ${stage} fuzz output" >&2
    cat "${out}" >&2 || true
    cat "${err}" >&2 || true
    exit 1
  fi
  printf 'PASS fuzz stage=%s runs=%s seed=%s\n' "${stage}" "${RUNS}" "${SEED}"
done

{
  echo 'schema=shorthand.fuzz.safety.v1'
  echo "runs_per_stage=${RUNS}"
  echo "seed=${SEED}"
  echo 'sanitizers=address,leak,undefined'
  echo 'stages=scanner,parser,semantic,module,lowering'
  echo 'result=pass'
} >/tmp/shorthand_fuzz_summary.out

cat /tmp/shorthand_fuzz_summary.out
printf 'PASS coverage-guided sanitizer fuzz gate\n'
