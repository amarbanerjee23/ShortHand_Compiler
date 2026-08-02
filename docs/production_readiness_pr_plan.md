# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-02-pr61
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 61
BASELINE_LANGUAGE_VERSION: beta-0.1
TARGET: enterprise production usage ready language

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

## Current baseline after PR #61

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

Important boundary: ShortHand is still not production ready. Operations exporters, diagnostics, parser robustness, modules, release security, deployment, developer tooling, C3-ECO completion and MLIR integration remain open.

## Recommended remaining PR count

Recommended path from PR #51 onward: 29 PRs total.

After PR #61 is merged, approximately 18 implementation PRs remain. No additional PR was discovered during the production packaging implementation.

## Next recommended PR

Next recommended PR after PR #61:

PR62 - Prometheus scrape endpoint host adapter.

Reason: runtime metrics text is already generated and the installable runtime boundary is now guarded. The next production gap is an optional host adapter that serves the metrics through a scrapeable endpoint without changing the frozen runtime ABI.

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
| PR62 - Prometheus scrape endpoint host adapter | PLANNED | Operations | Expose metrics through a host adapter. | adapter tests and docs |
| PR63 - OTLP exporter adapter | PLANNED | Operations | Add optional OTLP exporter and collector integration. | exporter tests and docs |
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

The project may claim a full production backend matrix only when every marketed backend has a live success fixture or is removed from production-supported claims.

C3-ECO output remains candidate-only unless an external certifier signs it.

## Current remaining PR count field

remaining_planned_prs_total_from_pr51: 29
remaining_planned_prs_after_pr61: 18
