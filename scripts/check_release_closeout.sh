#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOL="${SHORTHAND_RELEASE_CLOSEOUT_TOOL:-${ROOT_DIR}/scripts/release_closeout.py}"
PYTHONDONTWRITEBYTECODE=1 python3 "${ROOT_DIR}/tests/release/test_release_closeout.py" "${ROOT_DIR}" "${TOOL}"
echo 'PASS PR102 release claims audit closeout and no-go publication gate'
