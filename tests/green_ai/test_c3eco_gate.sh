#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
GATE="${BUILD_DIR}/c3eco_gate"
TOOL="${BUILD_DIR}/green_ai_tool"

make -C "${SRC_DIR}" c3eco_gate green_ai_tool

for manifest in \
  "${ROOT_DIR}/examples/green_ai/image_classification.greenai" \
  "${ROOT_DIR}/examples/green_ai/c3eco_ai_certification_candidate.greenai"; do
  echo "Validating C3-ECO manifest: ${manifest}"
  "${GATE}" --strict "${manifest}"
  "${TOOL}" validate "${manifest}" --strict strict
  out="/tmp/$(basename "${manifest}").report.json"
  "${TOOL}" report "${manifest}" --output "${out}" --strict strict
  test -s "${out}"
  grep -q '"certification_profile": "C3-ECO-AI"' "${out}"
  grep -q '"green_mode": "strict"' "${out}"
  grep -q '"offsets_reported_separately"' "${out}"
  grep -q 'Evidence report only; this tool does not grant certification.' "${out}"
done

echo "C3-ECO gate tests completed successfully."
