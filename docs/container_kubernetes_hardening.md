# ShortHand container and Kubernetes production hardening

container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1
roadmap_pr: 77+87
github_pr: 78+87
language_version: beta-0.6
production_claim: false
tst019_status: implemented_after_exact_head_runtime_qualification

## Scope

Roadmap PR77 hardened the ShortHand compiler/runtime as a deployable CLI workload. PR87 replaces the idle shell PID 1 with the real bounded `shorthand_serving_worker`. ShortHand is not represented as a public HTTP service merely to obtain Kubernetes health checks. Docker and Kubernetes probes execute the worker's versioned liveness/readiness state, while native compiler execution remains independently qualified against `core_control.short`.

This contract covers the compiler/runtime image, the process-scoped serving scheduler and Kubernetes workload isolation. It does not claim that an accelerator, public ingress, authenticated multi-tenant inference API or representative production model is qualified. The only live backend scope remains `linux-x64-cpu-v1`. Protected signed publication remains TST017 and must still be exercised independently.

## Container contract

The production `Dockerfile` is multi-stage. The builder stage contains the compiler toolchain and produces `short_hand`, `green_ai_tool`, `libshorthand_runtime.a`, `libshorthand_serving.a` and `shorthand_serving_worker`. The runtime stage contains only the packages needed to execute the compiler/runtime and native LLVM path. Build-only packages such as CMake, Ninja, Bison, development headers and `build-essential` are not retained in the runtime image.

The runtime image:

- executes as fixed uid/gid `10001:10001`,
- owns copied runtime artifacts as that uid/gid,
- declares the runtime library and native linker paths explicitly,
- includes a valid ShortHand smoke source and expected output,
- exposes no network port by default,
- uses a Docker `HEALTHCHECK` that reads bounded `shorthand.serving.health.v1` liveness through the worker,
- configures finite aggregate in-flight request and retained-result byte budgets in addition to per-body, queue and concurrency limits, and
- is qualified under read-only-root, non-executable scratch `/tmp`, bounded writable `/work`, no-network, no-new-privileges and drop-all-capabilities runtime options.

The runtime qualification does not stop at interpreter execution. It compiles `core_control.short` to a native executable in `/work`, executes that binary and compares its output with the semantic differential oracle. This proves the hardened compiler image remains usable while its immutable root filesystem is enforced.

Linux amd64 image execution is tested in the mandatory ephemeral-cluster qualification. Linux arm64 image execution is tested natively in the existing mandatory `linux-arm64` CI lane. An image architecture is not considered supported merely because a manifest label names it.

## Kubernetes contract

`deploy/k8s/production.yaml` defines a dedicated `shorthand-system` namespace with the Kubernetes Restricted Pod Security profile pinned to v1.36 for enforce, audit and warn modes. The workload uses a dedicated service account with token automount disabled.

The Deployment uses two replicas, zero unavailable pods during rolling updates, a bounded termination grace period and a PodDisruptionBudget requiring one available replica. Pod and container security contexts require non-root uid/gid 10001, RuntimeDefault seccomp, read-only root filesystem, no privilege escalation and all Linux capabilities dropped.

CPU and memory requests and limits are mandatory. A namespace ResourceQuota prevents unbounded workloads. Writable state is isolated from the image: a bounded memory-backed `emptyDir` is mounted at `/tmp` for parser/runtime scratch, and a separate bounded `emptyDir` is mounted at `/work` for generated compiler artifacts. Persistent or shared artifact storage is intentionally not claimed by this baseline.

Startup and liveness probes require the worker's bounded liveness state. Readiness additionally requires open admission and available queue/in-flight capacity, so drain and saturation close readiness without declaring the process dead. A `preStop` signal starts drain before Kubernetes sends termination. The live gate separately executes native compilation and the worker self-test inside the restricted pod.

The workload NetworkPolicy is default-deny for both ingress and egress. No Service or Ingress is declared because this compiler workload has no network API. A future network-facing ShortHand runtime must add explicit protocol/port policy and independent misuse tests rather than weakening this default-deny baseline.

## Mandatory live qualification

`scripts/check_kubernetes_ephemeral_cluster.sh` creates an ephemeral Kind v0.32.0 cluster using the pinned Kubernetes 1.36.1 node image digest. The Kind binary checksum is verified before execution. The test builds and runtime-qualifies the production amd64 image, loads it into the cluster and applies the production manifest.

The live gate requires:

1. two Ready deployment replicas,
2. runtime uid/gid 10001,
3. no mounted Kubernetes service-account token,
4. writable bounded `/tmp` and `/work` while the image root remains unwritable,
5. native compilation/execution plus the serving scheduler self-test in the hardened workload,
6. zero effective Linux capabilities,
7. `NoNewPrivs=1` and seccomp filtering active,
8. the runtime service account cannot read Secrets,
9. a pod missing resource requests/limits is rejected by ResourceQuota,
10. a control pod can reach the Kubernetes API service while a ShortHand pod selected by default-deny NetworkPolicy cannot,
11. deletion of a ShortHand pod is repaired back to two Ready replicas, and
12. serving liveness/readiness become healthy, drain makes readiness false while liveness stays true, and a SIGTERM container restart retains previous-log evidence of `graceful=true` before returning Ready.

The deployment gate is bounded by explicit timeouts. Failure to download/verify Kind, build the image, create the cluster, enforce policy or observe the required runtime state is a failure. There is no unconditional skip path.

## Negative and regression evidence

`tests/deployment/test_container_kubernetes_hardening_negative.sh` mutates the production contract and requires rejection of root execution, writable root filesystem, missing readiness probes, missing resource limits, missing aggregate serving byte budgets, missing pre-stop drain, open egress and privileged execution. PR77's earlier source-level deployment-security negatives remain mandatory as an inherited security baseline.

## Claim boundary

TST019 remains implemented for the hardened container/Kubernetes deployment family, now extended by PR87 serving-worker evidence. This does not qualify public ingress, authentication/TLS, representative model serving, TST017 signed publication, performance/energy comparison or the final production RC.
