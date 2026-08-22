# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-21-pr82
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend, platform, container runtime or cluster was skipped is not production success evidence.

## Current audit

The 27-area production test matrix records for the GitHub PR82 candidate:

- 21 implemented areas,
- 3 partial areas,
- 3 open areas.

Roadmap PR74, merged as GitHub PR75, closed compiler/platform portability, independent reproducibility, frozen ABI consumer and installed SDK lifecycle blockers. Roadmap PR75, merged as GitHub PR76, added fail-closed signed-release publication architecture, while TST017 remains partial until a real protected tag publication is cryptographically verified. GitHub PR77 closed TST018 for the current security contract. GitHub PR78 closed TST019 for the declared deployment contract. GitHub PR79 closed TST020 for formatter/linter. GitHub PR80 closed TST021 for the native LSP/editor baseline. GitHub PR81 closed TST022 for the versioned `linux-x64-cpu-v1` production backend support set. GitHub PR82 adds executable first-class C3-ECO language declarations to TST023 while keeping that area partial until measured scoring and authority-ready lineage are implemented.

Strong current coverage includes grammar/module conformance, deterministic package resolution, semantic differential execution, staged fuzzing, ASan/LSan/UBSan, TSan, source-aware diagnostics, GCC/Clang qualification, Linux x64/arm64, macOS arm64 and Windows x64 compiler execution, CTest parity, reproducible clean builds, frozen ABI consumers, install/reinstall/uninstall package lifecycle, fail-closed external security scanning, restricted container/Kubernetes deployment, formatter/linter preservation, native LSP/editor qualification and qualification-aware AI backend routing.

Production-critical gaps remain for protected signed-release execution, measured C3-ECO scoring and authority-ready evidence lineage, production MLIR lowering, measured performance/energy and the final release-candidate gate.

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

GPU, TPU and NPU are explicitly not production-supported in v1. If a future ShortHand version adds any accelerator or additional backend/platform to its production support matrix, that new row must provide real device-backed numerical execution before it can be called implemented.

## Syntax highlighting and LSP correctness contract

Roadmap PR79 was implemented as GitHub PR80 through `shorthand.tooling.lsp.v1`. It requires scanner-aligned configuration, native bounded JSON-RPC, UTF-16 positions, compiler-backed diagnostics, deterministic editor requests, cancellation, malformed-input handling, GCC/Clang/ASan/UBSan and inherited ubuntu-core execution.

LSP/editor success is not inferred from JSON/text presence. An unavailable compiler oracle in editor tooling must fail visibly. The v1 LSP contract does not claim rename/refactor, semantic tokens, workspace-wide indexing or debugger integration.

## Formatter and linter correctness contract

Roadmap PR78 was implemented as GitHub PR79 through native `shorthand_tool` and `shorthand.tooling.format_lint.v1`. Formatter success is not inferred from source-text checks. Parser acceptance, idempotence, behavior preservation, diagnostics and safe explicit-output fixes remain executable evidence.

## Required test layers for every implementation PR

Every remaining implementation PR through PR86 must include all applicable layers below. A PR description must explicitly mark non-applicable layers and explain why.

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
| Documentation guard | Roadmap, feature status and test coverage matrix update in the same PR. |
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

## CI profiles

### Pull-request profile

The mandatory DAG runs policy/status guards, grammar/module/semantic gates, fuzz and sanitizer/race safety, toolchain/platform qualification, installed consumers, CTest parity, reproducibility, external security and deployment qualification. Tooling runs formatter/linter and LSP/editor jobs under GCC, Clang and ASan/UBSan. The inherited `ubuntu-core` feature-plan gate repeats the tooling contracts and, for GitHub PR81, performs the mandatory pinned ONNX Runtime CPU live qualification. Normal PR jobs do not receive release OIDC or repository-write permissions.

### Scheduled profile

Extended fuzzing/race stress and external security rescans are additive evidence. Scheduled evidence does not replace stable PR/push status contexts.

### Release-candidate profile

A release candidate requires every declared platform/toolchain, signed artifacts, checksums, SPDX SBOM, provenance, independent reproducibility evidence, installed consumers, deployment/security/C3-ECO/MLIR qualification, performance/energy evidence and zero mandatory skips.

## Production test exit criteria

ShortHand may move from `controlled_beta` to release candidate only when every row in `tests/coverage/compiler_test_coverage_matrix.tsv` is `implemented` or explicitly declared out of scope in the versioned language/product contract.

ShortHand may claim enterprise production readiness only when:

1. PR70 and all PR72 through PR86 implementation completion gates are merged, with PR71 CI hygiene already merged,
2. no mandatory test in the declared production support set is skipped,
3. every production-supported backend/device/platform row has live numerical success evidence,
4. all release platforms pass installed-consumer tests,
5. reproducible and signed artifacts are produced and cryptographically verified,
6. measured performance and energy evidence supports any published claim,
7. the final PR86 RC gate reports zero open production blockers.

## Historical strategy audit anchors

The following exact strings are retained only for milestone guards and are not current counts:

- compiler_test_strategy_version: 2026-08-18-pr81
- 21 implemented areas
- 3 partial areas
- 3 open areas

- compiler_test_strategy_version: 2026-08-18-pr80
- 20 implemented areas
- 4 partial areas
- 3 open areas
- compiler_test_strategy_version: 2026-08-18-pr79
- 19 implemented areas
- 4 partial areas
- 4 open areas
- compiler_test_strategy_version: 2026-08-12-pr78
- 18 implemented areas
- 5 open areas
- compiler_test_strategy_version: 2026-08-12-pr77
- 17 implemented areas
- 6 open areas
- compiler_test_strategy_version: 2026-08-12-pr76
- 16 implemented areas
- 5 partial areas
- compiler_test_strategy_version: 2026-08-11-pr73
- 12 implemented areas
- 9 open areas
- compiler_test_strategy_version: 2026-08-09-pr70

The current strategy is `2026-08-21-pr82`.
