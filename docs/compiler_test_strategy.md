# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-09-pr70-resume
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

A test passing because a dependency, device, backend or platform was skipped is not production success evidence.

## Audit status at resumed PR70

The current test foundation is strong enough for controlled beta, but not sufficient for an enterprise production-ready claim.

The 27-area production test matrix records:

- 5 implemented areas,
- 11 partial areas,
- 11 open areas.

PR70 closes deterministic package-manifest resolution and package-graph construction. Its evidence includes exact module mappings, root-confinement security checks, package/module identity validation, transitive graph loading, cycle/ambiguity/collision rejection, deterministic lock regeneration, stale-lock rejection, multi-file bitcode/native binding, honest interpreter imported-call rejection, 128-module graph stress and sanitizer execution. Full interpreter/LLVM/native semantic equivalence remains PR72 and is deliberately not counted as resolver completion evidence.

PR71 is a merged CI-policy correction. It prevents cancelled superseded runs from publishing false terminal statuses onto a shared commit SHA. The remaining implementation roadmap is PR72 through PR86.

Strong current coverage exists for:

1. beta-0.2 base grammar conformance,
2. beta-0.3 module/package/import syntax,
3. deterministic manifest/resolver/lockfile behavior,
4. bounded malformed-input handling,
5. source-aware coded diagnostics,
6. selected semantic rejection cases,
7. LLVM metadata, multi-file function binding and native runtime linkage,
8. runtime ABI symbol stability,
9. serialized runtime concurrency behavior,
10. Linux packaging consumers,
11. Prometheus and OTLP adapter behavior,
12. fallback honesty and unavailable-backend behavior,
13. candidate C3-ECO schemas and claim safety,
14. ASan, LSan and UBSan smoke paths,
15. cancellation-safe event-specific CI status publication.

Production-critical gaps remain for:

1. interpreter/LLVM/native semantic equivalence including imported function execution,
2. complete positive and negative semantic coverage,
3. full-corpus sanitizer execution and continuous fuzzing,
4. ThreadSanitizer-backed concurrency verification,
5. compiler/LLVM/architecture/OS portability,
6. reproducible release artifacts,
7. real execution qualification for every production-supported backend and CPU/GPU/TPU/NPU execution tier,
8. external vulnerability, SAST and license-policy enforcement,
9. hardened container/Kubernetes deployment,
10. formatter/linter/LSP correctness,
11. production MLIR generation and lowering,
12. measured energy and performance comparison with equivalent Python workloads,
13. a zero-skip release-candidate blocker gate.

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

## CI target architecture

`docs/ci_pipeline_architecture.md` is authoritative for the staged pipeline design.

### Pull-request profile

PR70 preserves the existing mandatory Ubuntu gate while adding independent CI-policy and module-resolver checks. PR74 will split the monolithic job into a dependency DAG without changing the stable aggregate status contexts.

Required PR evidence grows to include:

- policy/status/roadmap guards,
- base/module grammar conformance,
- deterministic resolver/lock/multi-file codegen,
- semantic differential execution after PR72,
- sanitizer and fuzz smoke after PR73,
- Makefile, CMake and CTest parity,
- CPU runtime qualification,
- structured failure artifacts.

### Scheduled profile

- scanner/parser/module/semantic/lowering fuzzing with persisted corpora,
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
