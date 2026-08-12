#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${1:-shorthand:pr78}"
EXPECTED_ARCH="${2:-amd64}"
TMP="$(mktemp -d)"
CONTAINER_ID=""
cleanup() {
  if [[ -n "${CONTAINER_ID}" ]]; then docker rm -f "${CONTAINER_ID}" >/dev/null 2>&1 || true; fi
  rm -rf "${TMP}"
}
trap cleanup EXIT

fail_stage() {
  local stage="$1"
  echo "error: hardened container qualification failed at stage=${stage}" >&2
  [[ -f "${TMP}/${stage}.out" ]] && { echo "--- ${stage} stdout ---" >&2; cat "${TMP}/${stage}.out" >&2; }
  [[ -f "${TMP}/${stage}.err" ]] && { echo "--- ${stage} stderr ---" >&2; cat "${TMP}/${stage}.err" >&2; }
  exit 1
}

run_stage() {
  local stage="$1"; shift
  if ! "$@" >"${TMP}/${stage}.out" 2>"${TMP}/${stage}.err"; then fail_stage "${stage}"; fi
}

command -v docker >/dev/null 2>&1 || { echo "error: docker is required for container runtime qualification" >&2; exit 1; }
docker image inspect "${IMAGE}" >/dev/null 2>&1 || { echo "error: container image is unavailable: ${IMAGE}" >&2; exit 1; }

actual_arch="$(docker image inspect --format '{{.Architecture}}' "${IMAGE}")"
[[ "${actual_arch}" == "${EXPECTED_ARCH}" ]] || {
  echo "error: container architecture mismatch expected=${EXPECTED_ARCH} actual=${actual_arch}" >&2
  exit 1
}

common=(--rm --read-only --tmpfs /tmp:rw,noexec,nosuid,nodev,size=64m,mode=1777 --tmpfs /work:rw,nosuid,nodev,size=128m,mode=1777 --cap-drop ALL --security-opt no-new-privileges=true --network none)
if ! uid="$(docker run "${common[@]}" "${IMAGE}" id -u 2>"${TMP}/identity_uid.err")"; then fail_stage identity_uid; fi
if ! gid="$(docker run "${common[@]}" "${IMAGE}" id -g 2>"${TMP}/identity_gid.err")"; then fail_stage identity_gid; fi
[[ "${uid}" == 10001 ]] || { echo "error: runtime image uid must be 10001, got ${uid}" >&2; exit 1; }
[[ "${gid}" == 10001 ]] || { echo "error: runtime image gid must be 10001, got ${gid}" >&2; exit 1; }

run_stage interpreter docker run "${common[@]}" "${IMAGE}"
if ! diff -u "${ROOT_DIR}/tests/semantic/differential/core_control.expected" "${TMP}/interpreter.out"; then fail_stage interpreter; fi

run_stage writable docker run "${common[@]}" "${IMAGE}" /bin/bash -lc 'touch /tmp/shorthand-writable /work/shorthand-work-writable && test -f /tmp/shorthand-writable && test -f /work/shorthand-work-writable'
run_stage readonly docker run "${common[@]}" "${IMAGE}" /bin/bash -lc 'if touch /etc/shorthand-forbidden 2>/dev/null; then exit 1; else exit 0; fi'
run_stage native docker run "${common[@]}" --workdir /work "${IMAGE}" /bin/bash -lc \
  'short_hand /opt/shorthand/smoke/core_control.short compile-native && test -x ./core_control && ./core_control'
if ! diff -u "${ROOT_DIR}/tests/semantic/differential/core_control.expected" "${TMP}/native.out"; then fail_stage native; fi

health_test="$(docker image inspect --format '{{json .Config.Healthcheck.Test}}' "${IMAGE}")"
[[ "${health_test}" == *'core_control.short'* && "${health_test}" == *'parse'* ]] || {
  echo "error: image healthcheck does not execute the ShortHand parser" >&2
  exit 1
}

CONTAINER_ID="$(docker run -d --read-only --tmpfs /tmp:rw,noexec,nosuid,nodev,size=64m,mode=1777 --tmpfs /work:rw,nosuid,nodev,size=128m,mode=1777 --cap-drop ALL --security-opt no-new-privileges=true --network none \
  "${IMAGE}" /bin/bash -lc 'trap "exit 0" TERM INT; while true; do sleep 3600 & wait $!; done')"
healthy=0
for _ in $(seq 1 45); do
  status="$(docker inspect --format '{{if .State.Health}}{{.State.Health.Status}}{{else}}none{{end}}' "${CONTAINER_ID}")"
  if [[ "${status}" == healthy ]]; then healthy=1; break; fi
  if [[ "${status}" == unhealthy ]]; then
    docker inspect "${CONTAINER_ID}" >&2
    echo "error: container healthcheck became unhealthy" >&2
    exit 1
  fi
  sleep 2
done
[[ "${healthy}" == 1 ]] || { echo "error: container did not become healthy within bounded time" >&2; exit 1; }

docker stop --time 5 "${CONTAINER_ID}" >/dev/null
exit_code="$(docker inspect --format '{{.State.ExitCode}}' "${CONTAINER_ID}")"
[[ "${exit_code}" == 0 ]] || { echo "error: graceful container shutdown exit code=${exit_code}" >&2; exit 1; }
docker rm "${CONTAINER_ID}" >/dev/null
CONTAINER_ID=""

printf 'PASS hardened container runtime image=%s arch=%s uid=%s gid=%s native_compile=true health=healthy graceful_shutdown=true\n' "${IMAGE}" "${actual_arch}" "${uid}" "${gid}"
