# Enterprise Release Readiness Scorecard

enterprise_release_scorecard_version: 2026-09-12-pr94
current_maturity: controlled_beta
production_claim: false
current_state: ER3-controlled-beta
target_state: ER4-enterprise-release-candidate

This scorecard summarizes active release controls. The machine-readable authority is `docs/production_truth.tsv`; test coverage is `tests/coverage/compiler_test_coverage_matrix.tsv`; C3-ECO coverage is `docs/c3eco_traceability.tsv`.

## State model

| State | Meaning | Current claim boundary |
| --- | --- | --- |
| ER0 | Internal engineering review | No external pilot claim. |
| ER1 | Compiler hardening candidate | Core compiler validation only. |
| ER2 | Real-backend pilot | At least one scoped live backend, no enterprise claim. |
| ER3 | Controlled enterprise beta | Qualified narrow scope with explicit open blockers. |
| ER4 | Enterprise release candidate | All mandatory blockers closed and retained release evidence verified. |

## Control families

| Control family | Status | Evidence / remaining condition |
| --- | --- | --- |
| Build, grammar, diagnostics and cross-mode tests | Implemented for beta-0.5 execution, beta-0.6 enterprise schema and beta-0.7 typed profile | CI, Make, CMake/CTest, conformance, type/memory, control-flow, enterprise and typed-profile gates; GitHub PR94 additionally qualifies the bounded v2 composite source/SDK lowering contract on Linux x64/LLVM18. |
| Memory, UB, fuzz and concurrency safety | Implemented for current baseline | ASan/LSan/UBSan, libFuzzer and TSan include bounded serving load/fault/soak evidence. |
| Toolchain, platform, ABI and packaging | Implemented for declared tiers | Reproducible clean builds, frozen runtime/core ABIs and installed static/shared C/C++ consumers. |
| Security and dependency governance | Implemented for current contract | CodeQL, Trivy, dependency delta, license policy, pinned actions, expiring exceptions and package v2 SHA-256/exact-version/license gates. |
| Container and Kubernetes | Implemented for CLI/compiler deployment contract | Hardened multi-arch image and live Kind checks; no public service/ingress claim. |
| Backend execution | Implemented for `linux-x64-cpu-v1` | ONNX Runtime CPU output `42`; PR94 adds a representative AI workload. |
| SBOM and provenance generation | Implemented | SPDX 2.3 source/artifact bundles, package dependency output and candidate provenance. |
| Protected signed publication | Partial | Source contract exists; real protected tag exercise and verified attestations remain. |
| Process-scoped serving and observability | Implemented for `shorthand.serving.runtime.v1` | Bounded admission, deadlines, cancellation, tenant isolation, health, low-cardinality metrics and graceful drain; public ingress/authentication/TLS are not claimed. |
| C3-ECO readiness | Partial | Typed profile, instrument-backed accounting and deterministic candidate assessment exist; PR91 implements signed auditor lineage, retention-policy checks, recertification handling and redacted reporting. Independent certification and actual storage operations remain external. |
| MLIR production lowering | Implemented for Linux x64/LLVM18 | GitHub PR94 verifies source/SDK lowering, bounded composite execution, real runtime handoff and optimized evidence retention. |
| Measured performance and energy | Open | PR95 requires equivalent work, repeated trials, raw data, provenance and uncertainty. |
| Final production RC aggregate | Open | PR96 requires zero mandatory skips, enterprise pilot, upgrade/rollback/DR and retained evidence. |

## ER4 promotion rule

ER4 requires every production blocker in both matrices to be closed, the scoped roadmap PR96 aggregate and applicable PR97-PR102 audit closeout gates to pass on their final heads in `ci / ubuntu (push)` and `ci / ubuntu (pull_request)`, and TST017 to be closed by a verified protected release. A high C3-ECO score cannot override a failed critical gate. No efficiency improvement counts if required functionality, accuracy, reliability, security, privacy, safety or accessibility is weakened.

The retained release bundle must include exact commit/run identity, toolchains, build/test/sanitizer results, backend/workload evidence, measurement and uncertainty records, security results, SBOM/provenance/signatures, deployment/pilot/rollback evidence, known limitations and approved claim wording.

Historical scorecard marker: enterprise_release_scorecard_version: 2026-09-01-pr88.

GitHub PR94 candidate auditor evidence is implemented: signatures, replay, lifecycle policy, nonconformities and public redaction. Independent certification and physical evidence-store controls remain external.

GitHub PR94 implements original roadmap PR93 in the Linux x64/LLVM18 scope: verified SemanticIR, source and SDK lowering, bounded composite values, checked real ONNX runtime calls and optimization-preserved evidence. See [the lowering contract](mlir_lowering.md). TST024 is implemented within this scope. Merged GitHub PR93 is the separate gap assessment; roadmap PR94-PR102 remain future implementation IDs. Ten increments remain including this candidate, nine after it, subject to complete exit evidence and external operational blockers.
