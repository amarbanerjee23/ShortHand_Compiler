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

require_file "${PLAN}"
require_contains "${PLAN}" 'production_readiness_plan_version: 2026-07-25-pr53'
require_contains "${PLAN}" 'PLAN_STATUS: active'
require_contains "${PLAN}" 'LAST_COMPLETED_PR: 53'
require_contains "${PLAN}" 'TARGET: enterprise production usage ready language'
require_contains "${PLAN}" 'Desired outcome definition'
require_contains "${PLAN}" 'Audit correction applied in PR #51'
require_contains "${PLAN}" 'Runtime ABI and API version stability gate'
require_contains "${PLAN}" 'Runtime state isolation and thread-safety policy'
require_contains "${PLAN}" 'Parser robustness and negative corpus hardening'
require_contains "${PLAN}" 'STATUS values: PLANNED, IN_PROGRESS, MERGED, BLOCKED, DEFERRED'
require_contains "${PLAN}" 'Update rule for every future PR'
require_contains "${PLAN}" 'Recommended path from PR #51 onward: 28 PRs total.'
require_contains "${PLAN}" 'Next recommended PR after PR #53:'
require_contains "${PLAN}" 'PR54 - OpenVINO optional live execution fixture.'
require_contains "${PLAN}" 'PR51 - Production readiness plan and tracking contract | MERGED'
require_contains "${PLAN}" 'PR52 - Backend live SDK matrix harness | MERGED'
require_contains "${PLAN}" 'PR53 - TensorRT optional live execution fixture | MERGED'
require_contains "${PLAN}" 'docs/tensorrt_optional_fixture.md'
require_contains "${PLAN}" 'scripts/check_tensorrt_optional_fixture.sh'
require_contains "${PLAN}" 'TensorRT and ONNX Runtime TensorRT rows now have an explicit unavailable-path proof'
require_contains "${PLAN}" 'PR54 - OpenVINO optional live execution fixture'
require_contains "${PLAN}" 'PR55 - LibTorch optional live execution fixture'
require_contains "${PLAN}" 'PR56 - Llama.cpp optional live execution fixture'
require_contains "${PLAN}" 'PR57 - Backend failure-mode matrix finalization'
require_contains "${PLAN}" 'PR58 - Runtime ABI and API version stability gate'
require_contains "${PLAN}" 'PR59 - Runtime state isolation and thread-safety policy'
require_contains "${PLAN}" 'PR60 - Production build packaging for runtime and AI bridge'
require_contains "${PLAN}" 'PR61 - Prometheus scrape endpoint host adapter'
require_contains "${PLAN}" 'PR62 - OTLP exporter adapter'
require_contains "${PLAN}" 'PR63 - AST source ranges across parser nodes'
require_contains "${PLAN}" 'PR64 - Diagnostics coverage matrix'
require_contains "${PLAN}" 'PR65 - Full grammar and conformance matrix beta-0.2'
require_contains "${PLAN}" 'PR66 - Parser robustness and negative corpus hardening'
require_contains "${PLAN}" 'PR67 - Module/import/package design and parser scaffold'
require_contains "${PLAN}" 'PR68 - Module resolver and codegen integration'
require_contains "${PLAN}" 'PR69 - Signed release and protected release workflow'
require_contains "${PLAN}" 'PR70 - External dependency vulnerability scan gate'
require_contains "${PLAN}" 'PR71 - Container and Kubernetes hardening'
require_contains "${PLAN}" 'PR72 - Formatter and linter baseline'
require_contains "${PLAN}" 'PR73 - Syntax highlighting and LSP skeleton'
require_contains "${PLAN}" 'PR74 - C3-ECO certification language blocks'
require_contains "${PLAN}" 'PR75 - C3-ECO scoring, report generation, and eco-regression'
require_contains "${PLAN}" 'PR76 - Authority-ready C3-ECO auditor bundle'
require_contains "${PLAN}" 'PR77 - MLIR generated dialect build integration'
require_contains "${PLAN}" 'PR78 - MLIR lowering passes and production RC gate'
require_contains "${PLAN}" 'Production readiness exit criteria'
require_contains "${PLAN}" 'Production claim rules'
require_contains "${PLAN}" 'remaining_planned_prs_total_from_pr51: 28'
require_contains "${PLAN}" 'remaining_planned_prs_after_pr53: 25'

printf 'PASS production readiness PR plan gate\n'
