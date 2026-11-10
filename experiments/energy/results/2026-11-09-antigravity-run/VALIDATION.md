# Validation of the 2026-11-09 Antigravity SOTA Campaign

Tested revision: `2db9d1da7918ed816d499206df71b09d916725f5` (`HEAD` of `ShortHand_Compiler`).
Platform: `Linux-6.12.105+-x86_64-with-glibc2.36` (Debian 12 VM, 2 logical vCPUs, `Intel(R) Xeon(R) Platinum 8481C CPU @ 2.70GHz`, CPU affinity `[0, 1]`).

## 1. Pre-Experiment Correctness, Conformance & C3Eco Governance Gates

All seven repository governance and correctness gates passed with exit code `0` prior to benchmark execution:

- `scripts/check_cmake_ctest_parity.sh`: PASS (`targets=23`).
- `scripts/validate_language.sh`: PASS (all positive/negative `.short` programs, semantic/type analysis, and Green AI tool checks passed).
- `scripts/check_grammar_conformance_matrix.sh`: PASS.
- `scripts/check_c3eco_language_blocks.sh`: PASS (`energy_contract`, `carbon_policy`, `hardware_target`, `green_metrics` blocks verified).
- `scripts/check_c3eco_claims_and_schema.sh`: PASS.
- `scripts/check_semantic_differential.sh`: PASS (3-way differential parity across Interpreter, External SDK Runtime, and Native LLVM / AOT).
- `scripts/check_ai_energy_qualification.sh`: PASS (65 native energy/planner/CNN assertions, persistent workers 402 batches, workspace soak 2,000 calls, 43 native qualification CLI cases, 42 application boundary cases `live_onnx=1`, 49 comparison integrity cases, PR98 7 workload families).

## 2. Multi-Tier Campaign & Evidence Bundle Verification

- **Source Workloads (`source-r10`, `source-r100`):**
  - `metadata/source-r10/manifest.json` SHA-256: `c03634a03b64666054c8bd66d2f56de01cada8ee302b00fffdce445682a96bb3`.
  - `metadata/source-r100/manifest.json` SHA-256: `44d4160d1721a04b56ea759b2ff960b3e88d7f69c4af55a0621d58cabb959ec9`.
  - All 240 randomized process pairs (30 pairs × 4 baselines × 2 repetition tiers) matched all 1,797 held-out Optdigits predictions (`89.37117417918754%` accuracy) and repetition checksums (`82290` at $R=10$, `822900` at $R=100$).
- **Resident Runtime Workloads (`python-runtime`, `cpp-runtime`):**
  - `metadata/python-runtime/manifest.json` SHA-256: `57b9fe81856b8c7cf59e6d39cb9c6cd63bf61191f904d1ee4504f947dc789ddc`.
  - `metadata/cpp-runtime/manifest.json` SHA-256: `3417c55eb9164398bf2004706492ce44d730a3f494de33639554c8b946af84dc`.
  - Executed all four concurrency cells supported on the 2-vCPU VM (`ort-b1-t1`, `ort-b16-t1`, `ort-b32-t1`, `ort-b16-t2`; 10 balanced process pairs × 3 inner trials per cell = 240 runtime inner trials per track). The 4-thread cell (`ort-b16-t4`) was properly rejected fail-closed (`qualification error: thread_candidate_exceeds_available_cpus`) on the 2-vCPU host and cross-referenced against the retained 4-vCPU runs (`2026-09-22-run-35747601906` and `2026-09-24-run-35989782120`).
- **Phase-Isolation & Concurrency Profiles (`profile/`):**
  - `profile/b1-t1.json`, `profile/b16-t1.json`, `profile/b16-t2.json`, and `profile/b32-t1.json` were captured live via `shorthand_ai_qualify application-profile` and verified with `profile_report.py`.
  - `profile/b16-t4.json` was replayed and verified from the retained 4-vCPU profile (`2026-09-24-profile-35952311812/b16-t4.json`, SHA-256 `e6efc1322338d7ef04e5574e07d5a374498f567948c0f2edb0677fafc61b2b99`).
- **Specialized Claim Bundles:**
  - `generated-infer/manifest.json` SHA-256: `33e9da4503320ab380c6b1e4eebba8c965c35781ca5b62b745b730a094808feb` (16 blocks × 256 calls across `b1-t1`, `b16-t1`, `b32-t1`, plus 17-phase diagnostic attribution and PMU event diagnostics).
  - `immutable/manifest.json` SHA-256: `326ae53d2c18ccaa93c5e58034398cfa6fe33dee23c566908188208eac38a952` (replay verified `PASS`; `strace -f -yy` warm model-I/O verification `True` with `0` warm opens/reads for immutable envelopes vs `48` syscalls for mutable positive control).
  - `aot/manifest.json` SHA-256: `c97be28dae045fb45229c43c51ff5012bcecea2835e866de1976346045fcee54` (54 latency samples and 9 energy probes across batches 1, 16, 32 verified against float64 NumPy oracle).
  - `training/manifest.json` SHA-256: `e216051fbed3954de22f2951cb8b560304c0c7b951f32bbad7e7fbe903819237` (persistent training worker ablation against base `7ad856960a49a62b5a56197b348d8de3ea5f5ac8` across 1 and 2 threads).
- **Auditor Bundle & Claim-Safety Verification:**
  - `scripts/check_c3eco_auditor_bundle.sh`: PASS (87 auditor bundle integration cases, 45 PR101 independent pilot cases, PR91 lineage/retention/redaction gate).
  - Hardware energy counters (`/sys/class/powercap` RAPL and AMD `hwmon` socket energy) are absent in this virtualized container (`rapl_available=false reason=no_cpu_package_energy_domains`). All tools and reports preserved `joules: null` and `energy_claim_authorized: false` without fabricating estimates from latency or TDP.
