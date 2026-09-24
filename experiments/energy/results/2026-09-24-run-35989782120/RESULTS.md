# Executed comparison results

**Energy savings: not measured.** This capture records elapsed time and correctness. There is no calibrated whole-host power trace, so joules, energy savings, carbon savings and energy break-even are unavailable. Latency reductions below are not energy reductions.

Capture started: 2026-09-24T10:55:03.474702+00:00. [Workflow and full evidence artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35989782120).
Tested revision: `a8b646c0b3b38a39aa966bc692d031cf2af219e8`. Platform: `Linux-6.17.0-1022-azure-x86_64-with-glibc2.39`. Available CPU affinity: `[0, 1, 2, 3]`.

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
| 10 | numpy | 0.000942 | 0.006744 | 86.03 | [85.86, 86.15] | 89.371 |
| 10 | cpp17-o3 | 0.000927 | 0.001158 | 19.94 | [19.35, 20.64] | 89.371 |
| 10 | pytorch-eager | 0.000940 | 0.073884 | 98.73 | [98.72, 98.74] | 89.371 |
| 10 | pytorch-compile | 0.000940 | 0.310400 | 99.70 | [99.70, 99.70] | 89.371 |
| 100 | numpy | 0.000451 | 0.000781 | 42.28 | [42.11, 42.44] | 89.371 |
| 100 | cpp17-o3 | 0.000451 | 0.000465 | 2.94 | [2.77, 3.11] | 89.371 |
| 100 | pytorch-eager | 0.000451 | 0.007447 | 93.94 | [93.92, 93.98] | 89.371 |
| 100 | pytorch-compile | 0.000451 | 0.031121 | 98.55 | [98.55, 98.55] | 89.371 |

## Runtime workload: FP32 ONNX Runtime 1.30.0 CPU

The same FP32 ONNX model and raw dataset are used by Shorthand AIRuntime, Python/NumPy ONNX Runtime and the independent C++17 ONNX Runtime executable. This measures host integration overhead around the same inference backend, separately from the FP64 source experiment.

Ten balanced process pairs per cell; each process contains three resident-session trials, each classifying 1,797 images 10 times. One process mean is the bootstrap unit, so inner trials are not treated as independent samples. Session preparation, warmup and final serialization are outside the trial window; validation, normalization, inference and top-k processing are inside. Batch size varies at one thread; thread count varies at batch 16.

| Cell (batch / threads) | Baseline | Shorthand ms/image | Baseline ms/image | Latency reduction % | Paired 95% interval % |
| --- | --- | ---: | ---: | ---: | --- |
| ort-b1-t1 | Python / ONNX | 0.005142 | 0.041430 | 87.59 | [87.49, 87.70] |
| ort-b16-t1 | Python / ONNX | 0.000808 | 0.003175 | 74.55 | [74.45, 74.64] |
| ort-b32-t1 | Python / ONNX | 0.000678 | 0.001784 | 62.01 | [59.65, 63.39] |
| ort-b16-t2 | Python / ONNX | 0.000806 | 0.003173 | 74.61 | [74.50, 74.71] |
| ort-b16-t4 | Python / ONNX | 0.000809 | 0.003184 | 74.58 | [74.44, 74.74] |
| ort-b1-t1 | C++ / ONNX | 0.005130 | 0.002862 | -79.26 | [-80.15, -78.38] |
| ort-b16-t1 | C++ / ONNX | 0.000810 | 0.000444 | -82.33 | [-83.90, -80.69] |
| ort-b16-t2 | C++ / ONNX | 0.000807 | 0.000444 | -81.50 | [-82.81, -80.04] |
| ort-b16-t4 | C++ / ONNX | 0.000803 | 0.000449 | -78.87 | [-80.30, -77.48] |
| ort-b32-t1 | C++ / ONNX | 0.000656 | 0.000356 | -84.13 | [-85.03, -83.27] |

## What the measurements show

At ten repetitions, source-process latency was 86.03% lower than NumPy and 19.94% lower than C++. At 100 repetitions, these reductions were 42.28% and 2.94%. The smaller advantage with more work per process shows why startup and workload size matter.

**Shorthand AIRuntime was slower than independent C++/ONNX in every cell: 1.79–1.84× the latency.** The Python comparison also changes with batching: read every cell rather than selecting the batch-1 improvement. These results do not support a general runtime-efficiency advantage.

Large fresh-process reductions versus PyTorch include framework startup and tracing. They do not establish an advantage over resident PyTorch inference. Runtime gaps require profiling before their causes can be assigned to validation, memory handling, instrumentation or backend integration.

## Interpretation and evidence

Reduction = `100 × (1 − mean(Shorthand) / mean(baseline))`. Negative values mean Shorthand was slower. All pairs are retained; no outlier filtering or best-run selection is used. Intervals use 10,000 paired-block bootstrap resamples, seed 104. They are exploratory within-session intervals without multiple-comparison correction.

This is one shared hosted-runner session with one small classifier. Startup costs, virtualization, CPU scheduling, batching and implementation choices affect the numbers. The 10/100 repetition comparison changes startup amortization; it does not isolate it. These results cannot establish a universal language, framework or energy ranking.

- [summary.json](summary.json): exact numbers and all per-pair values used by the bootstrap.
- [observations.json](observations.json): ordered pairs, raw durations, completed work and inner trial windows.
- [metadata/](metadata/): frozen plans, source captures, build timings, environment, energy availability and original bundle digests.
- [VALIDATION.md](VALIDATION.md): stage completion, matrix counts and artifact digest.
- [artifact.json](artifact.json): immutable GitHub artifact ID, digest, retention and tested revision.
- Full artifact: raw stdout/stderr, predictions/scores, binaries, LLVM IR and replay manifests.

Reproduce with the `experiment-results` workflow, or follow [the experiment instructions](../../README.md). Use `run_verified.py` with the same pinned dependencies and built compiler. Real energy work requires the calibrated-energy protocol, an independently logging whole-host meter, and repeated dedicated-machine sessions.
