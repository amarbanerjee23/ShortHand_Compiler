#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BIN="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/green_ai_tool"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT
make -C "${SRC_DIR}" green_ai_tool >/dev/null
for f in "${ROOT_DIR}"/examples/green_ai/*.greenai; do
  "${BIN}" validate "$f" --strict strict >/dev/null
done
REPORT="${TMP_DIR}/report.json"
"${BIN}" report "${ROOT_DIR}/examples/green_ai/image_classification.greenai" --output "${REPORT}" --strict strict
grep -q '"report_schema_version": "green-ai-evidence/v1"' "${REPORT}"
grep -q 'Evidence report only; this tool does not grant certification.' "${REPORT}"
grep -q '"budget_results"' "${REPORT}"
BAD="${TMP_DIR}/missing_functional_unit.greenai"
cat >"${BAD}" <<'MANIFEST'
green { green_mode: "strict" success_criteria: { ok: true } boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" }
carbon { accounting_method: "location_based" region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false }
budgets { memory_peak_mb_max: 1 }
MANIFEST
if "${BIN}" validate "${BAD}" --strict strict >/tmp/short_hand_bad.out 2>/tmp/short_hand_bad.err; then
  echo "strict validation unexpectedly passed"
  exit 1
fi
grep -q 'functional_unit' /tmp/short_hand_bad.err
BAD_GRID="${TMP_DIR}/missing_grid.greenai"
cat >"${BAD_GRID}" <<'MANIFEST'
green { green_mode: "strict" functional_unit: "1000_inferences" success_criteria: { ok: true } boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" }
carbon { accounting_method: "location_based" region: "X" offsets_allowed_in_core_score: false }
budgets { memory_peak_mb_max: 1 }
MANIFEST
if "${BIN}" validate "${BAD_GRID}" --strict strict >/dev/null 2>"${TMP_DIR}/grid.err"; then exit 1; fi
grep -q 'grid_factor' "${TMP_DIR}/grid.err"
BAD_OFFSET="${TMP_DIR}/bad_offset.greenai"
sed 's/offsets_allowed_in_core_score: false/offsets_allowed_in_core_score: true/' "${ROOT_DIR}/examples/green_ai/image_classification.greenai" >"${BAD_OFFSET}"
if "${BIN}" validate "${BAD_OFFSET}" --strict strict >/dev/null 2>"${TMP_DIR}/offset.err"; then exit 1; fi
grep -q 'offsets' "${TMP_DIR}/offset.err"
BASE="${TMP_DIR}/baseline.json"
cat >"${BASE}" <<'JSON'
{"derived":{"energy_per_functional_unit_kwh":0.000001,"carbon_per_functional_unit_gco2e":0.000001},"measurements":{"memory_peak_mb":1}}
JSON
if "${BIN}" check "${ROOT_DIR}/examples/green_ai/image_classification.greenai" --baseline "${BASE}" --threshold-percent 10 >"${TMP_DIR}/check.json"; then exit 1; fi
grep -q 'remediation_hint' "${TMP_DIR}/check.json"
echo "C++ Green AI tool tests passed"
