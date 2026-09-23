# Comparison verification

The executed results belong to the experiment revision and environment recorded
in each results directory. This review covers the added comparison approaches;
it does not certify a universal language or energy ranking.

| Approach | Implemented in this repository | What is checked before accepting a result |
| --- | --- | --- |
| Shorthand FP64 source | Generated `classifier.short`, compiled through the real MLIR/LLVM backend | All 1,797 held-out predictions, completed work and repetition checksum |
| NumPy FP64 | Vectorized normalization, matrix multiplication and argmax | Same training-derived weights, inputs, rotation, predictions and checksum |
| C++17 FP64 | Independent generated source, Clang `-O3`, no fast-math | Same embedded weights, one normalization per row, predictions and checksum |
| PyTorch eager FP64 | CPU tensors, one intra-op/inter-op thread | Same FP64 parameters, inputs, predictions and checksum |
| `torch.compile` FP64 | Full-graph TorchInductor with dynamic shapes | Same correctness checks; nonzero exit fails capture; two cache warmups |
| Python/NumPy ONNX Runtime | Existing pinned CPU baseline | Same FP32 model, session options, data, complete predictions and all score tolerances |
| Independent C++ ONNX Runtime | Standalone source, does not link Shorthand runtime | Same FP32 model/options, complete predictions, all scores and stable top-three labels |
| Rust/Candle | External runner contract only | Binary digest and version declaration can be checked, but there is no supplied implementation to execute or audit |
| Mojo/MAX | External runner contract only | Binary digest and version declaration can be checked, but there is no supplied implementation to execute or audit |

## Corrections made before the retained capture

1. **PyTorch changing shapes.** Cyclic input rotation splits the dataset at a
   different position on each repetition. `dynamic=False` specialized every
   shape, risking Dynamo's recompilation limit. The compiled baseline now uses
   `dynamic=True`, with `fullgraph=True` retained. See the official
   [torch.compile documentation](https://docs.pytorch.org/docs/stable/generated/torch.compile.html).
2. **PyTorch boundary description.** A warmed TorchInductor disk cache does not
   make a fresh Python process a resident inference service. The report explicitly
   includes imports, tracing and cache loading in source-process latency.
3. **C++ FP64 preprocessing.** Normalize each row once before its ten class
   scores, as Shorthand does, and disable synchronized iostream I/O. This avoids
   attributing unnecessary baseline work to a language advantage.
4. **Short-process timer.** Python's POSIX `Popen.wait(timeout=...)` uses polling
   sleeps. A blocking wait with a separate watchdog removes that polling delay
   from measured process completion while retaining bounded execution. See
   [Python's subprocess documentation](https://docs.python.org/3.12/library/subprocess.html#subprocess.Popen.wait).
5. **ONNX warmup equivalence.** The independent C++ control previously warmed up
   over the entire test set while AIRuntime and Python warmed up the first batch.
   C++ now uses the same first-batch warmup count and batch size.
6. **ONNX cross-runner quality.** Exact top-one predictions alone can hide score
   or ranking differences. C++ now writes scores and top-three labels after the
   measured window. Capture and replay compare them against AIRuntime: absolute
   tolerance `1e-5`, relative tolerance `1e-4`, stable lower-label tie breaking.
7. **Explicit partial coverage.** The new `torch` profile runs all four supplied
   FP64 baselines without implying that Rust/Candle or Mojo/MAX ran. The `full`
   profile still requires all six declared source baselines.

8. **Bounded runtime work.** The initial 20-repetition capture hit AIRuntime's
   existing 100,000-batch report limit in the batch-1 cell (107,820 batches).
   The final plan uses ten repetitions (53,910 batches), and preparation rejects
   oversized plans before running. Failed captures remain in the execution history.

## Design and limits

- Train on the pinned 3,823-row Optdigits training split; evaluate on the pinned
  1,797-row test split. No test-label tuning. Minimum accuracy is 85%.
- Source: 30 balanced, randomized pairs per baseline at ten repetitions; repeat
  NumPy/C++ comparisons at 100 repetitions to examine workload-size sensitivity.
  Two warmup processes precede capture. Entire subprocess time is measured.
- Runtime: all five predeclared batch/thread cells, ten balanced process pairs,
  three inner trials per process, 10 dataset repetitions per inner trial. The
  bootstrap uses process means rather than treating inner trials as independent.
- ONNX session settings match: CPU provider, basic graph optimization,
  sequential execution, one inter-op thread, prescribed intra-op threads,
  spinning disabled. All implementations include raw range checks,
  normalization, padded tail batches, finite output checks and top-k processing.
- Source FP64 and runtime FP32 tracks remain separate. C++/ONNX comparisons
  test integration overhead around the same backend, not compiler speed.
- Native/C++ source weights are embedded in binaries; Python parses model JSON.
  Standard I/O implementations and framework startup differ. Those are inside
  the declared end-to-end source boundary and preclude pure-kernel attribution.
- Shorthand source links at `-O2`; the stronger C++ control uses `-O3`. The
  runtime host is the repository's `RelWithDebInfo` build; independent C++ uses
  `-O3`. Results describe these build configurations, not matched compiler flags.
- C++/ONNX retains and compares top-three labels across repetitions; the existing
  AIRuntime and Python runners validate scores and top-one labels internally.
  This small validation-work difference remains in the runtime implementations.
- All observations are retained, including negative reductions. A failed stage
  is reported and excluded from qualified summary tables; it is not silently
  replaced by a successful retry. There is no outlier filtering.
- The 95% paired bootstrap intervals are exploratory within one shared hosted
  runner session, with no multiplicity correction or cross-machine replication.
- No physical power trace is available in hosted capture. Energy savings and
  energy break-even are unknown. They cannot be derived from latency percentages.

## Validation evidence

The experiment unit suite tests signed reductions and bootstrap behavior,
compilation amortization, input/plan tampering, incomplete baseline matrices,
missing measured-energy evidence, external version mismatches, subprocess failure
and timeout rejection, and C++ score/top-three mismatch rejection. The actual
capture additionally checks every measured process's predictions and completed
work, replays bundle digests, and executes the real compiler and ORT backends.

Synthetic meter traces occur only in a labelled collector unit test. They are
never inputs to the experiment result tables.
