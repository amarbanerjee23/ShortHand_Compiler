# Local AOT feasibility capture — 2026-10-20

This records the implementation working tree on merged PR123, not a clean committed revision or a release qualification. The raw local bundle is `argon-validation/aot-final`; hosted CI separately produces a complete retained bundle for its exact checkout. This snapshot remains descriptive if the transient local raw bundle is no longer available; it must not be promoted to a verified public claim without the underlying evidence.

- Base revision: `44af710d7bf280a931b3d218299e11baf27a345c`.
- Working-tree diff SHA-256: `8e6a2fcd0a088710be1579da2b5957a6f1bc3ab8ed0c0e7192a868eb8326ecc0`; exact diff retained as `source.diff` in the local bundle.
- Manifest SHA-256: `96ae378688797fceb0a72348a059f38ca41fb7d4631f69de40bb8ccc0a97899a`.
- CPU: `AMD EPYC 9V74 80-Core Processor`; shared/container host, Linux x64.
- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`; strict FP32 flags retained in each command.
- ONNX build dependency: `1.23.2`; ORT SDK 1.30.0, library SHA-256 `245a6f8c38127551057a1cd1ffd59f0a186a227ade4f3492dea2494eb565542e`.
- All 54 latency samples and nine separate energy executions completed with checked outputs. All nine component-energy records are unavailable (`amd_hwmon_socket_energy_unavailable`); joules and joules/vector are null.
- Import/native-contract suite: 3 tests with positive and negative subcases; replay suite: 5 tests, including deliberately corrupt derivatives. Local LeakSanitizer cannot inspect `/proc` in this container; hosted leak detection remains mandatory.

## Retained final-dependency observations

AOT versus ORT backend experiment; no ShortHand source or language attribution.

Static FP32 MatMul+Add, deterministic nonzero input, batches 1/16/32, one thread. Both backends consume the same buffers and verify every output against a float64 NumPy oracle rounded to FP32 (atol=1e-5, rtol=1e-4). Six balanced three-runner permutations; every observed pair is retained. No shared-runner superiority claim is authorized.

| Batch | AOT ms/vector | ORT BASIC ms/vector | ORT ALL ms/vector | Paired AOT/BASIC median (range) | Paired AOT/ALL median (range) |
|---|---:|---:|---:|---|---|
| 1 | 0.000260 | 0.001287 | 0.001411 | 0.2073 (0.1626–0.2362) | 0.1852 (0.1360–0.3158) |
| 16 | 0.000386 | 0.000136 | 0.000125 | 2.7507 (2.5710–3.1058) | 2.9874 (2.7847–3.2875) |
| 32 | 0.000450 | 0.000119 | 0.000098 | 3.8656 (2.8205–4.6479) | 4.4223 (3.0385–5.8929) |

Ratios below 1 mean lower observed AOT time in this boundary. AOT is a backend candidate compiled through C++17, not evidence that ShortHand beats C++. No ShortHand+AOT same-artifact host comparison is implemented yet.

Energy is a separate whole-process component-counter pass, including setup, warmup, all verified calls and teardown. Missing counters or ambiguous wraps produce unavailable/null energy. No physical-system energy, calibrated estimate, carbon or certification claim is made.

| Batch | Code generation seconds | Native compile seconds | Object bytes | Explicit workspace bytes |
|---|---:|---:|---:|---:|
| 1 | 0.012203 | 0.214672 | 5760 | 40 |
| 16 | 0.001251 | 0.164538 | 5816 | 640 |
| 32 | 0.001097 | 0.164334 | 5816 | 1280 |

Compilation cost is excluded from resident times. Workspace bytes are the explicit caller buffer size, not a measured whole-process memory or allocation count. Detailed energy status, cold timing, compiler identity, raw outputs, commands and digests are retained in this bundle.

## Earlier diagnostic capture

The first local pilot used ONNX 1.18.0 before the dependency security review upgraded the importer to 1.23.2. Its manifest digest is `7722ca70193108d19d497bf30aa1688145c8633562969a9113d362ebffcad150`; its paired AOT/BASIC medians for batches 1/16/32 were 0.1943, 2.9338, 3.6970, and AOT/ALL medians were 0.1947, 3.0711, 3.9019. It is not pooled with the final-dependency capture. Both captures show the same qualitative mixed result; neither establishes a general language or energy saving.
