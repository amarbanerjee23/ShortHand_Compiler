# Public release implementation after PR119

Audit baseline: `3417f8ed261c32aa59f09080cc4d325b316edc0e` (merged GitHub PR119).

The requested public CPU-inference scope includes Linux x64, Windows x64 and macOS ARM64. Windows and macOS require real ONNX Runtime execution from installed release payloads before their support claims can be promoted. Existing compiler portability checks do not provide that inference qualification. Linux ARM64 compiler packaging is retained; other CPU architectures and accelerator inference need separate numerical evidence.

Implement and review the following changes sequentially. These are engineering acceptance conditions, not authorization to publish or declare GA.

| Order | PR scope | Required exit evidence |
| --- | --- | --- |
| 1 | Parser allocation lifetime | Session-owned ASTs, tokens and source ranges; retained multi-file graphs; deterministic repeated-parse regression; ASan/LSan/UBSan extended fuzz with the previously failing seed; mandatory CI green. |
| 2 | Usable CPU packages | CMake installs both CLI tools; release bundle selection excludes policy artifacts; pinned, verified ONNX SDKs and runtime dependencies for Linux x64, Windows x64 and macOS ARM64; relocated install compiles and executes a real ONNX model on each native OS; missing SDKs and missing payloads fail closed. |
| 3 | Release governance | Active roadmap/truth reconciled with merged work; closeout accepts verified evidence without hard-coded historical revisions or unconditional refusal; negative controls still reject missing, stale or mismatched evidence; branch/ruleset protection and signed-release exercise retained as explicit operational gates. |
| 4 | Public onboarding and qualification | Versioned platform/dependency support matrix, install/uninstall instructions, first CPU-inference example, language limitations and compatibility policy; a clean-machine release-candidate rehearsal on every claimed platform; final evidence-linked release decision. |

Each PR updates `docs/latency_energy_test_and_benchmark_results.md` with its exact base and observed results. A platform is only qualified after its native checks pass on the candidate revision and installed payload. The first releasable milestone is a publicly installable, accurately scoped controlled-beta/RC; GA remains subject to the existing production blockers and independently retained operational evidence.

The current `linux-x64-cpu-v1` qualification, `controlled_beta` maturity and `production_claim: false` remain the recorded state until the applicable evidence and executable guards are updated together. No latency or memory result substitutes for physical energy measurements, and these PRs do not grant C3-ECO certification.

## Packaging implementation boundary

PR120 passed hosted CI, tooling and the seed-56 extended sanitizer campaign and is ready for review. The following packaging slice introduces shared native archive qualification on all four existing package platforms; see [cpu_release_packages.md](cpu_release_packages.md). Its Windows/macOS scope covers installed interpreter inference and native C ABI consumers. Full typed `.short` source-to-native tensor execution on those platforms remains a required follow-up within the platform-qualification work, before public compiled-AI parity can be claimed. Keep this gate open even if package receipts pass.
