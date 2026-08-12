#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

cp "${ROOT_DIR}/.github/workflows/ci.yml" "${TMP_DIR}/ci.yml"
sed -i 's/macos-15/macos-pr75-missing-runner/g' "${TMP_DIR}/ci.yml"

set +e
SHORTHAND_CI_FILE="${TMP_DIR}/ci.yml" \
  bash "${ROOT_DIR}/scripts/check_toolchain_platform_contract.sh" >"${TMP_DIR}/out" 2>"${TMP_DIR}/err"
status=$?
set -e

if [[ ${status} -eq 0 ]]; then
  echo "error: portability contract checker accepted a workflow with the mandatory macOS runner removed" >&2
  exit 1
fi
grep -Fq 'missing PR75 portability contract text: macos-15' "${TMP_DIR}/err" || {
  cat "${TMP_DIR}/err" >&2
  echo "error: portability contract negative regression failed for an unexpected reason" >&2
  exit 1
}

echo "PASS PR75 portability contract negative boundary regression"
