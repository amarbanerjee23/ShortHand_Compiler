#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
TEMPLATE="${ROOT_DIR}/.github/pull_request_template.md"
CI="${ROOT_DIR}/.github/workflows/ci.yml"
TOOLING_CI="${ROOT_DIR}/.github/workflows/tooling.yml"
RELEASE_CI="${ROOT_DIR}/.github/workflows/release.yml"
SECURITY_CI="${ROOT_DIR}/.github/workflows/security.yml"
DEPLOY_DOC="${ROOT_DIR}/docs/container_kubernetes_hardening.md"
TOOLING_DOC="${ROOT_DIR}/docs/formatter_linter.md"
LSP_DOC="${ROOT_DIR}/docs/syntax_highlighting_lsp.md"
BACKEND_DOC="${ROOT_DIR}/docs/production_backend_hardware_qualification.md"
C3ECO_DOC="${ROOT_DIR}/docs/c3eco_language_contract.md"

require_file() { [[ -s "$1" ]] || { echo "error: missing or empty file: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing required text: $2" >&2; exit 1; }; }

for file in "${DOC}" "${MATRIX}" "${PLAN}" "${STATUS}" "${TEMPLATE}" "${CI}" "${TOOLING_CI}" "${RELEASE_CI}" "${SECURITY_CI}" "${DEPLOY_DOC}" "${TOOLING_DOC}" "${LSP_DOC}" "${BACKEND_DOC}" "${C3ECO_DOC}" \
  "${ROOT_DIR}/scripts/check_semantic_differential.sh" \
  "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" \
  "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" \
  "${ROOT_DIR}/scripts/check_signed_release_contract.sh" \
  "${ROOT_DIR}/scripts/check_external_security_policy.sh" \
  "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" \
  "${ROOT_DIR}/scripts/check_container_runtime.sh" \
  "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" \
  "${ROOT_DIR}/scripts/check_formatter_linter.sh" \
  "${ROOT_DIR}/scripts/check_lsp_editor.sh" \
  "${ROOT_DIR}/scripts/install_ci_onnxruntime_cpu.sh" \
  "${ROOT_DIR}/scripts/check_production_backend_hardware_qualification.sh" \
  "${ROOT_DIR}/scripts/check_c3eco_language_blocks.sh" \
  "${ROOT_DIR}/scripts/check_no_mandatory_test_skips.sh" \
  "${ROOT_DIR}/tests/integration/test_production_backend_hardware_qualification.sh" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h" \
  "${ROOT_DIR}/tests/deployment/test_container_kubernetes_hardening_negative.sh" \
  "${ROOT_DIR}/tests/tooling/formatter_messy.short" \
  "${ROOT_DIR}/tests/tooling/formatter_expected.short" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceTools.h" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceTools.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/SourceToolMain.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/LanguageServerMain.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/tooling/Makefile" \
  "${ROOT_DIR}/editors/vscode/package.json" \
  "${ROOT_DIR}/editors/vscode/language-configuration.json" \
  "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json" \
  "${ROOT_DIR}/docs/signed_release_publication.md" \
  "${ROOT_DIR}/docs/external_security_policy.md"; do
  require_file "${file}"
done

expected_header=$'id\tarea\tstatus\texisting_evidence\tmissing_evidence\tclosure_pr\tproduction_blocker'
[[ "$(head -n 1 "${MATRIX}")" == "${expected_header}" ]] || { echo "error: compiler test matrix header changed unexpectedly" >&2; exit 1; }
row_count="$(tail -n +2 "${MATRIX}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
implemented_count="$(awk -F '\t' 'NR > 1 && $3 == "implemented" { count++ } END { print count+0 }' "${MATRIX}")"
partial_count="$(awk -F '\t' 'NR > 1 && $3 == "partial" { count++ } END { print count+0 }' "${MATRIX}")"
open_count="$(awk -F '\t' 'NR > 1 && $3 == "open" { count++ } END { print count+0 }' "${MATRIX}")"
[[ "${row_count}" == 27 ]] || { echo "error: expected 27 compiler test coverage rows, found ${row_count}" >&2; exit 1; }
[[ "${implemented_count}" == 21 ]] || { echo "error: expected 21 implemented rows in the PR82 candidate" >&2; exit 1; }
[[ "${partial_count}" == 3 ]] || { echo "error: expected 3 partial rows in the PR82 candidate" >&2; exit 1; }
[[ "${open_count}" == 3 ]] || { echo "error: expected 3 open rows in the PR82 candidate" >&2; exit 1; }

invalid_status="$(awk -F '\t' 'NR > 1 && $3 != "implemented" && $3 != "partial" && $3 != "open" { print $1 ":" $3 }' "${MATRIX}")"
[[ -z "${invalid_status}" ]] || { echo "error: invalid compiler test matrix status values: ${invalid_status}" >&2; exit 1; }
duplicate_ids="$(tail -n +2 "${MATRIX}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_ids}" ]] || { echo "error: duplicate compiler test matrix IDs: ${duplicate_ids}" >&2; exit 1; }
for number in $(seq 1 27); do require_contains "${MATRIX}" "$(printf 'TST%03d' "${number}")"; done
for pr in $(seq 68 86); do require_contains "${PLAN}" "PR${pr} -"; done

for anchor in \
  'compiler_test_strategy_version: 2026-08-21-pr82' \
  'production_claim: false' \
  '21 implemented areas' \
  '3 partial areas' \
  '3 open areas' \
  'Required test layers for every implementation PR' \
  'A test passing because a dependency, device, backend, platform, container runtime or cluster was skipped is not production success evidence.' \
  'Signing source code is not signing evidence' \
  'External security scanners are mandatory evidence' \
  'A deployment manifest is not deployment evidence.' \
  'Formatter success is not inferred from source-text checks.' \
  'LSP/editor success is not inferred from JSON/text presence.' \
  'An unavailable compiler oracle in editor tooling must fail visibly' \
  'Backend SDK installation or hardware detection is not backend qualification.' \
  'CPU/GPU/TPU/NPU'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'Tests added or updated' \
  'Sanitizer coverage' \
  'Security or misuse tests' \
  'Performance or energy regression evidence' \
  'No mandatory production test is converted to an unconditional skip'; do
  require_contains "${TEMPLATE}" "${anchor}"
done

require_contains "${STATUS}" 'feature_status_version: 2026-08-21-pr82'
require_contains "${STATUS}" '21 implemented, 3 partial and 3 open'
require_contains "${STATUS}" 'Signed releases | Partial'
require_contains "${STATUS}" 'External vulnerability gate | Implemented'
require_contains "${STATUS}" 'Container and Kubernetes hardening | Implemented'
require_contains "${STATUS}" 'Formatter and linter | Implemented'
require_contains "${STATUS}" 'Syntax highlighting and LSP | Implemented for `shorthand.tooling.lsp.v1`'
require_contains "${STATUS}" 'Cross-platform reproducibility | Implemented'
require_contains "${STATUS}" 'Real ONNX Runtime CPU backend execution | Implemented for `linux-x64-cpu-v1` candidate'
require_contains "${MATRIX}" $'TST013\tplatform and compiler portability\timplemented'
require_contains "${MATRIX}" $'TST014\treproducible builds\timplemented'
require_contains "${MATRIX}" $'TST015\truntime ABI compatibility\timplemented'
require_contains "${MATRIX}" $'TST016\tpackaging and installed consumers\timplemented'
require_contains "${MATRIX}" $'TST017\tsigned protected release\tpartial'
require_contains "${MATRIX}" $'TST018\tdependency security and license policy\timplemented'
require_contains "${MATRIX}" $'TST019\tcontainer and Kubernetes deployment\timplemented'
require_contains "${MATRIX}" $'TST020\tformatter and linter correctness\timplemented'
require_contains "${MATRIX}" $'TST021\tsyntax highlighting and LSP protocol\timplemented'
require_contains "${MATRIX}" $'TST022\tlive backend and hardware qualification\timplemented'
require_contains "${MATRIX}" $'TST027\tproduction release-candidate gate\topen'

require_contains "${MATRIX}" $'TST023\tC3-ECO language and evidence\tpartial\tFirst-class C3-ECO grammar AST semantics evidence and SHD5101-SHD5104 claim-safety gate'
require_contains "${C3ECO_DOC}" 'c3eco_language_contract_version: shorthand.c3eco.language.v1'
require_contains "${C3ECO_DOC}" 'official_certification_granted: false'
require_contains "${ROOT_DIR}/scripts/check_c3eco_language_blocks.sh" 'PASS C3-ECO first-class language blocks grammar AST semantics evidence and claim-safety gate'
require_contains "${ROOT_DIR}/scripts/check_no_mandatory_test_skips.sh" 'PASS mandatory qualification zero-skip policy gate'

require_contains "${BACKEND_DOC}" 'backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1'
require_contains "${BACKEND_DOC}" 'production_scope: linux-x64-cpu-v1'
require_contains "${BACKEND_DOC}" 'accelerator_support_status: not_production_supported_until_live_qualified'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h" 'backend_device_not_production_qualified'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h" 'SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp" 'enforceProductionBackendQualification('
require_contains "${ROOT_DIR}/scripts/install_ci_onnxruntime_cpu.sh" '67db4dc1561f1e3fd42e619575c82c601ef89849afc7ea85a003abbac1a1a105'
require_contains "${ROOT_DIR}/scripts/check_production_backend_hardware_qualification.sh" 'PASS production backend and hardware qualification gate'
require_contains "${ROOT_DIR}/tests/integration/test_production_backend_hardware_qualification.sh" 'PASS production backend hardware qualification contract'
require_contains "${ROOT_DIR}/tests/integration/test_compiled_hook_onnxruntime_success.sh" 'Output: 42'

require_contains "${DEPLOY_DOC}" 'container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1'
require_contains "${DEPLOY_DOC}" 'tst019_status: implemented_after_exact_head_runtime_qualification'
require_contains "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" 'PASS container Kubernetes production hardening contract'
require_contains "${ROOT_DIR}/scripts/check_container_runtime.sh" 'PASS hardened container runtime'
require_contains "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" 'PASS ephemeral Kubernetes production gate'

require_contains "${TOOLING_DOC}" 'formatter_linter_contract_version: shorthand.tooling.format_lint.v1'
require_contains "${ROOT_DIR}/scripts/check_formatter_linter.sh" 'PASS formatter linter deterministic idempotent parse-preserving machine-diagnostic safe-fix gate'
require_contains "${LSP_DOC}" 'lsp_editor_contract_version: shorthand.tooling.lsp.v1'
require_contains "${ROOT_DIR}/scripts/check_lsp_editor.sh" 'PASS syntax highlighting LSP protocol compiler-diagnostics navigation cancellation UTF16 bounded-framing gate'
require_contains "${TOOLING_CI}" 'formatter-linter:'
require_contains "${TOOLING_CI}" 'lsp-editor:'
require_contains "${ROOT_DIR}/CMakeLists.txt" 'add_executable(shorthand_lsp'

require_contains "${RELEASE_CI}" 'environment: production-release'
require_contains "${RELEASE_CI}" 'id-token: write'
require_contains "${RELEASE_CI}" 'gh attestation verify'
require_contains "${CI}" 'security:'
require_contains "${CI}" 'queries: security-extended'
require_contains "${ROOT_DIR}/scripts/check_external_security_policy.sh" 'PASS external vulnerability SAST dependency and license policy gate'
require_contains "${ROOT_DIR}/scripts/check_signed_release_contract.sh" 'PASS signed release and protected publication contract gate'
require_contains "${ROOT_DIR}/scripts/check_semantic_differential.sh" 'PASS cross-mode semantic differential execution gate'
require_contains "${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh" 'PASS libFuzzer ASan LSan UBSan compiler-stage gate'
require_contains "${ROOT_DIR}/scripts/check_runtime_memory_sanitizer.sh" 'PASS runtime ASan LSan UBSan stress gate'
require_contains "${ROOT_DIR}/scripts/check_thread_sanitizer.sh" 'PASS mandatory ThreadSanitizer race gate'

printf 'TEST_COVERAGE implemented=%s partial=%s open=%s total=%s\n' "${implemented_count}" "${partial_count}" "${open_count}" "${row_count}"
printf 'PASS compiler test strategy and coverage audit gate\n'
