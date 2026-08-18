#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK_DIR="$(mktemp -d)"
BUILD_DIR="${WORK_DIR}/build"
INSTALL_DIR="${WORK_DIR}/install"
COLLECTOR="${WORK_DIR}/otlp_test_collector"
CURRENT_STAGE="initialization"
COLLECTOR_PIDS=()

cleanup() {
  local pid
  for pid in "${COLLECTOR_PIDS[@]:-}"; do
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill "${pid}" 2>/dev/null || true
      wait "${pid}" 2>/dev/null || true
    fi
  done
  rm -rf "${WORK_DIR}"
}

on_error() {
  local status=$?
  echo "FAIL OTLP exporter stage=${CURRENT_STAGE} line=${BASH_LINENO[0]} command=${BASH_COMMAND}" >&2
  find "${WORK_DIR}" -maxdepth 2 -type f -name '*.log' -exec sh -c 'echo "--- $1"; cat "$1"' _ {} \; >&2 2>/dev/null || true
  exit "${status}"
}

trap cleanup EXIT
trap on_error ERR

stage() {
  CURRENT_STAGE="$1"
  printf 'OTLP_EXPORTER_STAGE %s\n' "${CURRENT_STAGE}"
}

stage configure
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DSHORTHAND_BUILD_TESTING=OFF \
  -DSHORTHAND_BUILD_PROMETHEUS_ADAPTER=OFF

stage build
cmake --build "${BUILD_DIR}" --parallel 2 --target \
  shorthand_runtime shorthand_runtime_shared \
  shorthand_ai_bridge shorthand_ai_bridge_shared \
  shorthand_lsp \
  shorthand_otlp_exporter
EXPORTER="${BUILD_DIR}/shorthand_otlp_exporter"
[[ -x "${EXPORTER}" ]] || { echo "error: OTLP exporter executable was not produced" >&2; exit 1; }

stage build-test-collector
${CXX:-c++} -std=c++17 -Wall -Wextra -Wpedantic \
  "${ROOT_DIR}/tests/operations/OtlpTestCollector.cpp" \
  -o "${COLLECTOR}"

cat > "${WORK_DIR}/snapshot.json" <<'JSON'
{"schema":"shorthand.runtime.otlp_spans.v1","resource":{"service.name":"short-test"},"spans":[{"name":"short_ai_infer","status":"not_executed"}]}
JSON
cat > "${WORK_DIR}/invalid.json" <<'JSON'
{"schema":"unsupported"}
JSON

start_collector() {
  local name="$1"
  local statuses="$2"
  local collector_log="${WORK_DIR}/${name}.collector.log"
  local request_log="${WORK_DIR}/${name}.requests.log"
  "${COLLECTOR}" \
    --status-sequence "${statuses}" \
    --request-log "${request_log}" \
    >"${collector_log}" 2>&1 &
  COLLECTOR_PID=$!
  COLLECTOR_PIDS+=("${COLLECTOR_PID}")
  COLLECTOR_PORT=""
  for _ in $(seq 1 100); do
    COLLECTOR_PORT="$(sed -n 's/.*OTLP_TEST_COLLECTOR_LISTENING port=\([0-9][0-9]*\).*/\1/p' "${collector_log}" | tail -n 1)"
    [[ -n "${COLLECTOR_PORT}" ]] && break
    if ! kill -0 "${COLLECTOR_PID}" 2>/dev/null; then
      echo "error: test collector exited before readiness" >&2
      cat "${collector_log}" >&2 || true
      exit 1
    fi
    sleep 0.05
  done
  [[ "${COLLECTOR_PORT}" =~ ^[0-9]+$ ]] || { echo "error: collector readiness port was not reported" >&2; exit 1; }
  COLLECTOR_REQUEST_LOG="${request_log}"
  COLLECTOR_OUTPUT_LOG="${collector_log}"
}

wait_collector() {
  wait "${COLLECTOR_PID}"
  grep -Fq 'OTLP_TEST_COLLECTOR_STOPPED' "${COLLECTOR_OUTPUT_LOG}"
}

stage dry-run-runtime-snapshot
"${EXPORTER}" --dry-run >"${WORK_DIR}/dry-run.json" 2>"${WORK_DIR}/dry-run.log"
grep -Fq '"resourceSpans"' "${WORK_DIR}/dry-run.json"
grep -Fq '"shorthand.runtime.snapshot"' "${WORK_DIR}/dry-run.json"
grep -Fq 'status=dry_run delivered=false attempts=0 source=runtime' "${WORK_DIR}/dry-run.log"

stage successful-delivery
start_collector success 200
SHORTHAND_TEST_OTLP_AUTH='Bearer integration-secret' \
  "${EXPORTER}" \
    --host 127.0.0.1 \
    --port "${COLLECTOR_PORT}" \
    --input-file "${WORK_DIR}/snapshot.json" \
    --service-name shorthand-integration-test \
    --authorization-env SHORTHAND_TEST_OTLP_AUTH \
    --max-attempts 1 \
    >"${WORK_DIR}/success.out" 2>"${WORK_DIR}/success.err"
wait_collector
grep -Fq 'status=delivered delivered=true attempts=1 http_status=200 source=file' "${WORK_DIR}/success.out"
grep -Fq 'POST /v1/traces HTTP/1.1' "${COLLECTOR_REQUEST_LOG}"
grep -Fq 'Content-Type: application/json' "${COLLECTOR_REQUEST_LOG}"
grep -Fq 'Authorization: Bearer integration-secret' "${COLLECTOR_REQUEST_LOG}"
grep -Fq '"resourceSpans"' "${COLLECTOR_REQUEST_LOG}"
grep -Fq 'shorthand-integration-test' "${COLLECTOR_REQUEST_LOG}"
grep -Fq 'shorthand.runtime.otlp_spans.v1' "${COLLECTOR_REQUEST_LOG}"

stage retry-then-success
start_collector retry 503,200
"${EXPORTER}" \
  --host 127.0.0.1 \
  --port "${COLLECTOR_PORT}" \
  --input-file "${WORK_DIR}/snapshot.json" \
  --max-attempts 3 \
  --retry-backoff-ms 1 \
  >"${WORK_DIR}/retry.out" 2>"${WORK_DIR}/retry.err"
wait_collector
grep -Fq 'status=delivered delivered=true attempts=2 http_status=200 source=file' "${WORK_DIR}/retry.out"
grep -Fq 'attempt=1 outcome=retryable_failure http_status=503' "${WORK_DIR}/retry.err"
[[ "$(grep -c -- '---REQUEST' "${COLLECTOR_REQUEST_LOG}")" -eq 2 ]]

stage permanent-rejection
start_collector reject 400
if "${EXPORTER}" \
  --host 127.0.0.1 \
  --port "${COLLECTOR_PORT}" \
  --input-file "${WORK_DIR}/snapshot.json" \
  --max-attempts 3 \
  --retry-backoff-ms 1 \
  >"${WORK_DIR}/reject.out" 2>"${WORK_DIR}/reject.err"; then
  echo "error: HTTP 400 collector rejection unexpectedly reported success" >&2
  exit 1
fi
wait_collector
grep -Fq 'attempt=1 outcome=failure http_status=400' "${WORK_DIR}/reject.err"
grep -Fq 'status=failed delivered=false attempts=1 http_status=400' "${WORK_DIR}/reject.err"
[[ "$(grep -c -- '---REQUEST' "${COLLECTOR_REQUEST_LOG}")" -eq 1 ]]

stage invalid-snapshot-rejected
if "${EXPORTER}" --input-file "${WORK_DIR}/invalid.json" --dry-run \
  >"${WORK_DIR}/invalid.out" 2>"${WORK_DIR}/invalid.err"; then
  echo "error: unsupported snapshot schema unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'status=not_attempted delivered=false reason=unsupported_snapshot_schema' "${WORK_DIR}/invalid.err"

stage missing-authorization-rejected
env -u SHORTHAND_MISSING_AUTH \
  "${EXPORTER}" \
    --input-file "${WORK_DIR}/snapshot.json" \
    --authorization-env SHORTHAND_MISSING_AUTH \
    --dry-run \
    >"${WORK_DIR}/missing-auth.out" 2>"${WORK_DIR}/missing-auth.err" && {
      echo "error: missing authorization environment variable unexpectedly succeeded" >&2
      exit 1
    }
grep -Fq 'status=not_attempted delivered=false reason=authorization_env_missing' "${WORK_DIR}/missing-auth.err"

stage install
cmake --install "${BUILD_DIR}"
[[ -x "${INSTALL_DIR}/bin/shorthand_otlp_exporter" ]] || {
  echo "error: installed OTLP exporter executable is missing" >&2
  find "${INSTALL_DIR}" -maxdepth 4 -print >&2 || true
  exit 1
}
"${INSTALL_DIR}/bin/shorthand_otlp_exporter" --help >"${WORK_DIR}/installed-help.txt"
grep -Fq -- '--authorization-env NAME' "${WORK_DIR}/installed-help.txt"
grep -Fq -- '--snapshot-limit-bytes BYTES' "${WORK_DIR}/installed-help.txt"

stage abi-unchanged
bash "${ROOT_DIR}/scripts/check_runtime_abi_api_stability.sh"

stage complete
echo "PASS OTLP exporter adapter gate"
