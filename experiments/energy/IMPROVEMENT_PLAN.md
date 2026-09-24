# Energy improvement implementation plan

Status: conservative host cleanup, nested opt-in backend profiling and a balanced Shorthand/C++/Python ONNX capture are implemented. The first five-cell real-ONNX profile is reviewed and retained. Physical energy qualification and the wider language/workload matrix remain pending. This plan does not establish energy savings.

[Profiling instructions](PROFILING.md) describe v2 nested diagnostics and the replayable three-runner resident comparison. The existing Python/ONNX worker already reused a session outside its timed trials; resident source-kernel Python/PyTorch, C/Rust/Java and broader workloads are still outstanding. Package B is partially implemented.

Integration correction: #106 targeted the #105 feature branch and #107 targeted the #106 feature branch. Merging those PRs did not update `master`. Their combined code and Windows dependency-retry fix must enter through a PR targeting `master`; do not infer integration from a closed PR badge.

[PR107 evidence](results/2026-09-24-profile-35952311812/RESULTS.md) attributes 51.94–90.34% of instrumented classification time to the inclusive prepared call. The next decision is to inspect its v2 validation, tensor setup, ORT API, copy and telemetry partition. Optimize the largest measured avoidable component, then rerun the uninstrumented balanced comparison. Profile stages do not measure joules.

## Evidence and scope

The [retained results](results/2026-09-22-run-35747601906/RESULTS.md) tested revision d3f1c6d3fc98357c44689504daae15204de56b3f on one hosted Linux runner. AIRuntime took 3.03–11.58 times direct C++/ONNX latency. Its Python comparison regressed at batches 16 and 32. FP64 source-process advantage over C++ fell from 18.22% at 10 repetitions to 1.93% at 100. These are latency observations, not joules or universal language rankings.

The FP64 classifier already embeds weights in a native executable. General ONNX graphs still use ONNX Runtime. Packaging model bytes in a binary is not compiling their operations. Prepared sessions already exist and are reused.

No C, Rust or Java results have been captured. Rust/Candle and Mojo/MAX are external contracts without supplied runners. The small digits classifier cannot support broad AI-efficiency claims.

## Delivery order and acceptance

These are work packages, not reserved GitHub PR numbers. Reassess sizes after profiling; preserve focused, reviewable changes.

| Package | Changes | Acceptance and dependency |
| --- | --- | --- |
| A: conservative host cleanup | Failure-only diagnostic construction in element loops; move owned output scores and trim padding; reserve prediction/top-k vectors | Same full/partial batch scores, predictions, tie order and rejection reasons; concurrent request safety; existing real ONNX and repository gates |
| B: attribution and baseline parity | Profile validation, preprocessing, allocation/copy, backend, top-k and telemetry separately; add implemented C, Rust and Java controls and resident Python/PyTorch runners | Freeze versions, exact work, precision, model, session settings and timing boundaries; verify scores/top-k; report missing coverage as failure for a declared full matrix |
| C: prepared execution efficiency | Per-worker workspaces; explicit tensor lifetimes; reusable output buffers; structured telemetry serialized at export; evaluate graph optimization levels and I/O binding | Depends on B attribution; no shared mutable request buffers; preserve all required evidence and input/output validation; rerun all five original cells |
| D: model AOT prototype | Evaluate ONNX-MLIR first on the pinned FP32 classifier; import supported operators, compile native object, link through a versioned tensor ABI | Equal numerical contract; no ORT dependency on the declared AOT path; explicit unsupported-op/shape errors; separately declared fallback, never silent; capture compilation/startup/resident cost |
| E: tensor compiler optimization | Shape/type propagation, constant folding, fusion, buffer lifetime planning, tiling/vectorization; evaluate LTO and PGO | Depends on D proof; optimization remarks and numerical regression tests; no implicit fast-math or precision relaxation; portable and target-specific builds reported separately |
| F: energy qualification | Calibrated whole-host paired campaigns, workload expansion, energy-aware configuration selection | Dedicated hosts; frozen plans, raw power traces, correct outputs, measured uncertainty, replay and retained failures; claims limited to tested workload/hardware/configuration |

Package A deliberately preserves the prepared backend's telemetry, finite scans, session options, ABI and current request-local input allocation. Its reductions in work are visible in code; their latency/energy contribution remains unmeasured. No changes to historical captures.

## Architecture for compiled models

Proposed path: Shorthand application and immutable model -> typed tensor graph -> graph optimizations and memory plan -> target-specific native model object -> linked executable and model manifest.

Keep tensor semantics until fusion/layout/shape decisions are made; lower to scalar LLVM operations afterwards. Define ownership, strides, dtype, shape, error and concurrency contracts at the ABI. Bind weights, target features, compiler/backend versions and optimization settings by digest. Package required native support libraries explicitly; an executable need not be dependency-free.

Prototype one backend before adding a general plugin system. ONNX-MLIR emits native model code; IREE is an alternative with its own runtime/deployment contracts. Pin the selected compiler separately if its LLVM version conflicts with Shorthand's LLVM 18. A versioned C ABI avoids requiring both projects to share an in-process MLIR build.

Offline ORT optimization is a separate experiment; it does not turn an ORT graph into standalone native model code. Hardware-specific OpenVINO/TensorRT or other integrations require functional implementation and qualification, not merely backend names. Add them only when target hardware and workload evidence justify them.

## Fair comparison design

1. Same-backend track: Python, C, C++, Rust, Java and Shorthand use the same ONNX Runtime version, provider, model and options. Include FFI/JNI cost inside the application boundary and session initialization outside the resident boundary.
2. Best-tuned deployment track: each ecosystem may use appropriate compilation and optimized libraries, including the same model compiler. Separate FP64 source kernels from FP32 ONNX and any quantized track.
3. Source controls: implement the same classifier in C, Rust and Java; give each equivalent normalization, input residency and completed-work checksum. Report Java JIT warmup/GC and Python imports/tracing explicitly.
4. Report fresh-process, session preparation and resident application execution separately. A kernel-only submeasurement complements the application result; it does not replace it.
5. Expand beyond digits using pinned held-out classification, retrieval/embedding, vision and bounded sequence workloads. Agree accuracy/tolerance and latency constraints before running.
6. Match required safety and output contracts. Profile extra Shorthand operational features transparently; do not remove checks only from Shorthand to manufacture a win.

Initial performance objective: close the resident gap to direct C++/ONNX across the five existing cells. Any numeric improvement threshold is a prospective engineering target, not an observed result. Do not introduce noisy hosted-runner timing as a deterministic correctness gate.

## Physical energy protocol

Primary metric: whole-host joules per completed correct task at a declared latency/throughput service level. Also report watts, total joules, runtime, throughput and tail latency. Reduced watts with increased runtime may consume more energy.

Use randomized balanced pairs, multiple independent dedicated-machine sessions, controlled CPU affinity/governor/thermal conditions, and predeclared warmups. Record meter identity, calibration uncertainty, sampling cadence and synchronized windows. Use package counters only as separately labelled diagnostic measurements. Retain gross wall energy; label any idle-subtracted result separately.

The current batch-1 report limit prevents arbitrarily long repetition windows. Design a bounded streaming/aggregate capture with retained sufficient replay evidence before a longer physical campaign; do not disable the limit or minimum meter-window rule. Never substitute a TDP estimate for physical measurement.

Include all completed declared trials and failures without favorable-run selection. Confidence intervals should account for independent sessions; report meter uncertainty separately. Freeze primary comparisons and any multiple-comparison procedure before making superiority claims.

For each baseline: savings = 100 * (1 - Shorthand joules/task / baseline joules/task).
Claim improvement only when the predeclared evidence rule excludes zero savings with sufficient measurement precision and quality/service constraints pass.
Compilation break-even = incremental compilation energy / resident energy saved per task, only when the denominator is positive. Include equivalent baseline compilation/setup costs. Nonpositive savings have no energy break-even.

## Validation of package A

- Local nine experiment unit tests pass.
- Local host test uses a deterministic prepared-session double and executes the real ClassificationApplication implementation. It covers full/partial/repeated requests, score and top-k equality, lower-label ties, invalid batches/ranges/NaN/infinity, backend errors, output size, non-finite padded outputs and concurrent calls.
- Real-ONNX test_application.py adds score comparisons across batch sizes and full/partial/final-row serving requests, with top-k ordering checks.
- The existing energy qualification script runs the host test additionally; real ONNX remains mandatory wherever the existing gate requires it.
- Full real-ONNX/repository CI and before/after statistical capture remain required. No physical measurement or performance improvement is claimed by these tests.

## Primary integration references

- [ONNX-MLIR](https://onnx.ai/onnx-mlir/): native model compilation candidate.
- [IREE deployment](https://iree.dev/guides/deployment-configurations/): alternative AOT/runtime deployment candidate.
- [ORT graph optimization](https://onnxruntime.ai/docs/performance/model-optimizations/graph-optimizations.html).
- [ORT I/O binding](https://onnxruntime.ai/docs/performance/tune-performance/iobinding.html).

These are proposed integrations; none is claimed implemented by this plan.
