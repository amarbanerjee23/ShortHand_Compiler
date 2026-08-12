#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKERFILE="${SHORTHAND_SECURITY_DOCKERFILE:-${ROOT_DIR}/Dockerfile}"
K8S_DEMO="${SHORTHAND_SECURITY_K8S_DEMO:-${ROOT_DIR}/deploy/k8s/demo.yaml}"

[[ -s "${DOCKERFILE}" ]] || { echo "error: Dockerfile security target missing: ${DOCKERFILE}" >&2; exit 1; }
[[ -s "${K8S_DEMO}" ]] || { echo "error: Kubernetes security target missing: ${K8S_DEMO}" >&2; exit 1; }

last_user="$(awk '/^[[:space:]]*USER[[:space:]]+/ {line=$0} END {print line}' "${DOCKERFILE}")"
[[ -n "${last_user}" ]] || { echo "error: Dockerfile must declare a non-root USER" >&2; exit 1; }
case "${last_user}" in
  *" 0"|*" 0:0"|*" root"|*" root:root")
    echo "error: Dockerfile final USER must not be root: ${last_user}" >&2
    exit 1
    ;;
esac
grep -Eq '^[[:space:]]*USER[[:space:]]+10001:10001[[:space:]]*$' "${DOCKERFILE}" || {
  echo "error: Dockerfile must run the release image as fixed uid/gid 10001:10001" >&2
  exit 1
}

require_k8s() {
  local pattern="$1" description="$2"
  grep -Eq -- "${pattern}" "${K8S_DEMO}" || {
    echo "error: Kubernetes demo missing security baseline: ${description}" >&2
    exit 1
  }
}

require_k8s '^[[:space:]]*automountServiceAccountToken:[[:space:]]*false[[:space:]]*$' 'automountServiceAccountToken=false'
require_k8s '^[[:space:]]*runAsNonRoot:[[:space:]]*true[[:space:]]*$' 'runAsNonRoot=true'
require_k8s '^[[:space:]]*runAsUser:[[:space:]]*10001[[:space:]]*$' 'runAsUser=10001'
require_k8s '^[[:space:]]*runAsGroup:[[:space:]]*10001[[:space:]]*$' 'runAsGroup=10001'
require_k8s '^[[:space:]]*seccompProfile:[[:space:]]*$' 'seccomp profile'
require_k8s '^[[:space:]]*type:[[:space:]]*RuntimeDefault[[:space:]]*$' 'RuntimeDefault seccomp profile'
require_k8s '^[[:space:]]*allowPrivilegeEscalation:[[:space:]]*false[[:space:]]*$' 'allowPrivilegeEscalation=false'
require_k8s '^[[:space:]]*readOnlyRootFilesystem:[[:space:]]*true[[:space:]]*$' 'readOnlyRootFilesystem=true'
require_k8s '^[[:space:]]*capabilities:[[:space:]]*$' 'capabilities policy'
require_k8s '^[[:space:]]*drop:[[:space:]]*$' 'capability drop list'
require_k8s '^[[:space:]]*-[[:space:]]*ALL[[:space:]]*$' 'drop ALL capabilities'

non_root_count="$(grep -Ec '^[[:space:]]*runAsNonRoot:[[:space:]]*true[[:space:]]*$' "${K8S_DEMO}")"
(( non_root_count >= 2 )) || {
  echo "error: Kubernetes demo must declare runAsNonRoot at pod and container level" >&2
  exit 1
}

printf 'PASS deployment security baseline Dockerfile_non_root=10001 Kubernetes_restricted=true\n'
