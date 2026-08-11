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
  "docs/ci_pipeline_architecture.md"
  "docs/production_readiness_pr_plan.md"
  "docs/execution_semantics_beta_0_3.md"
  "tests/coverage/compiler_test_coverage_matrix.tsv"
  "docs/module_import_package_syntax.md"
  "docs/module_resolution_and_lockfile.md"
  "tests/conformance/module_matrix_beta_0_3.tsv"
  "scripts/check_module_resolution.sh"
  "scripts/check_semantic_differential.sh"
  "scripts/check_ci_status_hygiene.sh"
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
  "CPU/GPU/TPU/NPU"
  "CI status hygiene"
  "MLIR dialect scaffold"
  "Module/import/package model"
)

for term in "${required_status_terms[@]}"; do
  if ! grep -q "${term}" "${STATUS_FILE}"; then
    echo "error: feature implementation status is missing required tracking term: ${term}" >&2
    exit 1
  fi
done

for anchor in \
  'feature_status_version: 2026-08-11-pr72' \
  'language_version: beta-0.3' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  '9 implemented, 8 partial and 10 open' \
  'PR70 is merged' \
  'PR72 is the active semantic-correctness candidate' \
  'Cross-mode semantic equivalence | Implemented in PR72 candidate for defined core contract' \
  'PR74 adds declared platform/toolchain matrix' \
  'live production qualification remains PR80'; do
  if ! grep -Fiq "${anchor}" "${STATUS_FILE}"; then
    echo "error: feature implementation status missing PR72 anchor: ${anchor}" >&2
    exit 1
  fi
done

grep -Fq 'resolution_status: deterministic_manifest_locked_multi_file_codegen' docs/module_resolution_and_lockfile.md || {
  echo "error: module resolver contract is not marked as deterministic/locked" >&2
  exit 1
}

grep -Fq 'execution_semantics_contract: beta-0.3-pr72-v1' docs/execution_semantics_beta_0_3.md || {
  echo "error: PR72 executable semantic contract is missing" >&2
  exit 1
}

grep -Fq 'PASS deterministic module resolver, package lock and multi-file codegen gate' scripts/check_module_resolution.sh || {
  echo "error: module resolver executable gate missing" >&2
  exit 1
}

grep -Fq 'PASS cross-mode semantic differential execution gate' scripts/check_semantic_differential.sh || {
  echo "error: semantic differential executable gate missing" >&2
  exit 1
}

grep -Fq 'ci_pipeline_architecture_version: 2026-08-09-v1' docs/ci_pipeline_architecture.md || {
  echo "error: robust CI pipeline architecture contract is missing" >&2
  exit 1
}

grep -Fq 'PASS CI status hygiene guard' scripts/check_ci_status_hygiene.sh || {
  echo "error: CI status hygiene guard is missing" >&2
  exit 1
}

unsupported_claim_patterns=(
  "Current status: fully production-ready"
  "ShortHand is fully production-ready"
  "all production blockers are complete"
  "ShortHand uses less energy than Python"
  "all interpreter and compiled module execution is equivalent"
)

for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -qi "${pattern}" "${STATUS_FILE}"; then
    echo "error: status file contains unsupported readiness, equivalence or energy claim: ${pattern}" >&2
    exit 1
  fi
done

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == "1" ]]; then
  if grep -q "| Open |\|| Partial |" "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. PR72 semantic evidence and remaining production blockers remain explicit."
