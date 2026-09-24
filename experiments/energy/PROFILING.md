# Runtime stage profiling

This implements the first attribution step of [package B](IMPROVEMENT_PLAN.md).
C, Rust, Java and resident Python baseline expansion remains pending.

The additive `application-profile` command runs the real prepared ONNX session
against the existing hash-verified application/model/dataset configuration.
Ordinary `application` execution keeps the uninstrumented specialization of the
same classification implementation: stage clock calls compile out of that path.
No environment variable silently changes historical timing boundaries.

| Stage | Included work |
| --- | --- |
| preprocessing | Raw batch checks, tensor shape copy, input allocation, normalization and finite/range checks |
| prepared_call | Backend input validation, ONNX invocation, output allocation/copy and existing telemetry |
| output_validation | Backend status, output length and finite-value checks including padded rows |
| postprocessing | Score ownership transfer, tail trim, prediction/top-k allocation and sorting |

These are **instrumented diagnostics**, not qualified latency or energy results.
The prepared call does not isolate model-kernel time; sample inside that boundary
before attributing it to ONNX, memory handling or telemetry. Clock overhead is
included. Dataset slicing, ordinary reference execution, score comparison and
trial accumulation are outside the stage partition. The separate observer time
includes those operations inside each trial. Session preparation is excluded.
This profiler also excludes some work in the full qualification harness, so its
total must not be equated with the earlier application-trial boundary.

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

## Regression checks

The host test uses a clearly labelled prepared-session double to test normal vs
profiled output identity, exact partition arithmetic, clearing stale results after
errors and concurrent profiled requests. It supplies no ONNX performance evidence.
The existing real-ONNX application test checks the new command, quality agreement,
completed counts, claim rejection and missing-SDK failure. Reporter tests use
explicitly synthetic timings and reject incomplete work and fabricated claims.
