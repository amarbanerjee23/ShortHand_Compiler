# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-02-pr58
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 58
BASELINE_LANGUAGE_VERSION: beta-0.1
TARGET: enterprise production usage ready language

## Purpose

This document is the single planning source for moving ShortHand from the controlled beta foundation to an enterprise production usage ready language. Every roadmap PR must update its status, evidence, next recommended PR, and remaining count in this file.

The consolidated mission, priorities, non-goals, workload boundary, and production success criteria are maintained in `docs/language_objectives.md` and guarded by `scripts/check_language_objectives.sh`.

## Desired outcome definition

Enterprise production usage ready means:

1. The supported language surface is versioned, tested, documented, and compatible across releases.
2. The compiler and runtime are built, packaged, tested, and released through repeatable workflows.
3. Runtime inference, fallback, telemetry, hardware selection, and evidence paths are honest and claim-safe.
4. Supported AI backends are live-tested or clearly marked non-production or policy-compatible only.
5. CPU, GPU, TPU, and NPU inventory separates detection from accessibility, backend compatibility, and execution readiness.
6. Multi-file programs work through module, import, and package boundaries.
7. Deployment, observability, security, release, and audit evidence can survive enterprise review.
8. C3-ECO evidence is authority-ready but never presented as externally certified without a certifier.
9. MLIR lowering is integrated beyond scaffold status.

A production claim must not depend on optional SDKs being present in default CI. Unsupported or unavailable paths must never report successful execution.

## Audit correction applied in PR #51

The original plan was too optimistic. PR #51 added dedicated milestones for runtime ABI stability, runtime state/thread-safety, and parser robustness.

## Hardware routing expansion applied in PR #55

PR #55 added PR #56 as a separate production-critical milestone and increased the roadmap from 28 to 29 PRs from PR #51 onward.

PR #56 now:

1. Detects CPU and candidate GPU, TPU, and NPU classes through non-destructive probes.
2. Exposes machine-readable inventory and selection JSON.
3. Separates `detected`, `accessible`, `backend_compatible`, and `execution_ready`.
4. Selects a device only when model format, precision, policy, access, memory, and backend availability checks pass.
5. Supports preference, override, deny-list, minimum-memory, and CPU fallback policy.
6. Provides injectable fake probes for deterministic CI.
7. Records selected device class and backend in inference telemetry.
8. Keeps hardware detection separate from inference success.

Hardware presence alone must not create a `live_success` claim.

## Language objectives consolidation applied in PR #57

PR #57 re-assembles the project objectives into `docs/language_objectives.md` and protects them through the language correctness gate.

The objectives establish:

1. a simple C++/LLVM-first compiled AI language,
2. strict semantic and runtime correctness,
3. honest backend, hardware, fallback, and evidence behavior,
4. first-class Green AI and C3-ECO-aligned evidence,
5. enterprise requirements for compatibility, packaging, security, observability, deployment, modules, tooling, and release governance,
6. explicit non-goals and a priority order that keeps correctness and evidence integrity above performance or convenience.

The objective contract is versioned as `shorthand.language.objectives.version: 2026-07-29-v1` and retains `production_claim: false` while production blockers remain.

## Backend failure-mode finalization applied in PR #58

PR #58 adds a deterministic cross-layer matrix for eight required failure classes and protects it through the backend compatibility and enterprise hardening chain.

The matrix now proves:

1. invalid model formats are rejected before execution,
2. missing SDKs use honest `not_executed` fallback,
3. input shape/count mismatch is rejected without output copy,
4. unsupported precision cannot produce an execution-ready route,
5. output capacity mismatch preserves caller memory,
6. detected but inaccessible hardware is not execution-ready,
7. empty hardware probe results remain claim-safe,
8. fallback never reports success or returns inferred output.

The machine-readable report uses `shorthand.backend_failure_mode_matrix.v1`. Failure evidence is not backend success evidence, and `false_success` must remain false for every row.

## Status values

STATUS values: PLANNED, IN_PROGRESS, MERGED, BLOCKED, DEFERRED

## Update rule for every future PR

Every roadmap PR must update:

1. its row status,
2. actual evidence paths,
3. any newly discovered follow-up work,
4. the next recommended PR,
5. the remaining PR count.

A production-ready claim is blocked until all required rows are MERGED or intentionally DEFERRED with a clear production impact.

## Current baseline after PR #58

- Language contract and conformance marker are `beta-0.1`.
- Consolidated language objectives are versioned and guarded by CI.
- ONNX Runtime CPU has an optional SDK-backed compiled-hook success fixture.
- TensorRT, OpenVINO, LibTorch, and Llama.cpp have claim-safe unavailable-path proofs.
- Backend live SDK matrix reporting is guarded by CI.
- The common backend failure-mode matrix is finalized as `shorthand.backend_failure_mode_matrix.v1`.
- The matrix covers invalid format, missing SDK, shape, precision, capacity, hardware access, empty probe, and fallback honesty cases.
- `HardwareDiscovery.h` inventories CPU/GPU/TPU/NPU and applies execution-ready routing.
- Hardware inventory uses `shorthand.hardware.inventory.v1`.
- Hardware selection uses `shorthand.hardware.selection.v1`.
- `AIRuntime::infer` routes only through accessible, compatible, available backend/device pairs.
- Preference, override, deny-list, memory floor, CPU fallback, and fake probes are implemented.
- Inference telemetry includes hardware inventory and selection evidence.
- Runtime observability exports JSON, Prometheus-style text, and OTLP-like span JSON.

Important boundary: ShortHand is still not production ready. Runtime ABI/state work, packaging, operations exporters, diagnostics, parser robustness, modules, release security, developer tooling, C3-ECO completion, and MLIR integration remain open.

## Recommended remaining PR count

Recommended path from PR #51 onward: 29 PRs total.

After PR #58 is merged, approximately 21 implementation PRs remain. The count may split further only when implementation evidence identifies a concrete additional production blocker.

## Next recommended PR

Next recommended PR after PR #58:

PR59 - Runtime ABI and API version stability gate.

Reason: backend compatibility, hardware routing, backend-specific unavailable paths, and the common failure contract are now guarded. The next production-critical boundary is a stable public runtime ABI with version exports, symbol ownership, compatibility rules, and deprecation controls.

## PR roadmap table

| Planned PR | Status | Area | Goal | Expected evidence |
| --- | --- | --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | MERGED | Planning | Maintain this roadmap and guard it in CI. | `docs/production_readiness_pr_plan.md`, `scripts/check_production_readiness_pr_plan.sh` |
| PR52 - Backend live SDK matrix harness | MERGED | Backend coverage | Provide shared live/skip-safe backend reporting. | `docs/backend_live_sdk_matrix.md`, matrix test and gate |
| PR53 - TensorRT optional live execution fixture | MERGED | Backend coverage | Prove TensorRT unavailable paths cannot report false success. | TensorRT fixture docs, test and gate |
| PR54 - OpenVINO optional live execution fixture | MERGED | Backend coverage | Prove OpenVINO unavailable paths cannot report false success. | OpenVINO fixture docs, test and gate |
| PR55 - LibTorch optional live execution fixture | MERGED | Backend coverage | Prove LibTorch unavailable paths cannot report false success. | LibTorch fixture docs, test and gate |
| PR56 - Hardware capability discovery and accelerator-aware routing | MERGED | Runtime hardware selection | Inventory CPU/GPU/TPU/NPU, select execution-ready hardware, support policy controls, and emit telemetry. | `docs/hardware_capability_routing.md`, `HardwareDiscovery.h`, routing test and gate |
| PR57 - Llama.cpp optional live execution fixture | MERGED | Backend coverage and objectives | Prove Llama.cpp unavailable paths cannot report false success and consolidate the language objectives contract. | `docs/llamacpp_optional_fixture.md`, Llama.cpp test and gate, `docs/language_objectives.md`, objectives gate |
| PR58 - Backend failure-mode matrix finalization | MERGED | Runtime reliability | Cover invalid format, missing SDK, shape, precision, capacity, hardware probe, access, and fallback failures. | `docs/backend_failure_mode_matrix.md`, `tests/integration/test_backend_failure_mode_matrix.sh`, `scripts/check_backend_failure_mode_matrix.sh` |
| PR59 - Runtime ABI and API version stability gate | PLANNED | Runtime contract | Version public hooks, symbols, compatibility, and deprecation policy. | ABI API, symbol checks and docs |
| PR60 - Runtime state isolation and thread-safety policy | PLANNED | Runtime reliability | Enforce contexts/thread-safety or a tested explicit limitation. | isolation and concurrency tests |
| PR61 - Production build packaging for runtime and AI bridge | PLANNED | Build | Add repeatable production Make/CMake targets and link tests. | build targets and packaging evidence |
| PR62 - Prometheus scrape endpoint host adapter | PLANNED | Operations | Expose metrics through a host adapter. | adapter tests and docs |
| PR63 - OTLP exporter adapter | PLANNED | Operations | Add optional OTLP exporter/collector integration. | exporter tests and docs |
| PR64 - AST source ranges across parser nodes | PLANNED | Diagnostics | Store consistent source spans across AST nodes. | source-span tests |
| PR65 - Diagnostics coverage matrix | PLANNED | Diagnostics | Cover parser, semantic, AI, GreenAI, and lowering diagnostics. | diagnostics matrix and tests |
| PR66 - Full grammar and conformance matrix beta-0.2 | PLANNED | Language contract | Expand syntax coverage and version only with compatibility evidence. | grammar, manifest and version gate |
| PR67 - Parser robustness and negative corpus hardening | PLANNED | Language robustness | Add deterministic malformed-input and regression coverage. | negative corpus and robustness gate |
| PR68 - Module/import/package design and parser scaffold | PLANNED | Language scale | Add syntax and parser scaffold. | design and parser fixtures |
| PR69 - Module resolver and codegen integration | PLANNED | Language scale | Implement resolution, paths, boundaries, and codegen. | resolver and integration tests |
| PR70 - Signed release and protected release workflow | PLANNED | Release | Sign artifacts and protect release publication. | workflow, signing and provenance evidence |
| PR71 - External dependency vulnerability scan gate | PLANNED | Security | Add dependency vulnerability scanning. | CI gate and policy |
| PR72 - Container and Kubernetes hardening | PLANNED | Deployment | Add health, probes, limits, non-root and deployment validation. | deployment tests and docs |
| PR73 - Formatter and linter baseline | PLANNED | Developer experience | Standardize source formatting and lint behavior. | formatter/linter tests or spec |
| PR74 - Syntax highlighting and LSP skeleton | PLANNED | Developer experience | Add editor grammar and minimal LSP path. | syntax assets and LSP tests |
| PR75 - C3-ECO certification language blocks | PLANNED | C3-ECO language | Add declarations, boundaries, workload, measurement and lifecycle syntax. | grammar, parser and semantic tests |
| PR76 - C3-ECO scoring, report generation, and eco-regression | PLANNED | C3-ECO evidence | Expand candidate scoring/reporting and regression gates. | schema/report/workbook tests |
| PR77 - Authority-ready C3-ECO auditor bundle | PLANNED | C3-ECO evidence | Package claim-safe auditor handoff evidence. | bundle manifest and validation gate |
| PR78 - MLIR generated dialect build integration | PLANNED | MLIR | Integrate generated dialect build and parser/printer tests. | CMake and MLIR tests |
| PR79 - MLIR lowering passes and production RC gate | PLANNED | MLIR and release | Add lowering coverage and final production release-candidate blocker gate. | lowering tests, RC gate and claim scan |

## Production readiness exit criteria

ShortHand can be considered enterprise production usage ready only when:

1. Core protected-branch CI passes.
2. Language compatibility, deprecation, grammar, and conformance are complete.
3. Fallback never claims execution.
4. The common backend failure-mode matrix passes without false success.
5. Marketed backends are live-tested or removed from production claims.
6. Hardware inventory and routing separate detection from execution readiness.
7. ABI, runtime state, packaging, observability, diagnostics, parser robustness, modules, release security, deployment and tooling gates pass.
8. C3-ECO evidence is authority-ready and claim-safe.
9. MLIR lowering and the final production RC gate pass.

## Production claim rules

The project may claim controlled beta only until every required non-deferred row is MERGED.

The project may claim production-ready core language only if core grammar, diagnostics, modules, build, release, runtime ABI, runtime state, hardware routing, observability, deployment, and final RC gates pass.

Hardware detection never implies successful accelerator execution without backend execution-ready evidence and final inference success.

Failure-mode evidence never implies backend execution success.

The project may claim a full production backend matrix only when every marketed backend has a live success fixture or is removed from production-supported claims.

C3-ECO output remains candidate-only unless an external certifier signs it.

## Current remaining PR count field

remaining_planned_prs_total_from_pr51: 29
remaining_planned_prs_after_pr58: 21
