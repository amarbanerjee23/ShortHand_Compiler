# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-18-pr79
PLAN_STATUS: active
LAST_COMPLETED_PR: 78
MERGED_OUT_OF_BAND_PR: 71
CURRENT_IMPLEMENTATION_PR: 79
GITHUB_IMPLEMENTATION_PR: 80
NEXT_IMPLEMENTATION_PR_AFTER_PR79: 80
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime, unavailable compiler oracle or unavailable deployment cluster is not production execution evidence.

## Current baseline

Roadmap PR69 through PR78 are merged. PR71 was the out-of-band CI status-publication correction. Roadmap PR74 was implemented as GitHub PR75. Roadmap PR75 was implemented and merged as GitHub PR76, establishing immutable release tags, four-platform candidate construction, SHA-256/SPDX/provenance binding, OIDC attestations, protected-environment preflight, cryptographic verification and draft-release rollback. Roadmap PR76 was implemented and merged as GitHub PR77 with mandatory external security/SAST/dependency/license gates. Roadmap PR77 was implemented and merged as GitHub PR78 with native amd64/arm64 hardened containers and live Kubernetes qualification. Roadmap PR78 was implemented and merged as GitHub PR79 with the deterministic formatter/linter baseline.

Roadmap PR79 is now the active implementation and is carried by GitHub PR80. It adds scanner-aligned syntax highlighting and a native C++ language server with bounded JSON-RPC framing, compiler-backed diagnostics, UTF-16 editor coordinates, deterministic completion/hover/symbols, local/imported definition navigation, cancellation, malformed-input negatives and CMake installation.

ShortHand remains `controlled_beta` with `production_claim: false`. TST017 signed protected release remains partial until `production-release` is configured and a real version tag is successfully attested. TST021 closes only the versioned editor/LSP contract and does not claim live AI backends, complete C3-ECO evidence, production MLIR lowering, measured energy superiority or final release readiness.

## PR79 completion contract

Roadmap PR79 - Syntax highlighting and LSP implementation is IN PROGRESS as GitHub PR80.

Implementation requirements:

1. Provide a native compiled C++17 language server with no Python runtime dependency.
2. Define a versioned editor/LSP contract and explicit claim boundary.
3. Provide scanner-aligned TextMate syntax highlighting for `.short` files and machine-parseable editor configuration.
4. Use bounded stdio `Content-Length` framing and bounded JSON parsing; malformed, duplicate, truncated or oversized framing must fail closed.
5. Advertise and correctly implement UTF-16 editor positions, including multibyte UTF-8 regression evidence.
6. Publish diagnostics from the real `short_hand` compiler parser rather than an editor-only approximation.
7. If the compiler oracle is unavailable, publish explicit failure evidence rather than a false clean document.
8. Implement deterministic completion, hover and document-symbol requests for the v1 baseline.
9. Implement local definition navigation and imported-module navigation through the existing deterministic `shorthand.package` manifest.
10. Handle partial/invalid documents and prove recovery after a subsequent valid full-document change.
11. Support `$/cancelRequest` with the defined cancellation error for a canceled queued request.
12. Enforce correct initialize/shutdown/exit lifecycle, including non-zero exit without prior shutdown.
13. Compile the LSP in the normal CMake DAG and install it as a shipped executable.
14. Execute the exact LSP/editor implementation under GCC and Clang and under ASan/UBSan.
15. Run a fast dedicated tooling job on push and pull request events.
16. Execute the same LSP/editor gate from inherited mandatory `ubuntu-core`, so stable CI cannot pass when TST021 is broken.
17. Use explicit timeouts for protocol sessions and reject unbounded input behavior.
18. Update the feature tracker, compiler test strategy, roadmap and 27-area matrix together from 19/4/4 to 20/4/3 only on the candidate containing executable evidence.
19. Existing parser/semantic, formatter, sanitizer/race, portability/reproducibility, signed-release, security and deployment gates remain mandatory and unweakened.
20. Final PR head must have `ci / ubuntu (push)` successful.
21. Final PR head must have `ci / ubuntu (pull_request)` successful.
22. TST021 becomes implemented only after the exact final head passes all LSP/editor and inherited mandatory evidence. Live backend/hardware qualification remains roadmap PR80.

## Mandatory rule for every remaining PR

Every PR through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip, warning-only success or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification in mandatory compiler CI, and GitHub PR79 retains formatter/linter qualification. GitHub PR80 adds `tooling / lsp-editor` under GCC, Clang and ASan/UBSan and also invokes the same LSP/editor contract from `check_feature_plan_status.sh` inside `ubuntu-core`. CMake platform lanes compile `shorthand_lsp` as a first-class target. Release publication remains separated from PR CI so OIDC/repository-write privileges are not granted to pull-request code.

## Remaining implementation strategy

| Planned PR | Status | Implementation scope | Pipeline/CI implementation | Mandatory tests and exit evidence |
| --- | --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Versioned test strategy, 27-area matrix and PR contract. | Coverage governance. | Schema, IDs, counts and claim-safety gates. |
| PR69 - Module, import and package syntax with AST scaffold | MERGED | Beta-0.3 preamble grammar and AST provenance. | Module syntax gate. | Positive, negative, compatibility, stress and sanitizer tests. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED | Hermetic manifest resolution, lockfiles, graph ordering, visibility and multi-file codegen. | Resolver first-class gate. | Determinism, native binding, negative graph cases, stress, sanitizer and CTest evidence. |
| PR71 - CI status publication hygiene | MERGED | Cancellation-safe SHA-scoped status handling. | Stable event-specific status contexts. | Push/PR status hygiene and cancellation policy guard. |
| PR72 - Cross-mode semantic correctness and differential execution suite | MERGED | Reference executable semantics, interpreter calls/control flow, imported execution, LLVM/native parity and deterministic failure domains. | Mandatory semantic differential gate. | Interpreter/lli/native positive and negative parity. |
| PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening | MERGED | Scanner/parser/module/semantic/lowering fuzzing, ASan/LSan/UBSan and TSan. | First-class safety steps plus scheduled fuzz. | Replayable corpus and no sanitizer/race findings. |
| PR74 - CI/toolchain/platform matrix, CTest parity and reproducible builds | MERGED as GitHub PR75 | GCC12/14, Clang16/18, Linux x64/arm64, macOS arm64, Windows x64, installed consumers and deterministic artifacts. | Multi-job DAG and reproducibility/CTest parity jobs. | Native/platform execution, ABI consumers, SDK lifecycle and clean-build checksums. |
| PR75 - Signed release and protected publication workflow | MERGED as GitHub PR76 | Immutable tag/version/master-lineage policy, checksums, artifact SBOM/provenance, OIDC attestations, protected publication and rollback. | Separate tag/manual release workflow plus mandatory non-privileged contract tests. | Tamper/unsigned/environment negatives; real protected tag verification still closes TST017 externally. |
| PR76 - External vulnerability, SAST, dependency and license policy gate | MERGED as GitHub PR77 | CodeQL C++ SAST, Trivy CVE/secret/misconfiguration/license scanning, dependency delta review, license policy, action pins and expiring exceptions. | Dedicated mandatory security job plus daily rescan. | Vulnerable dependency, secret, prohibited-license, SARIF, exception and anti-weakening tests. |
| PR77 - Container and Kubernetes production hardening | MERGED as GitHub PR78 | Multi-stage native amd64/arm64 non-root images, read-only runtime, Restricted Pod Security, probes, quota, PDB and default-deny network policy. | Native container execution plus ephemeral Kind exact-head integration. | Image execution, runtime security, quota/network negatives, restart and graceful shutdown. |
| PR78 - Formatter and linter baseline | MERGED as GitHub PR79 | Native deterministic trivia-only formatter, stable machine diagnostics and explicit-output safe fixes. | Fast formatter/linter job plus inherited ubuntu-core execution. | Idempotence, parse roundtrip, execution preservation, diagnostics, fix safety, GCC/Clang and ASan/UBSan. |
| PR79 - Syntax highlighting and LSP implementation | IN PROGRESS as GitHub PR80 | Scanner-aligned editor grammar plus native compiler-backed diagnostics, hover, completion, definition, symbols and module navigation. | Dedicated protocol job under GCC/Clang/sanitizers plus inherited ubuntu-core execution and CMake platform compilation. | Protocol/framing, compiler diagnostics, partial docs, UTF-16, cancellation, lifecycle, malformed input and imported navigation. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | PLANNED | Production-supported backends and evidence-driven hardware target selection. | Capability-aware backend matrix. | Numerical outputs, route/fallback and no-false-success tests. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax/AST/semantics/evidence declarations without granting certification. | C3-ECO frontend gate. | Grammar/AST, units, valid/invalid contracts and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Energy/carbon/cost calculations with provenance, uncertainty, quality and baselines. | Deterministic eco-regression job. | Equation, threshold, stale-factor, unit and missing-sensor tests. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | RC bundle verification dependency. | Tamper, lineage, signature, schema, redaction and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR build/lit job. | FileCheck, verifiers, roundtrip, installed consumer and freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, LLVM lowering and AI/Green AI runtime handoff. | MLIR differential lowering tied to PR72 oracle. | Invalid ops/shapes, differential execution and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata and blocker aggregation. | Final zero-skip release-candidate aggregate. | Calibration, equivalent workloads, repeated measurements, uncertainty and clean RC matrix. |

## Current count

remaining_planned_implementation_prs_pr79_through_pr86: 8
remaining_planned_implementation_prs_after_pr79: 7

Next recommended roadmap PR after PR79 is merged:

PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-08-18-pr78
- LAST_COMPLETED_PR: 77
- CURRENT_IMPLEMENTATION_PR: 78
- GITHUB_IMPLEMENTATION_PR: 79
- NEXT_IMPLEMENTATION_PR_AFTER_PR78: 79
- Roadmap PR78 - Formatter and linter baseline is IN PROGRESS as GitHub PR79.
- remaining_planned_implementation_prs_pr78_through_pr86: 9
- remaining_planned_implementation_prs_after_pr78: 8
- | PR78 - Formatter and linter baseline | IN PROGRESS as GitHub PR79
- production_readiness_plan_version: 2026-08-12-pr77
- LAST_COMPLETED_PR: 76
- CURRENT_IMPLEMENTATION_PR: 77
- GITHUB_IMPLEMENTATION_PR: 78
- NEXT_IMPLEMENTATION_PR_AFTER_PR77: 78
- Roadmap PR77 - Container and Kubernetes production hardening is IN PROGRESS as GitHub PR78.
- remaining_planned_implementation_prs_pr77_through_pr86: 10
- remaining_planned_implementation_prs_after_pr77: 9
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
