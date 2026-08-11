# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-11-pr73
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend or platform was skipped is not production success evidence.

## Audit status at PR73

The current test foundation is substantially stronger after PR72 semantic equivalence and PR73 safety hardening, but it is not sufficient for an enterprise production-ready claim.

The 27-area production test matrix records:

- 12 implemented areas,
- 6 partial areas,
- 9 open areas.

PR72 closed the beta-0.3 executable semantic contract for the claimed core and established interpreter/`lli`/native differential behavior. PR73 closes the existing sanitizer, continuous-fuzzing and concurrency-race blockers for that baseline with staged libFuzzer targets, ASan/LSan/UBSan runtime stress and an actual TSan race-stress build. PR73 does not convert later platform, release, security, backend, MLIR, performance or energy work into completed evidence.

Strong current coverage exists for:

1. beta-0.2 base grammar conformance,
2. beta-0.3 module/package/import syntax,
3. deterministic manifest/resolver/lockfile behavior,
4. bounded malformed-input handling,
5. source-aware coded diagnostics,
6. executable beta-0.3 semantic validation,
7. interpreter/LLVM/native differential correctness,
8. scanner/parser/module/semantic/lowering coverage-guided fuzzing,
9. ASan/LSan/UBSan compiler and runtime stress,
10. ThreadSanitizer-backed runtime race stress,
11. runtime ABI symbol stability,
12. serialized runtime concurrency behavior,
13. Linux packaging consumers,
14. Prometheus and OTLP adapter behavior,
15. fallback honesty and unavailable-backend behavior,
16. candidate C3-ECO schemas and claim safety,
17. cancellation-safe event-specific CI status publication.

Production-critical gaps remain for:

1. compiler/LLVM/architecture/OS portability,
2. reproducible release artifacts and cross-release ABI/install consumers,
3. signed protected releases,
4. external vulnerability, SAST and license-policy enforcement,
5. hardened container/Kubernetes deployment,
6. formatter/linter/LSP correctness,
7. real execution qualification for every production-supported backend and CPU/GPU/TPU/NPU execution tier,
8. complete C3-ECO language/scoring/auditor evidence,
9. production MLIR generation and lowering,
10. measured energy and performance comparison with equivalent Python workloads,
11. a zero-skip release-candidate blocker gate.

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
| Pipeline behavior | New mandatory jobs fail closed, emit artifacts and cannot be satisfied by cancellation or unconditional skip. |

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
10. Module/package tests prove path confinement, graph determinism, lock freshness and false-success rejection, not only parser acceptance.
11. Deterministic compiler/test failures are fixed, not automatically retried to obtain a green result.
12. Fuzz failures record the target, seed and crash artifact; minimized reproductions are promoted to regression corpora where practical.
13. Release qualification includes at least one clean no-cache build.
14. CPU/GPU/TPU/NPU detection is not execution evidence; execution readiness additionally requires compatible runtime/SDK, model, precision, memory and policy.
15. Final release qualification executes mandatory tests from a clean checkout and installed-package consumer.
16. New parser, module, semantic, lowering or runtime C++ paths must join the appropriate PR73 sanitizer/fuzz/race contract instead of relying only on functional tests.

## CI target architecture

`docs/ci_pipeline_architecture.md` is authoritative for the staged pipeline design.

### Pull-request profile

PR73 preserves the existing mandatory Ubuntu gate while adding independent compiler-stage fuzz, runtime memory-sanitizer and runtime TSan checks. PR74 will split the monolithic job into a dependency DAG without changing the stable aggregate status contexts.

Required PR evidence includes:

- policy/status/roadmap guards,
- base/module grammar conformance,
- deterministic resolver/lock/multi-file codegen,
- semantic differential execution,
- staged libFuzzer ASan/LSan/UBSan smoke,
- runtime ASan/LSan/UBSan stress,
- ThreadSanitizer race stress,
- legacy sanitizer corpus,
- Makefile, CMake and CTest surfaces,
- structured failure artifacts.

### Scheduled profile

- scanner/parser/module/semantic/lowering fuzzing with persisted corpora and recorded seeds,
- TSan runtime suite,
- extended 1k+ module and large-program stress,
- platform/backend/hardware runners,
- dependency/container vulnerability rescans,
- runtime soak and concurrency tests.

### Release-candidate profile

- every declared production platform/toolchain,
- every declared production backend and hardware tier,
- signed artifacts, checksums, SBOM and provenance,
- reproducibility comparison from independent clean builds,
- installed compiler/runtime consumer verification,
- deployment/security/C3-ECO/MLIR qualification,
- performance and energy regression evidence,
- zero mandatory skips.

## Production test exit criteria

ShortHand may move from `controlled_beta` to release candidate only when every row in `tests/coverage/compiler_test_coverage_matrix.tsv` is `implemented` or explicitly declared out of scope in the versioned language contract.

ShortHand may claim enterprise production readiness only when:

1. PR70 and all PR72 through PR86 implementation completion gates are merged, with PR71 CI hygiene already merged,
2. no mandatory test is skipped,
3. all production-supported backends have live success evidence,
4. all declared CPU/GPU/TPU/NPU execution tiers have honest qualification evidence,
5. all release platforms pass installed-consumer tests,
6. reproducible and signed artifacts are produced,
7. measured performance and energy evidence supports any published claim,
8. the final PR86 RC gate reports zero open production blockers.

## Historical strategy audit anchors

The current strategy is `2026-08-11-pr73`. The following exact prior version marker is retained only because older production-readiness guards verify that the earlier strategy milestone has not disappeared:

- compiler_test_strategy_version: 2026-08-09-pr70
