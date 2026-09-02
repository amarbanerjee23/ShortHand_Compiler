# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-09-02-pr89
PLAN_STATUS: active
LAST_MERGED_GITHUB_PR: 88
CURRENT_GITHUB_PR: 89
LAST_PLANNED_GITHUB_PR: 96
CURRENT_IMPLEMENTATION_SCOPE: measurement_carbon_accounting_cost_workbook
BASELINE_LANGUAGE_VERSION: beta-0.7
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report production success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime, unavailable compiler oracle or unavailable deployment cluster is not production execution evidence.

## Current baseline

GitHub PR88 is merged. It established the beta-0.7 typed C3-ECO certification-preparation profile while preserving the beta-0.6 enterprise language/package/FFI and PR87 bounded serving runtime. PR89 adds instrument-backed energy measurement, allocation, PUE, carbon accounting, uncertainty and tariff provenance. It does not perform certification scoring or comparative ShortHand-versus-Python energy qualification.

ShortHand remains `controlled_beta` with `production_claim: false`. The declared production backend scope remains `linux-x64-cpu-v1`. GPU, TPU and NPU are inventory-only until separately live-qualified. TST017 remains partial until the protected `production-release` environment executes and verifies a real version-tag attestation.

The active machine-readable state is `docs/production_truth.tsv`; certification traceability is `docs/c3eco_traceability.tsv`. The corrected enterprise plan remains GitHub-native PR83 through PR96 and now has eight implementation PRs remaining including PR89.

## PR89 completion contract

PR89 - Measurement, carbon accounting and cost workbook is IN PROGRESS.

Implementation requirements:

1. Establish `shorthand.c3eco.measurement_workbook.v1` as an immutable measured-accounting evidence contract.
2. Accept only instrument-backed measurement sources: physical meter, RAPL, accelerator counter or cloud meter.
3. Reject modelled, declared-budget-only and unavailable telemetry from the measured evidence path.
4. Require instrument identity, calibration identity/date, measurement timestamp and raw energy in Joules.
5. Require bounded allocation fractions and reject cumulative shared-reading allocation above 1.0 to prevent double counting.
6. Apply bounded PUE to allocated IT energy and retain both IT and facility-energy quantities.
7. Require carbon-factor value, source and date, and reject factors that post-date the measurement.
8. Require tariff value, three-letter currency and tariff provenance without conflating currencies.
9. Carry explicit MQ, DQ and bounded uncertainty through record and aggregate outputs.
10. Emit deterministic CSV and JSON workbooks independent of input row ordering.
11. Reconcile Joules, kWh, kgCO2e and cost using explicit equations with deterministic totals.
12. Preserve offsets and avoided impact outside the base-footprint reduction path.
13. Preserve `official_certification_granted:false`; measured accounting evidence does not grant certification.
14. Keep scoring, levels and permitted claims assigned to PR90.
15. Keep repeated equivalent-workload ShortHand-versus-Python energy comparison assigned to PR95.
16. Add schema, documentation, positive, negative, determinism, provenance and double-count tests.
17. Register the measurement gate in direct CI/governance, Make and CTest/build parity without weakening inherited gates.
18. Preserve all compiler, sanitizer/race, portability, security, deployment, tooling, backend, package, serving and C3-ECO qualification gates.
19. Final PR head must have `ci / ubuntu (push)` successful.
20. Final PR head must have `ci / ubuntu (pull_request)` successful.

## Mandatory rule for every remaining PR

Every PR through PR96 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability, performance and energy tests. It must update production truth, traceability, feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip, warning-only success or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification, PR79 formatter/linter qualification, PR80 LSP/editor qualification, PR81 live versioned backend qualification and PR82 first-class C3-ECO language plus zero-skip qualification. PR83 adds production truth and certification traceability. PR84-PR88 add the production type/memory model, control flow, enterprise packages/FFI, serving runtime and typed certification profile. Release publication remains separated from PR CI so OIDC and repository-write privileges are not granted to pull-request code.

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
| PR86 - Enterprise packages, standard library and FFI | MERGED | Versioned composite/ownership ABI-schema prerequisite, cryptographic offline dependencies, namespaces, core libraries and safe C/C++ interop. Composite execution remains assigned to production lowering. | Language-surface, package supply-chain and installed-consumer gates. | Type/lifetime, tamper, reproducibility, ABI, license, SBOM and portability tests. |
| PR87 - Concurrent serving and operational runtime | MERGED | Versioned process-scoped cancellation, deadlines, backpressure, bounded concurrency, health, metrics, quotas and isolation. | Runtime load/fault/soak qualification plus installed worker lifecycle and Kubernetes drain probes. | TSan, sanitizer, saturation, timeout, restart, graceful-shutdown and Kubernetes tests. |
| PR88 - Typed C3-ECO certification profile | MERGED | Typed identities, units, functional links, boundary/materiality, AI roles, validity and migration. | Certification-profile conformance gate. | G1-G3/G7/G14 positive, negative, migration and claim-safety evidence. |
| PR89 - Measurement, carbon accounting and cost workbook | IN PROGRESS | Instrument-backed measurement, allocation, PUE, component accounting, MQ/DQ, uncertainty and tariff/carbon-factor provenance. | Deterministic fail-closed measurement/workbook gate. | Calibration, missing/modelled instrument, bounds, factor provenance, double-counting, deterministic reconciliation and claim-safety tests. |
| PR90 - Eligibility, scoring, claims and eco-regression | PLANNED | G1-G14 algorithm, A-K scoring, level caps, materiality decisions, AI routes, permitted claims and surveillance thresholds. | Golden scoring and claim-control gate. | Tier boundaries, critical-gate precedence, restricted claims and regression tests. |
| PR91 - Auditor bundle, retention, surveillance and reporting | PLANNED | Signed lineage, canonical schemas, redaction, replay, retention, expiry, recertification and public reports. | Auditor-bundle verification dependency. | Tamper, signature, schema, lineage, replay, expiry and redaction tests. |
| PR92 - Generated ShortHand MLIR dialect | PLANNED | TableGen-generated operations, types, attributes, verifiers, installation and downstream use. | MLIR build/lit gate. | FileCheck, verifiers, roundtrip, installed consumer and freshness tests. |
| PR93 - SemanticIR to MLIR and LLVM lowering | PLANNED | Full semantic lowering, canonicalization, verification, composite execution integration and runtime handoff. | MLIR differential lowering gate. | Invalid ops/shapes, execution equivalence and optimization preservation. |
| PR94 - Representative production AI qualification | PLANNED | Realistic models, tensor types/shapes, batching, concurrency, timeouts and numerical tolerances. | Live workload/backend qualification. | Numerical, load, malformed-model, recovery and evidence tests. |
| PR95 - Performance and measured-energy qualification | PLANNED | Compiler/runtime performance plus repeated equivalent-workload ShortHand/Python energy evidence, data/cloud component coverage and uncertainty. | Performance and eco-regression qualification. | Calibration, repeated trials, uncertainty, quality equivalence and raw traces. |
| PR96 - Enterprise pilot and production RC | PLANNED | Clean install/upgrade/rollback, deployment, security, evidence and final blocker aggregation. | Final zero-skip RC aggregate. | Pilot, soak, disaster recovery, retained evidence and release decision. |

## Why this remaining order is dependency-correct

1. PR89 creates trustworthy measured quantities before any score can consume them.
2. PR90 can then calculate eligibility, scores and permitted claims from typed profiles plus measured evidence.
3. PR91 packages those decisions into signed, retained and replayable auditor evidence.
4. PR92 defines the generated MLIR dialect before PR93 attempts production lowering into it.
5. PR93 closes the compiler execution path needed for representative production AI workloads.
6. PR94 qualifies realistic AI behavior before performance comparisons are treated as production evidence.
7. PR95 measures performance and equivalent-workload energy only after semantics, workloads and measurement contracts are stable.
8. PR96 is the final enterprise pilot/RC aggregation and cannot honestly precede any of the above.

## Current count

remaining_planned_implementation_prs_pr89_through_pr96: 8
remaining_planned_implementation_prs_after_pr89: 7

Next recommended PR after PR89 is merged:

PR90 - Eligibility, scoring, claims and eco-regression.

## External production blocker not counted as an implementation PR

TST017 remains partial until repository administration configures the `production-release` protected environment and a real version tag executes the signed publication workflow with attestations that verify cryptographically. The workflow implementation is already merged; this operational exercise is not counted as one of the eight remaining implementation PRs.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-09-01-pr88
- LAST_MERGED_GITHUB_PR: 87
- CURRENT_GITHUB_PR: 88
- remaining_planned_implementation_prs_pr88_through_pr96: 9
- remaining_planned_implementation_prs_after_pr88: 8
- PR88 - Typed C3-ECO certification profile is IN PROGRESS.
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
