#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

cp "${ROOT_DIR}/tests/ctest_parity/expected_make_targets.txt" "${TMP_DIR}/targets.txt"
printf 'test-pr75-nonexistent-target\n' >> "${TMP_DIR}/targets.txt"

set +e
SHORTHAND_CTEST_EXPECTED_FILE="${TMP_DIR}/targets.txt" \
SHORTHAND_CTEST_PARITY_BUILD_DIR="${TMP_DIR}/build" \
SHORTHAND_CTEST_PARITY_EXECUTE=0 \
  bash "${ROOT_DIR}/scripts/check_cmake_ctest_parity.sh" >"${TMP_DIR}/out" 2>"${TMP_DIR}/err"
status=$?
set -e

if [[ ${status} -eq 0 ]]; then
  echo "error: CTest parity checker accepted a nonexistent Makefile target" >&2
  exit 1
fi
grep -Fq 'parity manifest target is not defined by Makefile: test-pr75-nonexistent-target' "${TMP_DIR}/err" || {
  cat "${TMP_DIR}/err" >&2
  echo "error: CTest parity negative test failed for an unexpected reason" >&2
  exit 1
}

echo "PASS CTest parity negative boundary regression"
