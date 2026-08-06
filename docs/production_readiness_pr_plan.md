# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-06-pr69
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 69
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

Historical guard markers retained for old-task stability:

- production_readiness_plan_version: 2026-08-02-pr62
- LAST_COMPLETED_PR: 62
- production_readiness_plan_version: 2026-08-02-pr63
- LAST_COMPLETED_PR: 63
- production_readiness_plan_version: 2026-08-02-pr66
- LAST_COMPLETED_PR: 64
- LAST_COMPLETED_PR: 65
- LAST_COMPLETED_PR: 66
- production_readiness_plan_version: 2026-08-02-pr67
- LAST_COMPLETED_PR: 67
- production_readiness_plan_version: 2026-08-06-pr68
- LAST_COMPLETED_PR: 68

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, source-pattern check or transport acceptance response is not production execution evidence.

## Current baseline after PR69

Implemented or strongly guarded:

- beta-0.2 parser-accurate base grammar and executable conformance matrix,
- beta-0.3 package, module, dotted import and alias syntax,
- a provenance-aware module AST scaffold with deterministic `module-info` JSON,
- stable `SHD2011` through `SHD2016` module diagnostics,
- backward compatibility for sources without a module preamble,
- bounded malformed-input handling and parser resource ceilings,
- stable coded diagnostics and source ranges,
- honest backend failure behavior and hardware routing policy,
- runtime ABI 1.0.0 with 25 public symbols,
- serialized runtime calls, Linux packaging and observability adapters,
- compiler test strategy and 27-area coverage matrix,
- module tests in normal and sanitizer-backed paths.

Explicit boundary:

- imports are not resolved,
- package files and lockfiles do not exist,
- imported symbols are not bound,
- multi-file LLVM or native code generation is not implemented,
- `module-info` reports `resolver_status: not_resolved`.

ShortHand remains `controlled_beta` and `production_claim: false`.

## PR69 completion

PR69 - Module, import and package syntax with AST scaffold is complete in the planned post-merge state.

Implementation evidence:

- `Compiler_new_ws/Short_Hand/src/ast/ModuleAST.h`,
- scanner keywords and dotted paths,
- optional parser preamble,
- package, module, import, alias, source-path and source-range metadata,
- deterministic `module-info` output,
- stable ordering and duplicate diagnostics,
- beta-0.3 versioning and conformance evidence.

Test evidence:

- `tests/conformance/module_matrix_beta_0_3.tsv`,
- `tests/modules/valid/module_preamble.short`,
- `tests/modules/invalid/`,
- `scripts/check_module_ast_scaffold.sh`,
- grammar regeneration with Bison conflicts treated as errors,
- legacy beta-0.2 parse compatibility,
- large import-set stress,
- ASan, LSan and UBSan execution.

After PR #69 is merged, 16 implementation PRs remain.

## Mandatory rule for every remaining PR

Every PR from PR70 through PR85 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip.

## Remaining implementation strategy

| Planned PR | Status | Implementation scope | Mandatory tests and exit evidence |
| --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Versioned test strategy, 27-area matrix, PR template and CI audit. | Matrix schema, unique IDs, count and claim-safety gates. |
| PR69 - Module, import and package syntax with AST scaffold | MERGED | Beta-0.3 preamble grammar, AST provenance, stable diagnostics and inspection mode. | Positive, negative, compatibility, stress and sanitizer tests. Resolver remains deferred. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | PLANNED | Canonical resolution, hermetic roots, manifests, lockfiles, graph ordering, cycles, visibility and multi-file LLVM/native codegen. | Resolver unit tests; multi-file parse, compile, run and native fixtures; missing import, cycle, ambiguity, duplicate package and path-escape negatives; lockfile determinism; graph stress. |
| PR71 - Cross-mode semantic correctness and differential execution suite | PLANNED | Positive semantic matrix and normalized comparison across interpreter, bitcode and native binaries. | All core types, expressions, calls, returns, branches, loops and arrays; output and exit-status equivalence; undefined-behavior rejection; coverage baseline. |
| PR72 - Continuous fuzzing, full sanitizer and concurrency race hardening | PLANNED | Scanner, parser, semantic and lowering fuzz targets; full ASan, LSan, UBSan and TSan suites. | PR fuzz smoke, scheduled fuzzing, full corpus sanitizers, concurrent reset/registration/inference stress, regression fixture per finding. |
| PR73 - Cross-platform toolchain matrix, CTest parity and reproducible builds | PLANNED | GCC/Clang, supported LLVM versions, Linux x86-64/arm64, macOS, Windows, CTest parity and deterministic artifacts. | Platform conformance, installed consumers, independent checksums, ABI consumers and generated-parser consistency. |
| PR74 - Signed release and protected publication workflow | PLANNED | Protected environments, tag/version checks, OIDC signing, checksums, SBOM, provenance and attestations. | Reject unsigned, mismatched or modified artifacts; verify signatures; dry-run release and rollback. |
| PR75 - External vulnerability, SAST, dependency and license policy gate | PLANNED | C++ SAST, dependency/image CVE scans, license policy, pinned actions and expiring exceptions. | Vulnerable dependency, secret and prohibited-license fixtures; SBOM reconciliation; expired exception rejection. |
| PR76 - Container and Kubernetes production hardening | PLANNED | Multi-arch non-root images, read-only filesystem, dropped capabilities, probes, limits and network policy. | Image scan, non-root/read-only enforcement, ephemeral cluster deployment, health/shutdown/restart and network-negative tests. |
| PR77 - Formatter and linter baseline | PLANNED | Deterministic formatter, semantic-preserving lint rules, machine-readable diagnostics and safe fixes. | Idempotence, format-parse round trip, comment/string preservation, lint fixtures and malformed-input bounds. |
| PR78 - Syntax highlighting and LSP implementation | PLANNED | Editor grammar and compiler-backed diagnostics, hover, completion, definition and module navigation. | Golden protocol/token output, partial documents, cancellation, stale versions, import navigation and latency budget. |
| PR79 - Production backend and hardware qualification matrix | PLANNED | Declare production-supported backends and qualify real execution on controlled CPU, GPU and available NPU hardware. | Numerical model output, hardware route selection, unavailable SDK, incompatible format, device loss, OOM and no-false-success tests. |
| PR80 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax, AST, semantic rules and evidence declarations without granting certification. | Grammar/AST matrix, valid examples, missing functional unit, invalid boundary, unsupported claim, unit and sanitizer tests. |
| PR81 - Measured scoring, reports and eco-regression | PLANNED | Measured energy, carbon and cost calculations with factor provenance, uncertainty, quality and baselines. | Independent equation checks, deterministic reports, threshold pass/fail, stale factor, invalid unit and missing-sensor negatives. |
| PR82 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | Tamper, missing lineage, invalid signature, schema mismatch, redaction leakage and clean-room replay. |
| PR83 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR lit/FileCheck, verifier cases, parse-print-parse, installed consumer and freshness guard. |
| PR84 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, core LLVM lowering and AI/Green AI runtime handoff. | Pass tests, invalid operation/shape cases, differential execution, bridge tests and optimization preservation. |
| PR85 - Measured energy, performance and production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata, uncertainty and blocker aggregation. | Harness calibration, identical inputs/models/outputs, repeated measurements, noise-aware thresholds, sensor negatives and zero-skip clean release matrix. |

## Recommended next PR

Next recommended PR after PR #69:

PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen.

Reason: the parser and AST now represent source-unit identity and import intent. Enterprise-scale programs next require secure, deterministic resolution and actual multi-file compilation without undeclared host-path dependencies.

## Production readiness exit criteria

The final release gate must pass language compatibility, module resolution, semantic equivalence, fuzzing and sanitizers, live backends, hardware routing, ABI, concurrency, portability, reproducibility, signing, security, deployment, tooling, C3-ECO, MLIR, performance and measured energy checks with zero mandatory skips.

## Remaining PR count fields

remaining_planned_prs_total_from_pr51_original: 29
remaining_planned_prs_total_from_pr51_reaudited: 35
remaining_planned_prs_after_pr61: 18
remaining_planned_prs_after_pr62: 17
remaining_planned_prs_after_pr63: 16
remaining_planned_prs_after_pr64: 15
remaining_planned_prs_after_pr65: 14
remaining_planned_prs_after_pr66: 13
remaining_planned_prs_after_pr67_original_estimate: 12
remaining_planned_prs_after_pr67_reaudited: 18
remaining_planned_prs_after_pr68: 17
remaining_planned_prs_after_pr69: 16

## Historical compatibility record

Language objectives consolidation applied in PR #57

Backend failure-mode finalization applied in PR #58

Runtime ABI and API stability applied in PR #59

Runtime state and thread-safety applied in PR #60

Production build packaging applied in PR #61

Prometheus scrape endpoint host adapter applied in PR #62

Recommended path from PR #51 onward: 29 PRs total.

PR59 - Runtime ABI and API version stability gate | MERGED

PR60 - Runtime state isolation and thread-safety policy | MERGED

PR61 - Production build packaging for runtime and AI bridge | MERGED

PR62 - Prometheus scrape endpoint host adapter | MERGED

After PR #62 is merged, approximately 17 implementation PRs remain.

Next recommended PR after PR #62:

PR63 - OTLP exporter adapter.

After PR #63 is merged, approximately 16 implementation PRs remain.

Next recommended PR after PR #63:

PR64 - AST source ranges across parser nodes.

After PR #65 is merged, approximately 14 implementation PRs remain.

Next recommended PR after PR #65:

PR66 - Full grammar and conformance matrix beta-0.2.

After PR #66 is merged, approximately 13 implementation PRs remain.

Next recommended PR after PR #66:

PR67 - Parser robustness and negative corpus hardening.

After PR #67 is merged, approximately 12 implementation PRs remain.

Next recommended PR after PR #67:

PR68 - Module/import/package design and parser scaffold.

PR67 - Parser robustness and negative corpus hardening

PR79 - MLIR lowering passes and production RC gate

The historical PR67 recommendation is superseded by the test re-audit. PR68 established test governance and PR69 implements the syntax and AST scaffold.
