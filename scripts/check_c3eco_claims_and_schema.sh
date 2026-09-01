#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-/tmp/shorthand_c3eco_schema_gate}"
INPUT_SHORT="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

bash "${ROOT_DIR}/scripts/generate_certification_bundle.sh" "${INPUT_SHORT}" "${OUT_DIR}" >/tmp/shorthand_c3eco_schema_bundle.out 2>&1

require_file() {
  local file="$1"
  if [[ ! -s "${file}" ]]; then
    echo "error: required C3-ECO artifact missing or empty: ${file}" >&2
    exit 1
  fi
}

require_text() {
  local file="$1"
  local text="$2"
  require_file "${file}"
  if ! grep -Fq "${text}" "${file}"; then
    echo "error: ${file} missing required text: ${text}" >&2
    exit 1
  fi
}

forbid_text() {
  local file="$1"
  local text="$2"
  require_file "${file}"
  if grep -Fq "${text}" "${file}"; then
    echo "error: ${file} contains forbidden claim text: ${text}" >&2
    exit 1
  fi
}

REPORT="${OUT_DIR}/candidate_report.json"
CHECK="${OUT_DIR}/candidate_check.json"
WORKBOOK="${OUT_DIR}/carbon_workbook.csv"
MANIFEST="${OUT_DIR}/manifest.json"
README="${OUT_DIR}/README.md"
MIGRATION="${OUT_DIR}/profile_migration.json"

require_file "${REPORT}"
require_file "${CHECK}"
require_file "${WORKBOOK}"
require_file "${MANIFEST}"
require_file "${README}"
require_file "${MIGRATION}"
require_file "${OUT_DIR}/candidate_report.schema.json"
require_file "${OUT_DIR}/candidate_check.schema.json"
require_file "${OUT_DIR}/bundle_manifest.schema.json"
require_file "${OUT_DIR}/profile_v2.schema.json"
require_file "${OUT_DIR}/profile_migration.schema.json"

require_text "${REPORT}" '"schema": "shorthand.c3eco.candidate_report.v1"'
require_text "${REPORT}" '"report_status": "candidate_assessment_only"'
require_text "${REPORT}" '"minimum_c3eco_evidence_present":'
require_text "${REPORT}" '"official_certification_granted": false'
require_text "${REPORT}" '"c3eco_profile_contract": "shorthand.c3eco.profile.v2"'
require_text "${REPORT}" '"measurement_status": "declared_budget_only"'
require_text "${REPORT}" '"inference_status": "not_executed"'
require_text "${REPORT}" '"models": ['
require_text "${REPORT}" '"tensors": ['
require_text "${REPORT}" '"infer_calls": ['
require_text "${REPORT}" '"measurements": ['
require_text "${REPORT}" '"blocked_certification_items": ['
require_text "${REPORT}" '"real_backend_execution_not_yet_verified"'
require_text "${REPORT}" '"external_certifier_not_signed"'
require_text "${REPORT}" '"base_footprint_not_reduced_by_offsets": true'
require_text "${REPORT}" '"claim_safe_text": "Candidate evidence report only.'
require_text "${REPORT}" '"disclaimer": "Evidence report only; this tool does not grant certification."'

require_text "${CHECK}" '"schema": "shorthand.c3eco.check.v1"'
require_text "${CHECK}" '"official_certification_granted": false'
require_text "${CHECK}" '"c3eco_profile_contract": "shorthand.c3eco.profile.v2"'
require_text "${CHECK}" '"blocking_items": ['
require_text "${CHECK}" '"disclaimer": "Candidate readiness check only; this tool does not grant certification."'

require_text "${WORKBOOK}" 'component,functional_unit,activity_kwh,carbon_factor_gco2e_per_kwh,carbon_kgco2e,measurement_quality,data_quality,measurement_status,evidence_ref'
require_text "${MANIFEST}" '"schema": "shorthand.c3eco.bundle_manifest.v1"'
require_text "${MANIFEST}" '"claim_status": "candidate_evidence_only"'
require_text "${MANIFEST}" '"official_certification_granted": false'
require_text "${MIGRATION}" '"schema": "shorthand.c3eco.profile_migration.v1"'
require_text "${MIGRATION}" '"official_certification_granted": false'
require_text "${README}" 'candidate evidence only'

require_text "${OUT_DIR}/candidate_report.schema.json" '"official_certification_granted": { "const": false }'
require_text "${OUT_DIR}/candidate_check.schema.json" '"official_certification_granted": { "const": false }'
require_text "${OUT_DIR}/bundle_manifest.schema.json" '"claim_status": { "const": "candidate_evidence_only" }'
require_text "${OUT_DIR}/profile_v2.schema.json" '"profile_contract": { "const": "shorthand.c3eco.profile.v2" }'
require_text "${OUT_DIR}/profile_migration.schema.json" '"schema": { "const": "shorthand.c3eco.profile_migration.v1" }'

for target in "${REPORT}" "${CHECK}" "${MIGRATION}" "${MANIFEST}" "${README}"; do
  forbid_text "${target}" '"official_certification_granted": true'
  forbid_text "${target}" 'C3-ECO Certified'
  forbid_text "${target}" 'certified product'
  forbid_text "${target}" 'zero-carbon'
  forbid_text "${target}" 'carbon neutral'
  forbid_text "${target}" 'offsets reduce the base footprint'
  forbid_text "${target}" 'production-ready'
done

echo "PASS C3-ECO schema and claim-safety gate"
