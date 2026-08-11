#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
HARNESS="${ROOT_DIR}/tests/fuzz/FuzzCompiler.cpp"
CORPUS_ROOT="${ROOT_DIR}/tests/fuzz/corpus"
WORK_DIR="$(mktemp -d)"
ARTIFACT_DIR="${SHORTHAND_FUZZ_ARTIFACT_DIR:-/tmp/shorthand_fuzz_artifacts}"
CLANGXX="${SHORTHAND_FUZZ_CXX:-clang++}"
RUNS="${SHORTHAND_FUZZ_RUNS:-256}"
MAX_TOTAL_TIME="${SHORTHAND_FUZZ_MAX_TOTAL_TIME:-0}"
SEED="${SHORTHAND_FUZZ_SEED:-1337}"
MAX_LEN="${SHORTHAND_FUZZ_MAX_LEN:-4096}"
trap 'rm -rf "${WORK_DIR}"' EXIT

for tool in "${CLANGXX}" llvm-config bison flex; do
  command -v "${tool}" >/dev/null 2>&1 || {
    echo "error: required fuzzing tool unavailable: ${tool}" >&2
    exit 1
  }
done

for path in \
  "${HARNESS}" \
  "${CORPUS_ROOT}/parser" \
  "${CORPUS_ROOT}/module" \
  "${CORPUS_ROOT}/semantic" \
  "${CORPUS_ROOT}/lowering"; do
  [[ -e "${path}" ]] || { echo "error: missing fuzz input: ${path}" >&2; exit 1; }
done

mkdir -p "${ARTIFACT_DIR}"
rm -f "${ARTIFACT_DIR}"/* || true

# Ensure the checked-in/generated frontend and source-level lowering are exactly
# the same ones used by the ordinary compiler before building fuzz objects.
make -C "${SRC_DIR}" short_hand >/tmp/shorthand_fuzz_prepare.out 2>/tmp/shorthand_fuzz_prepare.err || {
  cat /tmp/shorthand_fuzz_prepare.out >&2 || true
  cat /tmp/shorthand_fuzz_prepare.err >&2 || true
  exit 1
}

# llvm-config emits linker flags on multiple lines on Ubuntu/LLVM 18. Flatten
# whitespace before array parsing so neither -L nor the LLVM library list is lost.
read -r -a LLVM_CXXFLAGS <<<"$(llvm-config --cxxflags | tr '\n' ' ')"
read -r -a LLVM_LDFLAGS <<<"$(llvm-config --ldflags --system-libs --libs core bitwriter | tr '\n' ' ')"

COMMON_FLAGS=(
  -O1
  -g
  -fno-omit-frame-pointer
  -fno-sanitize-recover=all
  -fsanitize=fuzzer-no-link,address,undefined
  -Wall
  -Wextra
  -Wpedantic
  -std=c++17
  -I"${SRC_DIR}"
)

COMMON_SOURCES=(
  "${SRC_DIR}/parser.tab.cc"
  "${SRC_DIR}/lex.yy.c"
  "${SRC_DIR}/ast/AST.cpp"
  "${SRC_DIR}/module/ModuleResolver.cpp"
  "${SRC_DIR}/visitors/SemanticAnalyzer.cpp"
  "${SRC_DIR}/visitors/Diagnostics.cpp"
  "${SRC_DIR}/visitors/IR_Generator.cpp"
  "${SRC_DIR}/visitors/IR_Generator_Module.cpp"
  "${SRC_DIR}/ai_runtime/AI_Types.cpp"
)

run_build() {
  local label="$1"
  shift
  local out="/tmp/shorthand_fuzz_build_${label}.out"
  local err="/tmp/shorthand_fuzz_build_${label}.err"
  if ! "$@" >"${out}" 2>"${err}"; then
    echo "error: fuzz build command failed: ${label}" >&2
    cat "${out}" >&2 || true
    cat "${err}" >&2 || true
    exit 1
  fi
}

COMMON_OBJECTS=()
index=0
for source in "${COMMON_SOURCES[@]}"; do
  object="${WORK_DIR}/common-${index}.o"
  run_build "common_${index}" \
    "${CLANGXX}" "${LLVM_CXXFLAGS[@]}" "${COMMON_FLAGS[@]}" -c "${source}" -o "${object}"
  COMMON_OBJECTS+=("${object}")
  index=$((index + 1))
done

build_target() {
  local name="$1"
  local stage="$2"
  local harness_object="${WORK_DIR}/${name}-harness.o"
  local binary="${WORK_DIR}/${name}_fuzz"
  run_build "${name}_harness" \
    "${CLANGXX}" "${LLVM_CXXFLAGS[@]}" "${COMMON_FLAGS[@]}" \
    -DSHORTHAND_FUZZ_STAGE="${stage}" -c "${HARNESS}" -o "${harness_object}"
  run_build "${name}_link" \
    "${CLANGXX}" -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all \
    "${harness_object}" "${COMMON_OBJECTS[@]}" "${LLVM_LDFLAGS[@]}" -lfl -o "${binary}"
}

build_target parser 1
build_target module 2
build_target semantic 3
build_target lowering 4

run_target() {
  local name="$1"
  local binary="${WORK_DIR}/${name}_fuzz"
  local corpus="${CORPUS_ROOT}/${name}"
  local log="/tmp/shorthand_fuzz_${name}.out"
  local -a budget
  if [[ "${MAX_TOTAL_TIME}" =~ ^[0-9]+$ ]] && (( MAX_TOTAL_TIME > 0 )); then
    budget=(-max_total_time="${MAX_TOTAL_TIME}")
  else
    budget=(-runs="${RUNS}")
  fi

  set +e
  ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" \
  UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
  "${binary}" "${corpus}" \
    "${budget[@]}" \
    -seed="${SEED}" \
    -max_len="${MAX_LEN}" \
    -timeout=5 \
    -rss_limit_mb=2048 \
    -artifact_prefix="${ARTIFACT_DIR}/${name}-" \
    >"${log}" 2>&1
  local status=$?
  set -e

  cat "${log}"
  if (( status != 0 )); then
    echo "error: ${name} fuzz target failed with status ${status}" >&2
    echo "FUZZ_REPRO target=${name} seed=${SEED} artifact_dir=${ARTIFACT_DIR}" >&2
    find "${ARTIFACT_DIR}" -maxdepth 1 -type f -printf 'FUZZ_ARTIFACT %p\n' >&2 || true
    exit "${status}"
  fi

  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|SUMMARY:.*Sanitizer|ERROR: libFuzzer' "${log}"; then
    echo "error: sanitizer/fuzzer marker found despite zero exit: ${name}" >&2
    exit 1
  fi
  printf 'PASS fuzz target=%s seed=%s\n' "${name}" "${SEED}"
}

run_target parser
run_target module
run_target semantic
run_target lowering

printf 'FUZZ_SANITIZER contract=shorthand.fuzz.sanitizers.v1 seed=%s runs=%s max_total_time=%s max_len=%s\n' \
  "${SEED}" "${RUNS}" "${MAX_TOTAL_TIME}" "${MAX_LEN}"
printf 'PASS libFuzzer ASan LSan UBSan compiler-stage gate\n'
