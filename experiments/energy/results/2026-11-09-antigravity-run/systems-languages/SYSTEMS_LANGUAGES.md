# Systems Languages (C11, C++17, Rust, Go) vs. ShortHand SOTA Benchmark Report

- **Generated UTC:** `2026-11-10T11:50:37.911608+00:00`
- **Platform:** `Linux-6.12.105+-x86_64-with-glibc2.36`
- **Energy Claim Authorized:** `False` (`joules: null` on virtualized cloud hardware)

## 1. Toolchain & Binary Provenance (Track 1 FP64 Classifier)

| Runner ID | Language & Compiler Flags | Compile Time (ms) | Binary Size (KB) | Peak RSS (MB) | Startup + I/O (ms) | Marginal Kernel (ns/img) | Binary SHA-256 |
|---|---|---:|---:|---:|---:|---:|---|
| `shorthand` | ShortHand Compiler (compile-mlir + Clang++ 14.0.6 -O2 -fno-fast-math) | `269.04` | `24.36` | `3.79` | `8.628` | `312.16` | `d1142dc00d59055c...` |
| `c11-clang-o3` | ISO C11 (Clang 14.0.6 -O3 -fno-fast-math, restrict + strtod) | `120.26` | `20.02` | `2.99` | `3.853` | `241.40` | `b9bd5230e01b33bf...` |
| `c11-gcc-o3` | ISO C11 (GCC 12.2.0 -O3 -fno-fast-math, restrict + strtod) | `78.40` | `19.95` | `2.52` | `3.767` | `283.71` | `baabb3eb198a1ac5...` |
| `cpp17-clang-o3` | C++17 (Clang++ 14.0.6 -O3 -fno-fast-math, constexpr array + iostream) | `438.17` | `20.98` | `3.93` | `11.143` | `265.52` | `39490f278908c7f9...` |
| `cpp17-gcc-o3` | C++17 (G++ 12.2.0 -O3 -fno-fast-math, constexpr array + iostream) | `405.44` | `20.91` | `4.09` | `10.898` | `306.55` | `9ac3afbe7c75deec...` |
| `rust-safe-o3` | Safe Rust 1.99.1 (-C opt-level=3, #![forbid(unsafe_code)], FromStr) | `196.35` | `4471.57` | `2.96` | `2.840` | `272.43` | `20b11dcc4bbef205...` |
| `rust-simd-o3` | Optimized Safe Rust 1.99.1 (-C opt-level=3 -C target-cpu=x86-64-v3, fast scanner) | `172.80` | `4450.05` | `2.93` | `1.933` | `230.08` | `4f0ceb7f5b141e9a...` |
| `go-native` | Go 1.27.3 (go build -ldflags='-s -w', GC systems baseline) | `59.73` | `1500.16` | `3.45` | `2.877` | `513.47` | `a8a9ab7250b4a464...` |

## 2. Track 1 — Source-Level FP64 Classifier ($R=10$ and $R=100$, 30 Paired AB/BA Runs)

| Repetitions | Baseline ID | ShortHand Mean (ms/img) | ShortHand p50 (ms/img) | Baseline Mean (ms/img) | Baseline p50 (ms/img) | Baseline p95 (ms/img) | ShortHand vs. Baseline (%) | Paired Bootstrap 95% CI (%) | Accuracy (%) |
|---:|---|---:|---:|---:|---:|---:|---:|---|---:|
| `10` | `c11-clang-o3` | `0.000791` | `0.000791` | `0.000456` | `0.000455` | `0.000468` | `-73.46%` | `[-74.96%, -71.88%]` | `89.371%` |
| `10` | `c11-gcc-o3` | `0.000792` | `0.000793` | `0.000493` | `0.000492` | `0.000510` | `-60.44%` | `[-61.53%, -59.41%]` | `89.371%` |
| `10` | `cpp17-clang-o3` | `0.000793` | `0.000792` | `0.000886` | `0.000880` | `0.000938` | `+10.45%` | `[+9.41%, +11.54%]` | `89.371%` |
| `10` | `cpp17-gcc-o3` | `0.000789` | `0.000789` | `0.000913` | `0.000915` | `0.000930` | `+13.58%` | `[+12.93%, +14.21%]` | `89.371%` |
| `10` | `rust-safe-o3` | `0.000793` | `0.000787` | `0.000430` | `0.000430` | `0.000455` | `-84.18%` | `[-87.13%, -81.54%]` | `89.371%` |
| `10` | `rust-simd-o3` | `0.000783` | `0.000786` | `0.000338` | `0.000334` | `0.000364` | `-131.93%` | `[-136.16%, -127.50%]` | `89.371%` |
| `10` | `go-native` | `0.000806` | `0.000793` | `0.000674` | `0.000673` | `0.000702` | `-19.66%` | `[-23.78%, -17.14%]` | `89.371%` |
| `100` | `c11-clang-o3` | `0.000366` | `0.000362` | `0.000263` | `0.000264` | `0.000275` | `-39.37%` | `[-42.26%, -37.19%]` | `89.371%` |
| `100` | `c11-gcc-o3` | `0.000359` | `0.000358` | `0.000305` | `0.000305` | `0.000309` | `-17.69%` | `[-18.41%, -16.92%]` | `89.371%` |
| `100` | `cpp17-clang-o3` | `0.000361` | `0.000361` | `0.000328` | `0.000328` | `0.000334` | `-10.12%` | `[-10.77%, -9.43%]` | `89.371%` |
| `100` | `cpp17-gcc-o3` | `0.000359` | `0.000360` | `0.000367` | `0.000367` | `0.000376` | `+2.20%` | `[+1.59%, +2.81%]` | `89.371%` |
| `100` | `rust-safe-o3` | `0.000358` | `0.000359` | `0.000288` | `0.000287` | `0.000302` | `-24.26%` | `[-25.49%, -22.96%]` | `89.371%` |
| `100` | `rust-simd-o3` | `0.000358` | `0.000358` | `0.000241` | `0.000240` | `0.000256` | `-48.44%` | `[-50.07%, -46.53%]` | `89.371%` |
| `100` | `go-native` | `0.000361` | `0.000361` | `0.000529` | `0.000529` | `0.000538` | `+31.84%` | `[+31.38%, +32.33%]` | `89.371%` |

## 3. Track 2 — Resident FP32 ONNX Runtime 1.30.0 Host Integration (10 Paired Runs)

| Cell | Batch | Threads | Pure C11 `OrtApi` Mean (ms/img) | Native Rust `OrtApi` FFI Mean (ms/img) | C++17 `Ort::Session` Mean (ms/img) | ShortHand `AIRuntime` Mean (ms/img) | Python ORT Mean (ms/img) | ShortHand vs. C11 (%) | ShortHand vs. Rust (%) | ShortHand vs. C++17 (%) | ShortHand vs. Python (%) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `ort-b1-t1` | `1` | `1` | `0.001290` | `0.001299` | `0.001647` | `0.002663` | `0.021729` | `-106.49%` | `-105.04%` | `-61.66%` | `+87.58%` |
| `ort-b16-t1` | `16` | `1` | `0.000183` | `0.000189` | `0.000313` | `0.000582` | `0.001738` | `-218.80%` | `-208.52%` | `-86.27%` | `+66.88%` |
| `ort-b16-t2` | `16` | `2` | `0.000184` | `0.000190` | `0.000322` | `0.000593` | `0.001789` | `-222.56%` | `-211.45%` | `-84.31%` | `+67.02%` |
| `ort-b32-t1` | `32` | `1` | `0.000141` | `0.000148` | `0.000271` | `0.000505` | `0.001047` | `-258.97%` | `-241.87%` | `-86.22%` | `+51.34%` |

## 4. Track 3 — Static Affine AOT Compilation (6 Latin-Square Blocks × 1,024 Iterations)

| Batch | Workspace (B) | ShortHand AOT p50 (us/vec) | Pure C11 AOT p50 (us/vec) | C++17 AVX2 AOT p50 (us/vec) | Native Rust AOT p50 (us/vec) | `ort_basic` p50 (us/vec) | `ort_all` p50 (us/vec) | C++17 AVX2 Ratio / `ort_basic` | Rust AOT Ratio / `ort_basic` | ShortHand AOT Ratio / `ort_basic` |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `1` | `40` | `0.2771` | `0.1199` | `0.1224` | `0.0839` | `1.2834` | `1.3033` | `0.0969x` | `0.0667x` | `0.2174x` |
| `16` | `640` | `0.3510` | `0.1097` | `0.1011` | `0.1169` | `0.1473` | `0.1490` | `0.6918x` | `0.8024x` | `2.3638x` |
| `32` | `1280` | `0.3458` | `0.1096` | `0.0971` | `0.1186` | `0.1223` | `0.1245` | `0.7979x` | `0.9536x` | `2.8358x` |
