# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-18-pr78
PLAN_STATUS: active
LAST_COMPLETED_PR: 77
MERGED_OUT_OF_BAND_PR: 71
CURRENT_IMPLEMENTATION_PR: 78
GITHUB_IMPLEMENTATION_PR: 79
NEXT_IMPLEMENTATION_PR_AFTER_PR78: 79
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner, missing container runtime or unavailable deployment cluster is not production execution evidence.

## Current baseline

Roadmap PR69 through PR77 are merged. PR71 was the out-of-band CI status-publication correction. Roadmap PR74 was implemented as GitHub PR75 because GitHub PR74 had already been consumed by a duplicate PR73 branch. Roadmap PR75 was implemented and merged as GitHub PR76, establishing immutable release tags, four-platform candidate construction, SHA-256/SPDX/provenance binding, OIDC attestations, protected-environment preflight, cryptographic verification and draft-release rollback.

Roadmap PR76 was implemented and merged as GitHub PR77. It added mandatory CodeQL C/C++ SAST, Trivy vulnerability/secret/misconfiguration/license scanning, a repository-owned fail-closed dependency-delta gate, redistribution license policy, immutable GitHub Action pins, expiring security exceptions and live vulnerable-dependency evidence.

Roadmap PR77 was implemented and merged as GitHub PR78. It hardened the production compiler/runtime container and Kubernetes workload, qualified native Linux amd64 and arm64 images and added live ephemeral-cluster evidence for restricted execution, health, quota, network denial and restart behavior.

Roadmap PR78 is now the active implementation and is carried by GitHub PR79. It adds a deterministic ShortHand formatter/linter baseline with stable machine diagnostics, explicit-output safe fixes, parser/execution preservation and compiler/sanitizer qualification.

ShortHand remains `controlled_beta` with `production_claim: false`. TST017 signed protected release remains partial until `production-release` is configured and a real version tag is successfully attested. TST020 closes only the versioned formatter/linter correctness contract and does not claim LSP/editor completion, live AI backends, measured energy superiority or final release readiness.

## PR78 completion contract

Roadmap PR78 - Formatter and linter baseline is IN PROGRESS as GitHub PR79.

Implementation requirements:

1. Provide a native compiled formatter/linter implementation that does not depend on Python.
2. Define a versioned formatter/linter contract and stable machine-readable diagnostic schema.
3. Canonical formatting must be deterministic and idempotent.
4. Safe formatting/fixes must be limited to source trivia and must not rename identifiers, reorder declarations/imports, change token spelling, rewrite expressions or perform semantic refactors.
5. The formatter must correctly ignore brace-like content inside strings and comments when computing indentation.
6. Parser-valid input must remain parser-valid after formatting.
7. A semantic preservation fixture must produce identical interpreter output before and after formatting/fixing.
8. Lint must return a non-zero status for non-canonical input while still producing consumable structured diagnostics.
9. Safe fix mode must require an explicit output path so source files are never mutated implicitly.
10. Boundary tests must cover tab indentation, trailing whitespace, non-canonical indentation, repeated/trailing blank lines, missing final newline and non-LF line endings.
11. The exact formatter implementation must execute under GCC and Clang and under ASan/UBSan.
12. A fast dedicated tooling workflow must run on push and pull request events.
13. The inherited mandatory `ubuntu-core` path must also execute the formatter/linter gate so stable CI cannot pass when TST020 is broken.
14. The feature tracker, compiler test strategy, roadmap and 27-area matrix must move together from 18/4/5 to 19/4/4 only on the candidate that contains executable evidence.
15. Existing parser/semantic, sanitizer/race, portability/reproducibility, signed-release, security and deployment gates remain mandatory and unweakened.
16. Final PR head must have `ci / ubuntu (push)` successful.
17. Final PR head must have `ci / ubuntu (pull_request)` successful.
18. TST020 becomes implemented only after the exact final head passes all formatter/linter and inherited mandatory evidence. Syntax highlighting and LSP remain TST021 / roadmap PR79.

## Mandatory rule for every remaining PR

Every PR through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. GitHub PR78 retains exact-head deployment qualification in the mandatory compiler CI: Linux amd64 builds/runs the production image and ephemeral Kind cluster from the existing `ubuntu-core` path, while the native Linux arm64 lane builds/runs the arm64 production image. GitHub PR79 adds a fast `tooling / formatter-linter` workflow with GCC, Clang and ASan/UBSan and also invokes the same formatter correctness gate from `check_feature_plan_status.sh` inside `ubuntu-core`. Release publication remains separated from PR CI so OIDC/repository-write privileges are not granted to pull-request code.

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
| PR77 - Container and Kubernetes production hardening | MERGED as GitHub PR78 | Multi-stage native amd64/arm64 non-root images, read-only runtime, Restricted Pod Security, probes, limits/quota, PDB and default-deny network policy. | Mandatory native container execution plus ephemeral Kind integration in exact-head CI. | Image execution, parser health, runtime security state, quota/network negatives, restart and graceful shutdown. |
| PR78 - Formatter and linter baseline | IN PROGRESS as GitHub PR79 | Native deterministic trivia-only formatter, stable machine lint diagnostics and explicit-output safe fixes. | Fast formatter/linter job plus inherited ubuntu-core execution. | Idempotence, parse roundtrip, execution preservation, diagnostics, fix safety, GCC/Clang and ASan/UBSan. |
| PR79 - Syntax highlighting and LSP implementation | PLANNED | Editor grammar plus compiler-backed diagnostics, hover, completion, definition and module navigation. | Protocol/golden job with cancellation and latency budget. | Token/protocol goldens, partial docs, cancellation and imported navigation. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | PLANNED | Production-supported backends and evidence-driven hardware target selection. | Capability-aware backend matrix. | Numerical outputs, route/fallback and no-false-success tests. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax/AST/semantics/evidence declarations without granting certification. | C3-ECO frontend gate. | Grammar/AST, units, valid/invalid contracts and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Energy/carbon/cost calculations with provenance, uncertainty, quality and baselines. | Deterministic eco-regression job. | Equation, threshold, stale-factor, unit and missing-sensor tests. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | RC bundle verification dependency. | Tamper, lineage, signature, schema, redaction and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR build/lit job. | FileCheck, verifiers, roundtrip, installed consumer and freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, LLVM lowering and AI/Green AI runtime handoff. | MLIR differential lowering tied to PR72 oracle. | Invalid ops/shapes, differential execution and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata and blocker aggregation. | Final zero-skip release-candidate aggregate. | Calibration, equivalent workloads, repeated measurements, uncertainty and clean RC matrix. |

## Current count

remaining_planned_implementation_prs_pr78_through_pr86: 9
remaining_planned_implementation_prs_after_pr78: 8

Next recommended roadmap PR after PR78 is merged:

PR79 - Syntax highlighting and LSP implementation.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-08-12-pr77
- LAST_COMPLETED_PR: 76
- CURRENT_IMPLEMENTATION_PR: 77
- GITHUB_IMPLEMENTATION_PR: 78
- NEXT_IMPLEMENTATION_PR_AFTER_PR77: 78
- Roadmap PR77 - Container and Kubernetes production hardening is IN PROGRESS as GitHub PR78.
- remaining_planned_implementation_prs_pr77_through_pr86: 10
- remaining_planned_implementation_prs_after_pr77: 9
- | PR77 - Container and Kubernetes production hardening | IN PROGRESS as GitHub PR78
- | PR78 - Formatter and linter baseline | PLANNED
- production_readiness_plan_version: 2026-08-12-pr76
- LAST_COMPLETED_PR: 75
- CURRENT_IMPLEMENTATION_PR: 76
- GITHUB_IMPLEMENTATION_PR: 77
- NEXT_IMPLEMENTATION_PR_AFTER_PR76: 77
- Roadmap PR76 - External vulnerability, SAST, dependency and license policy gate is IN PROGRESS as GitHub PR77.
- remaining_planned_implementation_prs_pr76_through_pr86: 11
- remaining_planned_implementation_prs_after_pr76: 10
- | PR76 - External vulnerability, SAST, dependency and license policy gate | IN PROGRESS as GitHub PR77
- | PR77 - Container and Kubernetes production hardening | PLANNED
- production_readiness_plan_version: 2026-08-12-pr75
- LAST_COMPLETED_PR: 74
- CURRENT_IMPLEMENTATION_PR: 75
- GITHUB_IMPLEMENTATION_PR: 76
- NEXT_IMPLEMENTATION_PR_AFTER_PR75: 76
- Roadmap PR75 - Signed release and protected publication workflow is IN PROGRESS as GitHub PR76.
- remaining_planned_implementation_prs_pr75_through_pr86: 12
- remaining_planned_implementation_prs_after_pr75: 11
- | PR75 - Signed release and protected publication workflow | IN PROGRESS as GitHub PR76
- production_readiness_plan_version: 2026-08-11-pr72
- PLAN_STATUS: active
- LAST_COMPLETED_PR: 70
- CURRENT_IMPLEMENTATION_PR: 72
- NEXT_IMPLEMENTATION_PR_AFTER_PR72: 73
- PR72 - Cross-mode semantic correctness and differential execution suite is IN PROGRESS.
- after PR72 is successfully merged, 14 implementation PRs remain.
- remaining_planned_prs_after_pr72: 14
- remaining_planned_implementation_prs_pr73_through_pr86: 14
- Next recommended PR after PR #72:
- PR73 - Continuous fuzzing, full sanitizer and concurrency race hardening.
- | PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | MERGED
- | PR71 - CI status publication hygiene | MERGED
- | PR72 - Cross-mode semantic correctness and differential execution suite | IN PROGRESS
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
