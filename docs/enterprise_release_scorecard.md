# Enterprise Release Readiness Scorecard

enterprise_release_scorecard_version: 2026-08-22-pr84
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
| Build, grammar, diagnostics and cross-mode tests | Implemented for defined beta-0.4 typed core | CI, Make, CMake/CTest, conformance, type/memory and differential gates; PR85 completes the next language layer. |
| Memory, UB, fuzz and concurrency safety | Implemented for current baseline | ASan/LSan/UBSan, libFuzzer and TSan; PR87 adds serving load/fault/soak evidence. |
| Toolchain, platform, ABI and packaging | Implemented for declared tiers | Reproducible clean builds, frozen ABI and installed consumers; PR86 adds enterprise packages/stdlib/FFI. |
| Security and dependency governance | Implemented for current contract | CodeQL, Trivy, dependency delta, license policy, pinned actions and expiring exceptions. |
| Container and Kubernetes | Implemented for CLI/compiler deployment contract | Hardened multi-arch image and live Kind checks; no public service/ingress claim. |
| Backend execution | Implemented for `linux-x64-cpu-v1` | ONNX Runtime CPU output `42`; PR94 adds a representative AI workload. |
| SBOM and provenance generation | Implemented | SPDX 2.3 source/artifact bundles and candidate provenance. |
| Protected signed publication | Partial | Source contract exists; real protected tag exercise and verified attestations remain. |
| Runtime observability | Partial | JSON, Prometheus and OTLP-shaped exports exist; PR87 adds production serving operations. |
| C3-ECO readiness | Partial | Candidate language/evidence and full traceability exist; PR88-PR91 close typed profile, measurement, scoring and auditor lifecycle. |
| MLIR production lowering | Partial | Hand-authored foundation exists; PR92-PR93 add generated dialect and full lowering. |
| Measured performance and energy | Open | PR95 requires equivalent work, repeated trials, raw data, provenance and uncertainty. |
| Final production RC aggregate | Open | PR96 requires zero mandatory skips, enterprise pilot, upgrade/rollback/DR and retained evidence. |

## ER4 promotion rule

ER4 requires every production blocker in both matrices to be closed, PR96 to pass on its final head in `ci / ubuntu (push)` and `ci / ubuntu (pull_request)`, and TST017 to be closed by a verified protected release. A high C3-ECO score cannot override a failed critical gate. No efficiency improvement counts if required functionality, accuracy, reliability, security, privacy, safety or accessibility is weakened.

The retained release bundle must include exact commit/run identity, toolchains, build/test/sanitizer results, backend/workload evidence, measurement and uncertainty records, security results, SBOM/provenance/signatures, deployment/pilot/rollback evidence, known limitations and approved claim wording.
