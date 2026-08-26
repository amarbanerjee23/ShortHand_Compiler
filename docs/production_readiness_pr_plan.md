# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-23-pr87
PLAN_STATUS: active
LAST_MERGED_GITHUB_PR: 86
CURRENT_GITHUB_PR: 87
LAST_PLANNED_GITHUB_PR: 96
CURRENT_IMPLEMENTATION_SCOPE: concurrent_serving_and_operational_runtime
BASELINE_LANGUAGE_VERSION: beta-0.6
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report production success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime, unavailable compiler oracle or unavailable deployment cluster is not production execution evidence.

## Current baseline

GitHub PR86 is merged. It established beta-0.6 enterprise schemas, cryptographically verified offline packages, an installable core library and the safe core FFI while preserving all earlier production-truth and C3-ECO traceability controls. The active machine-readable state is `docs/production_truth.tsv`; certification traceability is `docs/c3eco_traceability.tsv`.

ShortHand remains `controlled_beta` with `production_claim: false`. The declared production backend scope remains `linux-x64-cpu-v1`. GPU, TPU and NPU are inventory-only until separately live-qualified. TST017 remains partial until the protected `production-release` environment executes and verifies a real version-tag attestation.

The post-PR82 audit found that the previous five-PR plan did not include several enterprise language and operating requirements. The corrected plan is GitHub-native PR83 through PR96. PR86 is merged and PR87 is the active implementation. The plan covers production truth, core language completion, packages and standard library, serving operations, complete C3-ECO preparation, MLIR, representative AI execution, measured energy and the final release candidate.

## PR87 completion contract

PR87 - Concurrent serving and operational runtime is IN PROGRESS.

Implementation requirements:

1. Establish `shorthand.serving.runtime.v1` as an immutable process-scoped serving contract.
2. Bound worker concurrency, admission queue depth, in-flight work, retained results, request bytes, response bytes and deadline duration.
3. Reject overload immediately and deterministically without unbounded queues or blocking admission callers.
4. Enforce deadlines before admission, while queued and after handler return so late successes cannot be reported as successful.
5. Support cooperative cancellation for queued and running work with stable terminal states.
6. Isolate every runtime instance to one configured tenant scope and reject cross-tenant submission and lookup.
7. Expose versioned liveness/readiness JSON and low-cardinality Prometheus metrics without request or tenant labels.
8. Implement drain and bounded graceful shutdown semantics, including queued-work termination and running-work cancellation signals.
9. Install `ShortHand::serving`, its public header and `shorthand_serving_worker`; exercise the installed target and worker in existing platform lifecycle lanes.
10. Run deterministic unit, saturation, deadline, quota, isolation, restart, shutdown, load and bounded-soak tests.
11. Execute the concurrent stress path under ASan/LSan/UBSan and TSan.
12. Replace the container's idle PID 1 with the serving worker and connect Kubernetes startup, liveness, readiness and pre-stop drain to its bounded state contract.
13. Keep public ingress, authentication, authorization and TLS outside this process-scoped contract and fail closed on any broader production claim.
14. Add the serving gate to direct CI, Make governance/aggregate parity, CTest, packaging and production truth.
15. Preserve all existing compiler, sanitizer/race, portability, security, deployment, tooling, backend, enterprise-language and C3-ECO gates.
16. Keep `controlled_beta` and `production_claim: false`; this PR does not claim public service readiness, energy savings or certification.
17. Final PR head must have `ci / ubuntu (push)` successful.
18. Final PR head must have `ci / ubuntu (pull_request)` successful.

## Mandatory rule for every remaining PR

Every PR through PR96 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability, performance and energy tests. It must update the production truth, traceability, feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip, warning-only success or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification, PR79 formatter/linter qualification, PR80 LSP/editor qualification, PR81 live versioned backend qualification and PR82 first-class C3-ECO language plus zero-skip qualification. PR83 adds the production-truth and certification-traceability tier. Release publication remains separated from PR CI so OIDC and repository-write privileges are not granted to pull-request code.

## Remaining implementation strategy

| Planned PR | Status | Implementation scope | Pipeline/CI implementation | Mandatory tests and exit evidence |
| --- | --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Versioned test strategy, 27-area matrix and PR contract. | Coverage governance. | Schema, IDs, counts and claim-safety gates. |
| PR69 - Module, import and package syntax with AST scaffold | MERGED | Beta-0.3 preamble grammar and AST provenance. | Module syntax gate. | Positive, negative, compatibility, stress and sanitizer tests. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED | Hermetic manifest resolution, lockfiles, graph ordering, visibility and multi-file codegen. | Resolver first-class gate. | Determinism, native binding, negative graph cases, stress, sanitizer and CTest evidence. |
| PR71 - CI status publication hygiene | MERGED | Cancellation-safe SHA-scoped status handling. | Stable event-specific status contexts. | Push/PR status hygiene and cancellation policy guard. |
| PR72 - Cross-mode semantic correctness and differential execution suite | MERGED | Reference executable semantics and mode parity. | Mandatory semantic differential gate. | Interpreter/lli/native positive and negative parity. |
| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | MERGED | Scanner/parser/module/semantic/lowering fuzzing, ASan/LSan/UBSan and TSan. | First-class safety steps plus scheduled fuzz. | Replayable corpus and no sanitizer/race findings. |
| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | MERGED as GitHub PR75 | GCC12/14, Clang16/18, Linux x64/arm64, macOS arm64, Windows x64, installed consumers and deterministic artifacts. | Multi-job DAG and reproducibility/CTest parity jobs. | Native/platform execution, ABI consumers, SDK lifecycle and clean-build checksums. |
| PR75 - Signed release and protected publication workflow | MERGED as GitHub PR76 | Immutable release policy, SBOM/provenance, OIDC attestations, protected publication and rollback. | Separate tag/manual release workflow plus mandatory contract tests. | Real protected tag verification still closes TST017 externally. |
| PR76 - External vulnerability, SAST, dependency and license policy gate | MERGED as GitHub PR77 | CodeQL, Trivy, dependency delta review, license policy and immutable action pins. | Dedicated mandatory security job plus daily rescan. | Vulnerability, secret, prohibited-license and anti-weakening tests. |
| PR77 - Container and Kubernetes production hardening | MERGED as GitHub PR78 | Multi-stage native images, Restricted Pod Security, quota, PDB and default-deny network policy. | Native container execution plus ephemeral Kind integration. | Runtime security, quota/network negatives, restart and graceful shutdown. |
| PR78 - Formatter and linter baseline | MERGED as GitHub PR79 | Native deterministic formatter/linter and safe explicit-output fixes. | Fast tooling job plus inherited ubuntu-core execution. | Idempotence, parse roundtrip, behavior preservation and sanitizers. |
| PR79 - Syntax highlighting and LSP implementation | MERGED as GitHub PR80 | Scanner-aligned editor grammar plus native compiler-backed LSP. | Dedicated protocol job plus inherited ubuntu-core execution. | Framing, diagnostics, UTF-16, navigation, cancellation and sanitizers. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | MERGED as GitHub PR81 | Versioned production backend/device support with mandatory ONNX Runtime CPU live numerical evidence and fail-closed accelerator boundaries. | Pinned qualification SDK plus mandatory inherited ubuntu-core live gate. | Output `42`, no fallback/skip, route rejection and structured support matrix. |
| Roadmap PR81 / GitHub PR82 - C3-ECO language blocks and zero-skip CI | MERGED | Ten first-class C3-ECO parser/AST/semantic/evidence declarations without granting certification. | C3-ECO language and zero-skip policy gates. | Positive/negative grammar, semantics, evidence, sanitizer and exact-head CI. |
| PR83 - Production truth baseline and C3-ECO traceability | MERGED | Machine-readable active state and G1-G14/A-K/S9/S12 evidence ownership. | First-class production-truth gate in CI, Make and CTest. | Schema, duplicate, missing-row, evidence-path and contradiction tests. |
| PR84 - Production type system and memory model | MERGED | Beta-0.4 executable floats, strings and typed arrays plus guarded slices, records, enums, option/result, conversions and ownership descriptors. | Expanded differential, strict unit, sanitizer, Make and CTest gates. | Cross-mode values, lifetime, overflow, bounds and unchanged ABI evidence. |
| PR85 - Functions, structured control flow and error semantics | MERGED | Beta-0.5 expression calls, arbitrary arguments, lexical scopes, loops, deterministic errors and safe `goto` resolution. | First-class control-flow conformance and cross-mode differential gate. | Positive, negative, recursion, cleanup and compatibility tests. |
| PR86 - Enterprise packages, standard library and FFI | MERGED | Complete the versioned composite/ownership ABI-schema prerequisite, then add cryptographic offline dependencies, namespaces, core libraries and safe C/C++ interop. Composite execution remains assigned to production lowering. | Language-surface, package supply-chain and installed-consumer gates. | Type/lifetime, tamper, reproducibility, ABI, license, SBOM and portability tests. |
| PR87 - Concurrent serving and operational runtime | IN PROGRESS | Versioned process-scoped cancellation, deadlines, backpressure, bounded concurrency, health, metrics, quotas and isolation. | Runtime load/fault/soak qualification plus installed worker lifecycle and Kubernetes drain probes. | TSan, sanitizer, saturation, timeout, restart, graceful-shutdown and Kubernetes tests. |
| PR88 - Typed C3-ECO certification profile | PLANNED | Typed identities, units, functional links, boundary/materiality, AI roles, validity and migration. | Certification-profile conformance gate. | G1-G3/G7/G14 positive, negative, migration and claim-safety evidence. |
| PR89 - Measurement, carbon accounting and cost workbook | PLANNED | Real instrumentation, allocation, PUE, component accounting, MQ/DQ, uncertainty and tariff provenance. | Deterministic measurement/workbook gate. | Calibration, missing-instrument, unit, factor, double-counting and reconciliation tests. |
| PR90 - Eligibility, scoring, claims and eco-regression | PLANNED | G1-G14 algorithm, A-K scoring, level caps, AI routes, permitted claims and surveillance thresholds. | Golden scoring and claim-control gate. | Tier boundaries, critical-gate precedence, restricted claims and regression tests. |
| PR91 - Auditor bundle, retention, surveillance and reporting | PLANNED | Signed lineage, canonical schemas, redaction, replay, retention, expiry, recertification and public reports. | Auditor-bundle verification dependency. | Tamper, signature, schema, lineage, replay, expiry and redaction tests. |
| PR92 - Generated ShortHand MLIR dialect | PLANNED | TableGen-generated operations, types, attributes, verifiers, installation and downstream use. | MLIR build/lit gate. | FileCheck, verifiers, roundtrip, installed consumer and freshness tests. |
| PR93 - SemanticIR to MLIR and LLVM lowering | PLANNED | Full semantic lowering, canonicalization, verification and runtime handoff. | MLIR differential lowering gate. | Invalid ops/shapes, execution equivalence and optimization preservation. |
| PR94 - Representative production AI qualification | PLANNED | Realistic models, tensor types/shapes, batching, concurrency, timeouts and numerical tolerances. | Live workload/backend qualification. | Numerical, load, malformed-model, recovery and evidence tests. |
| PR95 - Performance and measured-energy qualification | PLANNED | Compiler/runtime performance and equivalent-workload ShortHand/Python energy evidence. | Performance and eco-regression qualification. | Calibration, repeated trials, uncertainty, quality equivalence and raw traces. |
| PR96 - Enterprise pilot and production RC | PLANNED | Clean install/upgrade/rollback, deployment, security, evidence and final blocker aggregation. | Final zero-skip RC aggregate. | Pilot, soak, disaster recovery, retained evidence and release decision. |

## Current count

remaining_planned_implementation_prs_pr87_through_pr96: 10
remaining_planned_implementation_prs_after_pr87: 9

Next recommended PR after PR87 is merged:

PR88 - Typed C3-ECO certification profile.

## External production blocker not counted as an implementation PR

TST017 remains partial until repository administration configures the `production-release` protected environment and a real version tag executes the signed publication workflow with attestations that verify cryptographically. The workflow implementation is already merged; this operational exercise is not counted as one of the ten remaining implementation PRs.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-08-21-pr81
- LAST_COMPLETED_PR: 80
- MERGED_OUT_OF_BAND_PR: 71
- CURRENT_IMPLEMENTATION_PR: 81
- GITHUB_IMPLEMENTATION_PR: 82
- NEXT_IMPLEMENTATION_PR_AFTER_PR81: 82
- Roadmap PR81 - Complete C3-ECO language blocks is IN PROGRESS as GitHub PR82.
- remaining_planned_implementation_prs_pr81_through_pr86: 6
- remaining_planned_implementation_prs_after_pr81: 5

- production_readiness_plan_version: 2026-08-18-pr80
- CURRENT_IMPLEMENTATION_PR: 80
- GITHUB_IMPLEMENTATION_PR: 81
- NEXT_IMPLEMENTATION_PR_AFTER_PR80: 81
- remaining_planned_implementation_prs_pr80_through_pr86: 7
- remaining_planned_implementation_prs_after_pr80: 6
- | PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | IN PROGRESS as GitHub PR81
- | PR81 - Complete C3-ECO language blocks | PLANNED

- production_readiness_plan_version: 2026-08-18-pr79
- LAST_COMPLETED_PR: 78
- CURRENT_IMPLEMENTATION_PR: 79
- GITHUB_IMPLEMENTATION_PR: 80
- NEXT_IMPLEMENTATION_PR_AFTER_PR79: 80
- Roadmap PR79 - Syntax highlighting and LSP implementation is IN PROGRESS as GitHub PR80.
- remaining_planned_implementation_prs_pr79_through_pr86: 8
- remaining_planned_implementation_prs_after_pr79: 7
- | PR79 - Syntax highlighting and LSP implementation | IN PROGRESS as GitHub PR80
- production_readiness_plan_version: 2026-08-18-pr78
- CURRENT_IMPLEMENTATION_PR: 78
- NEXT_IMPLEMENTATION_PR_AFTER_PR78: 79
- remaining_planned_implementation_prs_pr78_through_pr86: 9
- | PR78 - Formatter and linter baseline | IN PROGRESS as GitHub PR79
- production_readiness_plan_version: 2026-08-12-pr77
- CURRENT_IMPLEMENTATION_PR: 77
- NEXT_IMPLEMENTATION_PR_AFTER_PR77: 78
- remaining_planned_implementation_prs_pr77_through_pr86: 10
- | PR77 - Container and Kubernetes production hardening | IN PROGRESS as GitHub PR78
- production_readiness_plan_version: 2026-08-12-pr76
- CURRENT_IMPLEMENTATION_PR: 76
- NEXT_IMPLEMENTATION_PR_AFTER_PR76: 77
- remaining_planned_implementation_prs_pr76_through_pr86: 11
- | PR76 - External vulnerability, SAST, dependency and license policy gate | IN PROGRESS as GitHub PR77
- production_readiness_plan_version: 2026-08-12-pr75
- CURRENT_IMPLEMENTATION_PR: 75
- NEXT_IMPLEMENTATION_PR_AFTER_PR75: 76
- remaining_planned_implementation_prs_pr75_through_pr86: 12
- | PR75 - Signed release and protected publication workflow | IN PROGRESS as GitHub PR76
- production_readiness_plan_version: 2026-08-11-pr72
- CURRENT_IMPLEMENTATION_PR: 72
- NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73
- remaining_planned_implementation_prs_pr73_through_pr86: 14
- | PR72 - Cross-mode semantic correctness and differential execution suite | IN PROGRESS
- production_readiness_plan_version: 2026-08-02-pr62
- Recommended path from PR #51 onward: 29 PRs total.
- PR79 - MLIR lowering passes and production RC gate
