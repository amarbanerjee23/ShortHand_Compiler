#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

STATUS_FILE="docs/feature_implementation_status.md"
PLAN_FILES=(
  "docs/language_feature_implementation_plan.md"
  "docs/beta_enterprise_requirements.md"
  "docs/enterprise_release_scorecard.md"
)

for file in "${PLAN_FILES[@]}" "${STATUS_FILE}"; do
  if [[ ! -s "${file}" ]]; then
    echo "error: required feature plan/status file missing or empty: ${file}" >&2
    exit 1
  fi
done

required_status_terms=(
  "Implemented"
  "Partial"
  "Open"
  "Production blockers"
  "Real ONNX Runtime CPU backend execution"
  "Compiled-code metadata/runtime lowering"
  "Full backend compatibility"
  "Complete formal grammar"
  "Source-aware diagnostics"
  "Automated SBOM"
  "Runtime observability implementation"
  "Module/import/package model"
)

for term in "${required_status_terms[@]}"; do
  if ! grep -q "${term}" "${STATUS_FILE}"; then
    echo "error: feature implementation status is missing required tracking term: ${term}" >&2
    exit 1
  fi
done

if grep -qi "fully production-ready" "${STATUS_FILE}"; then
  echo "error: status file must not claim full production readiness while blockers remain" >&2
  exit 1
fi

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == "1" ]]; then
  if grep -q "| Open |\|| Partial |" "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. Current maturity is tracked; production blockers remain explicit."
