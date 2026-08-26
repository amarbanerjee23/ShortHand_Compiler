#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
CXX="${CXX:-g++}"
WORKER_PID=""

cleanup() {
  if [[ -n "${WORKER_PID}" ]] && kill -0 "${WORKER_PID}" 2>/dev/null; then
    kill -TERM "${WORKER_PID}" 2>/dev/null || true
    wait "${WORKER_PID}" 2>/dev/null || true
  fi
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

require_contains() {
  local file="$1"
  local text="$2"
  [[ -s "${file}" ]] || { echo "error: missing serving evidence: ${file}" >&2; exit 1; }
  grep -Fq "${text}" "${file}" || {
    echo "error: ${file} missing serving contract anchor: ${text}" >&2
    exit 1
  }
}

for tool in "${CXX}" timeout grep sed seq sleep head; do
  command -v "${tool}" >/dev/null 2>&1 || {
    echo "error: concurrent serving gate requires ${tool}" >&2
    exit 1
  }
done

COMMON_FLAGS=(
  -std=c++17
  -O2
  -g
  -pthread
  -Wall
  -Wextra
  -Wpedantic
  -I"${SRC_DIR}"
)
SERVING_SOURCE="${SRC_DIR}/serving/ServingRuntime.cpp"

"${CXX}" "${COMMON_FLAGS[@]}" \
  "${SERVING_SOURCE}" "${ROOT_DIR}/tests/runtime/test_serving_runtime.cpp" \
  -o "${WORK_DIR}/serving_unit"
timeout --signal=TERM --kill-after=2 30 "${WORK_DIR}/serving_unit"

"${CXX}" "${COMMON_FLAGS[@]}" \
  "${SERVING_SOURCE}" "${ROOT_DIR}/tests/runtime/serving_runtime_stress.cpp" \
  -o "${WORK_DIR}/serving_stress"
timeout --signal=TERM --kill-after=2 45 "${WORK_DIR}/serving_stress"

"${CXX}" "${COMMON_FLAGS[@]}" \
  "${SERVING_SOURCE}" "${SRC_DIR}/serving/ServingWorkerMain.cpp" \
  -o "${WORK_DIR}/shorthand_serving_worker"
timeout --signal=TERM --kill-after=2 20 "${WORK_DIR}/shorthand_serving_worker" self-test

STATE_FILE="${WORK_DIR}/health.json"
WORKER_LOG="${WORK_DIR}/worker.log"
"${WORK_DIR}/shorthand_serving_worker" serve \
  --tenant qualification \
  --state-file "${STATE_FILE}" \
  --workers 2 \
  --queue-capacity 16 \
  --max-in-flight 18 \
  --max-request-bytes 4096 \
  --max-response-bytes 4096 \
  --max-in-flight-request-bytes 65536 \
  --max-retained-result-bytes 65536 \
  --max-deadline-ms 5000 \
  --grace-ms 2000 \
  >"${WORKER_LOG}" 2>&1 &
WORKER_PID=$!

ready=0
for _ in $(seq 1 100); do
  if "${WORK_DIR}/shorthand_serving_worker" probe --state-file "${STATE_FILE}" --ready; then
    ready=1
    break
  fi
  kill -0 "${WORKER_PID}" 2>/dev/null || {
    echo "error: serving worker exited before readiness" >&2
    cat "${WORKER_LOG}" >&2 || true
    exit 1
  }
  sleep 0.05
done
[[ "${ready}" == 1 ]] || { echo "error: serving worker readiness deadline exceeded" >&2; exit 1; }
"${WORK_DIR}/shorthand_serving_worker" probe --state-file "${STATE_FILE}" --live

kill -USR1 "${WORKER_PID}"
draining=0
for _ in $(seq 1 100); do
  if ! "${WORK_DIR}/shorthand_serving_worker" probe --state-file "${STATE_FILE}" --ready \
     && "${WORK_DIR}/shorthand_serving_worker" probe --state-file "${STATE_FILE}" --live; then
    draining=1
    break
  fi
  sleep 0.05
done
[[ "${draining}" == 1 ]] || { echo "error: serving worker did not become unready while draining" >&2; exit 1; }

kill -TERM "${WORKER_PID}"
set +e
wait "${WORKER_PID}"
worker_status=$?
set -e
WORKER_PID=""
[[ "${worker_status}" == 0 ]] || {
  echo "error: serving worker graceful shutdown status=${worker_status}" >&2
  cat "${WORKER_LOG}" >&2 || true
  exit 1
}
grep -Fq 'SERVING_WORKER_READY contract=shorthand.serving.runtime.v1' "${WORKER_LOG}"
grep -Fq 'SERVING_WORKER_DRAINING' "${WORKER_LOG}"
grep -Fq 'SERVING_WORKER_STOPPED graceful=true' "${WORKER_LOG}"

if "${WORK_DIR}/shorthand_serving_worker" probe --state-file "${WORK_DIR}/missing.json" --live; then
  echo "error: missing health state unexpectedly passed liveness" >&2
  exit 1
fi

printf '%s\n' '{"schema":"shorthand.serving.health.v1","contract":"shorthand.serving.runtime.v1","live":true}trailing' \
  >"${WORK_DIR}/malformed-health.json"
if "${WORK_DIR}/shorthand_serving_worker" probe \
    --state-file "${WORK_DIR}/malformed-health.json" --live; then
  echo "error: malformed health state unexpectedly passed liveness" >&2
  exit 1
fi
head -c 65537 /dev/zero >"${WORK_DIR}/oversized-health.json"
if "${WORK_DIR}/shorthand_serving_worker" probe \
    --state-file "${WORK_DIR}/oversized-health.json" --live; then
  echo "error: oversized health state unexpectedly passed liveness" >&2
  exit 1
fi

require_contains "${ROOT_DIR}/docs/concurrent_serving_runtime.md" \
  'serving_runtime_contract: shorthand.serving.runtime.v1'
require_contains "${ROOT_DIR}/docs/concurrent_serving_runtime.md" \
  'tenant_isolation: dedicated_process_scope'
require_contains "${ROOT_DIR}/docs/concurrent_serving_runtime.md" \
  'production_claim: false'
require_contains "${ROOT_DIR}/CMakeLists.txt" 'add_library(shorthand_serving STATIC'
require_contains "${ROOT_DIR}/CMakeLists.txt" 'add_executable(shorthand_serving_worker'
require_contains "${ROOT_DIR}/Dockerfile" 'shorthand_serving_worker", "serve"'
require_contains "${ROOT_DIR}/deploy/k8s/production.yaml" 'shorthand_serving_worker'
require_contains "${ROOT_DIR}/deploy/k8s/production.yaml" '/tmp/shorthand-serving-health.json'
require_contains "${ROOT_DIR}/deploy/k8s/production.yaml" '                - --ready'
require_contains "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh" 'shorthand_serving_worker self-test'

printf 'PASS concurrent serving cancellation deadline backpressure quota isolation health load soak restart and graceful shutdown gate compiler=%s\n' "${CXX}"
