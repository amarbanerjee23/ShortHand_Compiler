#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN=${SHORTHAND_BIN:-"${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand"}
EXAMPLE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short"
OUT_FILE="/tmp/shorthand_ai_evidence.out"
REPORT_FILE="/tmp/shorthand_c3eco_report.json"
CHECK_FILE="/tmp/shorthand_c3eco_check.json"
WORKBOOK_FILE="/tmp/shorthand_c3eco_workbook.csv"

"${BIN}" "${EXAMPLE}" evidence >"${OUT_FILE}"
for needle in backend_preference runtime_backend inference_status disclaimer classifier int8 "1,3,224,224" tensors infer_calls candidate_assessment_only; do
  grep -q "$needle" "${OUT_FILE}"
done

"${BIN}" "${EXAMPLE}" c3eco-report --output "${REPORT_FILE}"
for needle in candidate_assessment_only minimum_c3eco_evidence_present blocked_certification_items claim_safe_text official_certification_granted; do
  grep -q "$needle" "${REPORT_FILE}"
done

"${BIN}" "${EXAMPLE}" c3eco-check --output "${CHECK_FILE}"
for needle in shorthand.c3eco.check.v1 candidate_ready_with_blockers external_certifier_not_signed; do
  grep -q "$needle" "${CHECK_FILE}"
done

"${BIN}" "${EXAMPLE}" c3eco-workbook --output "${WORKBOOK_FILE}"
for needle in component functional_unit activity_kwh carbon_kgco2e measurement_status; do
  grep -q "$needle" "${WORKBOOK_FILE}"
done
