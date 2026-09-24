# Executed comparison results

**Energy savings: not measured.** This capture records elapsed time and correctness. There is no calibrated whole-host power trace, so joules, energy savings, carbon savings and energy break-even are unavailable. Latency reductions below are not energy reductions.

Capture started: 2026-09-22T15:31:57.224135+00:00. [Workflow and full evidence artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35747601906).
Tested revision: `d3f1c6d3fc98357c44689504daae15204de56b3f`. Platform: `Linux-6.17.0-1022-azure-x86_64-with-glibc2.39`. Available CPU affinity: `[0, 1, 2, 3]`.

## Completion and coverage

| Stage | Status |
| --- | --- |
| source-r10 | PASS |
| source-r100 | PASS |
| runtime-plan | PASS |
| python-runtime | PASS |
| cpp-runtime | PASS |

Rust/Candle and Mojo/MAX were **not executed or implementation-verified**: the repository supplies external runner contracts but no runners. The `torch` profile covers the four implemented source baselines and does not qualify the `full` matrix.

## Source workload: FP64 nearest-centroid classification

UCI Optdigits: 3,823 training rows and 1,797 held-out test rows; train-only centroids, 64 features and 10 classes. Every invocation must match all 1,797 reference predictions and the repetition checksum. Shorthand uses LLVM/Clang 18 `-O2`; C++17 uses `-O3`; both disable fast-math. NumPy/PyTorch use one CPU thread.

Each row has 30 balanced randomized process pairs after two warmups. Timing includes process startup, Python/framework imports, model loading where applicable, input parsing, normalization, repeated classification, output and teardown. Shorthand/C++ embed weights in their binaries; Python loads JSON. TorchInductor reuses its warmed disk cache but **every measured process still performs startup and tracing**. This is not resident PyTorch inference or a pure kernel/language comparison.

| Repetitions | Baseline | Shorthand ms/image | Baseline ms/image | Latency reduction % | Paired 95% interval % | Accuracy % |
| ---: | --- | ---: | ---: | ---: | --- | ---: |
| 10 | numpy | 0.000964 | 0.007133 | 86.48 | [86.40, 86.57] | 89.371 |
| 10 | cpp17-o3 | 0.000970 | 0.001185 | 18.22 | [17.00, 19.77] | 89.371 |
| 10 | pytorch-eager | 0.000971 | 0.083335 | 98.83 | [98.83, 98.84] | 89.371 |
| 10 | pytorch-compile | 0.000972 | 0.348745 | 99.72 | [99.72, 99.72] | 89.371 |
| 100 | numpy | 0.000457 | 0.000831 | 44.96 | [44.70, 45.22] | 89.371 |
| 100 | cpp17-o3 | 0.000457 | 0.000466 | 1.93 | [1.64, 2.21] | 89.371 |

## Runtime workload: FP32 ONNX Runtime 1.30.0 CPU

The same FP32 ONNX model and raw dataset are used by Shorthand AIRuntime, Python/NumPy ONNX Runtime and the independent C++17 ONNX Runtime executable. This measures host integration overhead around the same inference backend, separately from the FP64 source experiment.

Ten balanced process pairs per cell; each process contains three resident-session trials, each classifying 1,797 images 10 times. One process mean is the bootstrap unit, so inner trials are not treated as independent samples. Session preparation, warmup and final serialization are outside the trial window; validation, normalization, inference and top-k processing are inside. Batch size varies at one thread; thread count varies at batch 16.

| Cell (batch / threads) | Baseline | Shorthand ms/image | Baseline ms/image | Latency reduction % | Paired 95% interval % |
| --- | --- | ---: | ---: | ---: | --- |
| ort-b1-t1 | Python / ONNX | 0.008640 | 0.042145 | 79.50 | [79.31, 79.67] |
| ort-b16-t1 | Python / ONNX | 0.004315 | 0.003248 | -32.85 | [-33.52, -32.23] |
| ort-b32-t1 | Python / ONNX | 0.004196 | 0.001819 | -130.72 | [-134.16, -127.26] |
| ort-b16-t2 | Python / ONNX | 0.004276 | 0.003278 | -30.43 | [-32.77, -27.47] |
| ort-b16-t4 | Python / ONNX | 0.004271 | 0.003265 | -30.81 | [-31.79, -29.79] |
| ort-b1-t1 | C++ / ONNX | 0.008661 | 0.002861 | -202.67 | [-204.65, -200.48] |
| ort-b16-t1 | C++ / ONNX | 0.004328 | 0.000448 | -865.99 | [-875.39, -856.25] |
| ort-b16-t2 | C++ / ONNX | 0.004316 | 0.000448 | -863.36 | [-872.51, -853.23] |
| ort-b16-t4 | C++ / ONNX | 0.004264 | 0.000450 | -847.99 | [-858.25, -838.20] |
| ort-b32-t1 | C++ / ONNX | 0.004169 | 0.000360 | -1058.25 | [-1066.86, -1049.45] |

## What the measurements show

At ten repetitions, source-process latency was 86.48% lower than NumPy and 18.22% lower than C++. At 100 repetitions, these reductions were 44.96% and 1.93%. The smaller advantage with more work per process shows why startup and workload size matter.

**Shorthand AIRuntime was slower than independent C++/ONNX in every cell: 3.03–11.58× the latency.** The Python comparison also changes with batching: read every cell rather than selecting the batch-1 improvement. These results do not support a general runtime-efficiency advantage.

Large fresh-process reductions versus PyTorch include framework startup and tracing. They do not establish an advantage over resident PyTorch inference. Runtime gaps require profiling before their causes can be assigned to validation, memory handling, instrumentation or backend integration.

## Interpretation and evidence

Reduction = `100 × (1 − mean(Shorthand) / mean(baseline))`. Negative values mean Shorthand was slower. All pairs are retained; no outlier filtering or best-run selection is used. Intervals use 10,000 paired-block bootstrap resamples, seed 104. They are exploratory within-session intervals without multiple-comparison correction.

This is one shared hosted-runner session with one small classifier. Startup costs, virtualization, CPU scheduling, batching and implementation choices affect the numbers. The 10/100 repetition comparison changes startup amortization; it does not isolate it. These results cannot establish a universal language, framework or energy ranking.

- [summary.json](summary.json): exact numbers and all per-pair values used by the bootstrap.
- [observations.json](observations.json): ordered pairs, raw durations, completed work and inner trial windows.
- [metadata/](metadata/): frozen plans, source captures, build timings, environment and original bundle digests.
- [VALIDATION.md](VALIDATION.md): replay checks, artifact integrity and evidence retention.
- [Earlier failed captures](../failed-runs/README.md): recorded failures and their source observations.
- Full artifact: raw stdout/stderr, predictions/scores, binaries, LLVM IR and replay manifests.

Reproduce with the `experiment-results` workflow, or follow [the experiment instructions](../../README.md). Use `run_verified.py` with the same pinned dependencies and built compiler. Real energy work requires the calibrated-energy protocol, an independently logging whole-host meter, and repeated dedicated-machine sessions.
