# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-11-pr73
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend or platform was skipped is not production success evidence.

## Audit status at PR73

PR72 is merged and established the beta-0.3 executable semantic oracle across interpreter, `lli` and native execution. PR73 adds continuous adversarial safety evidence around that oracle rather than replacing deterministic tests.

The 27-area production test matrix now records:

- 12 implemented areas,
- 6 partial areas,
- 9 open areas.

PR73 closes the current Ubuntu safety blockers for full sanitizer execution, continuous coverage-guided fuzzing and concurrency race detection. Evidence includes stage-specific scanner/parser/semantic/module/lowering fuzz corpora, ASan/LSan/UBSan-instrumented compiler execution, sanitized PR72 semantic differential coverage, a scheduled extended fuzz workflow, and ThreadSanitizer instrumentation of the runtime implementation plus a dedicated concurrent stress client.

Strong current coverage exists for:

1. beta-0.2 base grammar conformance,
2. beta-0.3 module/package/import syntax,
3. deterministic manifest/resolver/lockfile behavior,
4. bounded malformed-input handling,
5. source-aware coded diagnostics,
6. defined beta-0.3 semantic validation,
7. interpreter/LLVM/native differential correctness,
8. imported function execution equivalence,
9. scanner/parser/semantic/module/lowering coverage-guided fuzzing,
10. ASan/LSan/UBSan safety execution,
11. TSan runtime concurrency stress,
12. runtime ABI symbol stability,
13. Linux packaging consumers,
14. observability adapters,
15. fallback honesty and unavailable-backend behavior,
16. candidate C3-ECO schemas and claim safety,
17. cancellation-safe event-specific CI status publication.

Production-critical gaps remain for:

1. compiler/LLVM/architecture/OS portability,
2. reproducible release artifacts,
3. cross-release ABI and installed-consumer qualification,
4. signed release publication,
5. external vulnerability, SAST and license-policy enforcement,
6. hardened container/Kubernetes deployment,
7. formatter/linter/LSP correctness,
8. real execution qualification for every production-supported backend and CPU/GPU/TPU/NPU execution tier,
9. complete C3-ECO authority evidence,
10. production MLIR generation and lowering,
11. measured energy and performance comparison with equivalent Python workloads,
12. a zero-skip release-candidate blocker gate.

## Required test layers for every implementation PR

Every implementation PR from PR70 and PR72 through PR86 must include all applicable layers below. PR71 is the already-merged CI-hygiene correction. A PR description must explicitly mark non-applicable layers and explain why.

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
12. Fuzz failures record the seed and minimized reproducer.
13. Release qualification includes at least one clean no-cache build.
14. CPU/GPU/TPU/NPU detection is not execution evidence; execution readiness additionally requires compatible runtime/SDK, model, precision, memory and policy.
15. Final release qualification executes mandatory tests from a clean checkout and installed-package consumer.
16. Ordinary parser, semantic or module rejection is not a fuzz failure; sanitizer findings, crashes, signal exits and hangs are.
17. TSan evidence counts only when the runtime implementation itself is instrumented.

## PR73 safety profile

The mandatory pull-request safety profile includes:

- the existing deterministic parser/module/semantic tests,
- `make sanitize`, now including the PR72 semantic differential suite under the sanitized compiler,
- `scripts/check_fuzz_safety.sh` with a fixed seed and bounded libFuzzer run count,
- scanner, parser, semantic, module and lowering seed corpora,
- ASan/LSan/UBSan failure detection,
- `scripts/check_tsan_concurrency.sh`, which instruments both runtime objects and the stress executable,
- retained `/tmp/shorthand_fuzz_artifacts/` crash inputs and test logs.

The scheduled `fuzz-nightly` profile increases the mutation budget while preserving explicit bounds and artifact retention. Every newly fixed fuzz defect must add its minimized reproducer to the appropriate checked-in corpus.

## CI target architecture

`docs/ci_pipeline_architecture.md` is authoritative for the staged pipeline design.

### Pull-request profile

PR73 keeps the stable Ubuntu aggregate contexts while adding first-class fuzz and TSan gates. PR74 will split the monolithic job into a dependency DAG and add the platform/toolchain matrix without weakening these gates.

Required PR evidence includes:

- policy/status/roadmap guards,
- base/module grammar conformance,
- deterministic resolver/lock/multi-file codegen,
- semantic differential execution,
- coverage-guided sanitizer fuzz smoke,
- ThreadSanitizer concurrency stress,
- deterministic sanitizer suite,
- Makefile, CMake and CTest parity,
- structured failure artifacts.

### Scheduled profile

- extended scanner/parser/module/semantic/lowering fuzzing with persisted corpora,
- TSan runtime race stress,
- future scale/platform/backend/hardware qualification as later PRs land,
- dependency/container vulnerability rescans,
- runtime soak tests.

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
