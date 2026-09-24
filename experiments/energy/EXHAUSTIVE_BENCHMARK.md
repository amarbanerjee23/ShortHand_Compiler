# Exhaustive comparison campaign

This is the reproducible execution campaign for the merged `master` revision.
It is intentionally broader than a single “best” timing: every implemented
baseline, workload size, batch cell and thread cell is retained, including
negative results and unavailable external runners.

## Declared matrix

| Track | ShortHand side | Comparison baselines | Workload and repetitions | Paired observations |
| --- | --- | --- | --- | ---: |
| FP64 source, `source-r10` | LLVM/Clang 18, `-O2`, no fast-math | NumPy 2.3.5, C++17 `-O3`, PyTorch eager, `torch.compile` | UCI Optdigits nearest-centroid; 10 repetitions/process | 30 balanced process pairs per baseline |
| FP64 source, `source-r100` | Same | Same four implemented baselines | Same model; 100 repetitions/process | 30 balanced process pairs per baseline |
| FP32 resident runtime | Shorthand AIRuntime + ONNX Runtime 1.30.0 | Python/ONNX Runtime 1.30.0 and independent C++17/ONNX Runtime 1.30.0 | Batch/thread cells: (1,1), (16,1), (32,1), (16,2), (16,4); 10 repetitions and 3 resident trials per process | 10 balanced process pairs per cell and baseline |

The source matrix therefore contains 240 paired process observations (30 × 4 ×
2). The runtime matrix contains 100 paired processes (10 × 5 × 2), with three
resident inner trials in each process. Two warmups precede every runner. The
same model, input data, correctness checksum and completed-work count are
required on both sides.

Rust/Candle and Mojo/MAX are declared external full-matrix contracts in
`state_of_practice.py`, but this repository does not provide runner binaries.
They are recorded as unavailable rather than silently omitted or replaced with
an unrelated implementation. Adding either runner requires a frozen executable
and version, exact prediction parity, and a new declared capture.

## Timing boundaries

The source track measures a balanced fresh process, including startup, imports,
model loading/input parsing, repeated classification, output and teardown. The
runtime track reports resident-session inference windows separately from session
construction, warmup and final serialization. Negative reductions are valid
results. The report retains every ordered pair and uses a paired bootstrap
interval; it never selects only the fastest run.

These are elapsed-time observations. They are not energy measurements and must
not be converted to joules by multiplying by a TDP, CPU utilisation, RAPL,
NVML, or a virtual-machine estimate.

## Running and storing evidence

The `experiment-results` workflow runs on `workflow_dispatch` and on pull
requests that change the compiler/runtime or experiment protocol. It builds the
real compiler and ONNX Runtime host, installs the pinned Python environment,
runs correctness/rejection tests, executes `run_verified.py`, writes
`energy-status.json`, and uploads two directories:

* `experiment-output/` — the complete raw capture: plans, manifests, binaries,
  predictions, per-pair timings, inner trial windows, failures and environment;
* `experiment-report/` — the compact checked-in-friendly export: `RESULTS.md`,
  `summary.json`, `observations.json`, metadata and `SHA256SUMS.json`.

The compact export is stored under `experiments/energy/results/<run-id>/` after
the workflow completes. The raw artifact remains attached to the workflow run
for replay and is retained for 90 days. The run revision, artifact URL and
hashes are recorded in the compact report.

## Energy qualification boundary

The hosted workflow records `physical_energy_measured: false` explicitly. A
valid energy comparison requires a dedicated isolated Linux host, a calibrated
whole-host AC instrument, synchronized meter samples covering every declared
trial, calibration identity/uncertainty, and repeated independent sessions.
The existing `campaign.py`/`state_of_practice.py` calibrated mode enforces that
contract and rejects missing meter traces. Package counters are diagnostic only;
they do not qualify a whole-host energy claim.

Until such a trace is attached, report only latency, throughput, correctness,
memory and availability. In particular, “latency reduction” in `RESULTS.md`
is never labelled “energy reduction.”

## Interpretation guardrails

This small CPU classification workload cannot establish a universal language,
framework or carbon ranking. Fresh-process PyTorch results include import and
compilation/tracing costs; resident runtime results isolate a different boundary.
Any later physical result must keep these tracks separate, preserve all failed
or unfavorable sessions, and report joules per completed correct task alongside
runtime, throughput, tail latency and uncertainty.
