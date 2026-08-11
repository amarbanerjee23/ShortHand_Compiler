# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-09-pr70-resume
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 71
CURRENT_IMPLEMENTATION_PR: 70
NEXT_IMPLEMENTATION_PR_AFTER_PR70: 72
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, cancelled workflow, source-pattern check or transport acceptance response is not production execution evidence.

## Current ordering and numbering

PR69 is merged and established beta-0.3 module/import/package syntax and AST provenance.

PR70 is the active implementation PR. It owns deterministic package manifests, module resolution, lockfiles and multi-file code generation. Its implementation is present, but the PR is not considered complete until the refreshed final head passes both required CI contexts.

PR71 is already MERGED as an out-of-band CI-hygiene correction. It prevents cancelled feature-branch runs from contaminating a shared default-branch commit status and adds an executable CI-policy regression guard.

Because PR71 is now legitimately consumed, the old planned PR71 through PR85 sequence is renumbered to PR72 through PR86. The implementation count does not increase: after PR70 is successfully merged, 15 implementation PRs remain.

Historical guard marker only: LAST_COMPLETED_PR: 70

## Current baseline entering the resumed PR70

Implemented or strongly guarded before PR70 merge:

- beta-0.2 parser-accurate base grammar and executable conformance matrix,
- beta-0.3 package, module, dotted import and alias syntax,
- provenance-aware module AST scaffold and deterministic `module-info` JSON,
- bounded malformed-input handling and parser resource ceilings,
- stable coded diagnostics and source ranges,
- honest backend failure behavior,
- CPU/GPU/TPU/NPU hardware inventory model with detection separated from execution readiness,
- runtime ABI 1.0.0 with 25 public symbols,
- serialized runtime calls, Linux packaging and observability adapters,
- compiler test strategy and 27-area coverage matrix,
- cancellation-safe event-specific CI statuses,
- CI policy regression guard.

PR70 adds:

- hermetic `shorthand.package.v1` manifest resolution without ambient host search paths,
- nearest-ancestor package-root discovery,
- package-root canonical path confinement,
- exact package/module source identity validation,
- deterministic transitive import graphs and dependency-first ordering,
- missing-module and import-cycle rejection,
- direct-import function visibility,
- graph-wide linker-visible symbol-collision rejection,
- deterministic `shorthand.lock.v1` reachable-graph fingerprints,
- stale/missing lock rejection for executable modes,
- `shorthand.module.graph.v1` inspection,
- multi-file LLVM/bitcode/native lowering and linking,
- 128-module bounded graph stress,
- resolver execution in normal and sanitizer-backed tests.

Explicit boundary after PR70:

- imported interpreter calls are still rejected rather than falsely reported as executed,
- language-wide interpreter/LLVM/native semantic equivalence remains PR72,
- external package registries/acquisition are not part of beta-0.3,
- lockfile fingerprints are deterministic stale-content evidence, not release signatures,
- hardware detection is not hardware execution proof,
- ShortHand remains `controlled_beta` and `production_claim: false`.

## PR69 completion

PR69 - Module, import and package syntax with AST scaffold is complete.

Evidence includes `ModuleAST.h`, scanner keywords/dotted paths, optional parser preamble, package/module/import/alias provenance, deterministic `module-info`, beta-0.3 conformance matrices, duplicate/order diagnostics, legacy beta-0.2 compatibility, stress and sanitizer coverage.

## PR70 active completion contract

PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen is IN PROGRESS.

Implementation evidence:

- `Compiler_new_ws/Short_Hand/src/module/ModuleResolver.h`,
- `Compiler_new_ws/Short_Hand/src/module/ModuleResolver.cpp`,
- `docs/module_resolution_and_lockfile.md`,
- explicit `shorthand.package.v1` module mapping,
- canonical package-root confinement,
- package/module identity validation,
- deterministic reachable dependency graph and cycle rejection,
- direct-import binding and graph-wide collision rejection,
- deterministic `shorthand.lock.v1`,
- `module-graph` inspection,
- multi-file bitcode/native dependency lowering,
- stable module diagnostics,
- `scripts/check_module_resolution.sh`,
- positive/negative/native/stale-lock/path-escape/ambiguity/cycle/collision tests,
- generated 128-module stress.

PR70 merge gate:

1. `scripts/check_ci_status_hygiene.sh` passes.
2. `scripts/check_module_resolution.sh` passes independently.
3. full Makefile suite passes.
4. sanitizer suite passes.
5. CMake configure/build and CTest pass.
6. no mandatory test is weakened or converted to an unconditional skip.
7. final PR head has `ci / ubuntu (push)` successful.
8. final PR head has `ci / ubuntu (pull_request)` successful.

## PR71 completion

PR71 - CI status publication hygiene is MERGED.

It preserves `cancel-in-progress: true`, prevents cancelled runs from publishing terminal commit statuses, preserves PR-head SHA targeting, keeps stable event-specific contexts and runs `scripts/check_ci_status_hygiene.sh` before compiler build setup.

## Mandatory rule for every remaining PR

Every PR from PR72 through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip or `continue-on-error` success.

The final head of every PR must have both `ci / ubuntu (push)` and `ci / ubuntu (pull_request)` successful before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` is the pipeline design contract. The implementation evolves in stages rather than replacing the current gate all at once.

Pipeline principles:

1. **Fast fail first:** CI policy, generated-file freshness, grammar and diagnostics run before expensive compilation.
2. **Failure isolation:** split the monolithic workflow into separately named policy, frontend, compiler, sanitizer, matrix, backend and release jobs as the roadmap reaches PR74.
3. **Stable aggregate gate:** event-specific merge status remains stable even when internal jobs are added or renamed.
4. **Deterministic inputs:** pin runner/toolchain expectations, record versions and use deterministic seeds for replayable tests.
5. **Bounded execution:** parser, graph, fuzz, network and runtime tests receive explicit timeouts.
6. **Honest hardware evidence:** CPU/GPU/TPU/NPU detection never counts as execution proof. Backends report verified execution, explicit unavailability or failure.
7. **Safe caching:** cache keys include OS, architecture, compiler, LLVM, dependency fingerprints and source inputs; release qualification contains a clean no-cache build.
8. **Structured failure artifacts:** every mandatory job uploads logs/metadata on failure.
9. **PR/nightly/RC profiles:** PRs run bounded mandatory correctness; nightly runs fuzz/scale/concurrency/platform/backend expansion; release candidates require zero mandatory skips.
10. **No flaky-green policy:** infrastructure acquisition may use bounded retry; deterministic compiler/test failures are not retried merely to obtain a green run.

## Remaining implementation strategy

| Planned PR | Status | Implementation scope | Pipeline/CI implementation | Mandatory tests and exit evidence |
| --- | --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Versioned test strategy, 27-area matrix, PR template and CI audit. | Established coverage governance. | Matrix schema, unique IDs, count and claim-safety gates. |
| PR69 - Module, import and package syntax with AST scaffold | MERGED | Beta-0.3 preamble grammar, AST provenance, stable diagnostics and inspection mode. | Added module syntax gate. | Positive, negative, compatibility, stress and sanitizer tests. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | IN PROGRESS | Hermetic manifest resolution, lockfiles, graph ordering, cycles, visibility and multi-file LLVM/native codegen. | Resolver gets its own CI gate; PR71 status hygiene is ported into the branch. | Deterministic lock/graph, native binding, missing import, cycle, ambiguity, identity mismatch, path escape, collision, stress, Makefile, sanitizer and CTest evidence. |
| PR71 - CI status publication hygiene | MERGED | Cancellation-safe SHA-scoped status handling and policy guard. | Prevents cancelled branch runs from publishing false failure on shared SHAs. | Push/PR final-head checks and post-merge master check. |
| PR72 - Cross-mode semantic correctness and differential execution suite | PLANNED | Implement correct local/imported function execution and normalized observable semantics across interpreter, LLVM bitcode and native binaries. | Add a separate `semantic-differential` job with normalized output/exit comparison and machine-readable mismatch artifacts. | Core scalar/types, expressions, calls, returns, branches, loops, arrays, imported calls, error equivalence and undefined-behavior rejection. |
| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | PLANNED | Scanner/parser/semantic/module/lowering fuzz targets; ASan, LSan, UBSan and TSan coverage. | Split `safety` job from functional CI; PR fuzz smoke plus scheduled extended fuzz workflow with recorded seeds. | Corpus sanitizers, concurrent reset/registration/inference, regression fixture per finding, no leaks/races/UB. |
| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | PLANNED | GCC/Clang, supported LLVM versions, Linux x86-64/arm64, macOS Apple Silicon, Windows, installed consumers and deterministic artifacts. | Replace the giant Ubuntu job with a dependency DAG: policy -> frontend -> compiler/safety -> matrix -> aggregate gate. Add clean no-cache reproducibility job and safe deterministic caching. | Platform conformance, CTest parity, generated-parser consistency, ABI consumer tests, independent clean-build checksums and diagnostic artifact upload. |
| PR75 - Signed release and protected publication workflow | PLANNED | Protected environments, version/tag policy, OIDC signing, checksums, SBOM, provenance and attestations. | Release workflow depends on all mandatory qualification jobs and never runs from an unqualified commit. | Reject unsigned/mismatched/modified artifacts, verify signatures, dry-run publication and rollback. |
| PR76 - External vulnerability, SAST, dependency and license policy gate | PLANNED | C++ SAST, dependency/image CVE scanning, license policy, pinned actions and expiring exceptions. | Dedicated security job and scheduled database refresh; findings exported as structured artifacts. | Vulnerable dependency, secret and prohibited-license fixtures, SBOM reconciliation and expired-exception rejection. |
| PR77 - Container and Kubernetes production hardening | PLANNED | Multi-arch non-root images, read-only filesystem, dropped capabilities, probes, limits and network policy. | Ephemeral-cluster integration job after compiler/release artifacts are built. | Image scan, non-root/read-only enforcement, deployment, health, shutdown/restart, resource-limit and network-negative tests. |
| PR78 - Formatter and linter baseline | PLANNED | Deterministic formatter, semantic-preserving lint rules, machine-readable diagnostics and safe fixes. | Fast frontend job runs format/lint regression without requiring full backend SDKs. | Idempotence, format-parse roundtrip, comment/string preservation, malformed input bounds and fix safety. |
| PR79 - Syntax highlighting and LSP implementation | PLANNED | Editor grammar plus compiler-backed diagnostics, hover, completion, definition and module navigation. | Protocol/golden job with cancellation and latency budget. | Golden tokens/protocol, partial documents, stale versions, cancellation, imported definition navigation and response-time limits. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | PLANNED | Declare production-supported backends; inventory CPU/GPU/TPU/NPU; choose execution target using hardware + SDK/runtime + model/precision/memory/policy readiness; verify real execution. | Capability-aware backend matrix jobs. CPU is mandatory. Claimed accelerator tiers require controlled runners; unavailable hardware is explicit and cannot satisfy the RC gate. | Numerical outputs, route selection, CPU fallback, unavailable SDK/device, incompatible format, device loss, OOM, precision mismatch, topology mismatch and no-false-success tests. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax, AST, semantics and evidence declarations without granting certification. | Add grammar/semantic C3-ECO job to frontend suite. | Grammar/AST matrix, valid examples, missing functional unit, invalid boundary, unsupported claim, unit and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Measured energy/carbon/cost calculations with factor provenance, uncertainty, quality and baselines. | Add deterministic eco-regression job; measurement quality failures fail closed. | Independent equation checks, deterministic reports, thresholds, stale factor, invalid unit and missing-sensor negatives. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | Bundle verification becomes a release-candidate dependency. | Tamper, missing lineage, invalid signature, schema mismatch, redaction leakage and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | Dedicated MLIR build/lit job with generated-file freshness guard. | MLIR lit/FileCheck, verifier cases, parse-print-parse, installed consumer and generated-source freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, core LLVM lowering and AI/Green AI runtime handoff. | Differential MLIR lowering job tied to semantic equivalence corpus. | Pass tests, invalid operation/shape cases, differential execution, runtime bridge and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, compile/runtime latency, throughput, memory, energy, metadata, uncertainty and blocker aggregation. | Final release-candidate aggregate requires every mandatory platform/backend/security/deployment/C3-ECO/MLIR/performance job with zero mandatory skips. | Harness calibration, identical inputs/models/outputs, repeated measurements, noise-aware thresholds, sensor negatives, regression budgets and clean RC matrix. |

## Implementation details for the next five PRs

### PR72 - semantic differential execution

Implementation:

- remove the current imported-call interpreter rejection by resolving imported function bodies through the deterministic module graph,
- define normalized observable program result: stdout, stderr category, exit status and selected runtime/evidence output,
- execute the same fixture through interpreter, LLVM/bitcode and native paths,
- compare normalized results and stable diagnostics,
- keep undefined or unsupported behavior as explicit rejection rather than silently choosing one backend result.

Tests:

- local and imported functions,
- nested calls and return propagation,
- branching and loops,
- scalar and supported aggregate values,
- failures with stable source-aware diagnostics,
- deterministic repeated execution.

### PR73 - fuzz/sanitizer/race hardening

Implementation:

- libFuzzer-compatible scanner/parser/module/semantic/lowering targets,
- seed corpora from conformance and negative fixtures,
- ASan/LSan/UBSan mandatory PR smoke,
- TSan concurrency targets where toolchain supports it,
- scheduled extended fuzz runs that save crash seed and minimized reproducer.

Tests fail on any leak, UB, race, timeout or unbounded growth.

### PR74 - robust multi-job CI and portability

Implementation:

- `policy` job: CI/status/roadmap/claim guards,
- `frontend` job: grammar, AST, diagnostics, module graph, negative corpus,
- `compiler` job: Makefile/CMake/CTest/differential execution,
- `safety` job: sanitizers and fuzz smoke,
- matrix jobs for compiler/LLVM/platform combinations,
- `aggregate` job that depends on every mandatory job and publishes the stable event-specific status,
- cache only reproducible dependencies/intermediates with explicit versioned keys,
- one clean no-cache reproducibility comparison,
- always-uploaded per-job diagnostics.

### PR75 - signed release pipeline

Implementation:

- immutable release version/tag validation,
- protected release environment,
- SBOM/provenance/checksum generation,
- OIDC-backed artifact signing,
- signature verification before publication,
- rollback/revocation procedure and dry-run workflow.

### PR76 - security gate

Implementation:

- C++ SAST,
- dependency/SBOM CVE reconciliation,
- GitHub Action pinning policy,
- license allow/deny rules,
- expiring documented exceptions,
- secret scanning fixtures and release blocking on policy violations.

## Hardware-selection production contract for PR80

Automatic hardware selection must be evidence-driven:

1. discover CPU/GPU/TPU/NPU inventory,
2. discover installed execution SDKs/runtimes,
3. inspect model format, operators, precision, shape and memory requirements,
4. calculate execution-ready candidates,
5. rank candidates using explicit policy including correctness first, then latency/energy/cost goals,
6. execute a backend capability probe where required,
7. choose the highest-ranked verified candidate,
8. fall back to CPU only when policy permits and CPU execution is compatible,
9. emit structured reason codes for selection, fallback or failure.

A detected GPU/TPU/NPU with no compatible runtime is not an executable candidate. The system must never label a fallback as accelerator execution.

## Production readiness exit criteria

The final release gate must pass language compatibility, module resolution, semantic equivalence, fuzzing and sanitizers, live backends, CPU/GPU/TPU/NPU routing, ABI, concurrency, portability, reproducibility, signing, security, deployment, tooling, C3-ECO, MLIR, performance and measured energy checks with zero mandatory skips.

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
remaining_planned_prs_after_pr70: 15
remaining_planned_implementation_prs_pr72_through_pr86: 15

## Historical compatibility record

The following markers are retained so old-task stability guards and repository history remain auditable. They do not override the active PR72-PR86 plan above.

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
- production_readiness_plan_version: 2026-08-06-pr69
- LAST_COMPLETED_PR: 69
- production_readiness_plan_version: 2026-08-09-pr70

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

Historical superseded label: PR67 - Parser robustness and negative corpus hardening

Historical superseded label: PR79 - MLIR lowering passes and production RC gate

The historical PR67 recommendation is superseded by the test re-audit.

The old unmerged roadmap label `PR71 - Cross-mode semantic correctness and differential execution suite` is superseded by PR72 because PR71 was consumed by the merged CI-hygiene correction.

## Recommended next PR

Next recommended PR after PR #70:

PR72 - Cross-mode semantic correctness and differential execution suite.

Reason: deterministic source identity, package graphs and multi-file native linking exist after PR70. The highest-risk remaining language gap is proving equivalent observable semantics across interpreter and compiled execution, especially imported function calls.
