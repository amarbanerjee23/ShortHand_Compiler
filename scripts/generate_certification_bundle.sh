#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_SHORT="${1:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short}"
OUT_DIR="${2:-${ROOT_DIR}/reports/generated/c3eco_bundle}"
SHORT_BIN="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"

if [[ ! -x "${SHORT_BIN}" ]]; then
  make -C "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" short_hand >/tmp/shorthand_bundle_build.out 2>&1
fi

mkdir -p "${OUT_DIR}"

"${SHORT_BIN}" "${INPUT_SHORT}" c3eco-report --output "${OUT_DIR}/candidate_report.json"
"${SHORT_BIN}" "${INPUT_SHORT}" c3eco-check --output "${OUT_DIR}/candidate_check.json"
"${SHORT_BIN}" "${INPUT_SHORT}" c3eco-workbook --output "${OUT_DIR}/carbon_workbook.csv"
cp "${ROOT_DIR}/docs/backend_compatibility_matrix.md" "${OUT_DIR}/backend_compatibility_matrix.md"
cp "${ROOT_DIR}/docs/telemetry_schema.md" "${OUT_DIR}/telemetry_schema.md"
cp "${ROOT_DIR}/schemas/c3eco/candidate_report.schema.json" "${OUT_DIR}/candidate_report.schema.json"
cp "${ROOT_DIR}/schemas/c3eco/candidate_check.schema.json" "${OUT_DIR}/candidate_check.schema.json"
cp "${ROOT_DIR}/schemas/c3eco/bundle_manifest.schema.json" "${OUT_DIR}/bundle_manifest.schema.json"

cat > "${OUT_DIR}/README.md" <<EOF
# ShortHand C3-ECO Candidate Evidence Bundle

Source program: ${INPUT_SHORT}

Generated artifacts:

- candidate_report.json
- candidate_check.json
- carbon_workbook.csv
- backend_compatibility_matrix.md
- telemetry_schema.md
- candidate_report.schema.json
- candidate_check.schema.json
- bundle_manifest.schema.json

Important claim boundary: this bundle is candidate evidence only. It does not grant official C3-ECO certification and must be reviewed by the certifying authority/auditor before public certification claims.
EOF

cat > "${OUT_DIR}/manifest.json" <<EOF
{
  "schema": "shorthand.c3eco.bundle_manifest.v1",
  "source_program": "${INPUT_SHORT}",
  "claim_status": "candidate_evidence_only",
  "official_certification_granted": false,
  "artifacts": [
    "candidate_report.json",
    "candidate_check.json",
    "carbon_workbook.csv",
    "backend_compatibility_matrix.md",
    "telemetry_schema.md",
    "candidate_report.schema.json",
    "candidate_check.schema.json",
    "bundle_manifest.schema.json",
    "README.md"
  ]
}
EOF

grep -q '"official_certification_granted": false' "${OUT_DIR}/candidate_report.json"
grep -q 'candidate' "${OUT_DIR}/candidate_check.json"
grep -q 'activity_kwh' "${OUT_DIR}/carbon_workbook.csv"
grep -q 'candidate_evidence_only' "${OUT_DIR}/manifest.json"
grep -q '"shorthand.c3eco.candidate_report.v1"' "${OUT_DIR}/candidate_report.schema.json"
grep -q '"shorthand.c3eco.check.v1"' "${OUT_DIR}/candidate_check.schema.json"
grep -q '"shorthand.c3eco.bundle_manifest.v1"' "${OUT_DIR}/bundle_manifest.schema.json"

echo "Generated ShortHand C3-ECO candidate evidence bundle: ${OUT_DIR}"
