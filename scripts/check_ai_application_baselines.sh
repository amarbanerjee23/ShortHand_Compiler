#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOL="${1:?native qualification tool required}"
WORK_DIR="${2:?output directory required}"
export ORT_DISABLE_TELEMETRY=1
python3 -c 'import platform,sys; assert sys.version_info[:2] == (3,12) and platform.system() == "Linux" and platform.machine() == "x86_64", "baseline lock requires CPython 3.12 Linux x64"'
mkdir -p "${WORK_DIR}"
python3 -m venv "${WORK_DIR}/venv"
"${WORK_DIR}/venv/bin/python" -m pip install --disable-pip-version-check --only-binary=:all: --require-hashes \
  -r "${ROOT_DIR}/tests/ai_application/requirements-ai-baseline-linux-x64-py312.txt"
python3 "${ROOT_DIR}/scripts/create_digit_application_fixture.py" "${WORK_DIR}/fixture"
"${WORK_DIR}/venv/bin/python" "${ROOT_DIR}/scripts/compare_ai_application_baselines.py" \
  --tool "${TOOL}" --config "${WORK_DIR}/fixture/application.json" --output "${WORK_DIR}/evidence"
echo 'PASS mandatory paired native/Python real-dataset baseline gate'
