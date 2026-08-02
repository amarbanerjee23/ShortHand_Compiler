#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/otlp_exporter_adapter.md"
SOURCE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/operations/OtlpExporterAdapter.cpp"
TEST="${ROOT_DIR}/tests/operations/test_otlp_exporter_adapter.sh"
COLLECTOR="${ROOT_DIR}/tests/operations/OtlpTestCollector.cpp"
CMAKE_FILE="${ROOT_DIR}/CMakeLists.txt"

require_contains() {
  local file="$1"
  local needle="$2"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required OTLP exporter text: ${needle}" >&2
    exit 1
  fi
}

for file in "${DOC}" "${SOURCE}" "${TEST}" "${COLLECTOR}" "${CMAKE_FILE}"; do
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
done

bash -n "${TEST}"

for anchor in \
  'otlp_exporter_contract_version: 1.0.0' \
  'otlp_exporter_status: bounded_one_shot_otlp_http_trace_delivery' \
  'source_snapshot_schema: shorthand.runtime.otlp_spans.v1' \
  'runtime_abi_change: none' \
  'runtime_external_symbol_count: 25' \
  'delivery_claim_boundary: http_acceptance_is_not_end_to_end_trace_storage'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'host = "127.0.0.1"' \
  'port = 4318' \
  'path = "/v1/traces"' \
  'short_runtime_otlp_spans_json()' \
  'resourceSpans' \
  'Authorization: ' \
  'connect_failed_or_timed_out' \
  'response_limit_exceeded' \
  'status=delivered delivered=true' \
  'status=failed delivered=false'; do
  require_contains "${SOURCE}" "${anchor}"
done

for anchor in \
  'add_executable(shorthand_otlp_exporter' \
  'OtlpExporterAdapter.cpp' \
  'target_link_libraries(shorthand_otlp_exporter PRIVATE shorthand_runtime)' \
  'install(TARGETS shorthand_otlp_exporter' \
  'NAME otlp_exporter_adapter'; do
  require_contains "${CMAKE_FILE}" "${anchor}"
done

for anchor in \
  'OTLP_TEST_COLLECTOR_LISTENING' \
  'POST /v1/traces HTTP/1.1' \
  'status=delivered delivered=true attempts=2' \
  'attempt=1 outcome=retryable_failure http_status=503' \
  'status=failed delivered=false attempts=1 http_status=400' \
  'reason=unsupported_snapshot_schema' \
  'reason=authorization_env_missing' \
  'check_runtime_abi_api_stability.sh' \
  'PASS OTLP exporter adapter gate'; do
  require_contains "${TEST}" "${anchor}"
done

bash "${TEST}"

echo "PASS OTLP exporter adapter guard"
