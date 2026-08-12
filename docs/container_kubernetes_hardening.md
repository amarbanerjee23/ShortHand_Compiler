# ShortHand container and Kubernetes production hardening

container_kubernetes_contract_version: shorthand.deployment.kubernetes.v1
roadmap_pr: 77
github_pr: 78
language_version: beta-0.3
production_claim: false
tst019_status: implemented_after_exact_head_runtime_qualification

## Scope

Roadmap PR77 hardens the ShortHand compiler/runtime as a deployable CLI workload. ShortHand is not represented as an HTTP service merely to obtain Kubernetes health checks. The production container keeps a compiler worker process alive, while Docker and Kubernetes health probes execute the real `short_hand` parser against the bundled `core_control.short` conformance program.

This contract covers the compiler/runtime image and its Kubernetes workload isolation. It does not claim that an AI backend, accelerator, public ingress, model server or multi-tenant inference API is production-qualified. Live CPU/GPU/TPU/NPU backend qualification remains roadmap PR80. Protected signed publication remains TST017 and must still be exercised independently.

## Container contract

The production `Dockerfile` is multi-stage. The builder stage contains the compiler toolchain and produces `short_hand`, `green_ai_tool` and `libshorthand_runtime.a`. The runtime stage contains only the packages needed to execute the compiler/runtime and native LLVM path. Build-only packages such as CMake, Ninja, Bison, development headers and `build-essential` are not retained in the runtime image.

The runtime image:

- executes as fixed uid/gid `10001:10001`,
- owns copied runtime artifacts as that uid/gid,
- declares the runtime library and native linker paths explicitly,
- includes a valid ShortHand smoke source and expected output,
- exposes no network port by default,
- uses a Docker `HEALTHCHECK` that runs `short_hand ... parse`, and
- is qualified under read-only-root, non-executable scratch `/tmp`, bounded writable `/work`, no-network, no-new-privileges and drop-all-capabilities runtime options.

The runtime qualification does not stop at interpreter execution. It compiles `core_control.short` to a native executable in `/work`, executes that binary and compares its output with the semantic differential oracle. This proves the hardened compiler image remains usable while its immutable root filesystem is enforced.

Linux amd64 image execution is tested in the mandatory ephemeral-cluster qualification. Linux arm64 image execution is tested natively in the existing mandatory `linux-arm64` CI lane. An image architecture is not considered supported merely because a manifest label names it.

## Kubernetes contract

`deploy/k8s/production.yaml` defines a dedicated `shorthand-system` namespace with the Kubernetes Restricted Pod Security profile pinned to v1.36 for enforce, audit and warn modes. The workload uses a dedicated service account with token automount disabled.

The Deployment uses two replicas, zero unavailable pods during rolling updates, a bounded termination grace period and a PodDisruptionBudget requiring one available replica. Pod and container security contexts require non-root uid/gid 10001, RuntimeDefault seccomp, read-only root filesystem, no privilege escalation and all Linux capabilities dropped.

CPU and memory requests and limits are mandatory. A namespace ResourceQuota prevents unbounded workloads. Writable state is isolated from the image: a bounded memory-backed `emptyDir` is mounted at `/tmp` for parser/runtime scratch, and a separate bounded `emptyDir` is mounted at `/work` for generated compiler artifacts. Persistent or shared artifact storage is intentionally not claimed by this baseline.

Startup, readiness and liveness probes all execute the real ShortHand parser on `/opt/shorthand/smoke/core_control.short`. Probe success therefore demonstrates that the compiler executable, parser and bundled smoke input are usable inside the restricted workload rather than merely checking that a shell process exists.

The workload NetworkPolicy is default-deny for both ingress and egress. No Service or Ingress is declared because this compiler workload has no network API. A future network-facing ShortHand runtime must add explicit protocol/port policy and independent misuse tests rather than weakening this default-deny baseline.

## Mandatory live qualification

`scripts/check_kubernetes_ephemeral_cluster.sh` creates an ephemeral Kind v0.32.0 cluster using the pinned Kubernetes 1.36.1 node image digest. The Kind binary checksum is verified before execution. The test builds and runtime-qualifies the production amd64 image, loads it into the cluster and applies the production manifest.

The live gate requires:

1. two Ready deployment replicas,
2. runtime uid/gid 10001,
3. no mounted Kubernetes service-account token,
4. writable bounded `/tmp` and `/work` while the image root remains unwritable,
5. native compilation/execution in the hardened standalone container,
6. zero effective Linux capabilities,
7. `NoNewPrivs=1` and seccomp filtering active,
8. the runtime service account cannot read Secrets,
9. a pod missing resource requests/limits is rejected by ResourceQuota,
10. a control pod can reach the Kubernetes API service while a ShortHand pod selected by default-deny NetworkPolicy cannot,
11. deletion of a ShortHand pod is repaired back to two Ready replicas, and
12. the standalone container healthcheck becomes healthy and terminates cleanly on SIGTERM.

The deployment gate is bounded by explicit timeouts. Failure to download/verify Kind, build the image, create the cluster, enforce policy or observe the required runtime state is a failure. There is no unconditional skip path.

## Negative and regression evidence

`tests/deployment/test_container_kubernetes_hardening_negative.sh` mutates the production contract and requires rejection of root execution, writable root filesystem, missing readiness probes, missing resource limits, open egress and privileged execution. PR77's earlier source-level deployment-security negatives remain mandatory as an inherited security baseline.

## Claim boundary

TST019 may be promoted to implemented only after the exact GitHub PR78 head passes both stable CI contexts and the live amd64/arm64 container/deployment evidence described above. This closes the container/Kubernetes deployment blocker only for the CLI/compiler workload contract. It does not close TST017 signed publication, TST022 backend/hardware qualification, TST025 performance, TST026 energy comparison or TST027 final production RC qualification.
