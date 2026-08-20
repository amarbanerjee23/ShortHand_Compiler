#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KIND_VERSION="v0.32.0"
KIND_SHA256_AMD64="50030de23cf40a18505f20426f6a8506bedf13c6e509244bd1fa9463721b0f54"
KIND_SHA256_ARM64="b92cd615e97585de8ddade28ed5cd7feb4248d717c233eea5b03c37298900f5d"
KIND_NODE_IMAGE="kindest/node:v1.36.1@sha256:3489c7674813ba5d8b1a9977baea8a6e553784dab7b84759d1014dbd78f7ebd5"
CLUSTER_NAME="shorthand-pr78-${GITHUB_RUN_ID:-local}-$$"
IMAGE="shorthand:pr78"
TMP="$(mktemp -d)"
KIND_BIN="${TMP}/kind"
CONTROL_PLANE="${CLUSTER_NAME}-control-plane"

cleanup() {
  if [[ -x "${KIND_BIN}" ]]; then "${KIND_BIN}" delete cluster --name "${CLUSTER_NAME}" >/dev/null 2>&1 || true; fi
  rm -rf "${TMP}"
}
trap cleanup EXIT

for tool in docker curl sha256sum timeout grep sed awk diff; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: required deployment qualification tool missing: ${tool}" >&2; exit 1; }
done
docker info >/dev/null 2>&1 || { echo "error: Docker daemon unavailable for mandatory deployment qualification" >&2; exit 1; }

case "$(uname -m)" in
  x86_64)
    kind_arch=amd64
    image_arch=amd64
    kind_sha256="${KIND_SHA256_AMD64}"
    ;;
  aarch64|arm64)
    kind_arch=arm64
    image_arch=arm64
    kind_sha256="${KIND_SHA256_ARM64}"
    ;;
  *) echo "error: unsupported kind qualification host architecture: $(uname -m)" >&2; exit 1 ;;
esac

kind_download="${TMP}/kind-linux-${kind_arch}"
kind_url="https://github.com/kubernetes-sigs/kind/releases/download/${KIND_VERSION}/kind-linux-${kind_arch}"
echo "INFO acquiring pinned kind ${KIND_VERSION} arch=${kind_arch} from GitHub release assets"
if ! curl --fail-with-body --location --silent --show-error \
  --retry 4 --retry-all-errors --retry-delay 2 \
  --connect-timeout 15 --max-time 120 \
  -o "${kind_download}" "${kind_url}"; then
  echo "error: failed to download mandatory kind ${KIND_VERSION} binary for ${kind_arch}" >&2
  exit 1
fi

actual_kind_sha256="$(sha256sum "${kind_download}" | awk '{print $1}')"
if [[ "${actual_kind_sha256}" != "${kind_sha256}" ]]; then
  echo "error: kind ${KIND_VERSION} SHA-256 mismatch for ${kind_arch}: expected=${kind_sha256} actual=${actual_kind_sha256}" >&2
  exit 1
fi
echo "PASS verified pinned kind ${KIND_VERSION} SHA-256 arch=${kind_arch}"

mv "${kind_download}" "${KIND_BIN}"
chmod +x "${KIND_BIN}"
"${KIND_BIN}" version

cd "${ROOT_DIR}"
bash scripts/check_container_kubernetes_hardening.sh
bash tests/deployment/test_container_kubernetes_hardening_negative.sh

docker build --pull --platform "linux/${image_arch}" -t "${IMAGE}" .
bash scripts/check_container_runtime.sh "${IMAGE}" "${image_arch}"

"${KIND_BIN}" create cluster \
  --name "${CLUSTER_NAME}" \
  --image "${KIND_NODE_IMAGE}" \
  --wait 150s
"${KIND_BIN}" load docker-image "${IMAGE}" --name "${CLUSTER_NAME}"

action_kubectl() {
  docker exec "${CONTROL_PLANE}" kubectl --kubeconfig=/etc/kubernetes/admin.conf "$@"
}
apply_stdin() {
  docker exec -i "${CONTROL_PLANE}" kubectl --kubeconfig=/etc/kubernetes/admin.conf apply -f -
}

action_kubectl version --client
action_kubectl get nodes -o wide
action_kubectl get nodes --no-headers | grep -Eq '[[:space:]]Ready[[:space:]]'

apply_stdin < deploy/k8s/production.yaml
action_kubectl rollout status deployment/shorthand -n shorthand-system --timeout=150s
action_kubectl wait --for=condition=Ready pod -l app.kubernetes.io/name=shorthand,app.kubernetes.io/component=compiler-runtime -n shorthand-system --timeout=150s

ready_replicas="$(action_kubectl get deployment shorthand -n shorthand-system -o jsonpath='{.status.readyReplicas}')"
[[ "${ready_replicas}" == 2 ]] || { echo "error: expected 2 ready ShortHand replicas, got ${ready_replicas:-0}" >&2; exit 1; }

pod="$(action_kubectl get pods -n shorthand-system -l app.kubernetes.io/name=shorthand,app.kubernetes.io/component=compiler-runtime -o jsonpath='{.items[0].metadata.name}')"
[[ -n "${pod}" ]] || { echo "error: no ShortHand deployment pod found" >&2; exit 1; }

uid="$(action_kubectl exec -n shorthand-system "${pod}" -- id -u)"
gid="$(action_kubectl exec -n shorthand-system "${pod}" -- id -g)"
[[ "${uid}" == 10001 && "${gid}" == 10001 ]] || { echo "error: Kubernetes runtime identity mismatch uid=${uid} gid=${gid}" >&2; exit 1; }
action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc 'test ! -e /var/run/secrets/kubernetes.io/serviceaccount/token'
action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc 'touch /tmp/shorthand-k8s-writable /work/shorthand-k8s-work-writable && test -f /tmp/shorthand-k8s-writable && test -f /work/shorthand-k8s-work-writable'
if action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc 'touch /etc/shorthand-forbidden' >/dev/null 2>&1; then
  echo "error: Kubernetes root filesystem unexpectedly writable" >&2
  exit 1
fi

action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc \
  'cd /work && rm -f core_control core_control.o core_control.bc /tmp/shorthand-k8s-native-compile.log && if ! short_hand /opt/shorthand/smoke/core_control.short compile-native > /tmp/shorthand-k8s-native-compile.log 2>&1; then cat /tmp/shorthand-k8s-native-compile.log >&2; exit 1; fi && test -x ./core_control && ./core_control' \
  >"${TMP}/k8s-native.out"
diff -u "${ROOT_DIR}/tests/semantic/differential/core_control.expected" "${TMP}/k8s-native.out"

cap_eff="$(action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc "awk '/CapEff:/ {print \$2}' /proc/1/status")"
[[ "${cap_eff}" == 0000000000000000 ]] || { echo "error: effective Linux capabilities not fully dropped: ${cap_eff}" >&2; exit 1; }
no_new_privs="$(action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc "awk '/NoNewPrivs:/ {print \$2}' /proc/1/status")"
[[ "${no_new_privs}" == 1 ]] || { echo "error: no-new-privileges runtime evidence missing: ${no_new_privs}" >&2; exit 1; }
seccomp="$(action_kubectl exec -n shorthand-system "${pod}" -- /bin/bash -lc "awk '/Seccomp:/ {print \$2}' /proc/1/status")"
[[ "${seccomp}" == 2 ]] || { echo "error: seccomp filter is not active: ${seccomp}" >&2; exit 1; }

# A denied `kubectl auth can-i` verdict is represented by stdout `no` and a
# non-zero process status. Capture that status inside a conditional so `set -e`
# does not turn the expected fail-closed RBAC result into a gate failure. Only
# the exact denial is accepted; an allowed verdict or command/API error fails.
can_get_secrets=""
can_get_secrets_status=0
if can_get_secrets="$(action_kubectl auth can-i get secrets -n shorthand-system --as=system:serviceaccount:shorthand-system:shorthand-runtime 2>&1)"; then
  can_get_secrets_status=0
else
  can_get_secrets_status=$?
fi
if [[ "${can_get_secrets_status}" == 1 && "${can_get_secrets}" == no ]]; then
  :
elif [[ "${can_get_secrets_status}" == 0 && "${can_get_secrets}" == yes ]]; then
  echo "error: runtime service account unexpectedly allowed to read secrets" >&2
  exit 1
else
  echo "error: runtime service account Secret authorization probe failed unexpectedly status=${can_get_secrets_status} output=${can_get_secrets}" >&2
  exit 1
fi

cat >"${TMP}/quota-negative.yaml" <<'YAML'
apiVersion: v1
kind: Pod
metadata:
  name: quota-negative
  namespace: shorthand-system
spec:
  automountServiceAccountToken: false
  securityContext:
    runAsNonRoot: true
    runAsUser: 10001
    runAsGroup: 10001
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: shorthand
      image: shorthand:pr78
      imagePullPolicy: IfNotPresent
      command: ["/bin/bash", "-lc", "sleep 30"]
      securityContext:
        runAsNonRoot: true
        runAsUser: 10001
        runAsGroup: 10001
        allowPrivilegeEscalation: false
        capabilities:
          drop: ["ALL"]
YAML
if apply_stdin <"${TMP}/quota-negative.yaml" >"${TMP}/quota.out" 2>"${TMP}/quota.err"; then
  echo "error: resource quota negative pod unexpectedly admitted without requests/limits" >&2
  exit 1
fi
grep -Eqi 'must specify|exceeded quota|failed quota' "${TMP}/quota.err" || {
  echo "error: quota negative failed for an unexpected reason" >&2
  cat "${TMP}/quota.err" >&2
  exit 1
}

cat >"${TMP}/network-control.yaml" <<'YAML'
apiVersion: v1
kind: Pod
metadata:
  name: network-control
  namespace: shorthand-system
  labels:
    shorthand.test/network-control: allowed
spec:
  automountServiceAccountToken: false
  securityContext:
    runAsNonRoot: true
    runAsUser: 10001
    runAsGroup: 10001
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: shorthand
      image: shorthand:pr78
      imagePullPolicy: IfNotPresent
      command: ["/bin/bash", "-lc"]
      args: ["trap 'exit 0' TERM INT; while true; do sleep 3600 & wait $!; done"]
      securityContext:
        runAsNonRoot: true
        runAsUser: 10001
        runAsGroup: 10001
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop: ["ALL"]
      resources:
        requests:
          cpu: "25m"
          memory: "32Mi"
        limits:
          cpu: "100m"
          memory: "128Mi"
      volumeMounts:
        - name: tmp
          mountPath: /tmp
  volumes:
    - name: tmp
      emptyDir:
        medium: Memory
        sizeLimit: 16Mi
YAML
apply_stdin <"${TMP}/network-control.yaml"
action_kubectl wait --for=condition=Ready pod/network-control -n shorthand-system --timeout=120s
api_ip="$(action_kubectl get service kubernetes -n default -o jsonpath='{.spec.clusterIP}')"
[[ -n "${api_ip}" ]] || { echo "error: Kubernetes API service clusterIP unavailable" >&2; exit 1; }

probe_api_from_pod() {
  local probe_pod="$1"
  local output

  if ! output="$(
    action_kubectl exec -n shorthand-system "${probe_pod}" -- /bin/bash -lc \
      "if timeout 3 bash -lc 'exec 3<>/dev/tcp/${api_ip}/443' >/dev/null 2>&1; then printf '%s\\n' CONNECT_OK; else printf '%s\\n' CONNECT_BLOCKED; fi" \
      2>/dev/null
  )"; then
    printf '%s\n' EXEC_FAILED
    return 0
  fi

  case "${output}" in
    *CONNECT_OK*) printf '%s\n' CONNECT_OK ;;
    *CONNECT_BLOCKED*) printf '%s\n' CONNECT_BLOCKED ;;
    *) printf '%s\n' INVALID_PROBE ;;
  esac
}

network_policy_converged=0
control_result="NOT_PROBED"
protected_result="NOT_PROBED"
for _ in $(seq 1 20); do
  control_result="$(probe_api_from_pod network-control)"
  protected_result="$(probe_api_from_pod "${pod}")"

  if [[ "${control_result}" == CONNECT_OK && "${protected_result}" == CONNECT_BLOCKED ]]; then
    network_policy_converged=1
    break
  fi

  sleep 2
done

if [[ "${network_policy_converged}" != 1 ]]; then
  echo "error: NetworkPolicy enforcement did not converge (control_result=${control_result}, protected_result=${protected_result})" >&2
  action_kubectl get pods -n shorthand-system -o wide >&2 || true
  action_kubectl get networkpolicy shorthand-default-deny -n shorthand-system -o yaml >&2 || true
  action_kubectl describe networkpolicy shorthand-default-deny -n shorthand-system >&2 || true
  action_kubectl get service kubernetes -n default -o wide >&2 || true
  exit 1
fi

before="$(action_kubectl get pods -n shorthand-system -l app.kubernetes.io/name=shorthand,app.kubernetes.io/component=compiler-runtime -o name | sort)"
action_kubectl delete pod -n shorthand-system "${pod}" --wait=true --timeout=60s >/dev/null

replacement_converged=0
after=""
pod_count=0
ready_after=0
available_after=0
for _ in $(seq 1 75); do
  after="$(action_kubectl get pods -n shorthand-system -l app.kubernetes.io/name=shorthand,app.kubernetes.io/component=compiler-runtime -o name | sort)"
  pod_count="$(printf '%s\n' "${after}" | sed '/^$/d' | wc -l | tr -d ' ')"
  ready_after="$(action_kubectl get deployment shorthand -n shorthand-system -o jsonpath='{.status.readyReplicas}')"
  available_after="$(action_kubectl get deployment shorthand -n shorthand-system -o jsonpath='{.status.availableReplicas}')"

  if [[ "${pod_count}" == 2 &&
        "${before}" != "${after}" &&
        "${ready_after:-0}" == 2 &&
        "${available_after:-0}" == 2 ]] &&
     ! printf '%s\n' "${after}" | grep -Fxq "pod/${pod}"; then
    replacement_converged=1
    break
  fi
  sleep 2
done

if [[ "${replacement_converged}" != 1 ]]; then
  echo "error: deployment failed to converge after pod replacement (pods=${pod_count}, ready=${ready_after:-0}, available=${available_after:-0})" >&2
  action_kubectl get pods -n shorthand-system -l app.kubernetes.io/name=shorthand,app.kubernetes.io/component=compiler-runtime -o wide >&2 || true
  action_kubectl describe deployment shorthand -n shorthand-system >&2 || true
  exit 1
fi

printf 'PASS ephemeral Kubernetes production gate kind=%s kubernetes=1.36.1 arch=%s replicas=2 restricted=true native_compile=true native_output_isolated=true quota_negative=true network_negative=true restart=true\n' \
  "${KIND_VERSION}" "${image_arch}"
