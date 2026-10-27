# Immutable model hosted observations

Runtime revision: `4f187ac8732cb1fb9c6cb01f8095d18bdf75cf4b`; comparison base: `cb25d880e76f6017d960bba14783715e87a7ea24`. Captured on 2026-10-27 by [push CI 40464209029](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/40464209029), attempt 1. The [retained profile artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/40464209029/artifacts/12715379427) contains the complete immutable evidence bundle. Manifest SHA-256: `bae61d7fcac6b707f5c8e266c5a8cbf4bc12ab88dd1f15b2032dd6566ef4b47e`.

Host: AMD EPYC 9V74, Linux x86_64, Clang 18.1.3, ONNX Runtime 1.30.0. Identity FP32 [1,4], checked nonzero inputs, one inference thread, six balanced AB/BA pairs, eight warmups and 2,048 timed calls per child. The 64 KiB and 1 MiB files add legal doc-string padding to the same graph; they isolate model-byte validation cost and do not represent larger neural networks.

| Model bytes | Immutable median ns/call | Mutable median ns/call | Paired ratio median (min–max) |
|---:|---:|---:|---:|
| 88 | 18,910.4 | 26,454.1 | 0.7162 (0.7076–0.7318) |
| 65,536 | 18,846.5 | 29,677.8 | 0.6319 (0.6264–0.6472) |
| 1,048,576 | 18,747.9 | 88,250.2 | 0.2122 (0.2108–0.2142) |

The paired ratio is immutable/mutable, computed within each block before taking the median. Both native hosts validate inputs and check outputs using private scratch; immutable execution additionally includes the serialized facade. The mutable control uses the existing AIRuntime cached path, including full model-byte validation. This is an internal capability comparison, not generated ShortHand versus C++, an allocation count or a general superiority result. Short shared-host ratios are descriptive; no confidence intervals or broad latency claim are inferred.

Separate `strace -f -yy` runs include all threads and use flushed warm-region markers. Their durations never enter the table above.

| Mode | Warm calls | Model-I/O syscalls | Model opens | Model reads |
|---|---:|---:|---:|---:|
| Immutable | 16 | 0 | 0 | 0 |
| Mutable positive control | 16 | 48 | 16 | 32 |
| Injected-read positive control | 16 | 32 | 16 | 16 |

These counts concern model-path file activity in the declared warm region, not all process I/O or physical storage traffic. The controls prove that the tracing boundary detects the relevant reads. Contract tests separately cover freeing/mutating the source bytes after binding, digest and dependency rejection, failed replacement rollback, aliasing, finite outputs, policy changes, concurrent reset and stale handles.

All six separate whole-child-process component-energy attempts reported `amd_hwmon_socket_energy_unavailable`. Each includes setup, first call, eight warmups, 2,048 timed calls and teardown, with 2,057 verified calls. Joules and joules per verified call remain null. No energy saving, power reduction, calibrated estimate or carbon result is supported.

Both initial CI runs passed. The [development release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-40464209029-1-4f187ac8732c) includes all four native installers and 37 assets. Its tag resolves to the runtime revision above. The stricter source-snapshot replay follow-up also accepts the downloaded bundle and rejects all nine corrupted variants. Later commits need their own CI; these numbers remain attached to the recorded implementation revision.

To replay with the current repository code, extract the artifact and run:

```sh
python3 experiments/energy/immutable_model_evidence.py --output PROFILE/immutable --replay-sha256 bae61d7fcac6b707f5c8e266c5a8cbf4bc12ab88dd1f15b2032dd6566ef4b47e
python3 experiments/energy/test_immutable_model_evidence.py --bundle PROFILE/immutable
```

The artifact retains cold setup/first-call times, all 36 timing observations, six energy attempts, traces, source snapshots, SDK/executable bytes, commands and hashes for 90 days. A manifest digest provides integrity relative to a trusted digest, not independent measurement authentication. Archive the complete bundle before expiry if using this campaign beyond that retention period.
