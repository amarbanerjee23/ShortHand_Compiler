#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKERFILE="${SHORTHAND_CONTAINER_DOCKERFILE:-${ROOT_DIR}/Dockerfile}"
DOCKERIGNORE="${SHORTHAND_CONTAINER_DOCKERIGNORE:-${ROOT_DIR}/.dockerignore}"
MANIFEST="${SHORTHAND_K8S_PRODUCTION_MANIFEST:-${ROOT_DIR}/deploy/k8s/production.yaml}"

[[ -s "${DOCKERFILE}" ]] || { echo "error: production Dockerfile missing: ${DOCKERFILE}" >&2; exit 1; }
[[ -s "${DOCKERIGNORE}" ]] || { echo "error: production .dockerignore missing: ${DOCKERIGNORE}" >&2; exit 1; }
[[ -s "${MANIFEST}" ]] || { echo "error: production Kubernetes manifest missing: ${MANIFEST}" >&2; exit 1; }

require_file_pattern() {
  local file="$1" pattern="$2" description="$3"
  grep -Eq -- "${pattern}" "${file}" || {
    echo "error: ${file} missing production hardening contract: ${description}" >&2
    exit 1
  }
}

forbid_file_pattern() {
  local file="$1" pattern="$2" description="$3"
  if grep -Eq -- "${pattern}" "${file}"; then
    echo "error: ${file} violates production hardening contract: ${description}" >&2
    exit 1
  fi
}

require_file_pattern "${DOCKERFILE}" '^FROM[[:space:]]+ubuntu:24\.04[[:space:]]+AS[[:space:]]+builder[[:space:]]*$' 'multi-stage builder image'
require_file_pattern "${DOCKERFILE}" '^FROM[[:space:]]+ubuntu:24\.04[[:space:]]+AS[[:space:]]+runtime[[:space:]]*$' 'separate runtime image'
require_file_pattern "${DOCKERFILE}" '^USER[[:space:]]+10001:10001[[:space:]]*$' 'fixed non-root uid/gid 10001:10001'
require_file_pattern "${DOCKERFILE}" '^HEALTHCHECK[[:space:]]' 'container healthcheck'
require_file_pattern "${DOCKERFILE}" 'core_control\.short.*parse' 'healthcheck must execute the real ShortHand parser'
require_file_pattern "${DOCKERFILE}" 'COPY --from=builder --chown=10001:10001' 'runtime artifacts copied with non-root ownership'
require_file_pattern "${DOCKERFILE}" 'runtime_lib' 'runtime library must be built in builder stage'
require_file_pattern "${DOCKERFILE}" 'SHORTHAND_RUNTIME_LIB=' 'native runtime library path'
require_file_pattern "${DOCKERFILE}" 'SHORTHAND_NATIVE_LINKER=' 'native linker contract'

for context_guard in '^\.git$' '^Compiler_new_ws/Short_Hand/build$' '^Compiler_new_ws/\.metadata$' '^deprecated$'; do
  require_file_pattern "${DOCKERIGNORE}" "${context_guard}" "bounded container build context (${context_guard})"
done

runtime_section="$(awk 'BEGIN{runtime=0} /^FROM[[:space:]]+ubuntu:24\.04[[:space:]]+AS[[:space:]]+runtime/{runtime=1} runtime{print}' "${DOCKERFILE}")"
[[ -n "${runtime_section}" ]] || { echo "error: Dockerfile runtime stage could not be isolated" >&2; exit 1; }
for forbidden in 'build-essential' 'ninja-build' 'cmake' 'bison' 'libfl-dev'; do
  if printf '%s\n' "${runtime_section}" | grep -Fq "${forbidden}"; then
    echo "error: runtime image contains build-only package: ${forbidden}" >&2
    exit 1
  fi
done

require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+Namespace[[:space:]]*$' 'dedicated namespace'
require_file_pattern "${MANIFEST}" 'pod-security\.kubernetes\.io/enforce:[[:space:]]+restricted' 'restricted Pod Security enforcement'
require_file_pattern "${MANIFEST}" 'pod-security\.kubernetes\.io/enforce-version:[[:space:]]+v1\.36' 'pinned Pod Security version'
require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+ServiceAccount[[:space:]]*$' 'dedicated service account'
require_file_pattern "${MANIFEST}" 'automountServiceAccountToken:[[:space:]]+false' 'service-account token disabled'
require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+ResourceQuota[[:space:]]*$' 'resource quota'
require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+Deployment[[:space:]]*$' 'Deployment workload'
require_file_pattern "${MANIFEST}" 'replicas:[[:space:]]+2' 'two replicas for restart/availability evidence'
require_file_pattern "${MANIFEST}" 'maxUnavailable:[[:space:]]+0' 'zero unavailable rolling update'
require_file_pattern "${MANIFEST}" 'terminationGracePeriodSeconds:[[:space:]]+20' 'bounded graceful termination window'
require_file_pattern "${MANIFEST}" 'runAsNonRoot:[[:space:]]+true' 'non-root pod/container execution'
require_file_pattern "${MANIFEST}" 'runAsUser:[[:space:]]+10001' 'fixed uid 10001'
require_file_pattern "${MANIFEST}" 'runAsGroup:[[:space:]]+10001' 'fixed gid 10001'
require_file_pattern "${MANIFEST}" 'fsGroup:[[:space:]]+10001' 'non-root writable volume ownership'
require_file_pattern "${MANIFEST}" 'fsGroupChangePolicy:[[:space:]]+OnRootMismatch' 'bounded volume ownership change policy'
require_file_pattern "${MANIFEST}" 'allowPrivilegeEscalation:[[:space:]]+false' 'privilege escalation disabled'
require_file_pattern "${MANIFEST}" 'readOnlyRootFilesystem:[[:space:]]+true' 'read-only root filesystem'
require_file_pattern "${MANIFEST}" 'type:[[:space:]]+RuntimeDefault' 'RuntimeDefault seccomp profile'
require_file_pattern "${MANIFEST}" 'drop:[[:space:]]*$' 'capability drop list'
require_file_pattern "${MANIFEST}" '-[[:space:]]+ALL[[:space:]]*$' 'all Linux capabilities dropped'
require_file_pattern "${MANIFEST}" 'requests:[[:space:]]*$' 'resource requests'
require_file_pattern "${MANIFEST}" 'limits:[[:space:]]*$' 'resource limits'
require_file_pattern "${MANIFEST}" 'startupProbe:[[:space:]]*$' 'startup probe'
require_file_pattern "${MANIFEST}" 'readinessProbe:[[:space:]]*$' 'readiness probe'
require_file_pattern "${MANIFEST}" 'livenessProbe:[[:space:]]*$' 'liveness probe'
require_file_pattern "${MANIFEST}" '/opt/shorthand/smoke/core_control\.short' 'probes execute a bundled valid ShortHand program'
require_file_pattern "${MANIFEST}" 'mountPath:[[:space:]]+/tmp' 'bounded scratch volume'
require_file_pattern "${MANIFEST}" 'mountPath:[[:space:]]+/work' 'bounded compiler output workspace'
require_file_pattern "${MANIFEST}" 'medium:[[:space:]]+Memory' 'memory-backed tmp volume'
require_file_pattern "${MANIFEST}" 'sizeLimit:[[:space:]]+64Mi' 'bounded tmp volume size'
require_file_pattern "${MANIFEST}" 'sizeLimit:[[:space:]]+128Mi' 'bounded compiler workspace size'
require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+PodDisruptionBudget[[:space:]]*$' 'PodDisruptionBudget'
require_file_pattern "${MANIFEST}" 'minAvailable:[[:space:]]+1' 'minimum one replica available'
require_file_pattern "${MANIFEST}" '^kind:[[:space:]]+NetworkPolicy[[:space:]]*$' 'NetworkPolicy'
require_file_pattern "${MANIFEST}" '-[[:space:]]+Ingress[[:space:]]*$' 'ingress policy type'
require_file_pattern "${MANIFEST}" '-[[:space:]]+Egress[[:space:]]*$' 'egress policy type'
require_file_pattern "${MANIFEST}" 'ingress:[[:space:]]+\[\][[:space:]]*$' 'default-deny ingress'
require_file_pattern "${MANIFEST}" 'egress:[[:space:]]+\[\][[:space:]]*$' 'default-deny egress'

non_root_count="$(grep -Ec 'runAsNonRoot:[[:space:]]+true' "${MANIFEST}")"
(( non_root_count >= 2 )) || { echo "error: runAsNonRoot must be set at pod and container scope" >&2; exit 1; }
probe_count="$(grep -Ec '(startupProbe|readinessProbe|livenessProbe):[[:space:]]*$' "${MANIFEST}")"
[[ "${probe_count}" == 3 ]] || { echo "error: expected exactly startup/readiness/liveness probes, found ${probe_count}" >&2; exit 1; }

forbid_file_pattern "${MANIFEST}" 'privileged:[[:space:]]+true' 'privileged container'
forbid_file_pattern "${MANIFEST}" 'hostNetwork:[[:space:]]+true' 'host network access'
forbid_file_pattern "${MANIFEST}" 'hostPID:[[:space:]]+true' 'host PID namespace access'
forbid_file_pattern "${MANIFEST}" 'hostIPC:[[:space:]]+true' 'host IPC namespace access'
forbid_file_pattern "${MANIFEST}" 'image:[[:space:]]+[^[:space:]]*:latest([[:space:]]|$)' 'floating latest image tag'
forbid_file_pattern "${MANIFEST}" 'allowPrivilegeEscalation:[[:space:]]+true' 'privilege escalation enabled'

printf 'PASS container Kubernetes production hardening contract multi_stage=true non_root=true probes=3 bounded_workspace=true default_deny=true quota=true pdb=true\n'
