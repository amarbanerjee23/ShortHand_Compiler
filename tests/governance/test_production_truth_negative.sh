#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CHECKER="${ROOT_DIR}/scripts/check_production_truth.sh"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

expect_failure() {
  local name="$1"
  local truth="$2"
  local trace="$3"
  local diagnostic="$4"
  if SHORTHAND_PRODUCTION_TRUTH_FILE="${truth}" \
     SHORTHAND_C3ECO_TRACEABILITY_FILE="${trace}" \
     bash "${CHECKER}" >"${WORK_DIR}/${name}.out" 2>"${WORK_DIR}/${name}.err"; then
    echo "error: production truth negative case unexpectedly passed: ${name}" >&2
    exit 1
  fi
  grep -Fq "${diagnostic}" "${WORK_DIR}/${name}.err" || {
    echo "error: production truth negative case ${name} failed without expected diagnostic: ${diagnostic}" >&2
    cat "${WORK_DIR}/${name}.err" >&2
    exit 1
  }
}

cp "${ROOT_DIR}/docs/production_truth.tsv" "${WORK_DIR}/truth.tsv"
cp "${ROOT_DIR}/docs/c3eco_traceability.tsv" "${WORK_DIR}/trace.tsv"

cp "${WORK_DIR}/truth.tsv" "${WORK_DIR}/truth-production-claim.tsv"
awk -F '\t' 'BEGIN { OFS="\t" } $1 == "production_claim" { $2="true" } { print }' \
  "${WORK_DIR}/truth-production-claim.tsv" >"${WORK_DIR}/truth-production-claim.tmp"
mv "${WORK_DIR}/truth-production-claim.tmp" "${WORK_DIR}/truth-production-claim.tsv"
expect_failure production_claim "${WORK_DIR}/truth-production-claim.tsv" "${WORK_DIR}/trace.tsv" \
  'production truth production_claim expected false, found true'

cp "${WORK_DIR}/truth.tsv" "${WORK_DIR}/truth-serving-contract.tsv"
awk -F '\t' 'BEGIN { OFS="\t" } $1 == "serving_runtime_contract" { $2="shorthand.serving.runtime.v2" } { print }' \
  "${WORK_DIR}/truth-serving-contract.tsv" >"${WORK_DIR}/truth-serving-contract.tmp"
mv "${WORK_DIR}/truth-serving-contract.tmp" "${WORK_DIR}/truth-serving-contract.tsv"
expect_failure serving_contract "${WORK_DIR}/truth-serving-contract.tsv" "${WORK_DIR}/trace.tsv" \
  'production truth serving_runtime_contract expected shorthand.serving.runtime.v1, found shorthand.serving.runtime.v2'

awk -F '\t' '$1 != "G14" { print }' "${WORK_DIR}/trace.tsv" >"${WORK_DIR}/trace-missing-g14.tsv"
expect_failure missing_g14 "${WORK_DIR}/truth.tsv" "${WORK_DIR}/trace-missing-g14.tsv" \
  'C3-ECO traceability requires exactly one G14 row, found 0'

awk -F '\t' 'BEGIN { OFS="\t" } $1 == "G6" { $4="v0.6-g6+v0.7-g6" } { print }' \
  "${WORK_DIR}/trace.tsv" >"${WORK_DIR}/trace-v07-alias.tsv"
expect_failure v07_alias "${WORK_DIR}/truth.tsv" "${WORK_DIR}/trace-v07-alias.tsv" \
  'C3-ECO G6 mapping expected security_floor|v0.6-g6+v0.7-g7'

awk -F '\t' 'BEGIN { OFS="\t" } $1 == "G12" { $5="implemented"; $6="none"; $7="none" } { print }' \
  "${WORK_DIR}/trace.tsv" >"${WORK_DIR}/trace-implemented-without-evidence.tsv"
expect_failure implemented_without_evidence "${WORK_DIR}/truth.tsv" "${WORK_DIR}/trace-implemented-without-evidence.tsv" \
  'implemented traceability row G12 lacks execution evidence'

printf 'PASS production truth negative contradiction, contract, completeness, mapping and evidence cases\n'
