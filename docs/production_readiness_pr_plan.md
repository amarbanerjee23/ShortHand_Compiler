# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-12-pr76
PLAN_STATUS: active
LAST_COMPLETED_PR: 75
MERGED_OUT_OF_BAND_PR: 71
CURRENT_IMPLEMENTATION_PR: 76
GITHUB_IMPLEMENTATION_PR: 77
NEXT_IMPLEMENTATION_PR_AFTER_PR76: 77
BASELINE_LANGUAGE_VERSION: beta-0.3
TARGET: enterprise production usage ready language

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable semantics, deterministic compilation, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable paths must never report success. A skipped dependency, absent accelerator, unprotected release environment, cancelled workflow, unavailable security scanner or source-pattern check is not production execution evidence.

## Current baseline

Roadmap PR69 through PR75 are merged. PR71 was the out-of-band CI status-publication correction. Roadmap PR74 was implemented as GitHub PR75 because GitHub PR74 had already been consumed by a duplicate PR73 branch. Roadmap PR75 was implemented and merged as GitHub PR76, establishing immutable release tags, four-platform candidate construction, SHA-256/SPDX/provenance binding, OIDC attestations, protected-environment preflight, cryptographic verification and draft-release rollback.

Roadmap PR76 is now the active implementation and is carried by GitHub PR77. It adds mandatory external C/C++ SAST, source/dependency vulnerability scanning, pull-request dependency review, redistribution license policy, repository-wide immutable GitHub Action pins, expiring security exceptions and positive/negative scanner evidence.

ShortHand remains `controlled_beta` with `production_claim: false`. TST017 signed protected release remains partial until `production-release` is configured and a real version tag is successfully attested. PR77 does not claim absent optional AI SDKs are security-qualified; PR80 owns live backend/SDK qualification.

## PR76 completion contract

Roadmap PR76 - External vulnerability, SAST, dependency and license policy gate is IN PROGRESS as GitHub PR77.

Implementation requirements:

1. A dedicated mandatory `security` CI job must be a dependency of the stable Ubuntu aggregate for both push and pull-request events.
2. GitHub CodeQL must analyze C/C++ with the `security-extended` suite using a real manual compiler build.
3. CodeQL SARIF must be enforced locally and fail on unexcepted `security-severity >= 7.0` findings.
4. Trivy must perform an external repository scan for vulnerabilities, secrets, misconfiguration and licenses; scanner execution errors remain failures.
5. HIGH/CRITICAL vulnerabilities, any detected secret, HIGH/CRITICAL misconfiguration and HIGH/CRITICAL license findings must fail the repository policy.
6. Pull requests must execute GitHub dependency review with HIGH severity and an explicit redistribution license allowlist; `warn-only` is forbidden.
7. `security/third_party_inventory.tsv` must distinguish build tools, optional SDKs, linked runtime dependencies and vendored dependencies with version/license/source/redistribution policy.
8. Linked, vendored or redistributed dependencies must use an allowlisted SPDX license; unknown, `LicenseRef-*` and unbounded versions are rejected for redistribution.
9. Every external GitHub Action under `.github/workflows` must use an immutable 40-hex commit SHA rather than a floating tag or branch.
10. Trivy and CodeQL exceptions must be concrete, owned, ticketed, justified and expire no more than 90 days from the policy date; wildcard or expired exceptions fail.
11. Deterministic negative tests must reject prohibited licenses, unbounded versions, expired/wildcard exceptions, floating actions, high CodeQL SARIF and failing Trivy reports.
12. A live generated vulnerable dependency fixture must be detected by the external Trivy CVE database; the fixture must not be committed as a production package manifest.
13. A scheduled security rescan must re-evaluate the repository against newly published vulnerability data without overwriting stable PR/push statuses.
14. Existing PR73 safety, PR74 portability/reproducibility and PR75 signed-release gates remain mandatory and unweakened.
15. Final PR head must have `ci / ubuntu (push)` successful.
16. Final PR head must have `ci / ubuntu (pull_request)` successful.
17. TST018 becomes implemented only after the required live scanners and local enforcement pass on the exact final head; absent optional SDKs remain outside this evidence.

## Mandatory rule for every remaining PR

Every PR through PR86 must include all applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. It must update the feature tracker, this roadmap and the compiler coverage matrix. No mandatory production test may be converted to an unconditional skip or `continue-on-error` success.

The final head of every implementation PR must have both stable event-specific CI statuses green before merge.

## Robust pipeline architecture

`docs/ci_pipeline_architecture.md` remains the pipeline architecture contract. PR77 adds a mandatory external security branch to the CI DAG while release publication remains separated from PR CI so OIDC/repository-write privileges are not granted to pull-request code.

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
| PR76 - External vulnerability, SAST, dependency and license policy gate | IN PROGRESS as GitHub PR77 | CodeQL C++ SAST, Trivy CVE/secret/misconfiguration/license scanning, dependency review, license policy, action pins and expiring exceptions. | Dedicated mandatory security job plus daily rescan. | Vulnerable dependency, secret, prohibited-license, SARIF, exception and anti-weakening tests. |
| PR77 - Container and Kubernetes production hardening | PLANNED | Multi-arch non-root images, read-only filesystem, dropped capabilities, probes, limits and network policy. | Ephemeral-cluster integration. | Image scan, deployment, health, shutdown/restart, limits and network negatives. |
| PR78 - Formatter and linter baseline | PLANNED | Deterministic formatter, semantic-preserving lint rules, machine diagnostics and safe fixes. | Fast frontend format/lint job. | Idempotence, format-parse roundtrip, preservation and fix safety. |
| PR79 - Syntax highlighting and LSP implementation | PLANNED | Editor grammar plus compiler-backed diagnostics, hover, completion, definition and module navigation. | Protocol/golden job with cancellation and latency budget. | Token/protocol goldens, partial docs, cancellation and imported navigation. |
| PR80 - Production backend and CPU/GPU/TPU/NPU hardware qualification matrix | PLANNED | Production-supported backends and evidence-driven hardware target selection. | Capability-aware backend matrix. | Numerical outputs, route/fallback and no-false-success tests. |
| PR81 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented syntax/AST/semantics/evidence declarations without granting certification. | C3-ECO frontend gate. | Grammar/AST, units, valid/invalid contracts and sanitizer tests. |
| PR82 - Measured scoring, reports and eco-regression | PLANNED | Energy/carbon/cost calculations with provenance, uncertainty, quality and baselines. | Deterministic eco-regression job. | Equation, threshold, stale-factor, unit and missing-sensor tests. |
| PR83 - Authority-ready C3-ECO auditor bundle | PLANNED | Signed manifests, source/binary/model/measurement lineage, retention, redaction and verification. | RC bundle verification dependency. | Tamper, lineage, signature, schema, redaction and clean-room replay. |
| PR84 - Generated MLIR dialect build integration | PLANNED | TableGen-generated dialect, operations, types, verifiers, installation and downstream consumption. | MLIR build/lit job. | FileCheck, verifiers, roundtrip, installed consumer and freshness. |
| PR85 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | AST/SemanticIR lowering, canonicalization, verification, LLVM lowering and AI/Green AI runtime handoff. | MLIR differential lowering tied to PR72 oracle. | Invalid ops/shapes, differential execution and optimization preservation. |
| PR86 - Measured energy, performance and zero-skip production RC gate | PLANNED | Equivalent ShortHand/Python workloads, latency, throughput, memory, energy, metadata and blocker aggregation. | Final zero-skip release-candidate aggregate. | Calibration, equivalent workloads, repeated measurements, uncertainty and clean RC matrix. |

## Current count

remaining_planned_implementation_prs_pr76_through_pr86: 11
remaining_planned_implementation_prs_after_pr76: 10

Next recommended roadmap PR after PR76 is merged:

PR77 - Container and Kubernetes production hardening.

## Historical roadmap anchors

The following strings are immutable audit history and are not active state:

- production_readiness_plan_version: 2026-08-12-pr75
- LAST_COMPLETED_PR: 74
- CURRENT_IMPLEMENTATION_PR: 75
- GITHUB_IMPLEMENTATION_PR: 76
- NEXT_IMPLEMENTATION_PR_AFTER_PR75: 76
- Roadmap PR75 - Signed release and protected publication workflow is IN PROGRESS as GitHub PR76.
- remaining_planned_implementation_prs_pr75_through_pr86: 12
- remaining_planned_implementation_prs_after_pr75: 11
- | PR75 - Signed release and protected publication workflow | IN PROGRESS as GitHub PR76
- | PR76 - External vulnerability, SAST, dependency and license policy gate | PLANNED
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
