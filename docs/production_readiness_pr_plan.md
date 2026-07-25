# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-07-25-pr51
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
BASELINE_LANGUAGE_VERSION: beta-0.1
TARGET: enterprise production usage ready language

## Purpose

This document is the single planning source for moving ShortHand from the current controlled beta foundation to an enterprise production usage ready language.

Future PRs should update this file whenever they complete, split, defer, or materially change a production-readiness milestone. This avoids re-evaluating the full roadmap every time a PR is merged.

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

## Current baseline after PR #50

Current state after PR #50:

- Beta language contract is explicit as `beta-0.1`.
- Conformance manifest has a version row and required categories.
- Compiler, runtime, C3-ECO candidate evidence, MLIR scaffold, release supply-chain baseline, and runtime bridge paths are all guarded by CI.
- ONNX Runtime CPU execution exists when `ONNXRUNTIME_ROOT` is configured.
- The compiled-hook path has an optional ONNX Runtime success fixture.
- Runtime observability has JSON, Prometheus-style text, and OTLP-like span JSON exports.

Important boundary:

ShortHand is still not production ready. The open work includes live backend matrix coverage, production packaging, real observability export integration, full diagnostics, container hardening, release signing, external vulnerability scans, module/package support, developer tooling, full C3-ECO language support, and MLIR lowering integration.

## Recommended remaining PR count

Recommended path from PR #51 onward: 25 PRs total.

That includes this planning PR.

After this planning PR is merged, the remaining implementation path is expected to be about 24 PRs. Some PRs may split further if SDK availability, MLIR build tooling, or C3-ECO evidence generation becomes larger than expected.

## Next recommended PR

Next recommended PR after this planning PR:

PR52 - Backend live SDK matrix harness.

Reason: backend execution is currently strong for ONNX Runtime CPU but not yet live across the marketed backend matrix. Production claims should not advance until backend coverage and failure behavior are represented by a repeatable matrix harness.

## PR roadmap table

| Planned PR | Status | Area | Goal | Expected evidence |
| --- | --- | --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | IN_PROGRESS | Planning and governance | Add this plan and a lightweight gate so future PRs update the roadmap instead of re-evaluating from scratch. | `docs/production_readiness_pr_plan.md`, `scripts/check_production_readiness_pr_plan.sh` |
| PR52 - Backend live SDK matrix harness | PLANNED | AI runtime backend coverage | Add one matrix runner that can execute or skip SDK-backed backend probes consistently. Keep default CI skip-safe. | backend matrix runner, updated `docs/backend_compatibility_matrix.md`, CI gate |
| PR53 - TensorRT optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional TensorRT execution fixture or explicit unavailable-path proof with no false success. | TensorRT gate and fixture docs |
| PR54 - OpenVINO optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional OpenVINO execution fixture or explicit unavailable-path proof with no false success. | OpenVINO gate and fixture docs |
| PR55 - LibTorch optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional LibTorch execution fixture or explicit unavailable-path proof with no false success. | LibTorch gate and fixture docs |
| PR56 - Llama.cpp optional live execution fixture | PLANNED | AI runtime backend coverage | Add optional Llama.cpp execution fixture or mark the backend as policy-compatible only until a real fixture is available. | Llama.cpp gate or documented deferral |
| PR57 - Backend failure-mode matrix finalization | PLANNED | AI runtime reliability | Cover invalid model format, missing SDK, wrong tensor shape, unsupported precision, capacity mismatch, and fallback honesty across backends. | failure matrix tests and docs |
| PR58 - Production build packaging for runtime and AI bridge | PLANNED | Build and packaging | Provide explicit Make/CMake production targets for compiled-hook plus AI runtime bridge without ad hoc flags. | Makefile/CMake targets, link tests |
| PR59 - Prometheus scrape endpoint host adapter | PLANNED | Observability operations | Turn Prometheus-style metrics strings into a minimal host-exposable scrape endpoint or adapter. | host adapter test, docs |
| PR60 - OTLP exporter adapter | PLANNED | Observability operations | Add an optional OTLP exporter or collector adapter while preserving dependency-light core runtime. | OTLP adapter test, docs |
| PR61 - AST source ranges across parser nodes | PLANNED | Diagnostics | Add consistent source span storage for parser and AST nodes. | parser/AST changes and source-span tests |
| PR62 - Diagnostics coverage matrix | PLANNED | Diagnostics | Extend diagnostics so parser, semantic, AI, GreenAI, model/tensor, and runtime-lowering errors have file, line, column, source line, and caret/range. | diagnostics matrix and tests |
| PR63 - Full grammar and conformance matrix beta-0.2 | PLANNED | Language contract | Expand conformance manifest to a fuller syntax matrix and advance the language contract only if compatible. | updated grammar, manifest, versioning gate |
| PR64 - Module/import/package design and parser scaffold | PLANNED | Enterprise language scale | Add module/import/package syntax design and parser scaffold without full resolver risk. | design doc, parser fixtures |
| PR65 - Module resolver and codegen integration | PLANNED | Enterprise language scale | Implement module resolution, import path handling, package boundaries, and codegen integration. | resolver tests, integration tests |
| PR66 - Signed release and protected release workflow | PLANNED | Release governance | Add signed release artifacts, protected release workflow, and provenance linkage. | release workflow, signing docs, provenance tests |
| PR67 - External dependency vulnerability scan gate | PLANNED | Security and supply chain | Add external dependency vulnerability scanning while keeping current baseline secret/source scans. | CI scan gate and policy docs |
| PR68 - Container and Kubernetes hardening | PLANNED | Deployment | Add health checks, readiness/liveness probes, resource limits, non-root container guidance, and deployment validation. | container/Kubernetes tests and docs |
| PR69 - Formatter and linter baseline | PLANNED | Developer experience | Add baseline formatter/linter behavior or specification so enterprise users can standardize code style. | formatter/linter tests or spec gate |
| PR70 - Syntax highlighting and LSP skeleton | PLANNED | Developer experience | Add syntax highlighting assets and an LSP roadmap or minimal skeleton. | syntax grammar, LSP scaffold/tests |
| PR71 - C3-ECO certification language blocks | PLANNED | GreenAI/C3-ECO language | Add certification declaration, boundary, workload, measurement plan, lifecycle, and RAG/token/cache syntax blocks. | grammar, parser, semantic tests |
| PR72 - C3-ECO scoring, report generation, and eco-regression | PLANNED | GreenAI/C3-ECO evidence | Add candidate-only scoring, report generation expansion, and eco-regression gates. | schema/report/workbook tests |
| PR73 - Authority-ready C3-ECO auditor bundle | PLANNED | GreenAI/C3-ECO evidence | Package evidence for auditor handoff while retaining candidate-only claim safety. | bundle manifest, auditor docs, validation gate |
| PR74 - MLIR generated dialect build integration | PLANNED | MLIR lowering | Integrate generated MLIR dialect build artifacts and parser/printer tests. | MLIR CMake/build tests |
| PR75 - MLIR lowering passes and production RC gate | PLANNED | MLIR and release readiness | Add semantic IR to MLIR lowering pass coverage and a final production release-candidate gate that checks all blockers and claim safety. | lowering tests, RC gate, docs-wide claim scan |

## Production readiness exit criteria

ShortHand can be considered enterprise production usage ready only when the following are true:

1. All core CI jobs pass on protected branches.
2. The language contract has a versioned compatibility and deprecation policy.
3. The conformance matrix covers the supported syntax surface.
4. Runtime fallback never claims executed inference.
5. ONNX Runtime CPU has a real compiled-hook success fixture.
6. Other marketed backends are either live-tested or clearly marked non-executing and not marketed as production-supported.
7. Runtime observability can be exported through production-facing mechanisms.
8. Parser and semantic diagnostics include source locations and useful ranges.
9. Release artifacts are signed and backed by provenance.
10. Dependency vulnerability scanning is part of CI.
11. Container/Kubernetes deployment has health, readiness, and security posture checks.
12. Module/import/package support works for multi-file enterprise programs.
13. Developer tooling has at least baseline formatter, linter, syntax highlighting, and LSP path.
14. C3-ECO evidence generation is authority-ready but still claim-safe unless an external certifier signs it.
15. MLIR lowering is integrated beyond scaffold status.
16. The final production RC gate blocks unsupported production claims.

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

remaining_planned_prs_including_this_file: 25
remaining_planned_prs_after_this_file: 24
