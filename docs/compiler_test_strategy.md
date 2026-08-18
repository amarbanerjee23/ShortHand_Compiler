# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-12-pr78
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend, platform, container runtime or cluster was skipped is not production success evidence.

## Current audit

The 27-area production test matrix now records:

- 18 implemented areas,
- 4 partial areas,
- 5 open areas.

Roadmap PR74, merged as GitHub PR75, closed compiler/platform portability, independent reproducibility, frozen ABI consumer and installed SDK lifecycle blockers. Roadmap PR75, merged as GitHub PR76, added fail-closed signed-release publication architecture, while `TST017` remains partial until a real protected tag publication is cryptographically verified. GitHub PR77 implemented roadmap PR76 and closed `TST018` for the current source/dependency contract with mandatory CodeQL C/C++ SAST, Trivy external scanning, repository-owned dependency delta review, license inventory/allowlisting, immutable action pins, expiring exceptions and negative/live-vulnerability fixtures. GitHub PR78 implements roadmap PR77 and closes `TST019` for the declared CLI/compiler deployment contract through hardened multi-stage images, native amd64/arm64 runtime execution and a live ephemeral Kubernetes qualification gate.

Strong current coverage includes grammar/module conformance, deterministic package resolution, semantic differential execution, staged fuzzing, ASan/LSan/UBSan, TSan, source-aware diagnostics, GCC/Clang qualification, Linux x64/arm64, macOS arm64 and Windows x64 execution, CTest parity, reproducible clean builds, frozen ABI consumers, install/reinstall/uninstall package lifecycle, fail-closed external source/dependency security scanning and restricted container/Kubernetes deployment enforcement.

Production-critical gaps remain for protected signed-release execution, formatter/linter/LSP, live production backend/hardware qualification, complete C3-ECO evidence, production MLIR lowering, measured performance/energy and the final zero-skip release-candidate gate.

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
18. A deployment manifest is not deployment evidence. Container and Kubernetes blockers close only after the exact image is built and executed under the declared security constraints and the live cluster enforces the required policy negatives.
19. Multi-architecture image support requires native or otherwise execution-qualified evidence for each declared architecture. An OCI architecture label alone is insufficient.
20. Final release qualification executes mandatory tests from a clean checkout and reports zero mandatory skips.

## CI profiles

### Pull-request profile

The mandatory DAG runs policy/status guards, grammar/module/semantic gates, fuzz and sanitizer/race safety, toolchain/platform qualification, installed consumers, CTest parity, reproducibility, external security and deployment qualification. The PR78 deployment path runs its static/negative contract and a live Kind cluster on Linux amd64, while the mandatory Linux arm64 lane builds and executes the same production image natively. Normal PR jobs do not receive release OIDC or repository-write permissions.

### Scheduled profile

Extended fuzzing/race stress continues independently. `.github/workflows/security.yml` performs a daily external Trivy rescan so newly published CVEs can fail without waiting for source changes. Scheduled evidence is additive and does not publish the stable PR/push status contexts.

### Release-candidate profile

A release candidate requires every declared platform/toolchain, signed artifacts, checksums, SPDX SBOM, provenance, independent reproducibility evidence, installed consumers, deployment/security/C3-ECO/MLIR qualification, performance/energy evidence and zero mandatory skips.

## Production test exit criteria

ShortHand may move from `controlled_beta` to release candidate only when every row in `tests/coverage/compiler_test_coverage_matrix.tsv` is `implemented` or explicitly declared out of scope in the versioned language contract.

ShortHand may claim enterprise production readiness only when:

1. PR70 and all PR72 through PR86 implementation completion gates are merged, with PR71 CI hygiene already merged,
2. no mandatory test is skipped,
3. all production-supported backends and CPU/GPU/TPU/NPU tiers have live success evidence,
4. all release platforms pass installed-consumer tests,
5. reproducible and signed artifacts are produced and cryptographically verified,
6. measured performance and energy evidence supports any published claim,
7. the final PR86 RC gate reports zero open production blockers.

## Historical strategy audit anchors

The following exact strings are retained only for milestone guards and are not current counts:

- compiler_test_strategy_version: 2026-08-12-pr77
- 17 implemented areas
- 4 partial areas
- 6 open areas
- compiler_test_strategy_version: 2026-08-12-pr76
- 16 implemented areas
- 5 partial areas
- 6 open areas
- compiler_test_strategy_version: 2026-08-11-pr73
- 12 implemented areas
- 6 partial areas
- 9 open areas
- compiler_test_strategy_version: 2026-08-09-pr70

The current strategy is `2026-08-12-pr78`.
