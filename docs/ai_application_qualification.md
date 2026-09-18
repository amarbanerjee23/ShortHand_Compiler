# CPU application and equivalent baseline qualification

application_contract: shorthand.ai.application.v1
introduced_github_pr: 96
application_scope: bounded_cpu_digit_classification
controlled_hardware_evidence: pending
production_claim: false
comparative_energy_claim: false
official_certification_granted: false

PR96 combines roadmap PR94 application execution with the reusable portion of
roadmap PR95's equivalent-baseline harness. It extends PR95's native tool and
collectors, the existing AIRuntime prepared-session API and ServingRuntime.
Grammar, float semantics, runtime ABI and MLIR lowering remain unchanged.

## Reproduce the real workload

```sh
python3 scripts/create_digit_application_fixture.py /tmp/digits
shorthand_ai_qualify application /tmp/digits/application.json /tmp/digits/report.json
shorthand_ai_qualify application-serve /tmp/digits/application.json /tmp/digits/serving.json
```

The generator verifies the original UCI optical-digits split by byte count and
SHA-256. It fits a nearest-class-centroid classifier using only 3,823 training
images, then exports a small FP32 ONNX MatMul/Add model. The fixed normalization
is pixel count / 16. The independent 1,797-image author-separated test set is
never used for fitting or tuning. Dataset attribution, CC BY 4.0 terms and exact
source pins are in `tests/ai_application/data/README.md`. Native deployment and
serving require no Python. Python generates development artifacts and supplies
the independent comparison runner only.

The generated `digits.short` is also a complete standalone classifier: input
pixels arrive on stdin; native language arrays, loops and arithmetic perform
normalization, scoring and argmax. Mandatory tests execute every held-out image
through the interpreter and MLIR/LLVM at O0/O2, then compare predictions with the
ONNX application. ShortHand's existing floats are FP64; ONNX tensors are FP32.
This proves task correctness across the two paths, not equivalent-precision
performance. The source example consumes validated numeric rows; the native host
boundary below performs request validation for deployments.

Development validation: all 1,797 predictions agree across those paths. Test
accuracy is 89.37117417918754%, with top-3 accuracy 98.27490261547023%. The minimum
accuracy was fixed at 85% before test execution. This small real dataset is a
bounded application qualification, not an ImageNet, detection, retrieval, LLM,
medical or general training qualification. Broader applications remain roadmap
PR98 evidence.

## Native host and serving boundaries

The application configuration references the unchanged CPU qualification
configuration by digest, plus model, dataset, provenance, feature/class count,
normalization, thread, worker and quality contracts. Unknown fields, duplicate
JSON keys, unsafe files, mismatched hashes, malformed CSV, missing classes,
invalid ranges and oversized inputs fail. Artifact paths are host-controlled and
must remain immutable while sessions run; this is not a hostile-model sandbox.
Use an OS/container memory and CPU limit for deployment. Reported Linux peak RSS
is process-wide and cumulative; unsupported platforms report null, not zero.

One prepared CPU session is reused. Requests contain only raw numeric features.
Partial tail batches use zero-normalized padding; padding does not contribute
to task accuracy or completed-image functional units. Finite output and shape
checks precede deterministic top-k selection; tied scores prefer lower labels.
All repeats check prediction/numeric stability. Accuracy, confusion matrix,
raw scores, p95 latency per completed image, warmup/preparation overhead,
trials, measurements and failures are retained. No latency or energy values are
invented when telemetry is unavailable.

`application-stream CONFIG` accepts bounded JSON lines on stdin. Each line has
one `requests` array with 1–64 envelopes:

```json
{"requests":[{"id":"request-1","tenant":"application","values":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"timeout_ms":1000}]}
```

Provide exactly 64 values per image for the digit configuration. The example
uses one all-zero image. A frame is at most 1 MiB.
The configured batch size bounds images per request. The fixed tenant is
`application`; hosts isolate separate tenants in separate processes. Existing
ServingRuntime enforces admission quotas, isolation, duplicate IDs, deadlines,
result retention and drain. Invalid frames or requests produce errors; later
valid frames remain executable. All envelopes are checked before a frame admits
work. Capacity rejection is visible; clients choose their retry policy.

Cancellation is cooperative before and after inference. An active ONNX kernel
is not forcibly interrupted; expired work cannot return a successful result.
OS supervision supplies a hard process deadline where required. This transport
is local stdin/stdout; it makes no HTTP authentication or TLS claim.

## Equivalent native/Python comparison

```sh
bash scripts/check_ai_application_baselines.sh /path/to/shorthand_ai_qualify /tmp/baselines
```

The test-only lock pins CPython 3.12/Linux x64 wheels, including ONNX Runtime
1.30.0 and NumPy 2.3.5, by SHA-256. Four balanced alternating runner pairs each perform
three trials, using the same model/data/configuration hashes, CPU fingerprint,
threads, precision, batch/tail policy, normalization, top-k, quality and numeric
limits. Both reuse sequential ONNX sessions with basic graph optimization,
spinning disabled and one inter-op thread. The Python path uses vectorized
NumPy preprocessing/postprocessing. Startup, dataset loading, session creation
and warmup are reported separately from resident-data execution. Reports bind
each raw runner artifact by digest. Vendor telemetry is disabled before import
and native environment initialization.

The comparison is **ShortHand's native host AIRuntime path versus optimized
Python ONNX**, not a whole-language compiler ranking. The initial local result
favored Python latency (approximately 0.00223 versus 0.00375 ms/image); it does
not establish a general performance advantage for either language. Hardware,
compiler flags, load and instrumentation affect these observations. CI validates
fairness, numerical quality and completion rather than requiring a favorable
ratio. Controlled performance budgets and broader families remain release gates.

With `--require-energy`, the protocol must require a calibrated physical meter.
Both runners' exact Unix execution windows are integrated by the same native
collector; units, calibration, boundary, uncertainty and at least 11 original window
samples must agree. PR97 also retains sampling gaps and trace identity. Missing, insufficiently sampled or unqualified energy fails.
Raw J/image values are retained. Sampling a very short workload is insufficient;
operators must lengthen repetitions to suit the instrument. RAPL remains useful
for native profiling, but RAPL-versus-physical-meter comparisons are rejected.
Checksums and operator-supplied provenance are not independent calibration or
signatures. No general carbon, certification or fastest/lowest-energy claim is
emitted. Existing C3-ECO accounting, independent assessment and auditor controls
remain separate; application reports do not redefine their schemas.

## Required checks and remaining exits

The inherited 22-suite Make/CTest parity includes dataset/configuration failures,
SDK-off rejection, live batch 1/16/32, accuracy/SLA/unavailable-energy rejection,
serving isolation/recovery and comparison negatives. Both mandatory MLIR lanes
run the complete source/native workload; the Clang lane retains strict
ASan/LSan/UBSan. Existing serving/training TSan gates remain mandatory. The
unsanitized GCC MLIR lane additionally runs the actual paired Python baseline.
Test data are public, pinned and attributed; synthetic comparison-validator
fixtures never stand in for measurements.

The current plan has two implementation batches including PR101, and one afterward (PR102).
PR97 adds [native trace replay and declared engineering policies](ai_comparison_measurement.md). Physical measurements, performance budgets, larger workload
families, protected signed release operation, authenticated independent field reproduction and certification remain open. PR99 implements the CPU-scope pilot aggregate, PR100 the lifecycle/cloud validator and PR101 candidate independent-pilot verification; these contracts do not replace external evidence.
