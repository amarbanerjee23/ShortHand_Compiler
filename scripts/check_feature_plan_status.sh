#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"
STATUS_FILE=docs/feature_implementation_status.md
required_files=(
  "${STATUS_FILE}"
  docs/compiler_test_strategy.md
  docs/production_readiness_pr_plan.md
  docs/execution_semantics_beta_0_3.md
  docs/fuzz_sanitizer_race_hardening.md
  docs/signed_release_publication.md
  docs/toolchain_platform_reproducibility.md
  tests/coverage/compiler_test_coverage_matrix.tsv
  scripts/check_module_resolution.sh
  scripts/check_semantic_differential.sh
  scripts/check_fuzz_sanitizers.sh
  scripts/check_runtime_memory_sanitizer.sh
  scripts/check_thread_sanitizer.sh
  scripts/check_ci_status_hygiene.sh
  scripts/check_signed_release_contract.sh
  .github/workflows/release.yml
)
for file in "${required_files[@]}"; do [[ -s "${file}" ]] || { echo "error: required feature/status evidence missing: ${file}" >&2; exit 1; }; done

required_status_terms=(
  "Implemented" "Partial" "Open" "Production blockers"
  "Real ONNX Runtime CPU backend execution"
  "Compiled-code metadata/runtime lowering"
  "Full backend compatibility"
  "Base grammar and module extension matrices"
  "Source-aware diagnostics" "Automated SBOM"
  "Runtime observability implementation"
  "Module/import/package syntax and AST scaffold"
  "Deterministic module resolver and multi-file codegen"
  "Cross-mode semantic equivalence" "Full sanitizer coverage"
  "Continuous fuzzing" "Concurrency and race detection"
  "Cross-platform reproducibility"
  "Measured ShortHand versus Python energy evidence"
  "Zero-skip production RC gate" "CPU/GPU/TPU/NPU"
  "CI status hygiene" "MLIR dialect scaffold"
  "Module/import/package model" "Signed releases" "Protected publication"
)
for term in "${required_status_terms[@]}"; do
  grep -Fiq "${term}" "${STATUS_FILE}" || { echo "error: feature implementation status missing required tracking term: ${term}" >&2; exit 1; }
done

for anchor in \
  'feature_status_version: 2026-08-12-pr76' \
  'language_version: beta-0.3' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  '16 implemented, 5 partial and 6 open' \
  'Roadmap PR74 was implemented and merged as GitHub PR75' \
  'GitHub PR76 now implements roadmap PR75' \
  'Cross-platform portability | Implemented for PR74 tiers' \
  'Cross-platform reproducibility | Implemented' \
  'Signed releases | Partial' \
  'The repository did not have the required `production-release` environment at PR76 start' \
  'live production qualification remains PR80'; do
  grep -Fiq "${anchor}" "${STATUS_FILE}" || { echo "error: feature implementation status missing current anchor: ${anchor}" >&2; exit 1; }
done

grep -Fq 'resolution_status: deterministic_manifest_locked_multi_file_codegen' docs/module_resolution_and_lockfile.md
grep -Fq 'execution_semantics_contract: beta-0.3-pr72-v1' docs/execution_semantics_beta_0_3.md
grep -Fq 'fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1' docs/fuzz_sanitizer_race_hardening.md
grep -Fq 'toolchain_platform_contract_version: shorthand.portability.reproducibility.v1' docs/toolchain_platform_reproducibility.md
grep -Fq 'signed_release_contract_version: shorthand.release.protected.v1' docs/signed_release_publication.md
grep -Fq 'PASS CI status hygiene guard' scripts/check_ci_status_hygiene.sh
grep -Fq 'PASS signed release and protected publication contract gate' scripts/check_signed_release_contract.sh

unsupported_claim_patterns=(
  "Current status: fully production-ready"
  "ShortHand is fully production-ready"
  "all production blockers are complete"
  "ShortHand uses less energy than Python"
  "fuzzing proves the compiler has no bugs"
  "ThreadSanitizer proves the runtime has no races"
  "signed release blocker is complete"
)
for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -qi "${pattern}" "${STATUS_FILE}"; then
    echo "error: status file contains unsupported readiness/safety/signing claim: ${pattern}" >&2
    exit 1
  fi
done

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == 1 ]]; then
  if grep -Eq '\| (Open|Partial)(/[^|]+)? \|' "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. PR74 portability is recorded and PR75 signed publication remains fail-closed pending protected-environment execution."
