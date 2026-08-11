# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-11-pr73
PLAN_STATUS: active
LAST_COMPLETED_PR: 72
MERGED_OUT_OF_BAND_PR: 71
CURRENT_IMPLEMENTATION_PR: 73
NEXT_IMPLEMENTATION_PR_AFTER_PR73: 74
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, cancelled workflow, source-pattern check or transport acceptance response is not production execution evidence.

## Current baseline

PR69 is MERGED and established beta-0.3 package/module/import syntax and AST provenance.

PR70 is MERGED and established deterministic `shorthand.package.v1` resolution, package-root confinement, source identity validation, transitive dependency ordering, cycle rejection, `shorthand.lock.v1`, graph inspection and multi-file LLVM/native linking.

PR71 is MERGED as the out-of-band CI status publication hygiene correction.

PR72 is MERGED and established `execution_semantics_contract: beta-0.3-pr72-v1`, real interpreter call frames/control flow, imported execution through PR70's locked graph, deterministic runtime-domain failures and the interpreter/`lli`/native semantic differential oracle.

PR73 is the active implementation PR. It adds coverage-guided fuzzing across scanner, parser, semantic, module and lowering stages, ASan/LSan/UBSan-instrumented compiler execution, sanitized semantic differential coverage, scheduled extended fuzzing, and ThreadSanitizer instrumentation of the runtime implementation plus concurrency stress.

After PR72 merged, 14 implementation PRs remained. PR73 is now in progress; after PR73 is successfully merged, 13 implementation PRs remain.

## Controlled-beta boundary during PR73

ShortHand remains `controlled_beta` and `production_claim: false`.

PR73 is a safety-hardening PR. It does not change beta-0.3 syntax or expand production backend claims. Passing fuzz/sanitizer/race tests on Ubuntu does not prove cross-platform safety; PR74 owns the declared platform/toolchain matrix and reproducibility qualification.

AI backend/device qualification remains PR80. Full C3-ECO language/scoring/auditor evidence remains PR81-PR83. MLIR production lowering remains PR84-PR85. Measured ShortHand-versus-Python energy and performance evidence remains PR86.

## Completed foundation

Implemented or strongly guarded through the PR73 candidate:

- beta-0.2 parser-accurate base grammar and executable conformance matrix,
- beta-0.3 package/module/import extension and source provenance,
- bounded malformed-input handling and parser resource ceilings,
- stable source-aware diagnostics,
- deterministic manifest/lock/module graph and multi-file LLVM/native codegen,
- reference interpreter call frames, returns, break/continue and imported execution,
- deterministic int32 arithmetic and runtime-domain failures,
- interpreter/`lli`/native differential output and failure-code comparison,
- ASan/LSan/UBSan coverage-guided scanner/parser/semantic/module/lowering fuzz smoke,
- scheduled extended coverage-guided fuzzing with retained artifacts,
- ThreadSanitizer-instrumented runtime concurrency stress,
- honest backend fallback/unavailability behavior,
- CPU/GPU/TPU/NPU inventory and routing policy separated from execution proof,
- runtime ABI 1.0.0 with 25 public symbols,
- Linux packaging and observability baselines,
- compiler test strategy and 27-area coverage matrix,
- cancellation-safe event-specific CI status publication.

## PR73 completion contract

PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening is IN PROGRESS.

Implementation evidence includes:

- `docs/fuzz_sanitizer_concurrency.md`,
- `tests/fuzz/FuzzSubprocess.cpp`,
- stage corpora under `tests/fuzz/corpus/`,
- `scripts/check_fuzz_safety.sh`,
- `tests/runtime/runtime_tsan_stress.cpp`,
- `scripts/check_tsan_concurrency.sh`,
- sanitized PR72 semantic differential execution through `tests/run_language_tests.sh sanitize`,
- mandatory Actions steps for coverage-guided sanitizer fuzz smoke and ThreadSanitizer concurrency race testing,
- `.github/workflows/fuzz-nightly.yml` for scheduled extended mutation/race execution,
- CI artifact retention for fuzz logs, summary and crash inputs.

PR73 merge gate:

1. beta-0.2 grammar, beta-0.3 module syntax and PR72 semantic differential tests remain green.
2. scanner, parser, semantic, module and lowering fuzz stages execute the real ASan/LSan/UBSan compiler.
3. ordinary invalid-language rejection is accepted, but sanitizer findings, crashes, signal exits and hangs fail closed.
4. every fuzz stage has a checked-in seed corpus and deterministic PR seed/run budget.
5. fuzz findings retain artifacts suitable for minimization and regression promotion.
6. the scheduled extended fuzz workflow is bounded and uploads artifacts on failure.
7. the runtime implementation itself is built with TSan before concurrency stress; instrumenting only the test client is not sufficient.
8. TSan reports, races, deadlock markers and non-zero stress exits fail the gate.
9. the existing `make sanitize` suite remains mandatory and now includes the PR72 semantic differential path.
10. full Makefile suite, enterprise hardening, CMake configure/build and CTest remain green.
11. no mandatory test is weakened, skipped, converted to `continue-on-error`, or retried for deterministic green.
12. final PR head has `ci / ubuntu (push)` successful.
13. final PR head has `ci / ubuntu (pull_request)` successful.

## Mandatory rule for every remaining PR

Every PR through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip or `continue-on-error` success.

The final head of every PR must have both `ci / ubuntu (push)` and `ci / ubuntu (pull_request)` successful before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline design contract.

Pipeline principles:

1. **Fast fail first:** CI policy, generated-file freshness, grammar and diagnostics run before expensive compilation.
2. **Failure isolation:** policy, frontend, compiler, semantic differential, safety, matrix, backend and release concerns become separately named jobs as PR74 restructures the workflow.
3. **Stable aggregate gate:** event-specific merge status remains stable even when internal jobs evolve.
4. **Deterministic inputs:** toolchains, dependency fingerprints and random seeds are recorded or pinned.
5. **Bounded execution:** parser, graph, semantic, fuzz, network and runtime tests receive explicit timeouts.
6. **Honest hardware evidence:** CPU/GPU/TPU/NPU detection never counts as execution proof.
7. **Safe caching:** release qualification contains a clean no-cache build.
8. **Structured failure artifacts:** mandatory jobs emit usable logs/metadata.
9. **PR/nightly/RC profiles:** PR correctness stays bounded; nightly expands fuzz/scale; release candidates require zero mandatory skips.
10. **No flaky-green policy:** deterministic compiler/test failures are fixed at root cause rather than retried for green.

## Remaining implementation strategy

| Planned PR | Status | Implementation scope | Pipeline/CI implementation | Mandatory tests and exit evidence |
| --- | --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Versioned test strategy, 27-area matrix and PR contract. | Coverage governance. | Schema, IDs, counts and claim-safety gates. |
| PR69 - Module, import and package syntax with AST scaffold | MERGED | Beta-0.3 preamble grammar and AST provenance. | Module syntax gate. | Positive, negative, compatibility, stress and sanitizer tests. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED | Hermetic manifest resolution, lockfiles, graph ordering, visibility and multi-file codegen. | Resolver first-class gate. | Determinism, native binding, negative graph cases, stress, sanitizer and CTest evidence. |
| PR71 - CI status publication hygiene | MERGED | Cancellation-safe SHA-scoped status handling. | Stable event-specific status contexts. | Push/PR status hygiene and cancellation policy guard. |
| PR72 - Cross-mode semantic correctness and differential execution suite | MERGED | Reference executable semantics, interpreter calls/control flow, imported execution and LLVM/native parity. | Mandatory semantic-differential gate. | Core operators, arrays, calls, returns, loops, imported calls and runtime negatives across interpreter/lli/native. |
| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | IN PROGRESS | Scanner/parser/semantic/module/lowering fuzz targets; ASan, LSan, UBSan and TSan coverage. | Mandatory PR fuzz/TSan gates plus scheduled extended fuzz workflow. | Sanitizer fuzz stages, race stress, retained crash seed, no leaks/races/UB/hangs. |
| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | PLANNED | GCC/Clang, supported LLVM versions, Linux x86-64/arm64, macOS Apple Silicon, Windows, installed consumers and deterministic artifacts. | Multi-job DAG and clean no-cache reproducibility job. | Platform conformance, CTest parity, ABI consumers, clean-build checksums. |
| PR75 - Signed release and protected publication workflow | PLANNED | Protected environments, version/tag policy, OIDC signing, checksums, SBOM, provenance and attestations. | Qualified release workflow only. | Reject unsigned/mismatched artifacts, verify signatures, dry-run publication/rollback. |
| PR76 - External vulnerability, SAST, dependency and license policy gate | PLANNED | C++ SAST, dependency/image CVE scanning, license policy, pinned actions and expiring exceptions. | Dedicated security job. | Vulnerable dependency, secret, prohibited-license, SBOM and exception tests. |
| PR77 - Container and Kubernetes production hardening | PLANNED | Multi-arch non-root images, read-only filesystem, dropped capabilities, probes, limits and network policy. | Ephemeral-cluster integration. | Image scan, deployment, health, shutdown/restart, limits and network negatives. |
| PR78 - Formatter and linter baseline | PLANNED | Deterministic formatter, semantic-preserving lint rules, machine diagnostics and safe fixes. | Fast frontend format/lint job. | Idempotence, format-parse roundtrip, preservation and fix safety. |
| PR79 - Syntax highlighting and LSP implementation | PLANNED | Editor grammar plus compiler-backed diagnostics, hover, completion, definition and module navigation. | Protocol/golden job with cancellation and latency budget. | Token/protocol goldens, partial docs, cancellation and imported navigation. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | PLANNED | Production-supported backends and evidence-driven hardware target selection. | Capability-aware backend matrix. | Numerical outputs, route/fallback, unavailable device/SDK, OOM and no-false-success tests. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax/AST/semantics/evidence declarations without granting certification. | C3-ECO frontend gate. | Grammar/AST, valid/invalid contracts, units and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Energy/carbon/cost calculations with provenance, uncertainty, quality and baselines. | Deterministic eco-regression job. | Equation, threshold, stale-factor, invalid-unit and missing-sensor tests. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | RC bundle verification dependency. | Tamper, lineage, signature, schema, redaction and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR build/lit job. | FileCheck, verifiers, parse-print-parse, installed consumer and generated-source freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, LLVM lowering and AI/Green AI runtime handoff. | MLIR differential lowering tied to PR72 oracle. | Invalid ops/shapes, differential execution, runtime bridge and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata and blocker aggregation. | Final zero-skip release-candidate aggregate. | Harness calibration, identical models/inputs/outputs, repeated measurements, uncertainty and clean RC matrix. |

## Implementation details for the next four PRs after PR73

### PR74 - robust multi-job CI and portability

- policy -> frontend -> compiler/semantic/safety -> platform matrix -> aggregate DAG,
- GCC and Clang qualification,
- declared LLVM version matrix,
- Linux x86-64/arm64, macOS Apple Silicon and Windows qualification,
- cross-platform sanitizer/race qualification,
- generated-parser consistency,
- installed consumers and ABI compatibility,
- independent clean-build reproducibility comparison.

### PR75 - signed release pipeline

- immutable version/tag validation,
- protected release environment,
- SBOM/provenance/checksum generation,
- OIDC-backed artifact signing,
- signature verification and dry-run publication/rollback.

### PR76 - security gate

- C++ SAST,
- dependency/SBOM CVE reconciliation,
- action pinning,
- license allow/deny rules,
- expiring exceptions,
- secret scanning and negative policy fixtures.

### PR77 - deployment hardening

- multi-arch non-root container,
- read-only filesystem and dropped capabilities,
- probes and resource limits,
- Kubernetes policy and ephemeral-cluster tests.

## Hardware-selection production contract for PR80

Automatic hardware selection must be evidence-driven:

1. discover CPU/GPU/TPU/NPU inventory,
2. discover installed execution SDKs/runtimes,
3. inspect model format, operators, precision, shape and memory requirements,
4. calculate execution-ready candidates,
5. rank candidates using correctness first and explicit latency/energy/cost policy,
6. execute backend capability probes where required,
7. choose the highest-ranked verified candidate,
8. fall back to CPU only when policy permits and CPU execution is compatible,
9. emit structured reason codes for selection, fallback or failure.

A detected accelerator is not proof that a model executed on it.

## Current count

remaining_planned_prs_after_pr73: 13
remaining_planned_implementation_prs_pr74_through_pr86: 13

Next recommended PR after PR #73:

PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds.

## Historical roadmap anchors

The following strings are retained solely so earlier guarded milestones remain auditable and are superseded by `production_readiness_plan_version: 2026-08-11-pr73`.

- production_readiness_plan_version: 2026-08-02-pr62
- LAST_COMPLETED_PR: 62
- production_readiness_plan_version: 2026-08-02-pr63
- LAST_COMPLETED_PR: 63
- production_readiness_plan_version: 2026-08-02-pr66
- LAST_COMPLETED_PR: 66
- production_readiness_plan_version: 2026-08-02-pr67
- LAST_COMPLETED_PR: 67
- production_readiness_plan_version: 2026-08-06-pr68
- LAST_COMPLETED_PR: 68
- production_readiness_plan_version: 2026-08-06-pr69
- LAST_COMPLETED_PR: 69
- production_readiness_plan_version: 2026-08-09-pr70
- Recommended path from PR #51 onward: 29 PRs total.
- After PR #62 is merged, approximately 17 implementation PRs remain.
- PR63 - OTLP exporter adapter.
- After PR #65 is merged, approximately 14 implementation PRs remain.
- PR66 - Full grammar and conformance matrix beta-0.2.
- After PR #66 is merged, approximately 13 implementation PRs remain.
- PR67 - Parser robustness and negative corpus hardening.
- After PR #67 is merged, approximately 12 implementation PRs remain.
- PR68 - Module/import/package design and parser scaffold.
- PR79 - MLIR lowering passes and production RC gate
- The historical PR67 recommendation is superseded by the test re-audit.
- production_readiness_plan_version: 2026-08-11-pr72
- CURRENT_IMPLEMENTATION_PR: 72
- NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73
- PR72 - Cross-mode semantic correctness and differential execution suite is IN PROGRESS.
- after PR72 is successfully merged, 14 implementation PRs remain.
- remaining_planned_prs_after_pr72: 14
- remaining_planned_implementation_prs_pr73_through_pr86: 14
- Next recommended PR after PR #72:
- PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening.
