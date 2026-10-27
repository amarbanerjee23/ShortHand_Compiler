# Immutable model pilot

Revision: `cb25d880e76f6017d960bba14783715e87a7ea24`; manifest SHA-256: `b01d0ead60bf44bddb481ff78c7e625dc312b6d7e7329cdd8a651f9d893368f2`.

Identity FP32 [1,4], nonzero checked inputs; six balanced pairs, 8 warmups and 2048 timed calls per child. Compare internal immutable ownership with AIRuntime mutable-path caching through the same native host. Both validate inputs, check outputs and commit via private scratch. Immutable also includes the facade lock; this is not a source-language or C++ superiority comparison.

| Model bytes | Immutable ns/call | Mutable ns/call | Paired ratio median (min–max) |
|---:|---:|---:|---:|
| 88 | 9837.8 | 15430.6 | 0.6063 (0.4850–0.7750) |
| 65536 | 11539.2 | 19504.6 | 0.6066 (0.5363–0.7331) |
| 1048576 | 10978.3 | 91366.8 | 0.1106 (0.0914–0.1346) |

Warm model-I/O trace verified: **False**. Separate traces include all threads, the mutable positive control and an injected-read control. Instrumented durations never enter the latency rows.

Energy available: 0/6. Separate whole-child-process component energy includes setup, all 2057 verified calls and teardown. Missing energy is null, not zero. No calibrated estimate, phase attribution or energy saving is inferred.

Cold setup/first-call durations, all observations, commands, source snapshots, raw traces and hashes are retained. Shared-host short-run ratios are descriptive, without confidence intervals or general reduction claims.

Local capture: 2026-10-26, GCC 13.3.0, ONNX Runtime 1.30.0, AMD EPYC 9V74. The capture used the implementation working tree based on the revision above; source snapshots and the tracked diff are retained in the bundle. The local environment denied all three ptrace attempts; no measured zero-read result is asserted here. All six energy records report `amd_hwmon_socket_energy_unavailable`. CI creates a new exact-checkout bundle and requires successful tracing before accepting it.
