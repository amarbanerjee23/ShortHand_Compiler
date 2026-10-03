# LE1 implementation and continuous evidence

Implementation baseline: merged PR114 (`0860938d384259102ef3bb8474f2b2472a561a05`).

Implementation PR: [PR115](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/115).

Testing and benchmark report: [latency_energy_test_and_benchmark_results.md](latency_energy_test_and_benchmark_results.md).

Follow-up after PR116: the serialized C bridge retains its runtime/backend registry and one declaration descriptor keyed by all three registration names. Non-idempotent registrations invalidate the descriptor and prepared session; reset releases all retained state. Every call refreshes environment policy, probes and qualifies the current hardware route, checks input/output contracts, and rereads the entire model. The model comparison buffer is reused with a 16 MiB limit alongside the immutable 16 MiB session snapshot. No external tensor pointer is retained. New alternating-registration and repeated policy-change tests are mandatory in both native and sanitizer CI. The committed benchmark report is now required in every PR, with the exact base SHA.

This change implements the first runtime and evidence portion of LE1. It does not establish a language-wide latency, energy or power saving. CI now supplies measurements and explicit gaps for each revision so subsequent optimization decisions can use retained evidence.

## Implemented behavior

| Change | Contract | Verification |
|---|---|---|
| Prepared execution for the generated C ABI | One cached CPU ONNX FP32 session; full declared model/tensor identity and exact file bytes checked before reuse | Real SDK cache tests; every compiled head sample ends with `hit=true` and `preparations=1` |
| Safe model replacement | Read at most 16 MiB; construct the session from the exact bytes read. Replacements, corruption and deletion cannot reuse stale contents. Models requiring external data retain path-based inference | Valid replacement with preserved mtime, corrupt/deleted model recovery, external weights changed between calls |
| Idempotent registration | Identical model/tensor registration preserves preparation. Changed registration or reset invalidates it; public C ABI is unchanged | Real C ABI regression and existing ABI/installed-consumer gates |
| Preserved routing and ownership | Hardware routing and production qualification run on every inference; existing facade lock serializes cache, reset and registration | Live concurrent inference and policy-denial regression |
| Bounded bridge input reuse | Retain at most 65,536 floats of bridge input scratch; larger legacy requests use temporary storage. Prepared model snapshot is capped at 16 MiB | Bound in runtime; existing generated tensor limit; shape/capacity tests. ORT's internal allocations are not claimed to be eliminated or fully bounded by these limits |
| Resident input workspace | An explicit workspace belongs to one caller/worker. A pointer/length overload consumes dataset slices directly; real input slots are overwritten and padded tail slots zeroed on every call | Buffer-address reuse, full/tail calls, failure rollback/recovery and separate concurrent workspaces |
| Source FP32 lowering | Tensor/model `float` maps to FP32 in SemanticIR, consistent with the existing runtime. Scalar language floats remain FP64 | Actual `.short` → MLIR → LLVM → native execution |

The existing independent C++ resident control already reuses its input/output buffers and tensor wrappers. That control stays in the comparison. The existing vector host API remains available with a request-local workspace. Serving requests continue to use local storage; automatic worker-pool workspace ownership is not introduced here.

## What every completed CI run collects

`ci.yml` calls `runtime-profile.yml` as a required reusable workflow for every push and pull request, without path filters. The final `ci / ubuntu` status requires it to succeed. `release.yml` invokes the same workflow and publication depends on its result. A manual runtime-profile run is also available. New pushes do not automatically cancel earlier CI evidence runs.

| Evidence | Workload and boundary | Output |
|---|---|---|
| Cache/workspace correctness | Real ONNX cache tests, host fault injection and full digit application boundary tests | `cache-tests.txt`, `workspace-tests.txt`, `application-tests.txt` |
| Compiled FP32 execution | Actual `.short` source called repeatedly by a native harness; zero-input MatMul+Add; batch 1/16/32, one thread; every output score checked against a pinned Python ORT oracle | `generated-infer/manifest.json`, `.short`, LLVM IR, object/binaries, raw JSON/stdout/stderr/commands and `GENERATED.md` |
| Compiled same-runner controls | The same generated object linked to head and exact base runtime archives, plus an independent prepared C++/ORT control | Cold-session observations and block elapsed times; head/base and head/direct descriptive ratios |
| Resident head/base comparison | All 1,797 held-out Optdigits images; cells `(1,1)`, `(16,1)`, `(32,1)`, `(16,2)`, `(16,4)`; balanced ordering and quality/numerical checks | `pr-runtime-delta/` |
| Resident native/Python controls | Same full dataset, all five cells, pinned environment and balanced independent controls | `resident-baselines/` |
| Diagnostic stage clocks | All five resident cells; opt-in instrumented profiling | `b*-t*/raw-profile.json` and reports |
| Energy and power observations | Separate instrumented process runs. Accessible RAPL/AMD counters report E1 component joules and, for compiled probes, average component watts over the same process window | `ci-energy-probe.json`, `ci-energy/`, compiled `energy-*.energy.json` |
| Revision and integrity | Exact source/base SHA, compiler/SDK/binary/model/data hashes, environment, command lines, raw logs and replay digests | `run-evidence.json`, per-experiment manifests, build caches/logs, hardware and pip metadata |

For PRs the base is the PR base SHA; for existing-branch pushes it is the event's before SHA. New branches, tag pushes without a before SHA, and manual runs use the checked-out commit's first parent. A compiled runtime comparison uses the **head compiler for both runtime archives**: it isolates runtime changes and does not measure compiler-codegen changes.

The compiled synthetic functional unit is one numerically verified FP32 output vector. It is not a correctly classified digit. Source-level tensor declarations currently initialize zeros; populating arbitrary real source tensors is still required before claiming full generated-source digit parity. Compiled thread-2/thread-4 cells are explicitly unsupported, while the resident host measures them. Source model quality metadata is not treated as observed accuracy.

The compiled timed boundary includes initialization, registration, validation, routing, telemetry/logging and output verification. The direct prepared control includes its invocation and the same input/output verification; the reported ratio therefore exposes runtime overhead. Cold session setup is separate from resident block time. These block averages do not represent request p95/p99 latency.

## Claims and failure rules

- Missing/failed correctness, replay or required data fails the evidence job and mandatory CI status. An always-run finalizer retains an incomplete index and available files on failure.
- Noisy hosted-runner timing ratios are descriptive. They do not automatically authorize a performance claim or satisfy controlled-runner parity targets.
- Hardware counter absence produces **unavailable** with null joules/watts. It does not become zero energy or a latency-derived estimate. E2/E3 remain estimates and require the existing matching-profile rules; this change supplies no invented calibration profile.
- E1 measures a CPU package/socket, including co-tenant/background activity. It is not whole-system AC energy, process-attributed energy, or a certification measurement. Generated process energy includes cold setup, warmups, all verified calls and teardown; its denominator includes those verified calls.
- Shorter latency does not prove lower average power. Lower watts alone do not prove lower joules per completed task. Numerical and quality validity must accompany any comparison.
- All reduction-claim authorization flags remain false. A future claim must cite exact retained artifacts, matched workloads and boundaries, uncertainty and the relevant measurement class.
- Artifacts are named `runtime-profile-<run_id>-<attempt>` and retained for 90 days. Archive a cited bundle before expiry if a claim must outlive that window. Manual cancellation, runner loss or a failure before checkout can prevent finalization; an absent/incomplete bundle is never evidence of a successful comparison.

## Reproduction

Use the pinned Linux x64 ONNX SDK installed by `scripts/install_ci_onnxruntime_cpu.sh`, LLVM/MLIR 18 and the hash-locked Python requirements. Build `short_hand`, `shorthand_runtime` and `shorthand_ai_qualify` with MLIR and ONNX enabled. Build the base runtime/qualifier in a separate worktree using the same SDK and compiler flags.

```bash
CXX=clang++-18 python3 tests/ai_application/test_compiled_cache.py build-head "$SDK"
baseline-venv/bin/python experiments/energy/generated_infer_evidence.py \
  --build build-head --base-build build-base --sdk "$SDK" \
  --head-sha "$HEAD_SHA" --base-sha "$BASE_SHA" --output generated-evidence
python3 experiments/energy/generated_infer_evidence.py --output generated-evidence \
  --replay-sha256 "$(cat generated-evidence/manifest.sha256)"
```

The workflow is the authoritative full recipe. Local smoke runs validate mechanics; quantitative conclusions should reference a clean, exact-revision retained run.

## First hosted observations, verified 2026-10-02

These observations belong to implementation commit `55a9843bd58569a2a2aa68dcbcaf17841d28f732`, compared with baseline `0860938d384259102ef3bb8474f2b2472a561a05`. Both the [pull-request CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36745333166) and the [push CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36745334248) passed, including the required evidence job. The corresponding tooling and experiment-results workflows also passed. Later documentation commits do not change the revision to which these measurements apply.

| Verified bundle | Artifact | ZIP SHA-256 |
|---|---|---|
| Pull request | [runtime-profile-36745333166-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36745333166/artifacts/11111384752) | `77173342aa5caa371069c4df491b4d45423605d64d55022945b369e593ed4113` |
| Push | [runtime-profile-36745334248-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36745334248/artifacts/11112561875) | `46647c5f53aa94216758c5673b358858d9c0a8ca8d3ac17fa5ba1319f9176e66` |

Both downloaded ZIP digests matched GitHub's artifact metadata. All 1,180 files indexed by each `run-evidence.json` matched their recorded hashes; both indices reported complete evidence with no missing items. Compiled and resident-control analysis replayed successfully from both bundles.

The compiled synthetic lane reports the ratio of mean head latency to mean baseline/direct latency from 16 measured blocks per implementation and cell. Smaller ratios are better. Values below are rounded from `generated-infer/report.json`; do not pool the two runner environments.

| Cell | Head/base, PR | Head/base, push | Head/direct C++ ORT, PR | Head/direct C++ ORT, push |
|---|---:|---:|---:|---:|
| b1-t1 | 0.170 | 0.202 | 28.162 | 31.840 |
| b16-t1 | 0.190 | 0.205 | 16.456 | 18.670 |
| b32-t1 | 0.190 | 0.211 | 10.445 | 13.392 |

For this zero-input workload and measured boundary, mean latency was 78.9–83.0% lower than the old repeated-session runtime (`100 * (1 - head_base_ratio)`). It remained 10.4–31.8 times the direct prepared C++/ORT latency. These observations neither establish source-level digit quality nor meet the plan's direct-control parity targets.

The real-digit resident lane uses six paired blocks per cell. The following positive reductions are the median of paired reductions, not the ratio of separately calculated latency medians. Raw paired ratios, including individual regressions, remain in the `pr-runtime-delta/` report.

| Cell | Paired median reduction, PR | Paired median reduction, push |
|---|---:|---:|
| b1-t1 | 1.32% | 0.39% |
| b16-t1 | 4.46% | 7.20% |
| b32-t1 | 4.67% | 6.08% |
| b16-t2 | 3.69% | 6.53% |
| b16-t4 | 5.16% | 8.08% |

These are descriptive shared-runner results, with no confidence bound establishing a repeatable reduction. Some individual paired blocks were slower at head. No latency claim is automatically authorized.

**Energy and power remain unproven.** Neither runner exposed usable package/socket energy counters or a matching calibration profile. Resident and compiled energy reports recorded unavailable energy with null joules and watts. No measured or estimated energy saving, lower-power claim, or whole-language performance claim follows from these runs. Preserve the cited bundles before the 90-day retention period expires if these observations will be used beyond that period.

## Remaining LE1 work

The plan remains in progress: real source tensor population/quality parity, compiled multi-thread controls, automatic serving-worker scratch ownership, allocation/lock instrumentation, deferred telemetry/export behavior, independent ablations, sanitizer/soak closeout and the prescribed parity targets. This PR supplies a working measurement and correctness gate for that work. LE2–LE4 remain separately planned; neither lower precision nor AOT is enabled by this change.
