# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-18-pr80
PLAN_STATUS: active
LAST_COMPLETED_PR: 79
MERGED_OUT_OF_BAND_PR: 71
CURRENT_IMPLEMENTATION_PR: 80
GITHUB_IMPLEMENTATION_PR: 81
NEXT_IMPLEMENTATION_PR_AFTER_PR80: 81
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report production success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime, unavailable compiler oracle or unavailable deployment cluster is not production execution evidence.

## Current baseline

Roadmap PR69 through PR79 are merged. PR71 was the out-of-band CI status-publication correction. Roadmap PR74 was implemented as GitHub PR75. Roadmap PR75 was implemented as GitHub PR76. Roadmap PR76 was implemented as GitHub PR77. Roadmap PR77 was implemented as GitHub PR78. Roadmap PR78 was implemented as GitHub PR79. Roadmap PR79 was implemented and merged as GitHub PR80 with scanner-aligned syntax highlighting and the native compiler-backed LSP baseline.

Roadmap PR80 is now the active implementation and is carried by GitHub PR81. It establishes a versioned production backend/device support boundary, makes ONNX Runtime CPU live numerical execution mandatory for the declared Linux x64 CPU production row, and prevents detected or SDK-visible GPU/TPU/NPU paths from becoming production-qualified without real device-backed numerical evidence.

ShortHand remains `controlled_beta` with `production_claim: false`. TST017 signed protected release remains partial until `production-release` is configured and a real version tag is successfully attested. TST022 closes for the versioned `linux-x64-cpu-v1` support contract only after the exact GitHub PR81 head passes mandatory live ONNX Runtime CPU numerical execution and all inherited CI. GPU, TPU and NPU production support is not claimed by this version.

## PR80 completion contract

Roadmap PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix is IN PROGRESS as GitHub PR81.

Implementation requirements:

1. Define a versioned backend/hardware production-support contract and explicit claim boundary.
2. Keep CPU/GPU/TPU/NPU inventory visible while separating detection from production qualification.
3. Allow production routing only for backend/device pairs in the qualified support matrix.
4. Make `onnxruntime_cpu` on CPU the v1 production-supported backend/device pair.
5. Acquire the CI qualification SDK at a fixed version and verify a pinned SHA-256 before execution.
6. Execute a real ONNX Runtime C++ inference path through the compiled ShortHand runtime bridge.
7. Validate numerical output using the identity ONNX fixture with input `42` and required output `42`.
8. Reject fallback, not-executed, missing-SDK or skip evidence for the production-supported CPU row.
9. Reject unqualified GPU/NPU/TPU production routes even when a hardware signal or compatible backend is visible.
10. Preserve an explicit experimental override that remains machine-marked `production_qualified:false`.
11. Keep TensorRT, ONNX Runtime CUDA/TensorRT EP, OpenVINO NPU, LibTorch GPU, llama.cpp GPU and TPU outside production support until each receives live device-backed numerical qualification.
12. Emit structured qualification evidence with zero mandatory skips for the declared production scope.
13. Run deterministic qualification policy tests without requiring accelerator hardware.
14. Execute the live CPU qualification from inherited mandatory `ubuntu-core` on Linux x64.
15. Preserve all parser/semantic, sanitizer/race, portability/reproducibility, release, security, deployment and tooling gates.
16. Update feature status, compiler test strategy, roadmap, third-party inventory and the 27-area coverage matrix together.
17. Final PR head must have `ci / ubuntu (push)` successful.
18. Final PR head must have `ci / ubuntu (pull_request)` successful.
19. TST022 becomes implemented only for the versioned v1 production support set after exact-head evidence passes.
20. Adding any production-supported backend/device/platform later requires its own live numerical execution fixture before the support matrix can expand.

## Mandatory rule for every remaining PR

Every PR through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip, warning-only success or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification in mandatory compiler CI, GitHub PR79 retains formatter/linter qualification and GitHub PR80 retains LSP/editor qualification. GitHub PR81 adds live versioned backend/hardware qualification to inherited `ubuntu-core`. Release publication remains separated from PR CI so OIDC/repository-write privileges are not granted to pull-request code.

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
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | IN PROGRESS as GitHub PR81 | Versioned production backend/device support with mandatory ONNX Runtime CPU live numerical evidence and fail-closed accelerator boundaries. | Pinned qualification SDK plus mandatory inherited ubuntu-core live gate. | Output `42`, no fallback/skip, route rejection and structured support matrix. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax/AST/semantics/evidence declarations without granting certification. | C3-ECO frontend gate. | Grammar/AST, units, valid/invalid contracts and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Energy/carbon/cost calculations with provenance, uncertainty, quality and baselines. | Deterministic eco-regression job. | Equation, threshold, stale-factor, unit and missing-sensor tests. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | RC bundle verification dependency. | Tamper, lineage, signature, schema, redaction and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR build/lit job. | FileCheck, verifiers, roundtrip, installed consumer and freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, LLVM lowering and AI/Green AI runtime handoff. | MLIR differential lowering tied to PR72 oracle. | Invalid ops/shapes, differential execution and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata and blocker aggregation. | Final zero-skip release-candidate aggregate. | Calibration, equivalent workloads, repeated measurements, uncertainty and clean RC matrix. |

## Current count

remaining_planned_implementation_prs_pr80_through_pr86: 7
remaining_planned_implementation_prs_after_pr80: 6

Next recommended roadmap PR after PR80 is merged:

PR81 - Complete C3-ECO language blocks.

## External production blocker not counted as an implementation PR

TST017 remains partial until repository administration configures the `production-release` protected environment and a real version tag executes the signed publication workflow with attestations that verify cryptographically. The workflow implementation is already merged; this operational exercise is not counted as one of the seven remaining implementation PRs.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

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
