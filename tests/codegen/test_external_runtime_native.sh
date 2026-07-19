#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
WORK_DIR="/tmp/shorthand_external_runtime_native"
SHORT_BIN="${SHORTHAND_BIN:-${BUILD_DIR}/short_hand}"
FIXTURE="${ROOT_DIR}/tests/fixtures/external_runtime_ai.short"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${BUILD_DIR}/libshorthand_runtime.a}"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

case "${SHORT_BIN}" in
  /*) ;;
  *) SHORT_BIN="$(cd "${SRC_DIR}" && mkdir -p "$(dirname "${SHORT_BIN}")" && cd "$(dirname "${SHORT_BIN}")" && pwd)/$(basename "${SHORT_BIN}")" ;;
esac

case "${RUNTIME_LIB}" in
  /*) ;;
  *) RUNTIME_LIB="$(cd "${SRC_DIR}" && mkdir -p "$(dirname "${RUNTIME_LIB}")" && cd "$(dirname "${RUNTIME_LIB}")" && pwd)/$(basename "${RUNTIME_LIB}")" ;;
esac

make -C "${SRC_DIR}" short_hand runtime_lib >/tmp/shorthand_default_runtime_make.out 2>&1

test -x "${SHORT_BIN}"
test -s "${RUNTIME_LIB}"
grep -Fq 'IR_Generator.default_runtime.cpp' "${SRC_DIR}/Makefile"
grep -Fq 'precision float;' "${FIXTURE}"

(
  cd "${WORK_DIR}"
  if ! SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" "${SHORT_BIN}" "${FIXTURE}" compile >/tmp/shorthand_external_compile.out 2>&1; then
    echo "FAIL default runtime fixture did not compile" >&2
    cat /tmp/shorthand_external_compile.out >&2 || true
    exit 1
  fi
  grep -Fq 'declare i32 @short_ai_register_model' external_runtime_ai.ir
  grep -Fq 'declare i32 @short_ai_infer' external_runtime_ai.ir
  if grep -Fq 'define i32 @short_ai_register_model' external_runtime_ai.ir; then
    echo "FAIL generated IR still defines local short_ai_register_model stub" >&2
    exit 1
  fi

  if ! SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" "${SHORT_BIN}" "${FIXTURE}" compile-native >/tmp/shorthand_external_native.out 2>&1; then
    echo "FAIL default runtime fixture did not compile-native" >&2
    cat /tmp/shorthand_external_native.out >&2 || true
    exit 1
  fi
  ./external_runtime_ai >/tmp/shorthand_external_runtime_run.out 2>&1
)

grep -Fq 'Linked ShortHand runtime library' /tmp/shorthand_external_native.out
grep -Fq 'Native linker:' /tmp/shorthand_external_native.out
grep -Fq '[shorthand-runtime] model name=classifier' /tmp/shorthand_external_runtime_run.out
grep -Fq '[shorthand-runtime] infer model=classifier input=input output=output' /tmp/shorthand_external_runtime_run.out

echo "PASS default external runtime native linking"
