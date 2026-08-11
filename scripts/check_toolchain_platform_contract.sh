#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CI="${SHORTHAND_CI_FILE:-${ROOT_DIR}/.github/workflows/ci.yml}"
DOC="${SHORTHAND_PORTABILITY_DOC:-${ROOT_DIR}/docs/toolchain_platform_reproducibility.md}"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"

require_file() {
  [[ -s "$1" ]] || { echo "error: missing or empty PR75 portability file: $1" >&2; exit 1; }
}
require_contains() {
  require_file "$1"
  grep -Fq -- "$2" "$1" || { echo "error: $1 missing PR75 portability contract text: $2" >&2; exit 1; }
}

for file in \
  "${CI}" "${DOC}" \
  "${ROOT_DIR}/scripts/apply_external_runtime_to_ir_source.sh" \
  "${ROOT_DIR}/scripts/check_cmake_ctest_parity.sh" \
  "${ROOT_DIR}/scripts/check_reproducible_builds.sh" \
  "${ROOT_DIR}/scripts/check_installed_consumer_cmake.sh" \
  "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" \
  "${ROOT_DIR}/tests/ci/test_cmake_ctest_parity_negative.sh" \
  "${ROOT_DIR}/tests/ci/test_parser_diagnostic_stream_portability.sh" \
  "${ROOT_DIR}/tests/codegen/test_external_runtime_source_guard_negative.sh" \
  "${ROOT_DIR}/tests/ctest_parity/CMakeLists.txt" \
  "${ROOT_DIR}/tests/ctest_parity/expected_make_targets.txt" \
  "${ROOT_DIR}/tests/packaging/installed_consumer/CMakeLists.txt" \
  "${ROOT_DIR}/tests/packaging/installed_consumer/runtime_consumer.cpp" \
  "${ROOT_DIR}/tests/packaging/installed_consumer/abi_v1_consumer.cpp" \
  "${ROOT_DIR}/tests/packaging/installed_consumer/bridge_consumer.cpp"; do
  require_file "${file}"
done

for script in \
  "${ROOT_DIR}/scripts/apply_external_runtime_to_ir_source.sh" \
  "${ROOT_DIR}/scripts/check_cmake_ctest_parity.sh" \
  "${ROOT_DIR}/scripts/check_reproducible_builds.sh" \
  "${ROOT_DIR}/scripts/check_installed_consumer_cmake.sh" \
  "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" \
  "${ROOT_DIR}/tests/ci/test_cmake_ctest_parity_negative.sh" \
  "${ROOT_DIR}/tests/ci/test_parser_diagnostic_stream_portability.sh" \
  "${ROOT_DIR}/tests/codegen/test_external_runtime_source_guard_negative.sh"; do
  bash -n "${script}"
done

for anchor in \
  'toolchain_platform_contract_version: shorthand.portability.reproducibility.v1' \
  'ctest_parity_contract_version: shorthand.ctest.make-parity.v1' \
  'installed_consumer_contract_version: shorthand.installed.consumer.v1' \
  'reproducible_build_contract_version: shorthand.reproducible.build.v1' \
  'production_claim: false' \
  'ci / ubuntu (push)' \
  'ci / ubuntu (pull_request)'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'ubuntu-core:' \
  'toolchain:' \
  'linux-arm64:' \
  'macos-arm64:' \
  'windows-x64:' \
  'ctest-parity:' \
  'reproducible:' \
  'ubuntu:' \
  'ubuntu-24.04-arm' \
  'macos-15' \
  'windows-2025' \
  'gcc-12' \
  'gcc-14' \
  'clang-16' \
  'clang-18' \
  'check_cmake_ctest_parity.sh' \
  'check_reproducible_builds.sh' \
  'check_installed_consumer_cmake.sh' \
  'check_installed_sdk_lifecycle.sh' \
  'compile-native' \
  'SHD7001' \
  'Publish event-specific CI status' \
  'ci / ubuntu (%s)'; do
  require_contains "${CI}" "${anchor}"
done

publisher_count="$(grep -Fc -- '- name: Publish event-specific CI status' "${CI}")"
[[ "${publisher_count}" -eq 1 ]] || {
  echo "error: exactly one aggregate CI status publisher is required, found ${publisher_count}" >&2
  exit 1
}

if grep -Eq 'continue-on-error:[[:space:]]*true' "${CI}"; then
  echo "error: portability CI must not convert mandatory failures to success" >&2
  exit 1
fi
if grep -Eq 'fail-fast:[[:space:]]*true' "${CI}"; then
  echo "error: portability matrices must retain all failure evidence instead of stopping after the first dimension" >&2
  exit 1
fi

if grep -Eq 'python(3)?[[:space:]]+-' "${ROOT_DIR}/scripts/apply_external_runtime_to_ir_source.sh"; then
  echo "error: runtime source lowering must not require Python during compiler builds" >&2
  exit 1
fi
require_contains "${ROOT_DIR}/scripts/apply_external_runtime_to_ir_source.sh" 'PASS external runtime source lowering is canonical, target-aware and Python-free'
require_contains "${ROOT_DIR}/scripts/check_cmake_ctest_parity.sh" 'PASS CMake CTest parity gate'
require_contains "${ROOT_DIR}/scripts/check_reproducible_builds.sh" 'PASS clean reproducible build gate'
require_contains "${ROOT_DIR}/scripts/check_installed_consumer_cmake.sh" 'PASS installed ShortHand CMake consumer gate'
require_contains "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" 'PASS installed ShortHand SDK install/reinstall/uninstall lifecycle'

bash "${ROOT_DIR}/tests/codegen/test_external_runtime_source_guard_negative.sh"
bash "${ROOT_DIR}/tests/ci/test_parser_diagnostic_stream_portability.sh"

# The coverage matrix must keep these PR75-owned areas explicit. During branch
# development they may be partial; a merge-ready PR promotes them only when the
# corresponding mandatory CI jobs are actually green.
for id in TST013 TST014 TST015 TST016; do
  grep -q "^${id}"$'\t' "${MATRIX}" || { echo "error: coverage matrix missing ${id}" >&2; exit 1; }
done

echo "PASS PR75 toolchain platform CTest reproducibility contract guard"
