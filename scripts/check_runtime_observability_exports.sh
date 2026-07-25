#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

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

require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h
require_file Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp
require_file tests/codegen/test_runtime_observability_exports.sh
require_file docs/runtime_observability_exports.md

require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_prometheus_metrics'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h 'short_runtime_otlp_spans_json'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand.runtime.otlp_spans.v1'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'shorthand_runtime_infer_total'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'short_runtime_prometheus_metrics'
require_contains Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp 'short_runtime_otlp_spans_json'
require_contains tests/codegen/test_runtime_observability_exports.sh 'PASS runtime observability export gate'
require_contains tests/codegen/test_runtime_observability_exports.sh 'shorthand.runtime.otlp_spans.v1'
require_contains docs/runtime_observability_exports.md 'Prometheus-style metrics'
require_contains docs/runtime_observability_exports.md 'OTLP-like span JSON'

bash tests/codegen/test_runtime_observability_exports.sh

echo "PASS runtime observability export gate"
