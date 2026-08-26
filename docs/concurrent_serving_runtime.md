# Concurrent serving and operational runtime

serving_runtime_contract: shorthand.serving.runtime.v1
serving_runtime_status: bounded_concurrent_scheduler_and_operational_worker
tenant_isolation: dedicated_process_scope
health_contract: shorthand.serving.health.v1
production_claim: false

## Purpose

PR87 adds a dependency-light C++17 serving scheduler and an installable operational worker. The scheduler gives a host application a bounded request-execution boundary without changing the frozen runtime ABI or the separately versioned core FFI ABI.

This contract closes the repository's concurrent-serving implementation gap. It does not make ShortHand generally production ready. Generated MLIR/composite lowering, representative AI workloads, measured energy and performance, the final release-candidate aggregate, and the protected signed-release exercise remain open.

## Admission and resource bounds

Every runtime instance has immutable limits for worker threads, queued requests, total in-flight requests, retained results, per-request bytes, per-response bytes, aggregate in-flight request bytes, aggregate retained-result bytes, and maximum deadline. Configuration outside the compiled safety ceilings fails before worker threads start.

Admission is non-blocking and returns a stable reason:

| Outcome | Meaning |
| --- | --- |
| `accepted` | The request owns one bounded in-flight slot. |
| `rejected_not_ready` | The runtime is draining or stopped. |
| `rejected_capacity` | The bounded queue is full. Callers must retry with bounded jitter or reject upstream. |
| `rejected_quota` | The in-flight or request-byte quota is exhausted. |
| `rejected_isolation` | The request tenant does not match the process-scoped tenant. |
| `rejected_deadline` | The timeout is missing, non-positive, or exceeds the configured ceiling. |
| `rejected_invalid` | A request or tenant identity is empty, oversized, or non-portable. |
| `rejected_duplicate` | The request identifier is already live or retained. |

The queue never expands beyond `queue_capacity`. The runtime retains at most `completed_result_capacity` unclaimed results and evicts the oldest result deterministically when either its count or aggregate byte budget is reached. Request and response bodies are rejected or failed at their individual and aggregate byte limits. Completed request payload storage is released before result retention.

## Cancellation and deadlines

Queued cancellation completes immediately. Running cancellation is cooperative through `CancellationToken`; the host handler must check the token at bounded intervals. A result returned after its deadline is still recorded as `deadline_exceeded`, so late handler success cannot be reported as an on-time success.

`beginDrain()` atomically stops new admission while allowing accepted work to finish. `shutdown(grace)` waits for the declared grace period, then cancels remaining work, completes queued requests as `shutdown`, and joins workers. The owning host must invoke shutdown/destruction outside a handler thread. A handler that ignores cancellation can still delay in-process destruction; Kubernetes `terminationGracePeriodSeconds` and the container supervisor remain the final hard process boundary.

## Isolation boundary

Each runtime is configured with one `tenant_scope`. Admission and result lookup require an exact tenant match, which prevents accidental cross-tenant queue or result access. This is logical request isolation, not an authentication system.

Production tenant isolation requires one operating-system process or container per tenant. The worker accepts no public network traffic and stores no credentials. An ingress host must separately implement authentication, authorization, TLS, request parsing, rate policy, and audit identity before calling the library. PR87 does not qualify a public inference endpoint.

## Health, readiness and metrics

`healthJson()` emits bounded `shorthand.serving.health.v1` state. Liveness means the worker lifecycle is functioning. Readiness requires liveness, open admission, and available queue/in-flight capacity. Draining and saturated instances are not ready.

`prometheusMetrics()` exports process-level counters and gauges for admission, rejection reasons, terminal outcomes, active workers, queue depth, in-flight work, saturation, and retained-result eviction. Request identifiers and tenant identifiers are deliberately absent to prevent cardinality and tenant-information leakage. PR89 and PR95 own energy instrumentation and measured performance/energy evidence; PR87 metrics do not fabricate either.

The `shorthand_serving_worker` publishes its health state with an atomic same-directory replacement. POSIX temporary state uses owner-only permissions and no-follow/exclusive creation; Windows uses replace-existing/write-through movement. Operators must place the state file in a directory writable only by the workload identity. `SIGUSR1` starts drain on POSIX hosts and `SIGTERM` performs bounded graceful shutdown. Docker and Kubernetes liveness/readiness probes execute the installed worker against that state. The worker's built-in self-test exercises real scheduling, correlation, and clean shutdown without requiring a public listener.

## Operator defaults

- Run one tenant per process or pod.
- Size workers to the qualified CPU allocation, not to unbounded input concurrency.
- Keep queue and in-flight ceilings finite and expose upstream rejection when saturated.
- Set request deadlines below the platform termination grace period.
- Treat `rejected_capacity`, `rejected_quota`, cancellation, deadline, and handler errors as distinct operational outcomes.
- Send `SIGUSR1` before termination so readiness closes before the grace period begins.
- Preserve default-deny network policy, non-root execution, read-only root filesystem, seccomp, dropped capabilities, resource requests/limits, PodDisruptionBudget, and bounded memory-backed workspaces.

## Qualification evidence

`scripts/check_concurrent_serving_runtime.sh` runs deterministic unit, load/soak, worker lifecycle, drain, restart, fault, quota, timeout, cancellation, metrics and isolation tests. The mandatory memory-sanitizer and ThreadSanitizer gates run the serving stress fixture. The installed SDK consumer links `ShortHand::serving`, while Docker and the live Kind lane execute the worker self-test, readiness/liveness probes, graceful replacement, resource quota negative, and default-deny network negative.

C3-ECO alignment is limited to stronger evidence for the existing partial security, safeguards, repeatability, operational-efficiency, infrastructure, architecture, and platform-runtime rows. No C3-ECO gate, score, certification level, carbon reduction, energy saving, or financial saving is granted by this PR.
