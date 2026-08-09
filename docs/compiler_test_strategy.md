# ShortHand compiler test strategy and production coverage audit

compiler_test_strategy_version: 2026-08-09-pr70
baseline_commit: 2488cab3a3ab29d2c833bccf3aec6aeab6f1652a
language_version: beta-0.3
current_maturity: controlled_beta
production_claim: false

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware AI execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security, and auditable Green AI evidence.

A test passing because a dependency, device, backend, or platform was skipped is not production success evidence.

## Audit status after PR70

The current test foundation is strong enough for controlled beta, but not sufficient for an enterprise production-ready claim.

The 27-area production test matrix now records:

- 5 implemented areas,
- 11 partial areas,
- 11 open areas.

PR70 closes deterministic package-manifest resolution and package-graph construction. Its evidence includes exact module mappings, root-confinement security checks, package/module identity validation, transitive graph loading, cycle/ambiguity/collision rejection, deterministic lock regeneration, stale-lock rejection, multi-file bitcode/native binding, an honest interpreter imported-call rejection, 128-module graph stress, and sanitizer execution. Full interpreter/LLVM/native semantic equivalence remains PR71 and is deliberately not counted as resolver completion evidence.

Strong current coverage exists for:

1. beta-0.2 base grammar conformance,
2. beta-0.3 module, package and import syntax,
3. deterministic module manifest, resolver and lockfile behavior,
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
14. AddressSanitizer, LeakSanitizer and UndefinedBehaviorSanitizer smoke paths.

Production-critical gaps remain for:

1. interpreter, LLVM and native-output semantic equivalence, including imported function execution,
2. complete positive and negative semantic coverage,
3. full-corpus sanitizer execution and continuous fuzzing,
4. ThreadSanitizer-backed concurrency verification,
5. compiler, LLVM, architecture and operating-system portability,
6. reproducible release artifacts,
7. real execution qualification for every supported AI backend,
8. external vulnerability, SAST and license-policy enforcement,
9. hardened container and Kubernetes deployment,
10. formatter, linter and LSP protocol correctness,
11. production MLIR generation and lowering,
12. measured energy and performance comparison with equivalent Python workloads,
13. a final release-candidate blocker gate.

## Required test layers for every implementation PR

Every PR from PR70 onward must include all applicable layers below. A PR description must explicitly mark non-applicable layers and explain why.

| Layer | Required evidence |
| --- | --- |
| Contract | Updated specification, compatibility note, public API or schema when behavior changes. |
| Unit | Deterministic tests for new algorithms, validation rules and failure states. |
| Positive integration | At least one end-to-end success fixture proving the implemented behavior. |
| Negative integration | Invalid inputs, unsupported states and boundary failures must be rejected with stable evidence. |
| Regression | A fixture reproducing any bug fixed by the PR. |
| Sanitizer | New native C++ paths execute under the applicable sanitizer suite. |
| Security | Untrusted input, path, network, dependency or privilege boundaries receive misuse tests. |
| Portability | Platform-sensitive code has at least two independent toolchain or platform checks when available. |
| Performance | Hot paths receive a bounded regression benchmark or an explicit non-performance rationale. |
| Documentation guard | Roadmap, feature status and test coverage matrix are updated in the same PR. |

## Test quality rules

1. Tests must verify observable behavior, not only grep for source text.
2. Optional SDK tests may skip when infrastructure is absent, but a skip cannot close a production blocker.
3. Production-supported backends require real model execution and numerical-output validation.
4. Negative tests must assert stable diagnostic codes, status classes or structured evidence.
5. Golden outputs must be normalized for paths, timestamps and nondeterministic identifiers.
6. Network tests must use bounded timeouts and isolated loopback or ephemeral environments.
7. Concurrency tests must use ThreadSanitizer or an equivalent race detector before the runtime is called production safe.
8. Energy claims require repeatable measurements, hardware metadata, uncertainty and a comparable baseline.
9. No CI gate may disable LeakSanitizer, weaken assertions or convert a required production test into an unconditional skip.
10. Module/package tests must prove path confinement, graph determinism, lock freshness and false-success rejection rather than only parser acceptance.
11. The final release gate must execute all mandatory tests from a clean checkout and installed-package consumer.

## CI target architecture

### Required pull-request CI

- Ubuntu x86-64 build with GCC and Clang
- base and module-extension grammar conformance
- deterministic module resolver, lockfile and multi-file codegen gate
- parser, diagnostics and semantic conformance
- interpreter, LLVM and native differential suite
- ASan, LSan and UBSan full language corpus
- deterministic build and generated-parser consistency
- package, install and downstream consumer tests
- security and policy gates

### Required scheduled CI

- scanner, parser and semantic fuzzing with persisted corpora
- ThreadSanitizer runtime suite
- optional live SDK and hardware runners
- long-running module graph and large-program stress
- dependency and container vulnerability rescans

### Required release CI

- Linux x86-64 and arm64
- macOS arm64 and x86-64 where supported
- Windows x86-64
- signed artifacts, checksums, SBOM and provenance
- reproducibility comparison from independent clean builds
- installed compiler and runtime consumer verification
- performance and energy regression evidence

## Production test exit criteria

ShortHand may move from `controlled_beta` to release candidate only when every row in `tests/coverage/compiler_test_coverage_matrix.tsv` is `implemented` or explicitly declared out of scope in the versioned language contract.

ShortHand may claim enterprise production readiness only when:

1. all PR70 through PR85 completion gates are merged,
2. no mandatory test is skipped,
3. all production-supported backends have live success evidence,
4. all release platforms pass installed-consumer tests,
5. reproducible and signed artifacts are produced,
6. measured performance and energy evidence supports the published claim,
7. the final RC gate reports zero open production blockers.
