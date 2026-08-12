#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

expect_failure() {
  local name="$1"; shift
  if "$@" >"${TMP}/${name}.out" 2>"${TMP}/${name}.err"; then
    echo "error: container/Kubernetes hardening negative unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
}

reset_fixture() {
  cp "${ROOT_DIR}/Dockerfile" "${TMP}/Dockerfile"
  cp "${ROOT_DIR}/deploy/k8s/production.yaml" "${TMP}/production.yaml"
}

run_gate() {
  env SHORTHAND_CONTAINER_DOCKERFILE="${TMP}/Dockerfile" \
      SHORTHAND_K8S_PRODUCTION_MANIFEST="${TMP}/production.yaml" \
      bash "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh"
}

reset_fixture
run_gate >/dev/null

reset_fixture
sed -i 's/^USER 10001:10001$/USER root/' "${TMP}/Dockerfile"
expect_failure root-image run_gate
grep -Fq 'fixed non-root uid/gid' "${TMP}/root-image.err"

reset_fixture
sed -i 's/readOnlyRootFilesystem: true/readOnlyRootFilesystem: false/' "${TMP}/production.yaml"
expect_failure writable-rootfs run_gate
grep -Fq 'read-only root filesystem' "${TMP}/writable-rootfs.err"

reset_fixture
sed -i '/readinessProbe:/,/failureThreshold: 3/d' "${TMP}/production.yaml"
expect_failure missing-readiness run_gate
grep -Fq 'readiness probe' "${TMP}/missing-readiness.err"

reset_fixture
sed -i 's/^            limits:/            noLimits:/' "${TMP}/production.yaml"
expect_failure missing-limits run_gate
grep -Fq 'resource limits' "${TMP}/missing-limits.err"

reset_fixture
sed -i 's/^  egress: \[\]$/  egress: [{}]/' "${TMP}/production.yaml"
expect_failure open-egress run_gate
grep -Fq 'default-deny egress' "${TMP}/open-egress.err"

reset_fixture
sed -i '/allowPrivilegeEscalation: false/a\            privileged: true' "${TMP}/production.yaml"
expect_failure privileged-container run_gate
grep -Fq 'privileged container' "${TMP}/privileged-container.err"

printf 'PASS container Kubernetes hardening positive and negative matrix\n'
