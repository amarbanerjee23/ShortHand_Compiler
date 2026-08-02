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
  "${ROOT_DIR}/docs/backend_failure_mode_matrix.md"
  "${ROOT_DIR}/docs/runtime_abi_api_stability.md"
  "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md"
  "${ROOT_DIR}/docs/runtime_production_packaging.md"
  "${ROOT_DIR}/docs/prometheus_scrape_host_adapter.md"
  "${ROOT_DIR}/docs/otlp_exporter_adapter.md"
  "${ROOT_DIR}/docs/ast_source_ranges.md"
  "${ROOT_DIR}/docs/diagnostics_coverage_matrix.md"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.h"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/SourceRange.cpp"
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/DiagnosticCodes.h"
  "${ROOT_DIR}/tests/diagnostics/diagnostics_coverage_matrix.tsv"
  "${ROOT_DIR}/tests/diagnostics/test_source_diagnostics.sh"
  "${ROOT_DIR}/tests/diagnostics/test_diagnostics_coverage_matrix.sh"
  "${ROOT_DIR}/scripts/check_ast_source_ranges.sh"
  "${ROOT_DIR}/scripts/check_diagnostics_coverage_matrix.sh"
  "${ROOT_DIR}/abi/runtime_public_symbols_v1.txt"
)
for file in "${required_files[@]}"; do require_file "${file}"; done

for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr65' \
  'PLAN_STATUS: active' \
  'LAST_COMPLETED_PR: 65' \
  'TARGET: enterprise production usage ready language' \
  'Desired outcome definition' \
  'Audit correction applied in PR #51' \
  'Hardware routing expansion applied in PR #55' \
  'Language objectives consolidation applied in PR #57' \
  'Backend failure-mode finalization applied in PR #58' \
  'Runtime ABI and API stability applied in PR #59' \
  'Runtime state and thread-safety applied in PR #60' \
  'Production build packaging applied in PR #61' \
  'Prometheus scrape endpoint host adapter applied in PR #62' \
  'OTLP exporter adapter applied in PR #63' \
  'AST source ranges applied in PR #64' \
  'Diagnostics coverage matrix applied in PR #65' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'After PR #65 is merged, approximately 14 implementation PRs remain.' \
  'Next recommended PR after PR #65:' \
  'PR66 - Full grammar and conformance matrix beta-0.2.' \
  'PR64 - AST source ranges across parser nodes | MERGED' \
  'PR65 - Diagnostics coverage matrix | MERGED' \
  'PR66 - Full grammar and conformance matrix beta-0.2 | PLANNED' \
  'PR67 - Parser robustness and negative corpus hardening | PLANNED' \
  'PR79 - MLIR lowering passes and production RC gate | PLANNED' \
  'remaining_planned_prs_total_from_pr51: 29' \
  'remaining_planned_prs_after_pr64: 15' \
  'remaining_planned_prs_after_pr65: 14'; do
  require_contains "${PLAN}" "${anchor}"
done

# Preserve previously guarded roadmap history.
for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr62' \
  'LAST_COMPLETED_PR: 62' \
  'After PR #62 is merged, approximately 17 implementation PRs remain.' \
  'Next recommended PR after PR #62:' \
  'PR63 - OTLP exporter adapter.' \
  'remaining_planned_prs_after_pr61: 18' \
  'remaining_planned_prs_after_pr62: 17' \
  'remaining_planned_prs_after_pr63: 16'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/backend_failure_mode_matrix.md" 'backend_failure_mode_matrix_status: finalized_v1'
require_contains "${ROOT_DIR}/docs/runtime_abi_api_stability.md" 'runtime_external_symbol_count: 25'
require_contains "${ROOT_DIR}/docs/ast_source_ranges.md" 'ast_source_range_status: parser_propagated_line_column_ranges'
require_contains "${ROOT_DIR}/docs/diagnostics_coverage_matrix.md" 'diagnostics_coverage_status: stable_coded_stage_matrix_guarded'
require_contains "${ROOT_DIR}/scripts/check_ast_source_ranges.sh" 'PASS AST source range guard'
require_contains "${ROOT_DIR}/scripts/check_diagnostics_coverage_matrix.sh" 'PASS diagnostics coverage matrix guard'

printf 'PASS production readiness PR plan gate\n'
