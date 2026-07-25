# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-07-25-pr52
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 52
BASELINE_LANGUAGE_VERSION: beta-0.1
TARGET: enterprise production usage ready language

## Purpose

This document is the single planning source for moving ShortHand from the current controlled beta foundation to an enterprise production usage ready language.

Future PRs should update this file whenever they complete, split, defer, or materially change a production-readiness milestone. This avoids re-evaluating the full roadmap every time a PR is merged.

## Desired outcome definition

For this roadmap, enterprise production usage ready language means:

1. The supported language surface is versioned, tested, documented, and compatible across releases.
2. The compiler and runtime can be built, packaged, tested, and released through repeatable production workflows.
3. Runtime inference, fallback, telemetry, and evidence paths are honest and claim-safe.
4. Supported AI backends are either live-tested or clearly marked as non-production / policy-compatible only.
5. Multi-file enterprise programs can be organized through module/import/package boundaries.
6. Operators have deployment, observability, security, and release evidence that can survive enterprise review.
7. C3-ECO evidence is authority-ready for auditor handoff but does not claim external certification unless a certifier signs it.
8. MLIR lowering is integrated beyond scaffold status for production-grade compiler evolution.

A production-ready claim must not depend on optional SDKs being present in default CI. Optional backend checks may skip safely, but unsupported or unavailable paths must never report successful execution.

## Audit correction applied in PR #51

The first version of this plan listed 25 PRs from PR #51 onward. After reviewing the desired outcome more strictly, that count was too optimistic.

The corrected plan adds three production-critical slices:

1. Runtime ABI and API version stability.
2. Runtime state isolation and thread-safety / reentrancy policy.
3. Parser robustness and negative corpus hardening.

These are important because enterprise users need stable runtime contracts, predictable behavior under concurrent or repeated use, and parser resilience beyond happy-path conformance.

## Status values

STATUS values: PLANNED, IN_PROGRESS, MERGED, BLOCKED, DEFERRED

Use these values consistently in the plan table.

## Update rule for every future PR

Every PR that implements one of the items below must update this document in the same PR.

The update should change:

1. the PR row status,
2. the evidence path or script names,
3. any new follow-up PRs discovered during implementation,
4. the "next recommended PR" section,
5. the remaining PR count.

A PR must not mark ShortHand as production ready unless all production-readiness rows are MERGED or intentionally DEFERRED with a clear reason.

## Current baseline after PR #52

Current state after PR #52:

- Beta language contract is explicit as `beta-0.1`.
- Conformance manifest has a version row and required categories.
- Compiler, runtime, C3-ECO candidate evidence, MLIR scaffold, release supply-chain baseline, and runtime bridge paths are all guarded by CI.
- ONNX Runtime CPU execution exists when `ONNXRUNTIME_ROOT` is configured.
- The compiled-hook path has an optional ONNX Runtime success fixture.
- Runtime observability has JSON, Prometheus-style text, and OTLP-like span JSON exports.
- A shared backend live SDK matrix harness records row-level `live_success`, `skip_safe`, `policy_compatible_only`, and `dedicated_fixture_planned` status without making unsupported backend claims.

Important boundary:

ShortHand is still not production ready. The open work includes backend-specific live fixtures beyond ONNX Runtime CPU, stable runtime ABI/versioning, runtime state/thread-safety, production packaging, real observability export integration, full diagnostics, parser robustness, container hardening, release signing, external vulnerability scans, module/package support, developer tooling, full C3-ECO language support, and MLIR lowering integration.

## Recommended remaining PR count

Recommended path from PR #51 onward: 28 PRs total.

That includes PR #51, the planning PR.

After PR #52 is merged, the remaining implementation path is expected to be about 26 PRs. Some PRs may split further if SDK availability, MLIR build tooling, parser changes, or C3-ECO evidence generation becomes larger than expected.

## Next recommended PR

Next recommended PR after PR #52:

PR53 - TensorRT optional live execution fixture.

Reason: PR #52 provides the shared matrix harness. The next highest-value backend slice is to add the first non-ONNX-CPU dedicated fixture or a strict unavailable-path proof for TensorRT and ONNX Runtime TensorRT, without false success claims.

## PR roadmap table

| Planned PR | Status | Area | Goal | Expected evidence |
| --- | --- | --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | MERGED | Planning and governance | Add this plan and a lightweight gate so future PRs update the roadmap instead of re-evaluating from scratch. | `docs/production_readiness_pr_plan.md`, `scripts/check_production_readiness_pr_plan.sh` |
| PR52 - Backend live SDK matrix harness | MERGED | AI runtime backend coverage | Add one matrix runner that can execute or skip SDK-backed backend probes consistently. Keep default CI skip-safe. | `docs/backend_live_sdk_matrix.md`, `tests/integration/test_backend_live_sdk_matrix.sh`, `scripts/check_backend_live_sdk_matrix.sh`, updated `docs/backend_compatibility_matrix.md` |
| PR53 - TensorRT optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional TensorRT execution fixture or explicit unavailable-path proof with no false success. | TensorRT gate and fixture docs |
| PR54 - OpenVINO optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional OpenVINO execution fixture or explicit unavailable-path proof with no false success. | OpenVINO gate and fixture docs |
| PR55 - LibTorch optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional LibTorch execution fixture or explicit unavailable-path proof with no false success. | LibTorch gate and fixture docs |
| PR56 - Llama.cpp optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional Llama.cpp execution fixture or mark the backend as policy-compatible only until a real fixture is available. | Llama.cpp gate or documented deferral |
| PR57 - Backend failure-mode matrix finalization | PLANNED | AI runtime reliability | Cover invalid model format, missing SDK, wrong tensor shape, unsupported precision, capacity mismatch, and fallback honesty across backends. | failure matrix tests and docs |
| PR58 - Runtime ABI and API version stability gate | PLANNED | Runtime contract | Add runtime ABI version exports, API compatibility rules, symbol ownership checks, and deprecation policy for public C hooks. | ABI version API, symbol checks, compatibility docs |
| PR59 - Runtime state isolation and thread-safety policy | PLANNED | Runtime reliability | Decide and enforce production state model: thread-safe shared runtime, explicit context handles, or documented single-thread limitation with tests. | state isolation tests, concurrency or single-thread guardrails |
| PR60 - Production build packaging for runtime and AI bridge | PLANNED | Build and packaging | Provide explicit Make/CMake production targets for compiled-hook plus AI runtime bridge without ad hoc flags. | Makefile/CMake targets, link tests |
| PR61 - Prometheus scrape endpoint host adapter | PLANNED | Observability operations | Turn Prometheus-style metrics strings into a minimal host-exposable scrape endpoint or adapter. | host adapter test, docs |
| PR62 - OTLP exporter adapter | PLANNED | Observability operations | Add an optional OTLP exporter or collector adapter while preserving dependency-light core runtime. | OTLP adapter test, docs |
| PR63 - AST source ranges across parser nodes | PLANNED | Diagnostics | Add consistent source span storage for parser and AST nodes. | parser/AST changes and source-span tests |
| PR64 - Diagnostics coverage matrix | PLANNED | Diagnostics | Extend diagnostics so parser, semantic, AI, GreenAI, model/tensor, and runtime-lowering errors have file, line, column, source line, and caret/range. | diagnostics matrix and tests |
| PR65 - Full grammar and conformance matrix beta-0.2 | PLANNED | Language contract | Expand conformance manifest to a fuller syntax matrix and advance the language contract only if compatible. | updated grammar, manifest, versioning gate |
| PR66 - Parser robustness and negative corpus hardening | PLANNED | Language robustness | Add parser robustness checks, malformed-input corpus, and regression tests so syntax failures remain deterministic and useful. | parser negative corpus, robustness gate |
| PR67 - Module/import/package design and parser scaffold | PLANNED | Enterprise language scale | Add module/import/package syntax design and parser scaffold without full resolver risk. | design doc, parser fixtures |
| PR68 - Module resolver and codegen integration | PLANNED | Enterprise language scale | Implement module resolution, import path handling, package boundaries, and codegen integration. | resolver tests, integration tests |
| PR69 - Signed release and protected release workflow | PLANNED | Release governance | Add signed release artifacts, protected release workflow, and provenance linkage. | release workflow, signing docs, provenance tests |
| PR70 - External dependency vulnerability scan gate | PLANNED | Security and supply chain | Add external dependency vulnerability scanning while keeping current baseline secret/source scans. | CI scan gate and policy docs |
| PR71 - Container and Kubernetes hardening | PLANNED | Deployment | Add health checks, readiness/liveness probes, resource limits, non-root container guidance, and deployment validation. | container/Kubernetes tests and docs |
| PR72 - Formatter and linter baseline | PLANNED | Developer experience | Add baseline formatter/linter behavior or specification so enterprise users can standardize code style. | formatter/linter tests or spec gate |
| PR73 - Syntax highlighting and LSP skeleton | PLANNED | Developer experience | Add syntax highlighting assets and an LSP roadmap or minimal skeleton. | syntax grammar, LSP scaffold/tests |
| PR74 - C3-ECO certification language blocks | PLANNED | GreenAI/C3-ECO language | Add certification declaration, boundary, workload, measurement plan, lifecycle, and RAG/token/cache syntax blocks. | grammar, parser, semantic tests |
| PR75 - C3-ECO scoring, report generation, and eco-regression | PLANNED | GreenAI/C3-ECO evidence | Add candidate-only scoring, report generation expansion, and eco-regression gates. | schema/report/workbook tests |
| PR76 - Authority-ready C3-ECO auditor bundle | PLANNED | GreenAI/C3-ECO evidence | Package evidence for auditor handoff while retaining candidate-only claim safety. | bundle manifest, auditor docs, validation gate |
| PR77 - MLIR generated dialect build integration | PLANNED | MLIR lowering | Integrate generated MLIR dialect build artifacts and parser/printer tests. | MLIR CMake/build tests |
| PR78 - MLIR lowering passes and production RC gate | PLANNED | MLIR and release readiness | Add semantic IR to MLIR lowering pass coverage and a final production release-candidate gate that checks all blockers and claim safety. | lowering tests, RC gate, docs-wide claim scan |

## Production readiness exit criteria

ShortHand can be considered enterprise production usage ready only when the following are true:

1. All core CI jobs pass on protected branches.
2. The language contract has a versioned compatibility and deprecation policy.
3. The conformance matrix covers the supported syntax surface.
4. Runtime fallback never claims executed inference.
5. ONNX Runtime CPU has a real compiled-hook success fixture.
6. Other marketed backends are either live-tested or clearly marked non-executing and not marketed as production-supported.
7. Runtime ABI and public hook compatibility are versioned and guarded.
8. Runtime state behavior is production-safe, either through tested isolation/thread-safety or an explicit documented limitation.
9. Runtime observability can be exported through production-facing mechanisms.
10. Parser and semantic diagnostics include source locations and useful ranges.
11. Parser robustness is covered by negative fixtures and deterministic failure behavior.
12. Release artifacts are signed and backed by provenance.
13. Dependency vulnerability scanning is part of CI.
14. Container/Kubernetes deployment has health, readiness, and security posture checks.
15. Module/import/package support works for multi-file enterprise programs.
16. Developer tooling has at least baseline formatter, linter, syntax highlighting, and LSP path.
17. C3-ECO evidence generation is authority-ready but still claim-safe unless an external certifier signs it.
18. MLIR lowering is integrated beyond scaffold status.
19. The final production RC gate blocks unsupported production claims.

## Production claim rules

The project may claim controlled beta only until every non-deferred row required for the desired outcome is MERGED.

The project may claim production-ready core language only if:

- core grammar, diagnostics, module/import, build, release, runtime ABI, runtime state model, observability, and deployment rows are MERGED,
- unsupported optional backends are not marketed as production-supported,
- C3-ECO evidence remains candidate-only unless externally certified,
- the final RC gate passes.

The project may claim full production backend matrix only if every marketed backend has a live success fixture or is removed from production-supported claims.

## How to update this file

When a planned PR is merged:

- Change its status from PLANNED or IN_PROGRESS to MERGED.
- Add the actual PR number if the plan changed.
- Add concrete evidence paths.
- Update the next recommended PR.
- Update the remaining PR count.
- Add a new follow-up row if the implementation reveals a new blocker.

When a planned PR is split:

- Keep the original PR row and mark it as split.
- Add the new child PR rows immediately below it.
- Do not delete historical rows.

When a planned PR is deferred:

- Mark it DEFERRED.
- Explain the reason.
- State what production claim is still blocked by the deferral.

## Current remaining PR count field

remaining_planned_prs_total_from_pr51: 28
remaining_planned_prs_after_pr52: 26
