#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

expect_failure() {
  local name="$1"; shift
  if "$@" >"${TMP}/${name}.out" 2>"${TMP}/${name}.err"; then
    echo "error: deployment security negative unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
}

cp "${ROOT_DIR}/Dockerfile" "${TMP}/Dockerfile"
cp "${ROOT_DIR}/deploy/k8s/demo.yaml" "${TMP}/demo.yaml"
SHORTHAND_SECURITY_DOCKERFILE="${TMP}/Dockerfile" SHORTHAND_SECURITY_K8S_DEMO="${TMP}/demo.yaml" \
  bash "${ROOT_DIR}/scripts/check_deployment_security_baseline.sh" >/dev/null

sed -i 's/^USER 10001:10001$/USER root/' "${TMP}/Dockerfile"
expect_failure root-container env SHORTHAND_SECURITY_DOCKERFILE="${TMP}/Dockerfile" SHORTHAND_SECURITY_K8S_DEMO="${TMP}/demo.yaml" \
  bash "${ROOT_DIR}/scripts/check_deployment_security_baseline.sh"
grep -Eq 'final USER must not be root|fixed uid/gid' "${TMP}/root-container.err"

cp "${ROOT_DIR}/Dockerfile" "${TMP}/Dockerfile"
sed -i 's/readOnlyRootFilesystem: true/readOnlyRootFilesystem: false/' "${TMP}/demo.yaml"
expect_failure writable-rootfs env SHORTHAND_SECURITY_DOCKERFILE="${TMP}/Dockerfile" SHORTHAND_SECURITY_K8S_DEMO="${TMP}/demo.yaml" \
  bash "${ROOT_DIR}/scripts/check_deployment_security_baseline.sh"
grep -Fq 'readOnlyRootFilesystem=true' "${TMP}/writable-rootfs.err"

cp "${ROOT_DIR}/deploy/k8s/demo.yaml" "${TMP}/demo.yaml"
sed -i '/allowPrivilegeEscalation: false/d' "${TMP}/demo.yaml"
expect_failure privilege-escalation-default env SHORTHAND_SECURITY_DOCKERFILE="${TMP}/Dockerfile" SHORTHAND_SECURITY_K8S_DEMO="${TMP}/demo.yaml" \
  bash "${ROOT_DIR}/scripts/check_deployment_security_baseline.sh"
grep -Fq 'allowPrivilegeEscalation=false' "${TMP}/privilege-escalation-default.err"

printf 'PASS deployment security baseline positive and negative matrix\n'
