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
  docs/external_security_policy.md
  docs/toolchain_platform_reproducibility.md
  docs/container_kubernetes_hardening.md
  docs/formatter_linter.md
  tests/coverage/compiler_test_coverage_matrix.tsv
  tests/tooling/formatter_messy.short
  tests/tooling/formatter_expected.short
  scripts/check_module_resolution.sh
  scripts/check_semantic_differential.sh
  scripts/check_fuzz_sanitizers.sh
  scripts/check_runtime_memory_sanitizer.sh
  scripts/check_thread_sanitizer.sh
  scripts/check_ci_status_hygiene.sh
  scripts/check_signed_release_contract.sh
  scripts/check_external_security_policy.sh
  scripts/check_third_party_license_policy.sh
  scripts/check_security_exceptions.sh
  scripts/check_action_pinning.sh
  scripts/check_container_kubernetes_hardening.sh
  scripts/check_container_runtime.sh
  scripts/check_kubernetes_ephemeral_cluster.sh
  scripts/check_formatter_linter.sh
  Compiler_new_ws/Short_Hand/src/tooling/SourceTools.h
  Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp
  Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp
  Compiler_new_ws/Short_Hand/src/tooling/Makefile
  tests/deployment/test_container_kubernetes_hardening_negative.sh
  deploy/k8s/production.yaml
  .github/workflows/tooling.yml
  .github/workflows/release.yml
  .github/workflows/security.yml
  .github/dependency-review-config.yml
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
  "MLIR dialect scaffold"
  "Module/import/package model" "Signed releases" "Protected publication"
  "External vulnerability gate" "Container and Kubernetes hardening"
  "Formatter and linter" "Syntax highlighting and LSP"
)
for term in "${required_status_terms[@]}"; do
  grep -Fiq "${term}" "${STATUS_FILE}" || { echo "error: feature implementation status missing required tracking term: ${term}" >&2; exit 1; }
done

for anchor in \
  'feature_status_version: 2026-08-18-pr79' \
  'language_version: beta-0.3' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  '19 implemented, 4 partial and 4 open' \
  'Roadmap PR75 was implemented and merged as GitHub PR76' \
  'GitHub PR77 implemented and merged roadmap PR76' \
  'GitHub PR78 implemented and merged roadmap PR77' \
  'GitHub PR79 now implements roadmap PR78' \
  'Cross-platform portability | Implemented for PR74 tiers' \
  'Cross-platform reproducibility | Implemented' \
  'Signed releases | Partial' \
  'External vulnerability gate | Implemented' \
  'Container and Kubernetes hardening | Implemented' \
  'Formatter and linter | Implemented' \
  'Syntax highlighting and LSP | Open' \
  'live production qualification remains PR80'; do
  grep -Fiq "${anchor}" "${STATUS_FILE}" || { echo "error: feature implementation status missing current anchor: ${anchor}" >&2; exit 1; }
done

grep -Fq 'resolution_status: deterministic_manifest_locked_multi_file_codegen' docs/module_resolution_and_lockfile.md
grep -Fq 'execution_semantics_contract: beta-0.3-pr72-v1' docs/execution_semantics_beta_0_3.md
grep -Fq 'fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1' docs/fuzz_sanitizer_race_hardening.md
grep -Fq 'toolchain_platform_contract_version: shorthand.portability.reproducibility.v1' docs/toolchain_platform_reproducibility.md
grep -Fq 'signed_release_contract_version: shorthand.release.protected.v1' docs/signed_release_publication.md
grep -Fq 'external_security_policy_version: shorthand.security.external.v1' docs/external_security_policy.md
grep -Fq 'container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1' docs/container_kubernetes_hardening.md
grep -Fq 'formatter_linter_contract_version: shorthand.tooling.format_lint.v1' docs/formatter_linter.md
grep -Fq 'shorthand.lint.v1' Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp
grep -Fq 'fix mode requires --output' Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp
# CI status hygiene is executable evidence rather than duplicated roadmap prose.
# The dedicated fail-closed guards remain mandatory here and in ubuntu-core.
grep -Fq 'PASS CI status hygiene guard' scripts/check_ci_status_hygiene.sh
grep -Fq 'PASS signed release and protected publication contract gate' scripts/check_signed_release_contract.sh
grep -Fq 'PASS external vulnerability SAST dependency and license policy gate' scripts/check_external_security_policy.sh
grep -Fq 'PASS container Kubernetes production hardening contract' scripts/check_container_kubernetes_hardening.sh
grep -Fq 'PASS hardened container runtime' scripts/check_container_runtime.sh
grep -Fq 'PASS ephemeral Kubernetes production gate' scripts/check_kubernetes_ephemeral_cluster.sh
grep -Fq 'PASS native Linux arm64 production container qualification' scripts/check_installed_sdk_lifecycle.sh
grep -Fq 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate' scripts/check_formatter_linter.sh

grep -Fq 'GCC formatter and linter gate' .github/workflows/tooling.yml
grep -Fq 'Clang formatter and linter gate' .github/workflows/tooling.yml
grep -Fq 'ASan UBSan formatter and linter gate' .github/workflows/tooling.yml

bash scripts/check_container_kubernetes_hardening.sh
bash tests/deployment/test_container_kubernetes_hardening_negative.sh

# TST020 is not closed by source-text presence. Execute the actual formatter and
# linter against the compiler parser/execution oracle inside the inherited
# mandatory ubuntu-core path. The fast tooling workflow independently repeats
# this under GCC, Clang and ASan/UBSan.
bash scripts/check_formatter_linter.sh

# `check_feature_plan_status.sh` is a mandatory ubuntu-core CI step. The live
# cluster test therefore remains exact-head merge evidence without adding a
# weaker side workflow. Outside GitHub CI the deterministic static/negative
# contract still runs, while local machines are not silently treated as
# production cluster evidence.
if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" && "$(uname -m)" == "x86_64" ]]; then
  bash scripts/check_kubernetes_ephemeral_cluster.sh
fi

unsupported_claim_patterns=(
  "Current status: fully production-ready"
  "ShortHand is fully production-ready"
  "all production blockers are complete"
  "ShortHand uses less energy than Python"
  "fuzzing proves the compiler has no bugs"
  "ThreadSanitizer proves the runtime has no races"
  "signed release blocker is complete"
  "all dependencies are vulnerability-free"
  "all Kubernetes workloads are production qualified"
  "formatter proves semantic equivalence for all future grammar"
)
for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -qi "${pattern}" "${STATUS_FILE}"; then
    echo "error: status file contains unsupported readiness/safety/signing/security/deployment/tooling claim: ${pattern}" >&2
    exit 1
  fi
done

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == 1 ]]; then
  if grep -Eq '\| (Open|Partial)(/[^|]+)? \|' "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. PR78 formatter/linter is implemented for shorthand.tooling.format_lint.v1; PR75 signed publication remains fail-closed pending protected-environment execution."
