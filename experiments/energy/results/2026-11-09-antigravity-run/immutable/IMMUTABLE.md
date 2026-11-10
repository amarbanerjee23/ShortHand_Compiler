# Immutable model pilot

Revision: `2db9d1da7918ed816d499206df71b09d916725f5`; manifest SHA-256: `326ae53d2c18ccaa93c5e58034398cfa6fe33dee23c566908188208eac38a952`.

Identity FP32 [1,4], nonzero checked inputs; six balanced pairs, 8 warmups and 2048 timed calls per child. Compare internal immutable ownership with AIRuntime mutable-path caching through the same native host. Both validate inputs, check outputs and commit via private scratch. Immutable also includes the facade lock; this is not a source-language or C++ superiority comparison.

| Model bytes | Immutable ns/call | Mutable ns/call | Paired ratio median (min–max) |
|---:|---:|---:|---:|
| 88 | 10623.1 | 30062.7 | 0.3553 (0.3487–0.3691) |
| 65536 | 10436.2 | 33042.9 | 0.3174 (0.3040–0.3324) |
| 1048576 | 10712.0 | 148832.9 | 0.0721 (0.0677–0.0735) |

Warm model-I/O trace verified: **True**. Separate traces include all threads, the mutable positive control and an injected-read control. Instrumented durations never enter the latency rows.

Energy available: 0/6. Separate whole-child-process component energy includes setup, all 2057 verified calls and teardown. Missing energy is null, not zero. No calibrated estimate, phase attribution or energy saving is inferred.

Cold setup/first-call durations, all observations, commands, source snapshots, raw traces and hashes are retained. Shared-host short-run ratios are descriptive, without confidence intervals or general reduction claims.
