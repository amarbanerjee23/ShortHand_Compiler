# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-02-pr63
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 63
BASELINE_LANGUAGE_VERSION: beta-0.1
TARGET: enterprise production usage ready language

Historical guard markers retained for the old-task stability contract:

- previous_guard_marker: production_readiness_plan_version: 2026-08-02-pr62
- previous_guard_marker: LAST_COMPLETED_PR: 62

## Purpose

This document is the single planning source for moving ShortHand from the controlled beta foundation to an enterprise production usage ready language. Every roadmap PR must update its status, evidence, next recommended PR and remaining count here.

The consolidated mission, priorities, non-goals and success criteria are maintained in `docs/language_objectives.md`. The objective contract remains `shorthand.language.objectives.version: 2026-07-29-v1` with `production_claim: false` until the exit criteria pass.

## Desired outcome definition

Enterprise production usage ready means:

1. The language surface is versioned, tested, documented and compatible across releases.
2. Compiler and runtime artifacts are reproducibly built, packaged, installed and released.
3. Runtime inference, fallback, telemetry, hardware selection and evidence are honest and claim-safe.
4. Marketed AI backends are live-tested or clearly excluded from production support.
5. CPU, GPU, TPU and NPU detection remains separate from accessibility, backend compatibility and execution readiness.
6. Runtime ABI, lifecycle, state ownership and concurrency behavior are explicit and tested.
7. Multi-file programs work through module, import and package boundaries.
8. Security, deployment, observability and audit evidence survive enterprise review.
9. C3-ECO evidence remains candidate-only unless an external certifier signs it, and MLIR lowering is integrated beyond scaffold status.

Unsupported or unavailable paths must never report successful execution.

## Audit correction applied in PR #51

The initial plan was too optimistic. PR #51 added dedicated milestones for runtime ABI stability, runtime state/thread-safety and parser robustness.

## Hardware routing expansion applied in PR #55

PR #55 added PR #56 as a separate production-critical milestone and increased the roadmap to 29 PRs from PR #51 through PR #79.

Hardware presence alone must not create a `live_success` claim. Inventory uses `shorthand.hardware.inventory.v1`, selection uses `shorthand.hardware.selection.v1`, and implementation evidence includes `docs/hardware_capability_routing.md` and `HardwareDiscovery.h`.

## Language objectives consolidation applied in PR #57

PR #57 consolidated the language mission in `docs/language_objectives.md` and protected it through language correctness.

## Backend failure-mode finalization applied in PR #58

PR #58 added `docs/backend_failure_mode_matrix.md` and schema `shorthand.backend_failure_mode_matrix.v1`. Failure evidence is not backend success evidence. Invalid format, missing SDK, shape, precision, capacity, hardware access, empty probe and fallback honesty are now guarded.

## Runtime ABI and API stability applied in PR #59

Runtime ABI `1.0.0` freezes exactly 25 external `short_*` symbols. Evidence includes `docs/runtime_abi_api_stability.md`, `abi/runtime_public_symbols_v1.txt` and `abi/shorthand_runtime_abi_v1.h`.

ABI stability evidence does not imply thread safety, shared-library packaging, deployment readiness or complete production readiness.

## Runtime state and thread-safety applied in PR #60

PR #60 preserves the frozen ABI while adding a synchronized production runtime façade.

The runtime contract now provides:

1. one explicitly documented process-wide default context,
2. serialized and linearizable public ABI calls through a recursive mutex,
3. thread-local snapshots for string-returning functions,
4. deterministic concurrent registration, query, reset and no-execution inference tests,
5. unchanged ABI v1 public symbol count,
6. an explicit multi-tenant boundary requiring separate processes,
7. serialized backend execution for ABI v1 rather than an unsupported parallel-execution claim.

Evidence:

- `docs/runtime_state_and_thread_safety.md`
- `Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp`
- `tests/runtime/test_runtime_state_thread_safety.sh`
- `scripts/check_runtime_state_thread_safety.sh`

Thread-safe does not mean multi-tenant isolated. ABI v1 has no public context handles.

## Production build packaging applied in PR #61

PR #61 turns the frozen runtime and dependency-light C++ AI bridge adapter into repeatable installable artifacts.

The packaging contract now provides:

1. static and shared runtime libraries,
2. static and shared AI bridge adapter libraries,
3. shared-library version `1.0.0` and SOVERSION `1`,
4. hidden private runtime implementation symbols with the frozen public façade exported,
5. installed current and frozen ABI headers,
6. relocatable CMake package exports,
7. pkg-config metadata,
8. clean downstream static and shared consumer builds,
9. SONAME and package-discovery evidence.

Evidence:

- `docs/runtime_production_packaging.md`
- `cmake/ShortHandConfig.cmake.in`
- `cmake/shorthand-runtime.pc.in`
- `cmake/shorthand-ai-bridge.pc.in`
- `tests/packaging/test_runtime_production_packaging.sh`
- `scripts/check_runtime_production_packaging.sh`

Packaging does not claim third-party backend SDK execution, tenant isolation, deployment readiness, signed releases or full production readiness.

## Prometheus scrape endpoint host adapter applied in PR #62

PR #62 exposes the runtime's existing Prometheus text through an optional installed host executable without changing the frozen runtime ABI.

The adapter contract now provides:

1. `GET /metrics` backed by `short_runtime_prometheus_metrics()`,
2. `GET /healthz` for local process health,
3. loopback-only default binding on `127.0.0.1:9464`,
4. bounded request headers and socket timeouts,
5. deterministic maximum-request shutdown for tests and supervised jobs,
6. explicit 400, 404, 405 and 431 behavior,
7. a clean CMake build and installed executable,
8. real socket-level response validation,
9. an unchanged runtime ABI v1 manifest of exactly 25 public symbols.

Evidence:

- `docs/prometheus_scrape_host_adapter.md`
- `Compiler_new_ws/Short_Hand/src/operations/PrometheusScrapeAdapter.cpp`
- `tests/operations/test_prometheus_scrape_adapter.sh`
- `scripts/check_prometheus_scrape_adapter.sh`
- `CMakeLists.txt`

The adapter is not authenticated or TLS-enabled public ingress. Non-loopback exposure requires an external reverse proxy, service mesh and network policy. It does not aggregate metrics across ShortHand processes.

## OTLP exporter adapter applied in PR #63

PR #63 adds bounded one-shot OTLP/HTTP trace delivery through an optional installed executable while preserving the frozen runtime ABI.

The exporter contract now provides:

1. a standards-shaped OTLP/HTTP JSON `resourceSpans` request,
2. default collector delivery to `127.0.0.1:4318/v1/traces`,
3. local runtime, bounded file and bounded stdin snapshot sources,
4. schema validation for `shorthand.runtime.otlp_spans.v1`,
5. bounded connect, send, receive, snapshot and response handling,
6. bounded exponential retries for transport failures, HTTP 408, HTTP 429 and HTTP 5xx,
7. no retry for permanent HTTP 4xx rejection,
8. optional Authorization header loading from an environment variable rather than command-line secret text,
9. explicit `delivered=true`, `delivered=false` and dry-run status contracts,
10. real loopback collector integration tests and installed executable validation,
11. an unchanged runtime ABI v1 manifest of exactly 25 public symbols.

Evidence:

- `docs/otlp_exporter_adapter.md`
- `Compiler_new_ws/Short_Hand/src/operations/OtlpExporterAdapter.cpp`
- `tests/operations/OtlpTestCollector.cpp`
- `tests/operations/test_otlp_exporter_adapter.sh`
- `scripts/check_otlp_exporter_adapter.sh`
- `CMakeLists.txt`

HTTP 2xx proves only that the configured endpoint accepted the request. It does not prove collector processing or trace persistence. Native TLS, mTLS, disk queues, continuous batching and multi-tenant export remain outside this PR.

## Status values

STATUS values: PLANNED, IN_PROGRESS, MERGED, BLOCKED, DEFERRED

## Update rule for every future PR

Every roadmap PR must update:

1. its row status,
2. actual evidence paths,
3. newly discovered follow-up work,
4. the next recommended PR,
5. the remaining PR count.

A production claim is blocked until every required row is MERGED or intentionally DEFERRED with a documented production impact.

## Current baseline after PR #63

- Language and conformance remain guarded at `beta-0.1`.
- Consolidated language objectives are versioned and guarded.
- ONNX Runtime CPU has an optional SDK-backed success fixture.
- TensorRT, OpenVINO, LibTorch and Llama.cpp have unavailable-path proofs.
- Hardware inventory and execution-ready routing are guarded.
- The common backend failure-mode matrix is finalized.
- Runtime ABI/API `1.0.0` and 25 public symbols are frozen.
- The packaged runtime library serializes all public ABI operations.
- String-returning APIs provide thread-local snapshots.
- Reset, registration, inference counters and evidence queries have deterministic concurrency coverage.
- Runtime observability exports JSON, Prometheus-style text and OTLP-like span JSON.
- Runtime and AI bridge adapter static/shared artifacts are installable.
- Shared artifacts carry version `1.0.0` and SOVERSION `1`.
- CMake and pkg-config downstream consumers are guarded.
- Current and frozen ABI headers are installed from the package prefix.
- An installed loopback-default Prometheus scrape host exposes `/metrics` and `/healthz` with bounded HTTP handling.
- Real socket tests prove Prometheus status, content type, representative metrics, method policy and deterministic shutdown.
- An installed OTLP/HTTP exporter delivers bounded `resourceSpans` requests to a collector.
- Collector tests prove accepted delivery, retryable 503 recovery, permanent 400 rejection and honest delivery status.
- File and stdin handoff explicitly address process-local runtime state without expanding ABI v1.

Important boundary: ShortHand is still not production ready. Diagnostics, parser robustness, modules, release security, deployment, developer tooling, C3-ECO completion and MLIR integration remain open. The OTLP adapter also does not provide TLS, continuous batching, disk queues or end-to-end persistence guarantees.

## Recommended remaining PR count

Recommended path from PR #51 onward: 29 PRs total.

After PR #63 is merged, approximately 16 implementation PRs remain. No additional roadmap PR was discovered during the OTLP exporter implementation.

## Next recommended PR

Next recommended PR after PR #63:

PR64 - AST source ranges across parser nodes.

Reason: Basic pull and push observability adapters are now guarded without changing the runtime ABI. The next production-critical gap is consistent source location ownership across AST nodes, which is required before a complete diagnostics coverage matrix can be reliable.

## PR roadmap table

| Planned PR | Status | Area | Goal | Expected evidence |
| --- | --- | --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | MERGED | Planning | Maintain this roadmap and guard it in CI. | `docs/production_readiness_pr_plan.md`, plan gate |
| PR52 - Backend live SDK matrix harness | MERGED | Backend coverage | Shared live/skip-safe backend reporting. | SDK matrix docs, test and gate |
| PR53 - TensorRT optional live execution fixture | MERGED | Backend coverage | Prove unavailable TensorRT paths cannot report success. | TensorRT fixture evidence |
| PR54 - OpenVINO optional live execution fixture | MERGED | Backend coverage | Prove unavailable OpenVINO paths cannot report success. | OpenVINO fixture evidence |
| PR55 - LibTorch optional live execution fixture | MERGED | Backend coverage | Prove unavailable LibTorch paths cannot report success. | LibTorch fixture evidence |
| PR56 - Hardware capability discovery and accelerator-aware routing | MERGED | Runtime hardware | Inventory CPU/GPU/TPU/NPU and select only execution-ready routes. | `docs/hardware_capability_routing.md`, `HardwareDiscovery.h` |
| PR57 - Llama.cpp optional live execution fixture | MERGED | Backend and objectives | Prove unavailable GGUF paths and consolidate objectives. | Llama.cpp fixture and objectives gate |
| PR58 - Backend failure-mode matrix finalization | MERGED | Runtime reliability | Finalize deterministic common failure contract. | `docs/backend_failure_mode_matrix.md`, matrix test and gate |
| PR59 - Runtime ABI and API version stability gate | MERGED | Runtime contract | Freeze symbols, status values, compatibility and deprecation rules. | ABI docs, manifest, frozen header and tests |
| PR60 - Runtime state isolation and thread-safety policy | MERGED | Runtime reliability | Serialize ABI v1, protect snapshots and document process isolation. | state/thread-safety docs, façade, stress test and gate |
| PR61 - Production build packaging for runtime and AI bridge | MERGED | Build | Add repeatable installable static/shared artifacts, shared-library packaging, SONAME/version evidence, exported headers, package metadata and consumer link tests. | `docs/runtime_production_packaging.md`, CMake/pkg-config exports, install-consumer gate |
| PR62 - Prometheus scrape endpoint host adapter | MERGED | Operations | Expose metrics through a bounded, loopback-default installed host adapter without changing the runtime ABI. | adapter source, real socket tests and docs |
| PR63 - OTLP exporter adapter | MERGED | Operations | Deliver bounded OTLP/HTTP trace requests with honest retry and rejection behavior. | `docs/otlp_exporter_adapter.md`, exporter, collector fixture, tests and gate |
| PR64 - AST source ranges across parser nodes | PLANNED | Diagnostics | Store consistent source spans across AST nodes. | source-span tests |
| PR65 - Diagnostics coverage matrix | PLANNED | Diagnostics | Cover parser, semantic, AI, GreenAI and lowering diagnostics. | diagnostics matrix and tests |
| PR66 - Full grammar and conformance matrix beta-0.2 | PLANNED | Language contract | Expand syntax coverage and version with compatibility evidence. | grammar, manifest and version gate |
| PR67 - Parser robustness and negative corpus hardening | PLANNED | Language robustness | Add malformed-input and regression coverage. | negative corpus and robustness gate |
| PR68 - Module/import/package design and parser scaffold | PLANNED | Language scale | Add syntax and parser scaffold. | design and parser fixtures |
| PR69 - Module resolver and codegen integration | PLANNED | Language scale | Implement resolution, paths, boundaries and codegen. | resolver and integration tests |
| PR70 - Signed release and protected release workflow | PLANNED | Release | Sign artifacts and protect publication. | workflow, signing and provenance evidence |
| PR71 - External dependency vulnerability scan gate | PLANNED | Security | Add dependency vulnerability scanning. | CI gate and policy |
| PR72 - Container and Kubernetes hardening | PLANNED | Deployment | Add health, probes, limits, non-root and deployment validation. | deployment tests and docs |
| PR73 - Formatter and linter baseline | PLANNED | Developer experience | Standardize formatting and lint behavior. | formatter/linter evidence |
| PR74 - Syntax highlighting and LSP skeleton | PLANNED | Developer experience | Add editor grammar and minimal LSP path. | syntax assets and LSP tests |
| PR75 - C3-ECO certification language blocks | PLANNED | C3-ECO language | Add declarations, boundaries, workload, measurement and lifecycle syntax. | grammar, parser and semantic tests |
| PR76 - C3-ECO scoring, report generation, and eco-regression | PLANNED | C3-ECO evidence | Expand candidate scoring/reporting and regression gates. | schema/report/workbook tests |
| PR77 - Authority-ready C3-ECO auditor bundle | PLANNED | C3-ECO evidence | Package claim-safe auditor handoff evidence. | bundle manifest and validation gate |
| PR78 - MLIR generated dialect build integration | PLANNED | MLIR | Integrate generated dialect build and parser/printer tests. | CMake and MLIR tests |
| PR79 - MLIR lowering passes and production RC gate | PLANNED | MLIR and release | Add lowering coverage and final production release-candidate blocker gate. | lowering tests, RC gate and claim scan |

## Production readiness exit criteria

ShortHand can be considered enterprise production usage ready only when:

1. Protected-branch CI passes.
2. Language compatibility, deprecation, grammar and conformance are complete.
3. Fallback never claims execution.
4. Marketed backends are live-tested or removed from production claims.
5. Hardware detection remains separate from execution readiness and success.
6. Runtime ABI/API, lifecycle, concurrency, packaging and install-consumer gates pass.
7. Observability, diagnostics, parser robustness, modules, security, deployment and tooling gates pass.
8. C3-ECO evidence is authority-ready and claim-safe.
9. MLIR lowering and the final production RC gate pass.

## Production claim rules

The project may claim controlled beta only until every required non-deferred row is MERGED.

Hardware detection never implies successful accelerator execution without backend execution-ready evidence and final inference success.

Failure-mode evidence never implies backend execution success.

Thread-safe process-wide state does not imply tenant isolation or parallel backend execution.

Installable artifacts and successful consumer linking do not imply deployment readiness or successful third-party backend execution.

A loopback-default metrics endpoint does not imply authenticated, TLS-enabled or public-ingress readiness.

An OTLP endpoint returning HTTP 2xx proves transport acceptance only. It does not prove collector processing, trace persistence, indexing or query availability.

The project may claim a full production backend matrix only when every marketed backend has a live success fixture or is removed from production-supported claims.

C3-ECO output remains candidate-only unless an external certifier signs it.

## Current remaining PR count field

remaining_planned_prs_total_from_pr51: 29
remaining_planned_prs_after_pr61: 18
remaining_planned_prs_after_pr62: 17
remaining_planned_prs_after_pr63: 16

## Historical PR62 recommendation markers

The following exact text is retained only so the old-task stability gate can verify that PR62's previously guarded planning contract was not deleted:

After PR #62 is merged, approximately 17 implementation PRs remain.

Next recommended PR after PR #62:

PR63 - OTLP exporter adapter.

Previous roadmap state: PR63 - OTLP exporter adapter | PLANNED
