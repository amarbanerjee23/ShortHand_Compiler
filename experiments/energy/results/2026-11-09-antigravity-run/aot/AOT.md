# Bounded CPU AOT observations

AOT versus ORT backend experiment; no ShortHand source or language attribution.

Static FP32 MatMul+Add, deterministic nonzero input, batches 1/16/32, one thread. Both backends consume the same buffers and verify every output against a float64 NumPy oracle rounded to FP32 (atol=1e-5, rtol=1e-4). Six balanced three-runner permutations; every observed pair is retained. No shared-runner superiority claim is authorized.

| Batch | AOT ms/vector | ORT BASIC ms/vector | ORT ALL ms/vector | Paired AOT/BASIC median (range) | Paired AOT/ALL median (range) |
|---|---:|---:|---:|---|---|
| 1 | 0.000277 | 0.001290 | 0.001310 | 0.2131 (0.2083–0.2342) | 0.2099 (0.2014–0.2127) |
| 16 | 0.000351 | 0.000154 | 0.000151 | 2.2776 (2.1098–2.3889) | 2.2982 (2.2707–2.3652) |
| 32 | 0.000340 | 0.000122 | 0.000119 | 2.8243 (2.4392–3.0430) | 2.8783 (2.6419–2.9712) |

Ratios below 1 mean lower observed AOT time in this boundary. AOT is a backend candidate compiled through C++17, not evidence that ShortHand beats C++. No ShortHand+AOT same-artifact host comparison is implemented yet.

Energy is a separate whole-process component-counter pass, including setup, warmup, all verified calls and teardown. Missing counters or ambiguous wraps produce unavailable/null energy. No physical-system energy, calibrated estimate, carbon or certification claim is made.

| Batch | Code generation seconds | Native compile seconds | Object bytes | Explicit workspace bytes |
|---|---:|---:|---:|---:|
| 1 | 0.017792 | 0.164510 | 6008 | 40 |
| 16 | 0.001173 | 0.163971 | 5760 | 640 |
| 32 | 0.001081 | 0.163900 | 5760 | 1280 |

Compilation cost is excluded from resident times. Workspace bytes are the explicit caller buffer size, not a measured whole-process memory or allocation count. Detailed energy status, cold timing, compiler identity, raw outputs, commands and digests are retained in this bundle.
