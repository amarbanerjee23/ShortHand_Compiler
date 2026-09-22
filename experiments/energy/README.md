# Shorthand energy experiments after PR103

Question: **How much operational energy, if any, does Shorthand save for an
equivalent completed AI task on the same machine?** Zero savings and regressions
are valid results. This suite implements a bounded starting experiment for the
E2/E3 evidence milestones; those milestones remain open until real observations
and broader application coverage exist.

No measurements or savings percentages are committed here. Python is used to
orchestrate experiments and provide baselines. The compiled `.short` application
executes without Python.

## Experiments and hypotheses

All experiments use the already pinned UCI Optdigits data: 3,823 training images,
1,797 held-out test images, train-only nearest-centroid weights, fixed pixel/16
normalization, ten classes, deterministic lowest-index tie breaking and a
predeclared minimum 85% accuracy. Never tune on the held-out labels.

| ID | Comparison / intervention | Controlled quantities | Measurement boundary / purpose |
| --- | --- | --- | --- |
| L1 | Actual `.short` → MLIR/LLVM → native executable versus optimized NumPy | Same FP64 weights, inputs, repetitions, full predictions, one BLAS thread | Fresh process with warm OS caches: startup, data reading, normalization, classification, argmax, output and teardown; tests a compiled application approach |
| L1-scalar | Separate L1 campaign using ordinary Python loops | Same L1 task and FP64 arithmetic | Diagnostic interpreter-overhead baseline; report alongside NumPy, never replace the optimized baseline with this easier comparison |
| R1 | Native AIRuntime versus Python NumPy + ONNX Runtime at batch 1, 16, 32 | Same FP32 model/data; ORT 1.30.0, CPU provider, one thread, session options, quality, tail policy | Resident-data application execution; tests batching effects and runtime integration |
| R2 | R1 at batch 16 with 1, 2, 4 intra-op threads | All R1 settings except declared thread count | Tests CPU parallelism; do not equate more threads or less runtime with less energy |
| C1 | Repeated `.short` code generation and `clang++ -O2 -fno-fast-math` linking | Same source, compiler, flags, machine | Complete application builds; compute how many L1 executions would repay compilation energy |

L1 compares complete implementation approaches. NumPy calls optimized native
kernels; ordinary Python is not inherently an energy-intensive AI backend.
FP64 L1 and FP32 R1/R2 are **separate results**. R1/R2 measure the native host
AIRuntime, not a `.short` compiled program. No result is averaged across these
boundaries. Each L1 repetition rotates the input order by one image, making
runtime indexing depend on the repetition. NumPy uses contiguous views rather
than copying the whole input. Every repetition contributes to an observable
checksum; every run must reproduce all 1,797 final ordered predictions. These
controls resist dead-work elimination and simple loop-invariant reuse.

## Fixed experimental protocol

1. Use a dedicated Linux x64 AC-powered host with at least four available CPU
   threads and release builds. Record CPU model, RAM, OS/kernel, compiler/LLVM,
   NumPy/BLAS and ORT builds, CPU affinity, governor/turbo setting, power cap,
   cooling/temperature conditions and background services. Keep them unchanged.
   The bundle records platform, CPU details, affinity, executable hashes and
   compiler version; retain the remaining host observations with the instrument
   validation reference. Do not run sanitizers, other jobs or downloads during
   physical measurements. Use the same allocator/environment for repeats.
2. Use an independently operated calibrated **whole-host AC meter**, synchronized
   to the runner's Unix clock. Capture `unix_time_s,power_w` at least every 0.1 s,
   including samples before and after every window. Keep the logger running and
   flushing through the last trial. Record its ID, calibration certificate/date,
   validation reference, uncertainty and isolation description. Existing native
   integration is reused; there is no Python power estimator or idle subtraction.
   RAPL measures CPU packages and cannot substitute for this AC boundary.
3. First run an explicitly `execution_only` pilot. Choose repetition counts
   large enough that the **fastest** measured trial and each compilation block
   exceed one second with at least eleven original samples. Prefer >=10 seconds
   when practical; meet the existing maximum-gap/uncertainty policy. Compile
   blocks contain a fixed number of complete builds so short compilations can
   be measured; divide block joules by completed builds. Pilot defaults are not
   guaranteed to satisfy a physical meter. Adjust counts using a new pilot,
   then freeze the final plan before collecting confirmatory data.
4. L1 uses 30 paired blocks by default, equal AB/BA orders shuffled with seed
   104, two full warmup runs per implementation and three compilation blocks.
   Both implementations complete identical image counts. R1/R2 reuse the
   existing ten balanced process-pairs, three inner trials and two warmups per
   runner. Runtime cell order is deterministically shuffled. No selective
   retries, dropping outliers, early stopping or choosing a favorable baseline.
5. Repeat the **same frozen plan in at least three separate sessions**, after
   the same idle/thermal preparation; retain every session independently. For a
   portability statement, repeat on a second declared CPU model. The tool runs
   one session at a time and does not certify independence. Investigate unstable
   runs by retaining the failed run and starting a newly declared session.
6. Keep plan/manifest digests outside editable bundles. Failures retain raw
   artifacts and `failure.json`; they do not produce a successful measurement
   bundle. Missing meters, short/sparse windows, clock jumps, changed artifacts,
   omitted cells and quality mismatches fail. Synthetic unit-test meter traces
   are never physical evidence.

## Build and reproduce

Use CPython 3.12/Linux x64, LLVM/MLIR 18, the repository's real ONNX SDK and its
hash-locked Python baseline. From the repository root (install the prerequisites
listed in the root README and `libmlir-18-dev mlir-18-tools llvm-18-tools` first):

```sh
python3.12 -m venv /tmp/shorthand-energy-venv
/tmp/shorthand-energy-venv/bin/python -m pip install --only-binary=:all: --require-hashes \
  -r tests/ai_application/requirements-ai-baseline-linux-x64-py312.txt
bash scripts/install_ci_onnxruntime_cpu.sh /tmp/shorthand-energy-ort
cmake -S . -B build-energy -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_CONFIG=/usr/bin/llvm-config-18 -DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir \
  -DSHORTHAND_BUILD_MLIR=ON -DSHORTHAND_MLIR_TESTING=OFF -DSHORTHAND_BUILD_TESTING=OFF \
  -DSHORTHAND_ENABLE_ONNXRUNTIME=ON -DSHORTHAND_STRICT_OPTIONAL_BACKENDS=ON \
  -DONNXRUNTIME_ROOT=/tmp/shorthand-energy-ort
cmake --build build-energy --parallel 2 --target short_hand shorthand_ai_qualify

/tmp/shorthand-energy-venv/bin/python experiments/energy/campaign.py prepare \
  --mode execution_only --source-pairs 4 --runtime-pairs 4 \
  --source-repetitions 2 --runtime-repetitions 1 --compile-repetitions 1 \
  --output /tmp/shorthand-energy-pilot-plan
```

The preparation command prints a plan digest. Run with that exact value:

```sh
/tmp/shorthand-energy-venv/bin/python experiments/energy/campaign.py run \
  --plan /tmp/shorthand-energy-pilot-plan/plan.json --plan-sha256 PRINTED_PLAN_SHA256 \
  --compiler "$PWD/build-energy/short_hand" --clang /usr/bin/clang++-18 \
  --tool "$PWD/build-energy/shorthand_ai_qualify" --output /tmp/shorthand-energy-pilot-run
```

For a physical session, supply a real `instrument.json` using the existing
[instrument contract](../../docs/ai_cpu_energy_qualification.md#real-measurements):
`id`, `calibration_id`, `calibration_date` (ISO date), `validation_ref`,
`uncertainty_percent`, `boundary: "whole_host_ac"`, and `isolation`. Do not use
example or fabricated calibration values. After the pilot, freeze a new plan:

```sh
/tmp/shorthand-energy-venv/bin/python experiments/energy/campaign.py prepare \
  --mode calibrated_energy --instrument /evidence/instrument.json \
  --meter-csv /evidence/live-meter.csv --source-repetitions 1000 \
  --runtime-repetitions 1000 --compile-repetitions 10 \
  --output /evidence/declared-plan
```

The repetition values above are **illustrative starting values**, not validated
for your host/meter. Use the run command with this plan, its externally retained
digest and a fresh output directory for each session. The frozen plan and its
generated configuration paths must remain accessible during capture. There are
no downloads during the experiment. The same native meter tool replays a saved
bundle without the original model/data paths:

```sh
/tmp/shorthand-energy-venv/bin/python experiments/energy/campaign.py analyze \
  --bundle /evidence/session-1 --manifest-sha256 PRINTED_MANIFEST_SHA256 \
  --tool "$PWD/build-energy/shorthand_ai_qualify"
```

Use `prepare --source-baseline scalar` in a separate predeclared plan for the
diagnostic scalar comparison. Keep the NumPy campaign as the primary result.
The existing physical trace limit is 32 MiB; plan recording duration/sampling
accordingly. No missing rows are discarded to fit that limit.

## Outputs and interpretation

`summary.md` provides one row per experiment/cell. `summary.json` retains raw
paired values, mean J/image (or explicitly ms/image in execution-only mode),
signed reduction, paired bootstrap intervals, instrument-expanded ranges and
compilation amortization. Individual reports preserve startup, warmup and every
native trial. The manifest binds source, model, data/configuration digests,
executable identities, build outputs, predictions, errors and meter snapshots.

For each cell:

`energy reduction (%) = 100 × (1 − mean(Shorthand J/image) / mean(Python J/image))`

Zero is no observed saving; a negative number is greater Shorthand consumption.
The 10,000-resample percentile bootstrap resamples **paired process blocks**,
not the correlated inner ORT trials as independent observations. Its 95%
interval describes within-session sampling variation. A separate conservative
range expands the ratio endpoints by `(1+u)/(1-u)` and its reciprocal for
instrument relative uncertainty `u`. This is not an independent calibration
certificate, a correction for thermal drift, or simultaneous coverage across
all cells. Report all sessions/cells and their uncertainty; do not select only
the largest observed reduction.

`break-even runs = ceil(mean(application compilation J) / (Python J/run − Shorthand J/run))`

This is a point estimate for L1 and is null when execution savings are zero or
negative. Also report images to break even. Compiler/toolchain construction,
package installation, model fitting, network, hardware manufacture and disposal
are outside the boundary. Warm OS caches are not a cold machine boot. Source
startup and resident ORT timing are distinct metrics; trial-average timing is
not request tail latency. Energy is not carbon: any CO2e statement needs the
existing lifecycle/accounting evidence and a sourced electricity factor.

## Tests and remaining scope

```sh
/tmp/shorthand-energy-venv/bin/python experiments/energy/test_experiments.py \
  --compiler "$PWD/build-energy/short_hand" --clang /usr/bin/clang++-18 \
  --tool "$PWD/build-energy/shorthand_ai_qualify"
```

The existing mandatory unsanitized MLIR CI lane runs the complete execution-only
campaign, all five runtime cells, native meter unit tests and tamper rejection.
Hosted CI verifies mechanics, not physical energy savings. Existing sanitizer,
zero-skip, ONNX, stable-context and production-evidence gates remain intact.

This initial suite establishes a repeatable small-classification experiment.
Detection, retrieval, training, LLMs, GPU execution and enterprise-scale claims
need real task datasets, equal-quality optimized baselines and separate
predeclared experiments; the current tiny tensor fixtures cannot establish
those claims. E2/E3 and public comparative/certification claims remain open.

References: [existing comparison protocol](../../docs/ai_comparison_measurement.md),
[dataset attribution](../../tests/ai_application/data/README.md),
[Linux powercap boundaries](https://docs.kernel.org/power/powercap/powercap.html),
[ORT threading/session controls](https://onnxruntime.ai/docs/performance/tune-performance/threading.html).
