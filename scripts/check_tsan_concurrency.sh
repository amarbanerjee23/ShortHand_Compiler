#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
RUNTIME_LIB="${BUILD_DIR}/libshorthand_runtime.a"
STRESS_SRC="${ROOT_DIR}/tests/runtime/runtime_tsan_stress.cpp"
WORK_DIR="$(mktemp -d)"
CXX="${SHORTHAND_TSAN_CXX:-clang++}"

command -v "${CXX}" >/dev/null 2>&1 || { echo "error: ${CXX} is required for ThreadSanitizer" >&2; exit 1; }
[[ -f "${STRESS_SRC}" ]] || { echo "error: missing TSan stress source" >&2; exit 1; }

restore_runtime() {
  rm -f "${BUILD_DIR}/ShorthandRuntimeImpl.o" \
        "${BUILD_DIR}/RuntimeThreadSafeFacade.o" \
        "${RUNTIME_LIB}"
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_tsan_restore.out 2>/tmp/shorthand_tsan_restore.err || {
    cat /tmp/shorthand_tsan_restore.out >&2 || true
    cat /tmp/shorthand_tsan_restore.err >&2 || true
    return 1
  }
  rm -rf "${WORK_DIR}"
}
trap restore_runtime EXIT

rm -f "${BUILD_DIR}/ShorthandRuntimeImpl.o" \
      "${BUILD_DIR}/RuntimeThreadSafeFacade.o" \
      "${RUNTIME_LIB}"

make -C "${SRC_DIR}" \
  CXX="${CXX}" \
  CXXFLAGS="-O1 -g -fsanitize=thread -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -std=c++17" \
  runtime_lib >/tmp/shorthand_tsan_runtime_build.out 2>/tmp/shorthand_tsan_runtime_build.err || {
    cat /tmp/shorthand_tsan_runtime_build.out >&2 || true
    cat /tmp/shorthand_tsan_runtime_build.err >&2 || true
    exit 1
  }

"${CXX}" -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -pthread \
  -fsanitize=thread -fno-omit-frame-pointer \
  -I"${SRC_DIR}" \
  "${STRESS_SRC}" "${RUNTIME_LIB}" \
  -o "${WORK_DIR}/runtime_tsan_stress"

TSAN_OPTIONS="halt_on_error=1:exitcode=66:history_size=7:second_deadlock_stack=1" \
  timeout --signal=TERM --kill-after=5 60 \
  "${WORK_DIR}/runtime_tsan_stress" \
  >/tmp/shorthand_tsan_concurrency.out \
  2>/tmp/shorthand_tsan_concurrency.err || {
    status=$?
    echo "error: ThreadSanitizer race stress failed with status ${status}" >&2
    cat /tmp/shorthand_tsan_concurrency.out >&2 || true
    cat /tmp/shorthand_tsan_concurrency.err >&2 || true
    exit "${status}"
  }

if grep -Eqi 'ThreadSanitizer|data race|lock-order-inversion|deadlock' \
  /tmp/shorthand_tsan_concurrency.out /tmp/shorthand_tsan_concurrency.err; then
  echo "error: ThreadSanitizer/race marker found" >&2
  cat /tmp/shorthand_tsan_concurrency.out >&2 || true
  cat /tmp/shorthand_tsan_concurrency.err >&2 || true
  exit 1
fi

grep -Fq 'PASS ThreadSanitizer runtime race stress' /tmp/shorthand_tsan_concurrency.out
cat /tmp/shorthand_tsan_concurrency.out
printf 'PASS ThreadSanitizer concurrency gate\n'
