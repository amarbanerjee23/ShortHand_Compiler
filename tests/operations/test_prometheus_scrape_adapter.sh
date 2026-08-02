#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK_DIR="$(mktemp -d)"
BUILD_DIR="${WORK_DIR}/build"
INSTALL_DIR="${WORK_DIR}/install"
SERVER_LOG="${WORK_DIR}/server.log"
SERVER_PID=""
CURRENT_STAGE="initialization"

cleanup() {
  if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
    kill "${SERVER_PID}" 2>/dev/null || true
    wait "${SERVER_PID}" 2>/dev/null || true
  fi
  rm -rf "${WORK_DIR}"
}

on_error() {
  local status=$?
  echo "FAIL Prometheus adapter stage=${CURRENT_STAGE} line=${BASH_LINENO[0]} command=${BASH_COMMAND}" >&2
  cat "${SERVER_LOG}" >&2 2>/dev/null || true
  exit "${status}"
}

trap cleanup EXIT
trap on_error ERR

stage() {
  CURRENT_STAGE="$1"
  printf 'PROMETHEUS_ADAPTER_STAGE %s\n' "${CURRENT_STAGE}"
}

stage configure
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DSHORTHAND_BUILD_TESTING=OFF

stage build
cmake --build "${BUILD_DIR}" --parallel 2 --target \
  shorthand_runtime shorthand_runtime_shared \
  shorthand_ai_bridge shorthand_ai_bridge_shared \
  shorthand_prometheus_adapter

ADAPTER="${BUILD_DIR}/shorthand_prometheus_adapter"
[[ -x "${ADAPTER}" ]] || { echo "error: adapter executable was not produced" >&2; exit 1; }

stage start-loopback-server
"${ADAPTER}" \
  --listen 127.0.0.1 \
  --port 0 \
  --max-requests 4 \
  --read-timeout-ms 2000 \
  --request-limit-bytes 4096 \
  >"${SERVER_LOG}" 2>&1 &
SERVER_PID=$!

port=""
for _ in $(seq 1 100); do
  port="$(sed -n 's/.*PROMETHEUS_ADAPTER_LISTENING .* port=\([0-9][0-9]*\).*/\1/p' "${SERVER_LOG}" | tail -n 1)"
  [[ -n "${port}" ]] && break
  if ! kill -0 "${SERVER_PID}" 2>/dev/null; then
    echo "error: adapter exited before readiness" >&2
    cat "${SERVER_LOG}" >&2 || true
    exit 1
  fi
  sleep 0.05
done
[[ "${port}" =~ ^[0-9]+$ ]] || { echo "error: adapter readiness port was not reported" >&2; exit 1; }

http_request() {
  local request="$1"
  local output="$2"
  exec 3<>"/dev/tcp/127.0.0.1/${port}"
  printf '%b' "${request}" >&3
  cat <&3 >"${output}"
  exec 3<&-
  exec 3>&-
}

stage metrics-endpoint
http_request 'GET /metrics HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' "${WORK_DIR}/metrics.response"
grep -Fq 'HTTP/1.1 200 OK' "${WORK_DIR}/metrics.response"
grep -Fqi 'Content-Type: text/plain; version=0.0.4; charset=utf-8' "${WORK_DIR}/metrics.response"
grep -Fq 'Cache-Control: no-store' "${WORK_DIR}/metrics.response"
grep -Fq 'X-Content-Type-Options: nosniff' "${WORK_DIR}/metrics.response"
grep -Fq 'shorthand_runtime_infer_total 0' "${WORK_DIR}/metrics.response"
grep -Fq 'shorthand_runtime_models 0' "${WORK_DIR}/metrics.response"
grep -Fq 'shorthand_runtime_last_infer_info{status="not_executed",backend="none",reason="not_run"} 1' "${WORK_DIR}/metrics.response"

stage health-endpoint
http_request 'GET /healthz HTTP/1.1\r\nHost: localhost\r\n\r\n' "${WORK_DIR}/health.response"
grep -Fq 'HTTP/1.1 200 OK' "${WORK_DIR}/health.response"
grep -Fq $'\r\nok\n' "${WORK_DIR}/health.response"

stage not-found-endpoint
http_request 'GET /private HTTP/1.1\r\nHost: localhost\r\n\r\n' "${WORK_DIR}/not-found.response"
grep -Fq 'HTTP/1.1 404 Not Found' "${WORK_DIR}/not-found.response"

stage method-policy
http_request 'POST /metrics HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n' "${WORK_DIR}/method.response"
grep -Fq 'HTTP/1.1 405 Method Not Allowed' "${WORK_DIR}/method.response"
grep -Fq 'Allow: GET' "${WORK_DIR}/method.response"

stage deterministic-shutdown
wait "${SERVER_PID}"
SERVER_PID=""
grep -Fq 'PROMETHEUS_ADAPTER_STOPPED requests=4' "${SERVER_LOG}"

stage invalid-listen-rejected
if "${ADAPTER}" --listen not-an-ip --port 9464 >"${WORK_DIR}/invalid.out" 2>&1; then
  echo "error: invalid listen address unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'invalid IPv4 listen address' "${WORK_DIR}/invalid.out"

stage install
cmake --install "${BUILD_DIR}"
[[ -x "${INSTALL_DIR}/bin/shorthand_prometheus_adapter" ]] || {
  echo "error: installed Prometheus adapter executable is missing" >&2
  find "${INSTALL_DIR}" -maxdepth 4 -print >&2 || true
  exit 1
}

stage abi-unchanged
bash "${ROOT_DIR}/scripts/check_runtime_abi_api_stability.sh"

stage complete
echo "PASS Prometheus scrape endpoint host adapter gate"
