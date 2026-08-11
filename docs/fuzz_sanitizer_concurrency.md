# Fuzz, sanitizer and concurrency safety contract

fuzz_safety_contract_version: 1.0.0
fuzz_safety_status: mandatory_pr_smoke_and_scheduled_extended
production_claim: false
pr_owner: PR73

## Purpose

PR73 turns ShortHand safety testing from finite regression-only coverage into a continuously mutating safety contract. It does not replace deterministic conformance tests. Fuzzing is an additional adversarial layer that searches for crashes, hangs, leaks, undefined behavior and race defects while existing grammar, semantic, module, runtime and code-generation assertions remain mandatory.

## Coverage-guided stages

`scripts/check_fuzz_safety.sh` builds the real compiler with AddressSanitizer and UndefinedBehaviorSanitizer, enables leak detection through ASan/LSan, then executes libFuzzer-compatible targets for:

1. scanner input,
2. parser input,
3. semantic execution,
4. package/module manifest resolution,
5. LLVM bitcode lowering.

The stage driver is `tests/fuzz/FuzzSubprocess.cpp`. Each stage has a checked-in seed corpus under `tests/fuzz/corpus/`.

Malformed source, semantic rejection and invalid package manifests are expected outcomes and may exit non-zero. They are not fuzz failures. The following are always failures:

- ASan, LSan, UBSan or TSan diagnostics,
- segmentation faults or signal-style process termination,
- compiler hangs beyond the stage timeout,
- libFuzzer crashes,
- launcher/tool failures that prevent the target from being exercised.

Every PR runs a bounded deterministic smoke with an explicit seed. The scheduled `fuzz-nightly` workflow runs an extended corpus mutation budget and uploads fuzz outputs and crash artifacts. A finding must be fixed at root cause and its minimized reproducer must be retained as a regression seed before closure.

## Sanitizer contract

PR73 preserves the existing `make sanitize` gate and adds coverage-guided execution of an ASan/LSan/UBSan compiler. Existing parser robustness, module resolution, semantic differential and enterprise tests are not weakened or skipped.

Sanitizer success means no detected memory safety error, leak or undefined operation in the paths exercised by the mandatory corpus. It is not a proof that undiscovered memory defects do not exist.

## ThreadSanitizer contract

`scripts/check_tsan_concurrency.sh` rebuilds the runtime library itself with ThreadSanitizer instrumentation, links `tests/runtime/runtime_tsan_stress.cpp` with the same instrumentation and stresses concurrent registration, inference, observability snapshots, counters and resets.

Instrumenting only the test client is insufficient; the runtime implementation must be instrumented for race evidence to count. TSan findings, deadlock markers or non-zero stress exits fail the gate. The script restores the normal runtime build after the race test so later CI stages do not inherit TSan build artifacts.

## Bounded execution and reproducibility

PR smoke uses a fixed seed and bounded run count. The run count may be increased through `SHORTHAND_FUZZ_RUNS`, and the random seed through `SHORTHAND_FUZZ_SEED`. Inputs are capped and every compiler subprocess has a hard timeout. Fuzz artifacts are written beneath `/tmp/shorthand_fuzz_artifacts/` and uploaded by CI.

Scheduled fuzzing is intentionally longer than PR smoke but is still bounded. PR74 will add the platform/toolchain matrix; PR73 qualifies the safety machinery on the current Ubuntu/Clang CI platform only.

## Claim boundary

Passing PR73 means the current compiler and runtime pass the defined ASan/LSan/UBSan coverage-guided smoke, the TSan race stress and the existing deterministic sanitizer suite on the qualified CI platform. It does not establish cross-platform memory safety, formal correctness or production readiness. Those claims remain blocked by PR74 through PR86.
