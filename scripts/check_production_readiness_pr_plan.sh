#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  fi
}

for file in \
  "${PLAN}" \
  "${ROOT_DIR}/docs/language_objectives.md" \
  "${ROOT_DIR}/docs/backend_failure_mode_matrix.md" \
  "${ROOT_DIR}/docs/runtime_abi_api_stability.md" \
  "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md" \
  "${ROOT_DIR}/docs/runtime_production_packaging.md" \
  "${ROOT_DIR}/abi/runtime_public_symbols_v1.txt" \
  "${ROOT_DIR}/abi/shorthand_runtime_abi_v1.h" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp" \
  "${ROOT_DIR}/tests/runtime/test_runtime_state_thread_safety.sh" \
  "${ROOT_DIR}/tests/packaging/test_runtime_production_packaging.sh" \
  "${ROOT_DIR}/scripts/check_runtime_state_thread_safety.sh" \
  "${ROOT_DIR}/scripts/check_runtime_production_packaging.sh"; do
  require_file "${file}"
done

for anchor in \
  'production_readiness_plan_version: 2026-08-02-pr61' \
  'PLAN_STATUS: active' \
  'LAST_COMPLETED_PR: 61' \
  'TARGET: enterprise production usage ready language' \
  'Desired outcome definition' \
  'Audit correction applied in PR #51' \
  'Hardware routing expansion applied in PR #55' \
  'Language objectives consolidation applied in PR #57' \
  'Backend failure-mode finalization applied in PR #58' \
  'Runtime ABI and API stability applied in PR #59' \
  'Runtime state and thread-safety applied in PR #60' \
  'Production build packaging applied in PR #61' \
  'shorthand.language.objectives.version: 2026-07-29-v1' \
  'shorthand.backend_failure_mode_matrix.v1' \
  'Runtime ABI `1.0.0` freezes exactly 25 external `short_*` symbols.' \
  'Thread-safe does not mean multi-tenant isolated.' \
  'runtime_packaging_status: installable_static_shared_and_consumer_checked' \
  'Recommended path from PR #51 onward: 29 PRs total.' \
  'After PR #61 is merged, approximately 18 implementation PRs remain.' \
  'Next recommended PR after PR #61:' \
  'PR62 - Prometheus scrape endpoint host adapter.' \
  'PR51 - Production readiness plan and tracking contract | MERGED' \
  'PR52 - Backend live SDK matrix harness | MERGED' \
  'PR53 - TensorRT optional live execution fixture | MERGED' \
  'PR54 - OpenVINO optional live execution fixture | MERGED' \
  'PR55 - LibTorch optional live execution fixture | MERGED' \
  'PR56 - Hardware capability discovery and accelerator-aware routing | MERGED' \
  'PR57 - Llama.cpp optional live execution fixture | MERGED' \
  'PR58 - Backend failure-mode matrix finalization | MERGED' \
  'PR59 - Runtime ABI and API version stability gate | MERGED' \
  'PR60 - Runtime state isolation and thread-safety policy | MERGED' \
  'PR61 - Production build packaging for runtime and AI bridge | MERGED' \
  'PR62 - Prometheus scrape endpoint host adapter | PLANNED' \
  'PR63 - OTLP exporter adapter' \
  'PR64 - AST source ranges across parser nodes' \
  'PR65 - Diagnostics coverage matrix' \
  'PR66 - Full grammar and conformance matrix beta-0.2' \
  'PR67 - Parser robustness and negative corpus hardening' \
  'PR68 - Module/import/package design and parser scaffold' \
  'PR69 - Module resolver and codegen integration' \
  'PR70 - Signed release and protected release workflow' \
  'PR71 - External dependency vulnerability scan gate' \
  'PR72 - Container and Kubernetes hardening' \
  'PR73 - Formatter and linter baseline' \
  'PR74 - Syntax highlighting and LSP skeleton' \
  'PR75 - C3-ECO certification language blocks' \
  'PR76 - C3-ECO scoring, report generation, and eco-regression' \
  'PR77 - Authority-ready C3-ECO auditor bundle' \
  'PR78 - MLIR generated dialect build integration' \
  'PR79 - MLIR lowering passes and production RC gate' \
  'Production readiness exit criteria' \
  'Installable artifacts and successful consumer linking do not imply deployment readiness' \
  'remaining_planned_prs_total_from_pr51: 29' \
  'remaining_planned_prs_after_pr61: 18'; do
  require_contains "${PLAN}" "${anchor}"
done

require_contains "${ROOT_DIR}/docs/language_objectives.md" 'production_claim: false'
require_contains "${ROOT_DIR}/docs/backend_failure_mode_matrix.md" 'backend_failure_mode_matrix_status: finalized_v1'
require_contains "${ROOT_DIR}/docs/runtime_abi_api_stability.md" 'runtime_abi_contract_status: frozen_v1_symbol_manifest'
require_contains "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md" 'runtime_state_model: single_process_wide_default_context'
require_contains "${ROOT_DIR}/docs/runtime_state_and_thread_safety.md" 'runtime_multi_tenant_isolation: process_boundary_required'
require_contains "${ROOT_DIR}/docs/runtime_production_packaging.md" 'runtime_shared_soversion: 1'
require_contains "${ROOT_DIR}/docs/runtime_production_packaging.md" 'production_claim_boundary: packaging_gate_is_not_full_production_readiness'
require_contains "${ROOT_DIR}/scripts/check_runtime_state_thread_safety.sh" 'PASS runtime state isolation and thread-safety guard'
require_contains "${ROOT_DIR}/scripts/check_runtime_production_packaging.sh" 'PASS runtime production packaging guard'

printf 'PASS production readiness PR plan gate\n'
