#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"

require_file() {
  local file="$1"
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  grep -Fq "${needle}" "${file}" || {
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  }
}

required_files=(
  "${PLAN}"
  "${ROOT_DIR}/docs/language_objectives.md"
  "${ROOT_DIR}/docs/language_grammar_ebnf.md"
  "${ROOT_DIR}/docs/language_spec.md"
  "${ROOT_DIR}/docs/language_versioning_and_conformance.md"
  "${ROOT_DIR}/docs/backend_failure_mode_matrix.md"
  "${ROOT_DIR}/docs/runtime_abi_api_stability.md"
  "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md"
  "${ROOT_DIR}/docs/runtime_production_packaging.md"
  "${ROOT_DIR}/docs/prometheus_scrape_host_adapter.md"
  "${ROOT_DIR}/docs/otlp_exporter_adapter.md"
  "${ROOT_DIR}/docs/ast_source_ranges.md"
  "${ROOT_DIR}/docs/diagnostics_coverage_matrix.md"
  "${ROOT_DIR}/docs/parser_robustness.md"
  "${ROOT_DIR}/docs/compiler_test_strategy.md"
  "${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
  "${ROOT_DIR}/.github/pull_request_template.md"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.h"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.cpp"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/parser/ParserLimits.h"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/DiagnosticCodes.h"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/main.cpp"
  "${ROOT_DIR}/tests/diagnostics/diagnostics_coverage_matrix.tsv"
  "${ROOT_DIR}/tests/conformance/grammar_matrix_beta_0_2.tsv"
  "${ROOT_DIR}/tests/conformance/manifest.txt"
  "${ROOT_DIR}/tests/parser/robustness/malformed_cases.tsv"
  "${ROOT_DIR}/scripts/check_ast_source_ranges.sh"
  "${ROOT_DIR}/scripts/check_diagnostics_coverage_matrix.sh"
  "${ROOT_DIR}/scripts/check_grammar_conformance_matrix.sh"
  "${ROOT_DIR}/scripts/check_parser_robustness.sh"
  "${ROOT_DIR}/scripts/check_language_versioning.sh"
  "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh"
  "${ROOT_DIR}/abi/runtime_public_symbols_v1.txt"
)
for file in "${required_files[@]}"; do require_file "${file}"; done

for anchor in \
  'production_readiness_plan_version: 2026-08-06-pr68' \
  'PLAN_STATUS: active' \
  'LAST_COMPLETED_PR: 68' \
  'BASELINE_LANGUAGE_VERSION: beta-0.2' \
  'TARGET: enterprise production usage ready language' \
  'ShortHand must become a production-grade compiled AI language' \
  'Test audit correction applied in PR #68' \
  'After PR #67, 18 production-readiness PRs are required.' \
  'After PR #68 is merged, 17 implementation PRs remain.' \
  'The revised path contains 35 PRs from PR51 through PR85.' \
  'Mandatory rule for every remaining PR' \
  'PR68 - Production test strategy, coverage audit and per-PR test contract' \
  'PR69 - Module, import and package syntax with AST scaffold' \
  'PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen' \
  'PR71 - Cross-mode semantic correctness and differential execution suite' \
  'PR72 - Continuous fuzzing, full sanitizer and concurrency race hardening' \
  'PR73 - Cross-platform toolchain matrix, CTest parity and reproducible builds' \
  'PR74 - Signed release and protected publication workflow' \
  'PR75 - External vulnerability, SAST, dependency and license policy gate' \
  'PR76 - Container and Kubernetes production hardening' \
  'PR77 - Formatter and linter baseline' \
  'PR78 - Syntax highlighting and LSP implementation' \
  'PR79 - Production backend and hardware qualification matrix' \
  'PR80 - Complete C3-ECO language blocks' \
  'PR81 - Measured scoring, reports and eco-regression' \
  'PR82 - Authority-ready C3-ECO auditor bundle' \
  'PR83 - Generated MLIR dialect build integration' \
  'PR84 - Semantic IR to MLIR lowering and production backend handoff' \
  'PR85 - Measured energy, performance and production RC gate' \
  'remaining_planned_prs_total_from_pr51_reaudited: 35' \
  'remaining_planned_prs_after_pr67_reaudited: 18' \
  'remaining_planned_prs_after_pr68: 17' \
  'Next recommended PR after PR #68:'; do
  require_contains "${PLAN}" "${anchor}"
done

for pr in $(seq 69 85); do
  require_contains "${PLAN}" "PR${pr} -"
  require_contains "${PLAN}" "| PR${pr} -"
done

# Preserve old guarded history while explicitly superseding the earlier estimate.
for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr62' \
  'LAST_COMPLETED_PR: 62' \
  'production_readiness_plan_version: 2026-08-02-pr63' \
  'LAST_COMPLETED_PR: 63' \
  'production_readiness_plan_version: 2026-08-02-pr66' \
  'LAST_COMPLETED_PR: 66' \
  'production_readiness_plan_version: 2026-08-02-pr67' \
  'LAST_COMPLETED_PR: 67' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'After PR #62 is merged, approximately 17 implementation PRs remain.' \
  'PR63 - OTLP exporter adapter.' \
  'After PR #65 is merged, approximately 14 implementation PRs remain.' \
  'PR66 - Full grammar and conformance matrix beta-0.2.' \
  'After PR #66 is merged, approximately 13 implementation PRs remain.' \
  'PR67 - Parser robustness and negative corpus hardening.' \
  'After PR #67 is merged, approximately 12 implementation PRs remain.' \
  'PR68 - Module/import/package design and parser scaffold.' \
  'The historical PR67 recommendation is superseded by the 2026-08-06 test re-audit.'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/backend_failure_mode_matrix.md" 'backend_failure_mode_matrix_status: finalized_v1'
require_contains "${ROOT_DIR}/docs/runtime_abi_api_stability.md" 'runtime_external_symbol_count: 25'
require_contains "${ROOT_DIR}/docs/ast_source_ranges.md" 'ast_source_range_status: parser_propagated_line_column_ranges'
require_contains "${ROOT_DIR}/docs/diagnostics_coverage_matrix.md" 'diagnostics_coverage_status: stable_coded_stage_matrix_guarded'
require_contains "${ROOT_DIR}/docs/language_grammar_ebnf.md" 'grammar_conformance_status: parser_accurate_matrix_guarded'
require_contains "${ROOT_DIR}/docs/language_versioning_and_conformance.md" 'shorthand.language.version: beta-0.2'
require_contains "${ROOT_DIR}/docs/parser_robustness.md" 'parser_robustness_status: bounded_fail_fast_negative_corpus_guarded'
require_contains "${ROOT_DIR}/docs/compiler_test_strategy.md" 'compiler_test_strategy_version: 2026-08-06-pr68'
require_contains "${ROOT_DIR}/scripts/check_compiler_test_strategy.sh" 'PASS compiler test strategy and coverage audit gate'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/main.cpp" 'mode == "parse"'
require_contains "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/parser/ParserLimits.h" 'MaxNestingDepth = 256U'

printf 'PASS production readiness PR plan gate\n'
