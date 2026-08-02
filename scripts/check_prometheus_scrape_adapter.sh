#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/prometheus_scrape_host_adapter.md"
SOURCE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp"
TEST="${ROOT_DIR}/tests/operations/test_prometheus_scrape_adapter.sh"
CMAKE_FILE="${ROOT_DIR}/CMakeLists.txt"

require_contains() {
  local file="$1"
  local needle="$2"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required Prometheus adapter text: ${needle}" >&2
    exit 1
  fi
}

for file in "${DOC}" "${SOURCE}" "${TEST}" "${CMAKE_FILE}"; do
  [[ -f "${file}" ]] || { echo "error: missing required file: ${file}" >&2; exit 1; }
done

bash -n "${TEST}"

for anchor in \
  'prometheus_scrape_adapter_contract_version: 1.0.0' \
  'prometheus_scrape_adapter_status: loopback_default_bounded_http_metrics_host' \
  'prometheus_metrics_source: frozen_runtime_short_runtime_prometheus_metrics' \
  'runtime_abi_change: none' \
  'runtime_external_symbol_count: 25' \
  'production_claim_boundary: scrape_adapter_is_not_hardened_public_ingress'; do
  require_contains "${DOC}" "${anchor}"
done

for anchor in \
  'listen_address = "127.0.0.1"' \
  'request_limit_bytes = 8192' \
  'read_timeout_ms = 2000' \
  'short_runtime_prometheus_metrics()' \
  'text/plain; version=0.0.4; charset=utf-8' \
  'HTTP/1.1 ' \
  'Request Header Fields Too Large' \
  'Method Not Allowed' \
  'PROMETHEUS_ADAPTER_LISTENING' \
  'non-loopback binding requires external authentication, TLS, and network policy'; do
  require_contains "${SOURCE}" "${anchor}"
done

for anchor in \
  'add_executable(shorthand_prometheus_adapter' \
  'PrometheusScrapeAdapter.cpp' \
  'target_link_libraries(shorthand_prometheus_adapter PRIVATE shorthand_runtime)' \
  'install(TARGETS shorthand_prometheus_adapter' \
  'NAME prometheus_scrape_adapter'; do
  require_contains "${CMAKE_FILE}" "${anchor}"
done

for anchor in \
  '/dev/tcp/127.0.0.1/${port}' \
  'HTTP/1.1 200 OK' \
  'shorthand_runtime_infer_total' \
  'HTTP/1.1 404 Not Found' \
  'HTTP/1.1 405 Method Not Allowed' \
  'PROMETHEUS_ADAPTER_STOPPED requests=4' \
  'check_runtime_abi_api_stability.sh' \
  'PASS Prometheus scrape endpoint host adapter gate'; do
  require_contains "${TEST}" "${anchor}"
done

bash "${TEST}"

echo "PASS Prometheus scrape endpoint host adapter guard"
