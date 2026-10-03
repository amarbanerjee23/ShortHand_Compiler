# ShortHand testing, experimentation and benchmark results

This report records the testing expansion and retained benchmark evidence for the prepared-runtime work. The historical PR115 and PR116 measurements use different comparison baselines. Their ratios must not be treated as repeats of the same experiment. The initial implementation is from [PR115](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/115), with the testing/reporting expansion in [PR116](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/116).

## Current PR evidence

Comparison base: `a5a3e020943ef2f9005b6a44f4c8024a8b58ef25`

This follow-up reuses the C bridge's runtime/backend registry and a single parsed declaration descriptor. The descriptor key includes model, input and output names; changed registrations and reset invalidate it. Environment policy is refreshed and hardware routing is still evaluated on every call. The prepared cache reuses its model-comparison storage but still reads and compares every model byte, including replacements with unchanged timestamps. Two bounded model buffers retain up to 32 MiB; reset releases them. This trades retained scratch memory for avoiding repeated comparison-buffer allocation and initialization.

The public C ABI, finite checks, output ownership and failure rollback remain intact. This slice does not implement borrowed tensor pointers or bind ONNX outputs directly into the caller's buffer. Such binding would require a transactional runtime-owned workspace to preserve the current failure guarantee.

| Validation | Current result |
|---|---|
| Python energy/evidence regression discovery | 54 tests passed locally |
| Committed-report CI contract | 6 tests passed locally: committed update, missing/worktree-only report, stale base, symlink, truncated report and invalid revision |
| Native runtime and qualification build | Passed locally with Clang 18 and the real ORT SDK |
| Real ONNX application | 42 boundary cases passed locally; all 1,797 real dataset rows checked (`live_onnx=1`) |
| Extended native cache and sanitizer tests | Passed locally: all cache boundaries, registration/policy regressions, lifecycle and workspace checks; ASan/UBSan passed. Local LSan could not inspect `/proc` and the script explicitly reran with leak detection disabled; hosted LSan remains required when supported |
| Same-run head/base and direct C++/ORT measurements | Pending this PR's retained CI run; historical tables below are not measurements of this change |
| Energy/power improvement | Unproven; local probe found no RAPL/AMD domains and no matching E2/E3 profile, so the highest available evidence class is `null` |

The new native regression alternates 100 calls across model/input/output names, independently tests mismatched input/output ranks with a fixed model name, rejects changed input shapes without touching the output, and repeats eight deny/recover policy cycles on the retained runtime. The rank test exposed a pre-existing uncached-fallback gap: an incompatible concrete output shape could still return success after preparation failed. This PR rejects that mismatch before copying any output. The test is retained in native and sanitizer CI. CI now requires this Markdown document to change in every PR and to identify that PR's exact base commit. The generated per-run report uses the selected base revision rather than assuming it reconstructs the session on every call.

The authoritative pre-expansion hosted bundle is [runtime-profile-36963499649-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36963499649/artifacts/11209171583). Its ZIP SHA-256 is `4c0c86316bc608f77f2ca6b437583e2af87b3de012cb9978543e1965858792bd`. Under the evidence schema used by that run, `run-evidence.json` reported `status=complete`, `missing=[]`, and 1,180 indexed files. Every indexed file hash was verified after download. The expanded checks in this change are required for subsequent runs and are reported separately below.

The workflow keeps latency, power and energy claims disabled. The results below are evidence for the tested workload and boundary. They do not establish a language-wide result, a lower-power result, or superiority to optimized C++/ORT.

## Expanded tests

The follow-up test expansion adds checks at the C ABI, resident workspace, evidence parser and sanitizer boundaries. The tests are wired into `runtime-profile.yml`, which is required for every push and pull request.

| Area | Coverage added | Result |
|---|---|---|
| Real ONNX prepared cache | 2,000 sequential calls with idempotent re-registration; exact model-content replacement with preserved mtime; reset and re-registration invalidation; corruption/deletion/recovery; external-weight deletion and recovery | Pass locally and retained as a required CI gate |
| Concurrent runtime lifecycle | 4 worker threads perform 4,000 inference calls while a fifth thread performs 64 resets and registrations; successful calls and registration gaps are checked for valid statuses, counts and output canaries | Pass; all 4,000 calls accounted for |
| C ABI boundary matrix | Null/empty names, missing model/tensors, null buffers, invalid sizes/capacities, null output count, aliased buffers, spare-capacity canaries and extreme finite/nonfinite values | Pass |
| Bridge input boundary | 65,536-float retained scratch limit and 65,537-float temporary path, repeated three times with output canaries | Pass |
| Prepared snapshot bound | Exactly 16 MiB is cacheable; 16 MiB plus one byte uses the uncached path | Pass |
| Resident workspace | 2,000 alternating full/tail calls; all real slots rewritten and tail slots zeroed; 80 rejected-input rollback checks; concurrent independent workspaces | Pass |
| Evidence parser | 12 generated-evidence tests cover boolean/nonfinite/missing values, process and meter failure, energy denominator, wrong digest, invalid protocol, duplicates, unknown observations, unsafe paths and symlinks | Pass locally |
| Evidence index | Missing/failed evidence, unavailable energy, false authorization flags and malformed JSON are fail-closed | Pass locally |
| Markdown report | Report refuses unsuccessful reports and refuses numeric energy values when the meter is unavailable | Pass locally |
| Sanitizers | Prepared cache and workspace built with ASan/UBSan. LeakSanitizer runs when the runner permits `/proc` inspection; managed local execution reported its restriction and completed the ASan/UBSan rerun | Pass; local LSan unavailable due runner restriction |

The local evidence regression command ran 45 Python tests across the profile, baseline, energy, comparison, runtime-delta, energy-status, evidence-index, generated-evidence and Markdown-report suites. The existing native qualification suite also passed with the real ONNX SDK and `live_onnx=1`.

## Hosted compiled-runtime experiment

The compiled lane uses a real `.short` source program and a synthetic zero-input FP32 MatMul+Add graph. It validates every output vector against an independent pinned ONNX Runtime oracle. It measures one-thread batch sizes 1, 16 and 32. The timed boundary includes source initialization, registration, runtime validation, routing, telemetry/logging and output verification. The direct control uses a prepared C++/ORT invocation with matching output verification.

| Cell | Prepared head mean (ms/vector) | Head/base ratio | Head/direct prepared C++/ORT |
|---|---:|---:|---:|
| b1-t1 | 0.028256 | 0.1715 | 28.334× |
| b16-t1 | 0.001957 | 0.1947 | 17.746× |
| b32-t1 | 0.000939 | 0.1908 | 10.251× |

Against the repeated-session baseline, the prepared head ratio corresponds to 80.5–82.9% lower measured latency for these probes. The head path remains 10.3–28.3 times slower than direct prepared C++/ORT. This exposes runtime and generated-boundary overhead that remains to be reduced. The compiled lane is not a digit-accuracy benchmark and does not measure multi-threaded generated execution.

## Hosted resident real-digit experiment

The resident lane evaluates all 1,797 held-out Optdigits images. Head and base execute the same model, dataset, precision, backend, batch/thread cell and quality checks. Six balanced paired blocks are retained per cell.

| Cell | Head median (µs/image) | Base median (µs/image) | Paired median reduction | Paired ratio range |
|---|---:|---:|---:|---:|
| b1-t1 | 1.896141 | 1.892991 | 0.15% | 0.9515–1.0759 |
| b16-t1 | 0.401278 | 0.415128 | 5.83% | 0.8437–1.0916 |
| b32-t1 | 0.356938 | 0.366566 | 0.86% | 0.9189–1.0283 |
| b16-t2 | 0.390400 | 0.412192 | 6.18% | 0.9177–0.9781 |
| b16-t4 | 0.393680 | 0.416144 | 4.98% | 0.9420–1.0323 |

The paired result is descriptive. Individual blocks include regressions, especially on shared hosted runners. No confidence-bound latency claim is authorized.

## Resident controls

The same hosted bundle includes independent native C++/ORT and Python/ORT controls. Values are resident median microseconds per image from six blocks; they are not energy measurements.

| Cell | ShortHand native | C++/ORT control | Python/ORT control |
|---|---:|---:|---:|
| b1-t1 | 1.850874 | 1.282920 | 16.584142 |
| b16-t1 | 0.390342 | 0.290399 | 1.362800 |
| b32-t1 | 0.339801 | 0.268091 | 0.821868 |
| b16-t2 | 0.392837 | 0.292844 | 1.380353 |
| b16-t4 | 0.389666 | 0.293632 | 1.379824 |

ShortHand's resident host remains slower than the direct C++/ORT control in these cells. The optimization slice improves the host path relative to its previous ShortHand baseline while leaving a measurable control gap.

## Energy and power result

The hosted runner exposed no readable CPU package/socket energy counter and no matching E2/E3 calibration profile. The retained summary is:

```text
available=false
evidence_class=null
hardware_measured_joules=null
joules_per_task=null
average_component_watts=null
claim_authorized=false
```

The evidence pipeline records this as an unavailable measurement. It does not infer joules from latency, report zero joules, or authorize lower-power or lower-energy language. A future runner with a valid counter or matched calibration profile can populate the same schema without changing the benchmark protocol.

## Latest expanded hosted validation

The expanded checks passed in [PR116's required CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36970175339) at revision `996550347dbd9bbfae4f79e55340ebf697ea2295`. The retained [runtime-profile artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36970175339/artifacts/11211791482) has ZIP SHA-256 `a7493b45edba7a046c744138c0d6a5fbb21210eed9f66df4f9187a75c29e7edb`. Its finalized evidence index reports `status=complete`, `missing=[]`, and 1,183 indexed files; every indexed file hash was verified after download.

The expanded hosted run recorded the following additional pass evidence:

- Real ONNX cache tests passed with 2,000 sequential calls, 4,000 concurrent calls during 64 resets/registrations, both bridge boundaries, both snapshot limits, invalid-argument/output-canary checks, and external-weight recovery.
- The prepared cache and workspace ASan/UBSan step passed. LeakSanitizer was enabled by the hosted runner, so the local `/proc` restriction did not apply to this retained run.
- Evidence regression tests, report generation, application correctness, resident workspace checks and energy availability validation all passed.

The latest compiled probe was materially different from the earlier PR115 run on the shared hosted environment:

| Cell | Head mean ms/vector | Head/base | Head/direct prepared C++/ORT |
|---|---:|---:|---:|
| b1-t1 | 0.081271 | 0.9971 | 49.841× |
| b16-t1 | 0.005221 | 0.9940 | 31.649× |
| b32-t1 | 0.002642 | 0.9911 | 22.338× |

This PR116 run corresponds to 0.3–0.9% lower head latency than its selected base, `72cd42e91509aed4ad79d29fbc1a5bb9acd25c25`, while remaining 22.3–49.8 times slower than direct prepared C++/ORT. That base already includes PR115's session reuse. The earlier 80.5–82.9% observation compares against the pre-reuse implementation and measures a different change. The difference between these ratios must not be attributed solely to hosted-runner variance. Neither run authorizes a general speedup claim.

The latest resident paired median reductions were 0.21% (`b1-t1`), 0.44% (`b16-t1`), 3.06% (`b32-t1`), 0.27% (`b16-t2`) and −0.07% (`b16-t4`). Individual paired blocks included regressions. Energy remained unavailable with null joules and watts.

## Reproduction

The required workflow runs the following classes on every push and pull request:

```text
python3 experiments/energy/test_generated_infer_evidence.py
python3 experiments/energy/test_ci_run_evidence.py
python3 experiments/energy/test_render_benchmark_markdown.py
CXX=clang++-18 python3 tests/ai_application/test_compiled_cache.py build-profile build-profile/onnxruntime-1.30.0
scripts/check_prepared_cache_sanitizers.sh build-profile/onnxruntime-1.30.0 build-profile-sanitized
```

The full workflow additionally rebuilds the exact PR head and base runtime, captures the generated and resident comparisons, hashes source/model/data/compiler/SDK/binaries and raw logs, replays the generated and resident bundles, probes energy capability, renders `BENCHMARKS.md`, and finalizes `run-evidence.json`. Missing or altered evidence fails the required status.

The retained artifact expires after 90 days under the current workflow policy. Archive the cited bundle before expiry when the result must remain independently citable.

## Interpretation

The prepared-session and workspace changes are practically effective for the measured repeated-session and resident-host boundaries. The compiled path still has substantial overhead relative to direct C++/ORT, so the evidence supports continued runtime and generated-code optimization. The present data does not support a claim that ShortHand consumes less power or energy than C++, because no joules/task observation was available. Lower latency alone is not evidence of lower watts or joules.
