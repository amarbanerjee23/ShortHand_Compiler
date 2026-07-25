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
require_contains "${PLAN}" 'production_readiness_plan_version: 2026-07-25-pr51'
require_contains "${PLAN}" 'PLAN_STATUS: active'
require_contains "${PLAN}" 'STATUS values: PLANNED, IN_PROGRESS, MERGED, BLOCKED, DEFERRED'
require_contains "${PLAN}" 'Update rule for every future PR'
require_contains "${PLAN}" 'Recommended path from PR #51 onward: 25 PRs total.'
require_contains "${PLAN}" 'PR51 - Production readiness plan and tracking contract'
require_contains "${PLAN}" 'PR52 - Backend live SDK matrix harness'
require_contains "${PLAN}" 'PR53 - TensorRT optional live execution fixture'
require_contains "${PLAN}" 'PR54 - OpenVINO optional live execution fixture'
require_contains "${PLAN}" 'PR55 - LibTorch optional live execution fixture'
require_contains "${PLAN}" 'PR56 - Llama.cpp optional live execution fixture'
require_contains "${PLAN}" 'PR57 - Backend failure-mode matrix finalization'
require_contains "${PLAN}" 'PR58 - Production build packaging for runtime and AI bridge'
require_contains "${PLAN}" 'PR59 - Prometheus scrape endpoint host adapter'
require_contains "${PLAN}" 'PR60 - OTLP exporter adapter'
require_contains "${PLAN}" 'PR61 - AST source ranges across parser nodes'
require_contains "${PLAN}" 'PR62 - Diagnostics coverage matrix'
require_contains "${PLAN}" 'PR63 - Full grammar and conformance matrix beta-0.2'
require_contains "${PLAN}" 'PR64 - Module/import/package design and parser scaffold'
require_contains "${PLAN}" 'PR65 - Module resolver and codegen integration'
require_contains "${PLAN}" 'PR66 - Signed release and protected release workflow'
require_contains "${PLAN}" 'PR67 - External dependency vulnerability scan gate'
require_contains "${PLAN}" 'PR68 - Container and Kubernetes hardening'
require_contains "${PLAN}" 'PR69 - Formatter and linter baseline'
require_contains "${PLAN}" 'PR70 - Syntax highlighting and LSP skeleton'
require_contains "${PLAN}" 'PR71 - C3-ECO certification language blocks'
require_contains "${PLAN}" 'PR72 - C3-ECO scoring, report generation, and eco-regression'
require_contains "${PLAN}" 'PR73 - Authority-ready C3-ECO auditor bundle'
require_contains "${PLAN}" 'PR74 - MLIR generated dialect build integration'
require_contains "${PLAN}" 'PR75 - MLIR lowering passes and production RC gate'
require_contains "${PLAN}" 'remaining_planned_prs_including_this_file: 25'
require_contains "${PLAN}" 'remaining_planned_prs_after_this_file: 24'

printf 'PASS production readiness PR plan gate\n'
