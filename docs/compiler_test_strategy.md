# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-09-12-pr94
language_version: beta-0.7
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend, platform, container runtime or cluster was skipped is not production success evidence.

## Current audit

The 36-area production test matrix records for the GitHub PR94 candidate:

- 32 implemented areas,
- 1 partial area,
- 3 open areas.

GitHub PR75 through PR89 established portability/reproducibility, signed-release architecture, external security, container/Kubernetes qualification, formatter/linter, LSP/editor tooling, live ONNX Runtime CPU qualification, first-class C3-ECO declarations, production truth, production type/memory, functions/control flow, enterprise packages/FFI, bounded concurrent serving, the typed C3-ECO profile and instrument-backed measurement accounting.

GitHub PR89 adds TST034, `shorthand.c3eco.measurement_workbook.v1`, with native instrument-backed measurement accounting, provenance, deterministic reconciliation, double-count rejection and claim-safe separation from PR90 scoring and PR95 equivalent-workload energy comparison.

GitHub PR90 adds TST035, `shorthand.c3eco.assessment.v1`, with mandatory-gate precedence, complete 76-criterion A-K scoring, evidence/uncertainty caps, controlled N/A reallocation, tier prerequisites, claim restrictions, materiality, AI applicability, regressions and deterministic candidate recommendations. The assessment gate consumes real compiler/measurement outputs and separately exercises native scoring, DQ ceilings, penalty deductions, Markdown parity and fail-closed PR95 comparative-claim deferral.

Strong current coverage includes grammar/module conformance, deterministic package resolution, semantic differential execution, staged fuzzing, ASan/LSan/UBSan, TSan, source-aware diagnostics, GCC/Clang qualification, Linux x64/arm64, macOS arm64 and Windows x64 execution, CTest parity, reproducible builds, frozen ABI consumers, installed-package lifecycle, fail-closed external security scanning, restricted container/Kubernetes deployment, process-scoped serving, formatter/linter preservation, native LSP/editor qualification, qualification-aware AI backend routing, measured-accounting validation and candidate C3-ECO assessment.

Production-critical gaps remain for representative AI workloads, measured performance/equivalent-workload energy and the final release-candidate gate. TST017 also still requires a real protected signed-tag exercise.

## Production backend and hardware qualification contract

Roadmap PR80 is implemented as GitHub PR81 through `shorthand.backend_hardware_qualification.v1`.

TST022 requires all of the following on the exact candidate head:

1. a versioned production backend/device support matrix,
2. a declared production scope of `linux-x64-cpu-v1`,
3. CPU/GPU/TPU/NPU inventory that does not equate detection with production support,
4. production routing restricted to live-qualified backend/device pairs,
5. `onnxruntime_cpu` + CPU as the v1 production-supported pair,
6. a pinned ONNX Runtime SDK version and SHA-256 verified acquisition path,
7. real compiled C++ inference through the ShortHand runtime bridge,
8. numerical identity-model validation requiring input `42` to produce output `42`,
9. no fallback, not-executed, SDK-missing or skip result for the production-supported CPU row,
10. default rejection of unqualified GPU, TPU and NPU production routes,
11. machine-readable `backend_device_not_production_qualified` evidence,
12. an explicit development override that remains marked `production_qualified:false`,
13. zero mandatory skips for the declared production support set,
14. deterministic positive/negative policy tests that do not need accelerator hardware,
15. inherited `ubuntu-core` execution of the exact live CPU qualification,
16. unchanged parser, semantic, sanitizer/race, portability, security, deployment and tooling gates.

GPU, TPU and NPU are explicitly not production-supported in v1. A future support expansion must provide real device-backed numerical execution before it can be called implemented.

## Tooling correctness contracts

Roadmap PR79 was implemented as GitHub PR80 through `shorthand.tooling.lsp.v1`. LSP/editor success is not inferred from JSON/text presence. An unavailable compiler oracle in editor tooling must fail visibly. The contract requires scanner-aligned configuration, native bounded JSON-RPC, UTF-16 positions, compiler-backed diagnostics, deterministic requests, cancellation and malformed-input handling.

Roadmap PR78 was implemented as GitHub PR79 through native `shorthand_tool` and `shorthand.tooling.format_lint.v1`. Formatter success is not inferred from source-text checks. Parser acceptance, idempotence, behavior preservation, diagnostics and safe explicit-output fixes remain executable evidence.

## Required test layers for every implementation PR

Every remaining implementation PR through roadmap PR102 must include all applicable layers below. A PR description must explicitly mark non-applicable layers and explain why.

| Layer | Required evidence |
| --- | --- |
| Contract | Updated specification, compatibility note, public API or schema when behavior changes. |
| Unit | Deterministic tests for new algorithms, validation rules and failure states. |
| Positive integration | End-to-end success fixture proving the implemented behavior. |
| Negative integration | Invalid inputs, unsupported states and boundary failures rejected with stable evidence. |
| Regression | Fixture reproducing every bug fixed by the PR. |
| Sanitizer | New native C++ paths execute under applicable sanitizer suites. |
| Security | Untrusted input, path, network, dependency or privilege boundaries receive misuse tests. |
| Portability | Platform-sensitive code gets independent toolchain/platform checks when available. |
| Performance | Hot paths receive bounded regression evidence or an explicit non-performance rationale. |
| Energy/evidence | Measurement or energy changes prove units, provenance, uncertainty, allocation and claim boundaries. |
| Documentation guard | Roadmap, feature status, production truth, traceability and test coverage matrix update together. |
| Pipeline behavior | New mandatory jobs fail closed, emit usable evidence and cannot be satisfied by cancellation or unconditional skip. |

## Test quality rules

1. Verify observable behavior, not only source-text presence.
2. Optional SDK tests may report unavailable when infrastructure is absent, but unavailability cannot close a production blocker.
3. Production-supported backends require real model execution and numerical-output validation.
4. Negative tests assert stable diagnostic codes, status classes or structured evidence.
5. Golden outputs normalize paths, timestamps and nondeterministic identifiers.
6. Network and potentially unbounded tests use explicit timeouts.
7. Concurrency safety requires TSan or equivalent evidence.
8. Energy claims require repeatable measurements, hardware metadata, uncertainty and an equivalent-workload baseline.
9. No CI gate may disable LeakSanitizer, weaken assertions, use `continue-on-error` for a mandatory check or turn a production test into an unconditional skip.
10. Deterministic compiler/test failures are fixed, not automatically retried to obtain green.
11. Release qualification includes clean no-cache build evidence and installed-package consumers.
12. Signing source code is not signing evidence: a signed-release blocker closes only after cryptographic verification succeeds for the produced artifact.
13. A protected release workflow must prevent branch publication, bind version tags to exact commits, minimize privileges and fail closed when environment protections are absent.
14. CPU/GPU/TPU/NPU detection is not execution evidence.
15. External security scanners are mandatory evidence, not advisory decoration. A scanner/network execution failure is a gate failure.
16. Security exceptions are concrete, owned, ticketed, justified and expire within 90 days; wildcard or expired exceptions fail CI.
17. A committed source SBOM is not a substitute for package-version CVE scanning, and an absent optional SDK is not security qualification.
18. A deployment manifest is not deployment evidence. Container and Kubernetes blockers close only after the exact image is built and executed under declared security constraints and live cluster negatives.
19. Multi-architecture image support requires native or otherwise execution-qualified evidence for each declared architecture.
20. Formatter success is not inferred from source-text checks. The exact formatter binary must prove idempotence, parser preservation and safe fixes.
21. LSP/editor success is not inferred from JSON/text presence. The exact native server must execute bounded protocol, compiler-diagnostic, cancellation, UTF-16 and navigation tests.
22. An unavailable compiler oracle in editor tooling must fail visibly and may not be represented as a clean document.
23. Backend SDK installation or hardware detection is not backend qualification. Every production-supported pair needs live numerical evidence.
24. An experimental hardware override cannot satisfy production qualification and must remain machine-marked unqualified.
25. Final release qualification executes mandatory tests from a clean checkout and reports zero mandatory skips.
26. Function, scope and label changes must pass the beta-0.5 interpreter, `lli` and native differential plus every stable negative diagnostic mode.
27. Enterprise schema, package, standard-library or FFI changes must pass the beta-0.6 language, tamper, exact-version, license, SPDX, sanitizer, ABI-symbol and installed-consumer gate.
28. Serving changes must preserve bounded admission, deadlines, cooperative cancellation, tenant isolation, low-cardinality telemetry and graceful drain under unit, load, sanitizer, TSan, installed-consumer and Kubernetes lifecycle evidence.
29. Typed C3-ECO profile changes must pass beta-0.7 positive, eight-code negative, migration, native-JSON, schema, metadata and claim-safety evidence without granting certification.
30. Measured-accounting changes must reject declared/modelled evidence, require instrument/calibration/factor/tariff provenance, prevent double counting, preserve offsets outside the base footprint and reconcile energy/carbon/cost deterministically.
31. Assessment changes must validate profile/workbook structure, run G1-G14 before scoring, cover all 76 criteria, enforce evidence/N/A/materiality/tier/regression/claim controls and keep every decision explicitly non-certifying.

## CI profiles

### Pull-request profile

The mandatory DAG runs production-truth/status guards, grammar/module/semantic gates, fuzz and sanitizer/race safety, toolchain/platform qualification, installed consumers, CTest parity, reproducibility, external security and deployment qualification. Tooling runs formatter/linter and LSP/editor jobs under GCC, Clang and ASan/UBSan. The inherited `ubuntu-core` feature-plan gate preserves the mandatory pinned ONNX Runtime CPU live qualification. Normal PR jobs do not receive release OIDC or repository-write permissions.

PR90 additionally requires both native measured-accounting and assessment gates to run through the repository governance/build paths. A missing executable, malformed input accepted as evidence, incomplete criterion catalog or failed negative fixture is a hard failure.

### Scheduled profile

Extended fuzzing/race stress and external security rescans are additive evidence. Scheduled evidence does not replace stable PR/push status contexts.

### Release-candidate profile

A release candidate requires every declared platform/toolchain, signed artifacts, checksums, SPDX SBOM, provenance, independent reproducibility evidence, installed consumers, deployment/security/C3-ECO/MLIR qualification, performance/energy evidence and zero mandatory skips.

## Production test exit criteria

ShortHand may move from `controlled_beta` to release candidate only when every row in `tests/coverage/compiler_test_coverage_matrix.tsv` is `implemented` or explicitly declared out of scope in the versioned language/product contract.

ShortHand may claim enterprise production readiness only when:

1. all applicable implementation completion gates through roadmap PR102 are merged,
2. no mandatory test in the declared production support set is skipped,
3. every production-supported backend/device/platform row has live numerical success evidence,
4. all release platforms pass installed-consumer tests,
5. reproducible and signed artifacts are produced and cryptographically verified,
6. measured performance and energy evidence supports any published claim,
7. the scoped roadmap PR96 RC and PR102 general-release gates report zero applicable open production blockers.


PR91 implements `shorthand.c3eco.auditor_bundle.v1`: signed artifact/reference lineage, native assessment replay, retention-policy and surveillance verification, nonconformity handling, redacted public reports and separate estimated readiness. See [the auditor contract](c3eco_auditor_bundle.md). It does not grant certification or independently verify storage retention.

Auditor changes must reject invalid signatures, missing evidence, validly signed false assessments, unsafe filesystem inputs, expired current reviews and private-data leakage. The native gate must run in direct CI, Make, CTest, sanitizers and installed consumers.

## Historical strategy audit anchors

The following exact strings are retained only for milestone guards and are not current counts:

- compiler_test_strategy_version: 2026-09-02-pr89
- 34-area production test matrix
- 28 implemented areas
- compiler_test_strategy_version: 2026-09-01-pr88
- 27 implemented areas
- 3 partial areas
- 3 open areas
- compiler_test_strategy_version: 2026-08-21-pr82
- 21 implemented areas
- compiler_test_strategy_version: 2026-08-18-pr81
- compiler_test_strategy_version: 2026-08-18-pr80
- 20 implemented areas
- compiler_test_strategy_version: 2026-08-18-pr79
- 19 implemented areas
- compiler_test_strategy_version: 2026-08-12-pr78
- 18 implemented areas
- compiler_test_strategy_version: 2026-08-12-pr77
- 17 implemented areas
- compiler_test_strategy_version: 2026-08-12-pr76
- 16 implemented areas
- 5 partial areas
- compiler_test_strategy_version: 2026-08-11-pr73
- 12 implemented areas
- 9 open areas
- compiler_test_strategy_version: 2026-08-09-pr70

The current strategy is `2026-09-12-pr94`.

PR92 adds mandatory `scripts/check_mlir_dialect.sh`, GCC/Clang sanitizer qualification, lit/FileCheck diagnostics, typed SSA/symbol rejection, bytecode/location roundtrips, relocated installed API consumers and TableGen freshness mutations. GitHub PR94 closes TST024 for Linux x64/LLVM18 with source/module and composite execution, relocated SDK lowering, independent invalid-IR/ABI attacks, O0/O2 output/evidence preservation, real ONNX CPU inference and failure tests. The same gate runs under ASan/LSan/UBSan without suppressions. The matrix now has 32 implemented, 1 partial and 3 open areas.

GitHub PR94 implements original roadmap PR93 in the Linux x64/LLVM18 scope: verified SemanticIR, source and SDK lowering, bounded composite values, checked real ONNX runtime calls and optimization-preserved evidence. See [the lowering contract](mlir_lowering.md). TST024 is implemented within this scope. Merged GitHub PR93 is the separate gap assessment; roadmap PR94-PR102 remain future implementation IDs. Ten increments remain including this candidate, nine after it, subject to complete exit evidence and external operational blockers.
