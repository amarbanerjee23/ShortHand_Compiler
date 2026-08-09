#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

STATUS_FILE="docs/feature_implementation_status.md"
PLAN_FILES=(
  "docs/language_feature_implementation_plan.md"
  "docs/beta_enterprise_requirements.md"
  "docs/enterprise_release_scorecard.md"
  "docs/compiler_test_strategy.md"
  "tests/coverage/compiler_test_coverage_matrix.tsv"
  "docs/module_import_package_syntax.md"
  "tests/conformance/module_matrix_beta_0_3.tsv"
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
  "Base grammar and module extension matrices"
  "Source-aware diagnostics"
  "Automated SBOM"
  "Runtime observability implementation"
  "Module/import/package syntax and AST scaffold"
  "Deterministic module resolver and multi-file codegen"
  "Compiler test strategy and coverage matrix"
  "Cross-mode semantic equivalence"
  "Cross-platform reproducibility"
  "Measured ShortHand versus Python energy evidence"
  "Zero-skip production RC gate"
)

for term in "${required_status_terms[@]}"; do
  if ! grep -q "${term}" "${STATUS_FILE}"; then
    echo "error: feature implementation status is missing required tracking term: ${term}" >&2
    exit 1
  fi
done

unsupported_claim_patterns=(
  "Current status: fully production-ready"
  "ShortHand is fully production-ready"
  "all production blockers are complete"
  "ShortHand uses less energy than Python"
  "imports are fully resolved"
)

for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -qi "${pattern}" "${STATUS_FILE}"; then
    echo "error: status file contains unsupported readiness, resolver or energy claim: ${pattern}" >&2
    exit 1
  fi
done

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == "1" ]]; then
  if grep -q "| Open |\|| Partial |" "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. Current maturity, test gaps and production blockers remain explicit."
