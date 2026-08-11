#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/fuzz_sanitizer_race_hardening.md"
FUZZ="${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh"
REPLAY="${ROOT_DIR}/scripts/replay_fuzz_reproducer.sh"
MEMORY="${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh"
TSAN="${ROOT_DIR}/scripts/check_thread_sanitizer.sh"
CI="${ROOT_DIR}/.github/workflows/ci.yml"
NIGHTLY="${ROOT_DIR}/.github/workflows/fuzz.yml"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
HARNESS="${ROOT_DIR}/tests/fuzz/FuzzCompiler.cpp"
RACE="${ROOT_DIR}/tests/runtime/tsan_runtime_stress.cpp"

require_file() {
  [[ -s "$1" ]] || { echo "error: missing or empty PR73 safety file: $1" >&2; exit 1; }
}
require_contains() {
  require_file "$1"
  grep -Fq -- "$2" "$1" || { echo "error: $1 missing PR73 safety contract text: $2" >&2; exit 1; }
}

for file in "${DOC}" "${FUZZ}" "${REPLAY}" "${MEMORY}" "${TSAN}" "${CI}" "${NIGHTLY}" "${MATRIX}" "${HARNESS}" "${RACE}"; do
  require_file "${file}"
done
for script in "${FUZZ}" "${REPLAY}" "${MEMORY}" "${TSAN}"; do bash -n "${script}"; done

for directory in parser module semantic lowering; do
  [[ -d "${ROOT_DIR}/tests/fuzz/corpus/${directory}" ]] || { echo "error: missing fuzz corpus ${directory}" >&2; exit 1; }
  [[ -n "$(find "${ROOT_DIR}/tests/fuzz/corpus/${directory}" -type f -print -quit)" ]] || { echo "error: empty fuzz corpus ${directory}" >&2; exit 1; }
done

for anchor in \
  'fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1' \
  'runtime_memory_sanitizer_contract_version: shorthand.runtime.asan_lsan_ubsan.v1' \
  'runtime_tsan_contract_version: shorthand.runtime.tsan.v1' \
  'production_claim: false' \
  'scripts/check_runtime_memory_sanitizer.sh' \
  'scripts/replay_fuzz_reproducer.sh'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  '-fsanitize=fuzzer-no-link,address,undefined' \
  '-fno-sanitize-recover=all' \
  'detect_leaks=1:halt_on_error=1' \
  'SHORTHAND_FUZZ_SEED' \
  'artifact_prefix=' \
  'PASS libFuzzer ASan LSan UBSan compiler-stage gate'; do
  require_contains "${FUZZ}" "${anchor}"
done
require_contains "${REPLAY}" 'REPLAY_EXECUTED'
require_contains "${REPLAY}" '--minimize'
require_contains "${MEMORY}" '-fsanitize=address,undefined'
require_contains "${MEMORY}" 'detect_leaks=1:halt_on_error=1'
require_contains "${MEMORY}" 'PASS runtime ASan LSan UBSan stress gate'
require_contains "${TSAN}" '-fsanitize=thread'
require_contains "${TSAN}" 'halt_on_error=1'
require_contains "${TSAN}" 'PASS mandatory ThreadSanitizer race gate'

for anchor in \
  'Fuzz ASan LSan UBSan compiler stages' \
  'Runtime ASan LSan UBSan stress' \
  'ThreadSanitizer race stress'; do
  require_contains "${CI}" "${anchor}"
done
require_contains "${NIGHTLY}" 'Extended staged libFuzzer campaign'
require_contains "${NIGHTLY}" 'Runtime ASan LSan UBSan extended stress'
require_contains "${NIGHTLY}" 'SHORTHAND_FUZZ_SEED: ${{ github.run_number }}'
require_contains "${MATRIX}" $'TST008\tfull sanitizer coverage\timplemented'
require_contains "${MATRIX}" $'TST009\tcontinuous fuzzing\timplemented'
require_contains "${MATRIX}" $'TST010\tconcurrency and race detection\timplemented'

if grep -Eq 'continue-on-error:[[:space:]]*true|detect_leaks=0|halt_on_error=0' "${CI}" "${NIGHTLY}" "${FUZZ}" "${MEMORY}" "${TSAN}"; then
  echo "error: PR73 safety contract contains a weakening/false-success setting" >&2
  exit 1
fi

printf 'PASS PR73 fuzz sanitizer and race safety contract guard\n'
