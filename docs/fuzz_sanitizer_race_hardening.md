# Fuzz, sanitizer and concurrency race hardening

fuzz_safety_contract_version: shorthand.fuzz.sanitizers.v1
runtime_tsan_contract_version: shorthand.runtime.tsan.v1
language_version_change: none
runtime_abi_change: none
production_claim: false
production_claim_boundary: fuzz_and_tsan_evidence_is_not_cross_platform_release_or_live_backend_qualification

PR73 makes memory/undefined-behavior fuzzing and runtime race detection mandatory evidence rather than optional developer tooling. It preserves the beta-0.3 syntax and PR72 execution semantics.

## Mandatory PR fuzz stages

`scripts/check_fuzz_sanitizers.sh` builds four libFuzzer entry targets from the production scanner/parser/resolver/semantic/lowering sources:

| Target | Stage exercised | Seed corpus |
| --- | --- | --- |
| `parser` | scanner, parser, AST construction and parser resource guards | `tests/fuzz/corpus/parser` |
| `module` | package-manifest parsing, root confinement and module lookup | `tests/fuzz/corpus/module` |
| `semantic` | parser plus semantic validation and diagnostics | `tests/fuzz/corpus/semantic` |
| `lowering` | parser, semantics and LLVM lowering construction | `tests/fuzz/corpus/lowering` |

Every PR invocation uses Clang libFuzzer with AddressSanitizer and UndefinedBehaviorSanitizer instrumentation. LeakSanitizer is enabled through `ASAN_OPTIONS=detect_leaks=1`. Sanitizer recovery is disabled. A non-zero fuzzer result or sanitizer marker is a hard failure.

The bounded PR profile records these deterministic defaults:

- seed: `1337`,
- run budget: `256` executions per stage,
- maximum fuzz input: `4096` bytes,
- per-input timeout: `5` seconds,
- RSS ceiling: `2048` MiB.

These values may be overridden only by explicit environment variables for a larger campaign. Reducing them is not valid production evidence.

## Corpus policy

Checked-in corpus entries contain both valid and malformed language/package inputs. New deterministic fuzz findings must be minimized where practical and promoted into the appropriate checked-in corpus or a focused regression fixture after root-cause repair.

The scheduled `.github/workflows/fuzz.yml` campaign runs the same four targets for 180 seconds per stage. It records the Actions `run_number` as the fuzz seed so a campaign is reproducible while remaining within libFuzzer's integer seed range.

## Crash artifacts and replay

Fuzzer failures are written under `/tmp/shorthand_fuzz_artifacts` in CI and uploaded even when the job fails. The gate prints the target, seed and artifact directory.

A saved artifact is replayed with:

```text
bash scripts/replay_fuzz_reproducer.sh parser /path/to/crash-artifact
```

Use `--minimize` to invoke libFuzzer crash minimization with the identical ASan/LSan/UBSan build policy:

```text
bash scripts/replay_fuzz_reproducer.sh lowering /path/to/crash-artifact --minimize
```

A replay that never reaches the target is reported as a build/harness failure rather than being misreported as a reproduced compiler crash.

## ThreadSanitizer contract

`scripts/check_thread_sanitizer.sh` compiles the actual runtime implementation and public thread-safe facade with ThreadSanitizer, PIE and pthread support. `tests/runtime/tsan_runtime_stress.cpp` overlaps:

- model/tensor/contract/measurement registration,
- inference and runtime counters,
- observability/Prometheus/OTLP snapshots,
- returned C-string snapshot lifetime across mutation from another thread,
- runtime reset while snapshot readers are active.

Any `ThreadSanitizer` warning, data race, lock-order inversion, non-zero exit, timeout or functional consistency failure fails the gate. This does not replace the existing runtime functional thread-safety test; it is additive race evidence.

## Existing sanitizer suite remains mandatory

PR73 does not replace `make sanitize`. The legacy ASan/UBSan language suite remains mandatory and now explicitly enables leak detection and fail-fast sanitizer options. The first-class libFuzzer and TSan gates run separately so a pass cannot be hidden inside aggregate output.

## CI evidence

The normal Actions workflow contains separate mandatory steps:

- `Fuzz ASan LSan UBSan compiler stages`,
- `ThreadSanitizer race stress`,
- the existing `Sanitizer tests` step.

The scheduled extended workflow is additional evidence, not a substitute for PR gates. PR74 owns the broader toolchain/platform CI DAG and CTest parity.

## Claim boundary

Passing PR73 demonstrates bounded sanitizer-backed adversarial testing and a clean TSan stress run on the declared Ubuntu CI environment. It does not prove absence of all memory bugs or races, cross-platform correctness, hardened hostile-input security, live accelerator execution, or lower energy than Python. Those remain later production-readiness gates.
