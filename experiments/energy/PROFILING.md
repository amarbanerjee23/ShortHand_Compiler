# Runtime stage profiling

This implements nested runtime attribution in [package B](IMPROVEMENT_PLAN.md).
The [reviewed PR107 capture](results/2026-09-24-profile-35952311812/RESULTS.md)
puts 51.94–90.34% of instrumented classification time in the prepared call.
The v2 profiler subdivides that boundary. C, Rust, Java and resident source-kernel
Python/PyTorch expansion remain pending; the existing Python/ONNX worker already
has a resident boundary and is reused in the new three-runner capture.

The additive `application-profile` command runs the real prepared ONNX session
against the existing hash-verified application/model/dataset configuration.
Ordinary `application` execution keeps the uninstrumented specialization of the
same classification implementation: stage clock calls compile out of that path.
No environment variable silently changes historical timing boundaries.

| Stage | Included work |
| --- | --- |
| preprocessing | Raw batch checks, tensor shape copy, input allocation, normalization and finite/range checks |
| prepared_call | Inclusive backend call, subdivided in v2 as shown below |
| output_validation | Backend status, output length and finite-value checks including padded rows |
| postprocessing | Score ownership transfer, tail trim, prediction/top-k allocation and sorting |

These are **instrumented diagnostics**, not qualified latency or energy results.
The prepared call does not isolate model-kernel time. Clock overhead is
included. Dataset slicing, ordinary reference execution, score comparison and
trial accumulation are outside the stage partition. The separate observer time
includes those operations inside each trial. Session preparation is excluded.
This profiler also excludes some work in the full qualification harness, so its
total must not be equated with the earlier application-trial boundary.

| Nested backend stage | Included work |
| --- | --- |
| setup | Result metadata and local telemetry timer setup |
| input_validation | Shape/dtype/count checks and finite-input scan |
| tensor_setup | CPU memory descriptor, zero-copy input wrapper and I/O name arrays |
| session_run | Entire ORT `Run` call, including ORT allocation and scheduling |
| output_copy | Output tensor/type/shape checks and copy into owned scores |
| telemetry | Local telemetry finish and JSON serialization |
| unattributed call overhead | Dispatch, timing bookkeeping, return and local object destruction outside nested clocks |

Nested stages sum exactly to their backend total; backend total plus residual
equals the inclusive prepared call. Do not add nested and enclosing totals.
The reporter accepts historical v1 captures and requires the complete nested
partition for v2. Unsupported prepared backends reject profiling explicitly.
Both normal APIs retain the uninstrumented template specialization. Every failed
profile leaves zeroed timing data, including failures after backend execution.

Every profiled output is compared against the ordinary path for scores,
predictions and top-k. Both execute the same operational checks. Configured
quality and bounded-work rules remain enforced. Instrumented capture is rejected
when the configuration requires measured energy, and every report explicitly
disables energy and latency claims. A failed call clears its timing result;
failed processes must be retained as failures, never accepted as observations.

## Reproduce one cell

Build `shorthand_ai_qualify` with the pinned real ONNX Runtime SDK and an optimized
build configuration as in `.github/workflows/runtime-profile.yml`. Then:

```sh
python3 scripts/create_digit_application_fixture.py /tmp/profile-b16-t1 --batch 16 --threads 1
build-profile/shorthand_ai_qualify application-profile \
  /tmp/profile-b16-t1/application.json /tmp/profile-b16-t1/profile.json
PROFILE_SHA="$(sha256sum /tmp/profile-b16-t1/profile.json | cut -d' ' -f1)"
python3 experiments/energy/profile_report.py \
  --input /tmp/profile-b16-t1/profile.json --input-sha256 "$PROFILE_SHA" \
  --output /tmp/profile-b16-t1/report
```

The reporter verifies completed work and the exact timing partition and preserves
the native input bytes. `PROFILE.md` contains descriptive aggregate milliseconds,
nanoseconds/image and stage shares. It reports neither speedup nor confidence
intervals. Retain the digest independently when replaying archived evidence.

The `runtime-profile` workflow runs the five original batch/thread cells and
retains raw reports, model/configuration fixtures, CMake options, executable
digest, source revision and machine metadata in its artifact. It also renders
each Markdown table in the workflow summary. Failure logs remain in the workflow;
the artifact step runs even if verification or capture fails. Hosted-runner
diagnostics guide optimization; dedicated-machine paired captures are still
required for comparative claims. Download and commit reviewed captures to a new
results directory; never overwrite the September 22 evidence.

## Resident baseline capture

The same workflow runs `resident_baselines.py` on uninstrumented Shorthand,
independent C++/ONNX and the existing NumPy/Python/ONNX worker. All use ORT 1.30.0,
the same FP32 model and held-out inputs, two first-batch warmups, two dataset
repetitions per trial, three trials and the same five batch/thread cells.
Every cell uses all six runner permutations, shuffled from a recorded seed.
The plan, inputs and executable digests are frozen before measured blocks.
Score tolerance, predictions, stable top-3, quality, exact completed counts,
revision and environment identity are checked. Incomplete captures fail and
retain their logs and failure record.

`BASELINES.md` reports median resident microseconds per image, whole-process wall
and CPU time, peak RSS, and explicitly unavailable joules. Resident time includes
each existing application's harness overhead; native batch timing and telemetry
remain included. This is an application comparison, not pure model-kernel cost.
CPU/RSS measurements use GNU time and include child work; peak RSS is not summed
across processes. Per-image resident time is amortized dataset time, not p95
request latency. Session setup is outside resident trials and inside process
timing. C++ does not yet report a separate session-setup duration.

After building the real host and C++ control and installing the hash-locked
Python requirements (as in the workflow):

```sh
baseline-venv/bin/python experiments/energy/resident_baselines.py run \
  --tool "$PWD/build-profile/shorthand_ai_qualify" \
  --cpp "$PWD/profile-output/cpp-build/cpp-onnx-baseline" \
  --output "$PWD/resident-output"
python3 experiments/energy/resident_baselines.py analyze \
  --bundle resident-output --manifest-sha256 SHA_PRINTED_BY_CAPTURE
```

Replay verifies all raw file hashes and reconstructs the summary without executing
models or following the captured machine's absolute paths. Keep the manifest
digest independently. A hosted capture enforces correctness and completeness,
not numerical timing thresholds or energy superiority. The workflow also runs
on relevant pushes to `master`, so integration gets a fresh capture.

## Regression checks

The host test uses a clearly labelled prepared-session double to test normal vs
profiled output identity, exact partition arithmetic, clearing stale results after
errors and concurrent profiled requests. It supplies no ONNX performance evidence.
The existing real-ONNX application test checks the new command, quality agreement,
completed counts, claim rejection and missing-SDK failure. Reporter tests use
explicitly synthetic timings and reject incomplete work and fabricated claims.
The real-backend C++ test additionally covers telemetry preservation, stale-data
clearing on invalid inputs and concurrent profiled ONNX execution.
