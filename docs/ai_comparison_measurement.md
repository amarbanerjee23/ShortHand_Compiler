# Measurement-grade CPU comparison harness

comparison_contract: shorthand.ai.application.comparison.v2
assessment_contract: shorthand.ai.comparison.assessment.v1
policy_contract: shorthand.ai.comparison.policy.v1
current_github_pr: 97
controlled_hardware_evidence: pending
production_claim: false
comparative_energy_claim: false
official_certification_granted: false

PR97 implements measurement/replay and engineering regression controls for
roadmap PR95 + PR97 on merged PR96. It reuses the existing native collectors and
C3-ECO accounting, assessment and auditor contracts. Actual calibrated hardware
observations are still required to close the physical evidence exit.

## Evidence and execution boundary

Four alternating native/Python pairs balance runner order. Each runner uses the
same held-out data, FP32 ONNX model, session options, quality, repetitions,
threads and completed-image units. The pair count may be increased to an even
number up to ten, never selectively reduced after observing results.

Application, qualification and policy bytes are frozen before execution and
checked afterward. Every run owns a fresh evidence directory. The v2 manifest
hashes these snapshots and every runner report, records native binary and Python
source digests, and bounds execution timestamps. The assessor rejects missing,
reused, reordered, overlapping, modified, oversized or symlinked artifacts,
mismatched environments/quality and wall/monotonic clock disagreement.
Preparation, warmup and complete process time remain visible outside
resident-data execution. Trial energy excludes those phases and logger
acquisition/report overhead. This is not whole-lifecycle application energy.

An independently operated physical meter writes `unix_time_s,power_w` CSV.
After the balanced runs, the harness freezes one bounded, newline-complete CSV
snapshot and its instrument metadata. All trial windows are integrated from
that snapshot by the existing native collector. The assessor replays these
integrations using the supplied native executable; every measurement field
must match. No second Python energy integrator is introduced. Replay needs no
original model/data paths:

```sh
shorthand_ai_qualify meter-window meter.csv instrument.json START_UNIX END_UNIX UNITS measurement.json
```

The native report distinguishes actual samples inside the window from
interpolated endpoints, retains the largest original sampling gap supporting
either endpoint and hashes exactly the trace bytes parsed. Cutting a short
window from a long gap cannot make it densely sampled. Sampling-gap comparisons
allow only two floating-point ULPs of timestamp roundoff. Missing measurements
are not supplied by idle subtraction or extrapolation.

## Predeclared engineering policy

The installed `ai_comparison_policy_v1.schema.json` describes the policy.
`tests/ai_application/comparison_execution_policy.json` is the CI execution
policy; `comparison_measurement_policy.json` is the physical-measurement
starting policy. These are repository engineering defaults, not thresholds
mandated by the certification standard. Operators must externally retain the
chosen policy digest before an experiment, together with calibration and
boundary validation.

The measurement policy requires four balanced pairs, at least three trials per
runner, one-second windows, eleven original samples, a maximum 0.1 s source gap
covering at most 10% of the window, and at most 0.01 s clock skew. It checks
calibration age, instrument identity/boundary/provenance, functional units,
quality, latency/startup/warmup budgets and measured J/image. Increase the
original short fixture's repetitions and, if necessary, explicitly declared
batch size before recording physical observations. Existing work/memory/report
bounds still apply. Missing or insufficiently sampled measurements cannot pass
the measured gate. Synthetic CI fixtures validate logic, not physical results.

All trial values, mean/median/p95, dispersion and paired ratios remain visible,
including unfavorable native results. P95 is across whole-trial per-image
averages, not individual-request tail latency. Instrument uncertainty plus
twice relative sample standard deviation is a conservative engineering index,
**not a confidence interval**. It does not assume independent trials or cancel
shared instrument uncertainty. A favorable native/Python ratio is not required
merely to validate an experiment.

CI checks real execution, complete quality and declared absolute timing budgets.
Its execution policy produces `energy_evidence_qualified: false` without
calibrated evidence. This does not close controlled hardware, compiler-time
performance or broader-workload release requirements.

## Run, replay and compare revisions

The mandatory command prints its fresh bundle path and trusted digest values:

```sh
bash scripts/check_ai_application_baselines.sh /path/to/shorthand_ai_qualify /tmp/comparison-run
```

For physical experiments, configure the existing qualification file with
`energy_source: physical_meter`, `require_measured_energy: true`, `meter_csv`
and calibrated `instrument` fields. Update its hash in the application config.
Start the actual logger before the harness and keep it running through the final
native collector flush. Choose a declared policy and fresh output directory:

```sh
python3 scripts/compare_ai_application_baselines.py --tool /path/to/shorthand_ai_qualify \
  --config /path/to/application.json --policy /path/to/declared-policy.json \
  --require-energy --output /tmp/physical-comparison
python3 scripts/assess_ai_application_comparison.py --tool /path/to/shorthand_ai_qualify \
  --bundle /tmp/physical-comparison --comparison-sha256 TRUSTED_COMPARISON_SHA256 \
  --policy-sha256 PREDECLARED_POLICY_SHA256 --output /tmp/assessment.json
```

For regression checks, also pass `--previous PREVIOUS_BUNDLE
--previous-sha256 TRUSTED_PREVIOUS_COMPARISON_SHA256`. The earlier bundle must
replay and pass the same policy. Workload/protocol, hardware, backend, Python
implementation/version and instrument must match; native compiler revisions
may differ. Regression checks use the declared budget with conservative
variability/uncertainty margins. Unstable timing or insufficient energy margin
fails, without discarding pairs or selecting a convenient baseline. Failure
writes a failed assessment and exits 2; crashes/sanitizer findings are hard
failures. Keep trusted digests outside the editable bundle.

Hashes and operator-provided calibration references establish internal
consistency, not authenticity, independent calibration or certification. Use
the existing signed auditor and retention controls for those requirements.
Bundles include private provenance and paths; the public-redaction process
still applies before publication.

## Release status

The mandatory native energy suite adds synthetic replay/negative cases in both
MLIR lanes, including strict sanitizers. The GCC lane executes the real locked
Python comparison and assessor. All 22 Make/CTest parity suites and existing
language, race, security and portability gates remain intact. TST025/TST026,
repeatability and operational-energy evidence remain partial.

Six delivery batches remain including PR97, five afterward. This batch's
physical evidence is still open. Later batches cover broader AI families,
enterprise pilot with explicit CPU scope, lifecycle/cloud accounting,
independent reproduction/certification and final claims/GA closeout.
