#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"
STATUS_FILE=docs/feature_implementation_status.md
required_files=(
  "${STATUS_FILE}"
  docs/compiler_test_strategy.md
  docs/production_readiness_pr_plan.md
  docs/production_backend_hardware_qualification.md
  docs/c3eco_language_contract.md
  docs/execution_semantics_beta_0_3.md
  docs/fuzz_sanitizer_race_hardening.md
  docs/signed_release_publication.md
  docs/external_security_policy.md
  docs/toolchain_platform_reproducibility.md
  docs/container_kubernetes_hardening.md
  docs/formatter_linter.md
  docs/syntax_highlighting_lsp.md
  tests/coverage/compiler_test_coverage_matrix.tsv
  tests/tooling/formatter_messy.short
  tests/tooling/formatter_expected.short
  tests/integration/test_production_backend_hardware_qualification.sh
  tests/integration/test_compiled_hook_onnxruntime_success.sh
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
  scripts/check_lsp_editor.sh
  scripts/install_ci_onnxruntime_cpu.sh
  scripts/check_production_backend_hardware_qualification.sh
  scripts/check_c3eco_language_blocks.sh
  scripts/check_no_mandatory_test_skips.sh
  Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h
  Compiler_new_ws/Short_Hand/src/tooling/SourceTools.h
  Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp
  Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp
  Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp
  Compiler_new_ws/Short_Hand/src/tooling/Makefile
  editors/vscode/package.json
  editors/vscode/language-configuration.json
  editors/vscode/syntaxes/shorthand.tmLanguage.json
  tests/deployment/test_container_kubernetes_hardening_negative.sh
  deploy/k8s/production.yaml
  security/third_party_inventory.tsv
  .github/workflows/tooling.yml
  .github/workflows/release.yml
  .github/workflows/security.yml
  .github/dependency-review-config.yml
)
for file in "${required_files[@]}"; do
  [[ -s "${file}" ]] || { echo "error: required feature/status evidence missing: ${file}" >&2; exit 1; }
done

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
  'feature_status_version: 2026-08-21-pr82' \
  'language_version: beta-0.3' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  '21 implemented, 3 partial and 3 open' \
  'Roadmap PR75 was implemented and merged as GitHub PR76' \
  'GitHub PR77 implemented and merged roadmap PR76' \
  'GitHub PR78 implemented and merged roadmap PR77' \
  'GitHub PR79 implemented and merged roadmap PR78' \
  'GitHub PR80 implemented and merged roadmap PR79' \
  'GitHub PR81 implemented and merged roadmap PR80' \
  'GitHub PR82 now implements roadmap PR81' \
  'Cross-platform portability | Implemented for PR74 tiers' \
  'Cross-platform reproducibility | Implemented' \
  'Signed releases | Partial' \
  'External vulnerability gate | Implemented' \
  'Container and Kubernetes hardening | Implemented' \
  'Formatter and linter | Implemented' \
  'Syntax highlighting and LSP | Implemented for `shorthand.tooling.lsp.v1`' \
  'Real ONNX Runtime CPU backend execution | Implemented for `linux-x64-cpu-v1` candidate'; do
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
grep -Fq 'lsp_editor_contract_version: shorthand.tooling.lsp.v1' docs/syntax_highlighting_lsp.md
grep -Fq 'backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1' docs/production_backend_hardware_qualification.md
grep -Fq 'production_scope: linux-x64-cpu-v1' docs/production_backend_hardware_qualification.md
grep -Fq 'shorthand.backend_hardware_qualification.v1' Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h
grep -Fq 'backend_device_not_production_qualified' Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h
grep -Fq 'enforceProductionBackendQualification(' Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
grep -Fq 'shorthand.lint.v1' Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp
grep -Fq 'fix mode requires --output' Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp
grep -Fq 'constexpr std::size_t kMaxMessageBytes = 1024 * 1024' Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp
grep -Fq 'SHLSP900' Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp
grep -Fq 'shorthand_lsp' CMakeLists.txt

# CI status hygiene and all prior production guards remain executable evidence.
grep -Fq 'PASS CI status hygiene guard' scripts/check_ci_status_hygiene.sh
grep -Fq 'PASS signed release and protected publication contract gate' scripts/check_signed_release_contract.sh
grep -Fq 'PASS external vulnerability SAST dependency and license policy gate' scripts/check_external_security_policy.sh
grep -Fq 'PASS container Kubernetes production hardening contract' scripts/check_container_kubernetes_hardening.sh
grep -Fq 'PASS hardened container runtime' scripts/check_container_runtime.sh
grep -Fq 'PASS ephemeral Kubernetes production gate' scripts/check_kubernetes_ephemeral_cluster.sh
grep -Fq 'PASS native Linux arm64 production container qualification' scripts/check_installed_sdk_lifecycle.sh
grep -Fq 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate' scripts/check_formatter_linter.sh
grep -Fq 'PASS syntax highlighting LSP protocol compiler-diagnostics navigation cancellation UTF16 bounded-framing gate' scripts/check_lsp_editor.sh
grep -Fq 'PASS production backend and hardware qualification gate' scripts/check_production_backend_hardware_qualification.sh
grep -Fq 'PASS verified ONNX Runtime CPU qualification SDK acquisition' scripts/install_ci_onnxruntime_cpu.sh

grep -Fq 'GCC formatter and linter gate' .github/workflows/tooling.yml
grep -Fq 'Clang formatter and linter gate' .github/workflows/tooling.yml
grep -Fq 'ASan UBSan formatter and linter gate' .github/workflows/tooling.yml
grep -Fq 'GCC LSP editor protocol gate' .github/workflows/tooling.yml
grep -Fq 'Clang LSP editor protocol gate' .github/workflows/tooling.yml
grep -Fq 'ASan UBSan LSP editor protocol gate' .github/workflows/tooling.yml

# TST019-TST022 are executable contracts. Run deterministic portions on every
# invocation. Live device/SDK evidence is mandatory on the inherited Linux x64
# ubuntu-core CI lane and is never represented as a skip.
bash scripts/check_container_kubernetes_hardening.sh
bash tests/deployment/test_container_kubernetes_hardening_negative.sh
bash scripts/check_formatter_linter.sh
bash scripts/check_lsp_editor.sh
bash scripts/check_no_mandatory_test_skips.sh
bash scripts/check_c3eco_language_blocks.sh
bash tests/integration/test_production_backend_hardware_qualification.sh

if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" && "$(uname -m)" == "x86_64" ]]; then
  ORT_ROOT="${ONNXRUNTIME_ROOT:-${RUNNER_TEMP:-/tmp}/shorthand-onnxruntime-1.20.1}"
  if [[ ! -s "${ORT_ROOT}/include/onnxruntime_cxx_api.h" ]]; then
    bash scripts/install_ci_onnxruntime_cpu.sh "${ORT_ROOT}"
  fi
  ONNXRUNTIME_ROOT="${ORT_ROOT}" bash scripts/check_production_backend_hardware_qualification.sh
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
  "LSP supports all IDE features"
  "editor tooling proves backend execution"
  "GPU production support is implemented"
  "TPU production support is implemented"
  "NPU production support is implemented"
)
for pattern in "${unsupported_claim_patterns[@]}"; do
  if grep -qi "${pattern}" "${STATUS_FILE}"; then
    echo "error: status file contains unsupported readiness/safety/signing/security/deployment/tooling/backend claim: ${pattern}" >&2
    exit 1
  fi
done

if [[ "${REQUIRE_PRODUCTION_READY:-0}" == 1 ]]; then
  if grep -Eq '\| (Open|Partial)(/[^|]+)? \|' "${STATUS_FILE}"; then
    echo "error: production-ready check failed because open or partial items remain" >&2
    exit 1
  fi
fi

echo "Feature plan status check passed. GitHub PR82 implements roadmap PR81 C3-ECO language contract shorthand.c3eco.language.v1 while preserving the roadmap PR80 backend/hardware qualification; signed publication and later production blockers remain fail-closed."

grep -Fq 'c3eco_language_contract_version: shorthand.c3eco.language.v1' docs/c3eco_language_contract.md
grep -Fq 'official_certification_granted: false' docs/c3eco_language_contract.md
grep -Fq 'PASS C3-ECO first-class language blocks grammar AST semantics evidence and claim-safety gate' scripts/check_c3eco_language_blocks.sh
grep -Fq 'PASS mandatory qualification zero-skip policy gate' scripts/check_no_mandatory_test_skips.sh
