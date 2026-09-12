# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-09-12-pr94
PLAN_STATUS: active
LAST_MERGED_GITHUB_PR: 93
CURRENT_GITHUB_PR: 94
CURRENT_ROADMAP_PR: 93
LAST_PLANNED_GITHUB_PR: unassigned
LAST_PLANNED_ROADMAP_PR: 102
CURRENT_IMPLEMENTATION_SCOPE: semantic_ir_mlir_llvm_lowering
BASELINE_LANGUAGE_VERSION: beta-0.7
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report production success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime, unavailable compiler oracle or unavailable deployment cluster is not production execution evidence.

## Current baseline

GitHub PR89 through PR93 are merged. GitHub PR92 delivered the generated dialect; GitHub PR93 separately added the enterprise AI/C3-ECO gap assessment. GitHub PR94 implements the original roadmap PR93 lowering scope. Subsequent PR labels below are stable roadmap IDs, not reserved GitHub numbers. PR91 added signed candidate auditor lineage, assessment replay, lifecycle verification and redacted reports. PR89 established instrument-backed energy measurement, allocation, PUE, carbon accounting, uncertainty and tariff provenance on top of the PR88 typed C3-ECO profile. PR90 added deterministic eligibility, scoring, claims and eco-regression assessment. It emits candidate recommendations only and does not perform independent certification or comparative ShortHand-versus-Python energy qualification.

ShortHand remains `controlled_beta` with `production_claim: false`. The declared production backend scope remains `linux-x64-cpu-v1`. GPU, TPU and NPU are inventory-only until separately live-qualified. TST017 remains partial until the protected `production-release` environment executes and verifies a real version-tag attestation.

The active machine-readable state is `docs/production_truth.tsv`; certification traceability is `docs/c3eco_traceability.tsv`. The original roadmap retains PR83 through PR96. The merged PR93 assessment adds six follow-on increments, now tracked as roadmap PR97 through PR102. Ten implementation increments remain including this candidate; actual future GitHub numbers are unassigned. This is a planning count, not a guarantee of general release readiness.

## Current completion contract

PR93 - SemanticIR to MLIR and LLVM lowering is IN PROGRESS as GitHub PR94.

1. Own source semantics and locations in public SemanticIR after module and semantic validation.
2. Independently verify the public SDK input, then lower scalar/control/function/array and bounded composite values into MLIR and LLVM.
3. Execute record, enum, option/result and borrowed-slice values through explicit `shorthand.enterprise_language.v2`; preserve legacy schema and runtime ABI contracts.
4. Check arithmetic, bounds, tags, missing returns, runtime statuses and output sizes before use.
5. Preserve inference, measurement and evidence metadata before and after optimization; declarations remain candidate evidence.
6. Require relocated SDK consumers, invalid-IR and runtime-ABI attacks, source/module differential execution and real ONNX CPU output at O0/O2.
7. Require GCC and Clang ASan/LSan/UBSan, CodeQL, Make/CTest parity and Linux x64 LLVM18 release staging, with no mandatory skips.
8. Update production truth and PR93 audit reconciliation. TST024 closes only within this documented scope; broader platform, workload and measurement claims stay open.
9. Require both ci / ubuntu (push) and ci / ubuntu (pull_request) green on the final head.

## Mandatory rule for every remaining PR

Every implementation PR through roadmap PR102 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability, performance and energy tests. It must update production truth, traceability, feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip, warning-only success or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification, PR79 formatter/linter qualification, PR80 LSP/editor qualification, PR81 live versioned backend qualification and PR82 first-class C3-ECO language plus zero-skip qualification. PR83 adds production truth and certification traceability. PR84-PR89 add the production type/memory model, control flow, enterprise packages/FFI, serving runtime, typed certification profile and instrumented measurement. Release publication remains separated from PR CI so OIDC and repository-write privileges are not granted to pull-request code.

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
| PR89 - Measurement, carbon accounting and cost workbook | MERGED | Instrument-backed measurement, allocation, PUE, component accounting, MQ/DQ, uncertainty and tariff/carbon-factor provenance. | Deterministic fail-closed measurement/workbook gate. | Calibration, missing/modelled instrument, bounds, factor provenance, double-counting, deterministic reconciliation and claim-safety tests. |
| PR90 - Eligibility, scoring, claims and eco-regression | MERGED | G1-G14 precedence, complete 76-criterion A-K scoring, evidence/uncertainty caps, N/A/materiality decisions, AI routes, controlled claims, regression actions and surveillance thresholds. | Deterministic assessment and claim-control gate across CI, Make, CTest, sanitizers and installed packaging. | Exact tier boundaries, gate precedence, evidence caps, N/A reallocation, materiality, restricted claims, regression, AI, malformed-input and determinism tests. |
| PR91 - Auditor bundle, retention, surveillance and reporting | MERGED | Native Ed25519 signed lineage, strict schemas, redacted public envelopes, assessment replay, retention/expiry/surveillance/nonconformity checks and separate estimated readiness. | Mandatory auditor verification in direct CI, Make, CTest, sanitizers, compiler matrix and installed SDK lifecycle. | Tamper, signature, schema, lineage, replay, expiry and redaction tests. |
| PR92 - Generated ShortHand MLIR dialect | MERGED | TableGen-generated operations, types, attributes, verifiers, installation and downstream use. | MLIR build/lit gate. | FileCheck, verifiers, roundtrip, installed consumer and freshness tests. |
| PR93 - SemanticIR to MLIR and LLVM lowering | IN PROGRESS as GitHub PR94 | Verified Linux x64 LLVM18 source/SDK lowering, bounded composite execution and checked runtime handoff. | MLIR differential lowering gate. | Invalid ops/shapes, execution equivalence and optimization preservation. |
| PR94 - Representative production AI qualification | PLANNED | Complete representative preprocessing/inference/postprocessing/serving applications, secure host boundary, realistic models/tensor shapes, batching, concurrency, timeouts and numerical quality. | Live workload/backend qualification. | Numerical, load, malformed-model, recovery and evidence tests. |
| PR95 - Performance and measured-energy qualification | PLANNED | Calibrated workload-coupled collectors, compiler/runtime performance, repeated equivalent-quality optimized Python and native C++ baselines, material data/cloud/client/embodied/lifecycle carbon coverage and uncertainty. | Performance and eco-regression qualification. | Calibration, repeated trials, uncertainty, quality equivalence and raw traces. |
| PR96 - Enterprise pilot and production RC | PLANNED | Clean install/upgrade/rollback, deployment, security, evidence and final blocker aggregation. | Final zero-skip RC aggregate. | Pilot, soak, disaster recovery, retained evidence and release decision. |

## Why this remaining order is dependency-correct

1. PR89 created trustworthy measured quantities before any score consumes them.
2. PR90 calculates candidate eligibility, scores and controlled recommendation text from typed profiles plus measured evidence.
3. PR91 packages those decisions into signed, retained and replayable auditor evidence.
4. PR92 defines the generated MLIR dialect before PR93 attempts production lowering into it.
5. PR93 closes the compiler execution path needed for representative production AI workloads.
6. PR94 qualifies realistic AI behavior before performance comparisons are treated as production evidence.
7. PR95 measures performance and equivalent-workload energy only after semantics, workloads and measurement contracts are stable.
8. PR96 aggregates a scoped enterprise pilot/RC. The audit-derived PR97-PR102 increments separately qualify broader measurements, workloads, lifecycle coverage, independent reproduction and general-release claims.

## PR93 audit reconciliation and additional increments

The merged [gap assessment](ENTERPRISE_AI_C3ECO_GAP_ASSESSMENT.md) is historical evidence at its stated base SHA. It is not overwritten by implementation progress. SH-EA-001 and the execution portion of SH-EA-016 are addressed in GitHub PR94 within [the lowering contract](mlir_lowering.md); nested ownership/FFI remain explicit limits. SH-EA-008/022 are addressed by this numbering and truth reconciliation. Realistic applications, measured carbon superiority and external certification remain open.

PR89-PR91 already implement accounting, assessment and signed auditor preparation. Follow-on work must consume these contracts and qualify actual workloads and operating evidence, avoiding duplicate implementations of existing validators.

| Roadmap increment | Status | Additional release evidence |
| --- | --- | --- |
| PR97 - Measurement-grade harness | PLANNED | Calibrated collectors attached to workload phases, raw traces, repeated trials, MQ/DQ and uncertainty; extend PR95 collectors and PR89 workbooks. |
| PR98 - Realistic benchmark families | PLANNED | Classification, retrieval, preprocessing, quantized/batched inference and concurrent serving with equivalent optimized baselines, numerical quality and energy. |
| PR99 - Accelerator execution or explicit CPU scope | PLANNED | Device-backed execution and measured routing, or a versioned CPU-only GA scope with every accelerator claim excluded. |
| PR100 - Data lifecycle and cloud carbon boundary | PLANNED | Material data/storage/network, AI lifecycle, shared-cloud allocation, factors and hardware lifetime evidence using existing accounting controls. |
| PR101 - Independent reproduction and certification pilot | PLANNED | Independent repeated measurements and organizational evidence-retention, surveillance and draft-standard pilot review. |
| PR102 - Claims and general-release closeout | PLANNED | Review every audit gap, bounded claims, quality/safety controls, operational evidence and final GA decision. |

## Current count

remaining_planned_implementation_increments_including_current: 10
remaining_planned_implementation_increments_after_current: 9
audit_follow_on_increments: 6

Four original roadmap increments remain including the current lowering candidate (PR93-PR96), plus six audit-derived increments (PR97-PR102). Reconcile overlap at each PR; combine only when the combined exit evidence is complete. This count excludes repository administration and independent certification decisions.

Next recommended implementation after GitHub PR94 merges: roadmap PR94, representative production AI qualification (expected GitHub PR95 if no intervening PR is created).

## External production blocker not counted as an implementation PR

TST017 remains partial until repository administration configures the `production-release` protected environment and a real version tag executes the signed publication workflow with attestations that verify cryptographically. The workflow implementation is already merged; this operational exercise is not counted as one of the ten remaining implementation increments.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-09-09-pr91
- LAST_MERGED_GITHUB_PR: 90
- CURRENT_GITHUB_PR: 91
- CURRENT_IMPLEMENTATION_SCOPE: auditor_bundle_retention_surveillance_reporting
- remaining_planned_implementation_prs_pr91_through_pr96: 6
- remaining_planned_implementation_prs_after_pr91: 5
- PR91 - Auditor bundle, retention, surveillance and reporting is IN PROGRESS.

- production_readiness_plan_version: 2026-09-08-pr90
- LAST_MERGED_GITHUB_PR: 89
- CURRENT_GITHUB_PR: 90
- CURRENT_IMPLEMENTATION_SCOPE: eligibility_scoring_claims_eco_regression
- remaining_planned_implementation_prs_pr90_through_pr96: 7
- remaining_planned_implementation_prs_after_pr90: 6
- PR90 - Eligibility, scoring, claims and eco-regression is IN PROGRESS.

- production_readiness_plan_version: 2026-09-02-pr89
- LAST_MERGED_GITHUB_PR: 88
- CURRENT_GITHUB_PR: 89
- CURRENT_IMPLEMENTATION_SCOPE: measurement_carbon_accounting_cost_workbook
- remaining_planned_implementation_prs_pr89_through_pr96: 8
- remaining_planned_implementation_prs_after_pr89: 7
- PR89 - Measurement, carbon accounting and cost workbook is IN PROGRESS.
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
