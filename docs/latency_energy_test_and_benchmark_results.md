# ShortHand testing, experimentation and benchmark results

Comparison base: `7ad856960a49a62b5a56197b348d8de3ea5f5ac8`

## Current PR evidence

This is the single consolidated implementation PR following the October 2026 proposal audit. It is a draft until all bounded implementation packages and exact-head qualification are complete. This checkpoint fixes C++ powercap counter wrap qualification: the maximum possible gap includes the complete ending read, matching the conservative Python sampler. It also adds deterministic delayed-read, hidden-wrap, power-bound, changed-domain and invalid-window regressions.

The existing session cache, PR123 borrowed input/output scratch and PR125 immutable capability are reused. Source tensors, bounded AOT integration, diagnostics and package qualification will be added in independently tested commits on this same PR. Lossy precision and broader operator/adaptive work remain conditional follow-ups. No unimplemented item is represented as complete.

Local validation: PASS `check_ai_energy_qualification.sh`: 65 native energy/planner/training assertions, 2,000 alternating workspace calls with 80 rollbacks, 38 qualification CLI cases, 28 application cases, 49 comparison-integrity cases and 13 benchmark-contract cases. This invocation used the host-only backend; it is not real ONNX or hardware energy evidence. Full exact-head CI and four native installer lanes are required before ready-for-review. The existing successful trusted-push development release workflow remains enabled.

### Persistent bounded training checkpoint

The fixed reference CNN now reuses per-sample gradient buffers and a bounded worker pool across all batches. Convolution and backward scratch use fixed-size arrays. Sample order, FP32 reduction, optimizer, RNG, data and public training configuration remain unchanged. Worker creation failure joins previously started threads; tasks finish before shutdown; exceptions are propagated at the batch boundary. This only qualifies the existing synthetic trainer.

The [local observations](benchmark_results/persistent-training-local.json) retain all 44 runs, compiler identity and the full bundle digest. Both seeds (42 and 7), both thread counts and both versions produced bit-identical final parameter hashes, loss and accuracy. Seed 42 reached 1.0 accuracy and final loss 0.016677837818861008. Timing uses separate executables without allocation instrumentation and includes data initialization, training, validation and worker teardown; it excludes destruction of the returned result. The source ablation links both trainer versions against identical common support code and the same host.

| Threads | Base C++ new calls | Candidate C++ new calls | Median paired candidate/base time |
|---|---:|---:|---:|
| 1 | 10,729 | 521 | 0.9198 |
| 2 | 11,241 | 527 | 0.8804 |

Allocation counts cover ordinary C++ `new`/`new[]` during the training call, not all malloc/aligned allocations or whole-process memory. A separately injected 64-byte allocation is detected as exactly one additional call in all four cells. Six balanced timing pairs per cell are descriptive shared-host observations; no general reduction claim is authorized.

Validation: the existing 65 numerical/energy/planner assertions and application gate pass. The worker test passes 402 batches with bounded construction, task exception recovery, partial construction cleanup, concurrent callers and idempotent shutdown. Two replay regression methods reject corrupted counts, quality, seed, time, parameters, omitted or modified source evidence and altered reports. The worker lifecycle test passes ASan/UBSan with leak detection disabled. Local LeakSanitizer cannot inspect `/proc`, so leak/race qualification remains mandatory in hosted CI. The runtime-profile job now captures and replays a fresh training comparison for each exact checkout, and its evidence index requires both the report and successful replay.

## Energy and power result

The new counter fixtures are synthetic correctness tests, not hardware measurements. The training observations do not authorize a general timing improvement, measured joules, energy saving, carbon reduction, GA or certification claim. Unavailable measurements retain null energy values.

## Reproduction

Run `bash scripts/check_ai_energy_qualification.sh`. Capture the training comparison with `python3 experiments/energy/training_runtime_delta.py --base 7ad856960a49a62b5a56197b348d8de3ea5f5ac8 --output training-evidence --cxx c++`, then run `python3 experiments/energy/test_training_runtime_delta.py --bundle training-evidence`. Its synthetic counter tests also exercise the bounded training and existing application qualification contracts. The real ONNX, sanitizers, compiler differential and native installer gates remain mandatory in CI. Each following implementation commit updates this report with its actual results and limitations.

## Historical PR125 evidence


Comparison base: `cb25d880e76f6017d960bba14783715e87a7ea24`

### PR125 implementation evidence

This PR implements the R2 [owned immutable model capability](immutable_model_capability.md) on merged PR124. It verifies and owns bounded self-contained ONNX bytes once, prepares one CPU session per successful binding and reuses it behind a versioned internal handle. All calls and reset share the facade mutex; failed replacements preserve the prior slot, and released/replaced/reset handles reject. Input borrowing, private output scratch, finite validation and current routing policy remain enforced. The public C ABI, mutable-path content revalidation and default source lowering are unchanged.

| Validation | Local result before hosted CI |
|---|---|
| Real ORT native immutable contracts | PASS: source ownership, digest validation, 25 envelope/graph negatives, failed replacement, 16 MiB boundary, bad I/O, aliasing, finite-output rollback and policy changes |
| Concurrent lifecycle | PASS: 2,000 calls across four workers during 32 resets/replacements; stale handles reject without output mutation |
| Existing mutable cache regressions | PASS: complete content replacement/preserved mtime, external weights, invalidation, recovery, argument matrix, overlap, size boundaries and concurrent reset |
| Frozen C ABI | PASS: exact 25 symbols and frozen C consumer |
| Evidence replay | PASS: 3 tests including nine corrupted-bundle variants and synthetic worker-thread trace parser controls; missing source snapshots reject even with a recomputed manifest digest. Synthetic traces are not measurements. |
| Native candidate receipts | PASS: three receipt-validation tests; omission of immutable lifecycle evidence rejects |
| Sanitizers | PASS: real runtime and contract driver with GCC ASan/UBSan. LeakSanitizer failed because local `/proc` inspection is blocked; the successful local rerun explicitly disabled leak detection. Hosted sanitizer execution remains mandatory. |
| Model file-read tracing | Unavailable locally: ptrace is denied. CI requires real all-thread traces, a mutable positive control and an injected-read control; no structural assertion substitutes for those traces. |
| Native installers and full compiler/security gates | Pending exact-head hosted CI at initial PR creation. The local feature-plan aggregate stops at the missing built compiler; it is not recorded as passing. Existing successful trusted-push development release publication remains enabled. |

### Local timing observations

The [committed pilot snapshot](benchmark_results/immutable-model-local.md) retains the table and manifest identity. On GCC 13.3 / ORT 1.30.0 / AMD EPYC 9V74, paired immutable/mutable median ratios were **0.6063 / 0.6066 / 0.1106** for 88 B / 64 KiB / 1 MiB snapshots. This compares internal native hosts with checked nonzero Identity outputs, six balanced pairs and 2,048 timed calls per process. The larger artifacts use doc-string padding to isolate byte-validation work. These are short shared-host observations, not neural-network quality results or a general language-performance claim.

The immutable candidate includes its facade lock and transactional checks. The mutable control uses the existing AIRuntime cache with complete model-byte validation and private scratch. Cold preparation/first-call times, all 36 latency samples and six separate energy executions are retained. This is a working-tree pilot based on the comparison base, not a claim that the local tree equals a published commit. Hosted CI captures fresh evidence for its exact checkout.

### Hosted implementation qualification

Implementation revision `4f187ac8732cb1fb9c6cb01f8095d18bdf75cf4b` passed all 21 [push CI jobs](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/40464209029), [PR CI](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/40464288027), tooling and experiment-results. Native ownership/lifecycle, hidden shared symbols, real-runtime sanitizers, compiler/toolchain, security, reproducibility and all four native package/installer gates passed. The sanitizer log contains successful immutable contracts and concurrency with leak detection enabled and no fallback warning; the pinned vendor ORT binary itself is not instrumented. The retained run-evidence index is complete with no missing items.

The [committed hosted snapshot](benchmark_results/immutable-model-hosted.md) records a real all-thread warm trace: immutable **0** model-I/O syscalls; mutable **16 opens / 32 reads**; injected-read control **16 opens / 16 reads** across 16 calls each. Uninstrumented paired median time ratios were **0.7162 / 0.6319 / 0.2122** for 88 B / 64 KiB / 1 MiB snapshots. These characterize the declared synthetic Identity boundary and do not establish general language performance or an energy reduction. All six separate hosted energy records are unavailable and retain null joules.

The [runtime artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/40464209029/artifacts/12715379427) retains the raw traces, source, executable, commands and timings. Its immutable manifest SHA-256 is `bae61d7fcac6b707f5c8e266c5a8cbf4bc12ab88dd1f15b2032dd6566ef4b47e`. The stricter follow-up replay validator and all three regression methods, including nine corruption variants, successfully replay this downloaded bundle. The [development release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-40464209029-1-4f187ac8732c) contains 37 assets, including Windows x64, Linux x64/arm64 and macOS arm64 installers; its tag resolves to the tested implementation revision.

This supersedes the implementation's initial pending hosted status above. Subsequent commits tighten source-snapshot evidence validation and record these results without changing runtime code. Their own complete CI and release status will be linked in the PR; the implementation run is not presented as a test of a later commit.

## Energy and power result

All six local component-energy observations are unavailable (`amd_hwmon_socket_energy_unavailable`); measured joules and joules per verified call are null. Each separate child-process boundary includes setup, first call, eight warmups, 2,048 timed calls and teardown. No physical-system energy, calibrated estimate, power reduction, carbon benefit or energy amortization is claimed. All reduction/production claim flags remain false.

## Reproduction

Follow [the immutable capability contract and commands](immutable_model_capability.md#reproduction). The hosted runtime-profile job requires native contract/sanitizer markers, hidden-symbol validation and a replayed report with real trace verification. Installed archive and native installer checks execute the contract suite on Windows x64, Linux x64/arm64 and macOS arm64. The existing generated ShortHand head/base, AOT and real-application comparisons remain separate. Full raw evidence is retained for 90 days; final run, artifact and release links will be recorded in the PR.

## Historical PR124 final status

PR124 merged as the comparison base. Its final revision `29b3e44092f690ec93efd302bd0511535d369843` passed [push CI](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39465977274), [PR CI](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39465985522), security, tooling and experiment-results. The [runtime/AOT artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39465977274/artifacts/12286000514) and [development installer release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-39465977274-1-29b3e44092f6) completed. This supersedes the historical pending status below, which records its initial failed dependency-registration attempt.

Historical PR124 comparison base: `44af710d7bf280a931b3d218299e11baf27a345c`

### PR124 initial evidence

The base is merged PR123. The [Argon proposal review](argon_aot_energy_proposal_review.md) audits the supplied ideas and implements an opt-in static FP32 MatMul+Add CPU AOT experiment plus a validity fix for CI RAPL counter sampling. The experiment emits C++17 source and compares native arithmetic against prepared ORT BASIC/ALL using the same C++ host and checked nonzero inputs. It is not `.short`/MLIR AOT integration and does not alter default source compilation, runtime ABI, precision or model-path semantics.

RAPL readings now include the full read-window/scheduling gap when deciding whether wraps can be disambiguated. A small positive delta after a long interval is rejected instead of being accepted as a small energy reading. No calibration, measured energy saving or power reduction is implied by this correction.

| Validation | Local result before hosted CI |
|---|---|
| Bounded importer and native execution | PASS: 3 test methods with shape/opset/IR/encoding/external-data/unknown-field negatives; nonzero numerical oracle; aliasing, failure rollback and 100 concurrent-call cases |
| AOT vs real ORT 1.30.0 | PASS: all 54 latency samples and nine separately executed energy-boundary samples have checked outputs; batches 1/16/32, one thread |
| Hashed replay and corruption rejection | PASS: 5 tests; incomplete/duplicate/misordered/unhashed observations, artifact sizes, invalid energy/counts/times, incorrect digests and premature report publication rejected |
| CI energy collector regressions | PASS: 11 tests, including ambiguous/single wraps, counter range, power bound and delayed read windows; synthetic counters are not measurements |
| Run-evidence and Markdown replay | PASS: 4 tests; new AOT report and sanitizer success marker required |
| Action pinning/status hygiene and diff whitespace | PASS |
| Local sanitizer and full production-truth aggregate | Unavailable locally: LeakSanitizer cannot inspect `/proc`; full aggregate stops at missing `bison`. Hosted sanitizer/native/compiler gates remain mandatory, without skips added by this PR. |
| Windows/Linux/macOS installer lifecycle and release | Existing CI gates retained; exact-head hosted execution and release publication are pending at initial PR creation. |

### Initial hosted validation and dependency-policy correction

Implementation revision `2d952d4b6e2b5d62ab77d29ccf56ab72f7c49957` completed the [push CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39377992189) and [PR CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39378037843). All compiler/toolchain, CTest parity, MLIR, reproducibility, AOT/profile and four native installer jobs passed. The [hosted AOT/profile job](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39377992189/job/118480359234) passed the importer/native suite with ASan/LSan/UBSan enabled, real AOT/ORT capture, all five replay tests and mandatory evidence validation. This resolves the local sanitizer-environment limitation for that revision.

The overall CI result failed because `experiments/aot/requirements-linux-x64-py312.txt` was missing from the repository-owned dependency registry; the security job stopped before its remaining scans. The aggregate gate therefore blocked release publication. This follow-up registers that manifest and records ONNX 1.23.2, ml_dtypes 0.5.4 (including its bundled Eigen MPL-2.0 notice) and typing_extensions 4.15.0 as hash-pinned build/test dependencies, not redistributed language-runtime components. The existing security checks remain mandatory; the follow-up revision needs its own complete CI run.

Retained exact-implementation evidence: [runtime profile artifact 12244735376](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39377992189/artifacts/12244735376), including the complete AOT bundle. Native release candidate artifacts were produced for all four targets, but no release was published from the failed aggregate. Hosted benchmark values are not substituted for the separately identified local observations below.

### Local backend observations

The [committed local benchmark Markdown](benchmark_results/argon-aot-local.md) records the final dependency capture and the earlier diagnostic pilot, provenance, all ratio ranges and compilation costs. On the final local GCC 13.3/AMD EPYC 9V74 capture, paired AOT/BASIC median ratios were **0.2073 / 2.7507 / 3.8656** for batches 1/16/32; paired AOT/ALL ratios were **0.1852 / 2.9874 / 4.4223**. Lower is faster. Batch 1 favored AOT; batches 16 and 32 favored ORT. These are short shared-host observations with a checked-call boundary, not a statistical superiority claim. AOT performs additional internal transactional checks, and compilation cost is outside resident timing.

Manifest SHA-256: `96ae378688797fceb0a72348a059f38ca41fb7d4631f69de40bb8ccc0a97899a`. This identifies a working-tree capture based on the comparison base; it is not a claim that uncommitted source equals a published revision. CI produces fresh exact-checkout artifacts containing the full source diff, model/oracle/input bytes, generated source/object/binaries, commands, raw outputs, reports and digests. The final run/artifact links will be recorded in the PR after hosted checks finish; no hosted result is inferred from local tests.

### Energy and power result

All nine local AOT/ORT component-energy records are unavailable: `amd_hwmon_socket_energy_unavailable`. Measured joules and joules per verified vector are null. No dedicated meter, calibrated estimate, phase-energy attribution, power comparison, carbon reduction or energy break-even value is reported. The whole-child-process energy boundary includes setup, warmup, all verified calls and teardown; it is separate from resident latency and is not process-attributed even when CPU counters exist.

The RAPL power bound is an explicit conservative assumption, not a calibration. Counter integrity and valid comparisons still require domain coverage, reset handling, host activity, uncertainty and independent repeated measurements. `latency_claim_authorized`, `energy_claim_authorized` and production superiority claims remain false.

### Reproduction

Follow the locked dependency, native compiler, sanitizer, capture and replay commands in [the proposal review](argon_aot_energy_proposal_review.md#reproduction). `runtime-profile.yml` executes them against Clang 18/ORT 1.30.0 on every normal push and PR. The existing generated `.short` head/base comparison and native installer matrix continue independently. Replay is standard-library-only and requires the expected root digest; a digest is not a signature or measurement-authenticity proof. Hosted raw evidence uses the existing 90-day retention policy.

### Historical PR123 evidence

Previous comparison base: `72142acd9750fe32b1d0e1f70b8dd06e25ba7430`. The following results belong to the earlier borrowed-input/lazy-observation change, not the new AOT experiment.


#### Borrowed input and deferred bridge observations

The base is merged PR122. This PR implements the first bounded slice of the supplied post-PR122 proposal and records the corrected full sequence in [post_pr122_energy_implementation_plan.md](post_pr122_energy_implementation_plan.md). It removes the synchronous C bridge input-vector copy, introduces explicit read-only FP32 borrowing for ONNX prepared and path-based execution, and clears caller pointers on all exits. The owning adapter remains owning. Runtime-owned transactional output scratch, per-call model-content comparison, policy invalidation and the frozen public C ABI remain unchanged.

Request JSON and the bridge telemetry envelope are formatted on demand from owned snapshots. Registrations may change before observation without corrupting the previous call's evidence. Backend telemetry is still generated; this is not an allocation-free or fully zero-copy inference claim. The proposal's unchecked model-binding hook, metadata-only file validation, unproved LLVM pointer forwarding and hard-coded zero-allocation training assertions are not included.

| Validation | Observed result before hosted CI |
|---|---|
| Frozen public ABI v1 and real SDK execution bridge | PASS: exact 25-symbol manifest, frozen/current C consumers, successful ONNX execution and no-SDK failure behavior |
| GCC normal and phase-profiled runtime archives with verified ONNX Runtime 1.30.0 | PASS, strict warnings enabled |
| Real ONNX cache and ownership regression | PASS: mutable file replacement including preserved size/mtime, external weights, policy changes, reset/recovery, finite/capacity negatives, lifecycle concurrency, 65536/65537-element inputs and partial input/output overlap |
| Real phase-profile isolation and failure paths | PASS; ordinary runtime archive contains no diagnostic symbols |
| Borrowed input and observation snapshot regression | PASS: pointer identity, owning adapter compatibility, null/empty/wrong-count rejection, deferred registration snapshots, repeated query, observability/OTLP consistency, failure/reset clearing |
| Generated-evidence regression suite | PASS, 17 tests; observer/head/base matrices remain separate; missing, duplicate, unhashed, misordered and wrong-boundary samples rejected |
| Run-evidence and Markdown replay regressions | PASS, 4 tests |
| Hosted native packages, full regression matrix, sanitizers and same-runner performance | PASS on runtime revision `ca62eca1dc33635fffeaa74cf613e19a373e6cdf`: 21 push CI jobs and 20 applicable PR CI jobs; both evidence collections complete with no missing items. See the retained [two-run results](benchmark_results/pr123-runtime-observations.md). |

The compiled benchmark now retains an additional ABBA head/base series for b1-t1, b16-t1 and b32-t1 with all four JSON exporters consumed on every timed call. Its results appear separately in `GENERATED.md` and `report.json`; no observer measurements are mixed with inference-only or direct-ORT timings. The original diagnostic phase and component-energy boundaries remain separate. Existing `ci.yml` runs the evidence workflow on every push and PR; the extra experiment workflow now also reacts to compiler, MLIR and benchmark changes.

#### Verified hosted observations and installers

The [committed result snapshot](benchmark_results/pr123-runtime-observations.md) records both hosted runs for runtime revision `ca62eca1dc33635fffeaa74cf613e19a373e6cdf` against the exact merged PR122 base. Compiled head/base mean-time ratios were 0.8643–0.8946 on the Intel push runner and 0.9091–0.9146 on the AMD PR runner. Consuming every JSON exporter on every timed call gave much smaller differences (ratios 0.9651–0.9948 across the two runs). The resident real-digit control had mixed small changes, and every cell's paired range crossed 1. Direct prepared ORT remained faster. These are descriptive observations from shared runners, not confidence-bound or language-wide performance claims.

The [development release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-39146759972-1-ca62eca1dc33) published all 37 assets, including native Windows x64, Linux x64/ARM64 and macOS ARM64 installers. Its tag resolves to the tested runtime revision. The successful publisher verifies all four native lifecycle receipts and downloads the complete uploaded asset set for SHA-256 comparison before publication. Installers remain unsigned development candidates with the existing platform opt-in and production restrictions.

### Energy and power result

Both hosted capability probes found no readable CPU package/socket energy counters and no matching E2/E3 calibration/model inputs. The highest available energy evidence class is null, physical system energy was not measured, and no joule saving or lower-power claim is supported. Removing a copy and deferring serialization are engineering changes, not physical-energy measurements; latency is not converted to joules. No dedicated metered runner is assumed. Comparative energy, lowest-carbon, production-scope and certification flags remain false.

The attached predictions of 1.05–1.15x overhead and 35–70% energy savings are unverified and are not reported as results. Historical PR117 ratios below are not a current PR122 baseline. See the new plan for same-artifact controls, calibration limits, quality constraints, independent confirmation, uncertainty and scoped claim wording.

### Reproduction

Use `.github/workflows/runtime-profile.yml` for the complete compiler/normal/profiled SDK builds and exact-base comparison. Run `bash scripts/check_runtime_ai_bridge_link_build.sh`, then set `ONNXRUNTIME_ROOT` to the verified SDK for `bash scripts/check_runtime_ai_bridge_execution_path.sh`. Run `tests/ai_application/test_compiled_cache.py BUILD SDK` and `tests/ai_application/test_runtime_phase_profile.py BUILD SDK` with the matching `CXX`. Run `python3 experiments/energy/test_generated_infer_evidence.py`, `test_ci_run_evidence.py` and `test_render_benchmark_markdown.py` from their full repository paths. `generated_infer_evidence.py --build BUILD --base-build BASE_BUILD --sdk SDK --head-sha SHA --base-sha BASE --output OUTPUT` captures both observation frequencies automatically; replay using the retained `manifest.sha256`. ASan/UBSan and TSan plus all four archive/installer lifecycle gates remain mandatory in CI.

### Completed PR122 release baseline

PR122 head `6923c7f378b36fff7fe59d702bba58fd19228acb` passed all 21 [push CI jobs](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38566168461), all 20 applicable [PR CI jobs](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38566174171), and [tooling](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38566173888). The [public development release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-38566168461-1-6923c7f378b3) includes 37 verified assets and native Windows x64, Linux x64/ARM64 and macOS ARM64 installers. Its tag and target revision were verified after publication. This closes the then-pending PR122 entries preserved historically below; it does not qualify the new runtime changes.

Comparison base: `30bfee63809307c48b2de20ddb10d9c16666730d`

### Historical PR122 evidence

#### Native compilation and installable build releases

The base is merged PR121. Its final PR and push CI matrices and merged-master run [38069633969](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38069633969) passed all 20 mandatory jobs. This PR adds native MLIR compilation in all four package lanes, target-specific metadata and diagnostic ABI handling, real compiled source inference at O0/O2, installer lifecycle qualification, and automatic development prereleases after a successful complete push CI run.

| Validation | Current evidence |
| --- | --- |
| Candidate receipt negative controls | Local pass: missing/native/revision/dirty/hash/production-claim mutations |
| Installer receipt negative controls | Local pass for all four platforms: byte tampering, stale revision, missing uninstall checks and unsafe paths rejected |
| Build publication policy | Local pass: PR, PR-target, workflow-run, fork, tag and malformed identities rejected; reruns have distinct tags |
| Initial hosted push run | Four toolchain lanes and macOS SDK lifecycle passed; new fixture required a leading declaration, and Windows LLVM definitions required argument splitting; both corrected |
| Hosted revision `86c5be1` | Push and PR matrices each passed 17 jobs; Linux x64/ARM64 and macOS installers completed their full native lifecycle. Windows exposed MLIR 22 GEP/entry-block API changes; compatibility added. Aggregate correctly failed and no release was published |
| Revision `af5dfa1` native follow-up | Linux/macOS installers and both Linux MLIR regression lanes pass. Windows still fails before packaging; connector log retrieval returned transport errors, so its full compiler log is now preserved as a downloadable diagnostic artifact |
| Revision `5890c3d` Windows diagnostic | [Push run 38344446298](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38344446298) passed 17 jobs. Preserved log artifact `11787839500` confirms compilation passes; GNU ld fails in `libMLIRLLVMDialect.a` with unresolved symbols and COFF relocation overflows. Windows Clang MLIR targets now propagate `-fuse-ld=lld`, matching MSYS2's MLIR linker choice; hosted verification of this correction is pending |
| Revision `cd7902d` Windows archive | [Push run 38510447930](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38510447930/job/115677481822) confirms LLD resolves the link failure. The installed Windows archive passes all five CTest consumers, typed source O0/O2, production refusal, missing-model checks and the numeric interpreter check. NSIS builds the installer but rejects the MSYS2-style forward-slash destination; normalize installer/uninstaller CLI and script paths to Windows backslashes. A local regression covers spaces and both unquoted override forms; native lifecycle verification remains pending |
| Revision `0ceda2a` NSIS follow-up | [PR run 38511217850](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38511217850/job/115680183271) again passes the Windows archive suite but exposes MSYS2 Python 3.14's patched `ntpath.sep`: even `PureWindowsPath` prints forward slashes. The final NSIS boundary now explicitly replaces them; a local reproducer and regression simulate this behavior, and the regression also runs under native Windows Python before compilation |
| Revision `46024c0` native preflight | [Push run 38512222972](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38512222972) and [PR run 38512226789](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38512226789) each pass 17 jobs. The Windows NSIS command regression passes; installer receipt fixtures expose a separate `scandir`/constructed-Path spelling mismatch in MSYS2 Python 3.14. Candidate verification now compares exact entry names and retains count/hash validation; regressions cover equivalent parent spelling and extra-installer rejection. Native lifecycle and publication remain pending |
| Final native Windows and complete matrix | Required hosted gates; awaiting updated revision results |
| Release upload/download integrity | Local injected upload-error and downloaded-byte corruption tests pass: draft rollback occurs and publication is blocked. Actual publication remains pending |

The compiled source test uses zero-initialized typed tensors; separate interpreter and C ABI tests verify exact numeric output 42. Arbitrary typed tensor input values remain an API capability, not new source syntax. No platform is added to the default production allowlist.

### Energy and power result

This change adds packaging and qualification, not a claimed runtime speedup. The unchanged mandatory runtime-profile workflow continues collecting latency, allocation and modeled energy evidence on each push. No new measured joules/task or lower-power claim is made. Prior benchmark results below retain their original revision scope.

### Reproduction

Run `python3 tests/packaging/test_cpu_release_candidates.py`, `python3 tests/packaging/test_build_releases.py` and `bash scripts/check_signed_release_contract.sh`. Native end-to-end reproduction is `.github/workflows/cpu-packages.yml`; it runs `check_installed_cpu_package.py` against each archive, then `build_native_installer.py` against the actual native installer. Each successful lifecycle receipt binds installer/archive bytes to the exact tested revision.

### Previous PR121 evidence (historical comparison base `813c899c10f016785f5c2280aaaaedfeced78f3d`)

#### CPU release packaging

This change follows merged PR120 and repairs the release distribution path. The current comparison base is its master merge above; the first hosted run used the earlier stacked PR120 head. CMake now installs both primary CLIs, packages the enabled ONNX Runtime dependency with its license notices, and exports a relocatable ONNX CMake target. A single reusable workflow builds CPU archives for Linux x64, Linux ARM64, Windows x64 and macOS ARM64 for both normal CI and releases. The mandatory CI aggregate includes every native package job.

The archive is extracted into a path containing spaces. The original SDK and build directories are temporarily hidden, other ONNX-containing loader paths are removed, and installed static/shared C ABI consumers must produce exact identity outputs for 42, -7.25, 0 and 1024.5. They also check warm route reuse, nonfinite-input rollback and reset. The installed interpreter must execute the real model and return 42; a core ShortHand source is compiled, linked and run separately. Removing the bundled ONNX library must make a fresh consumer configuration fail. Native receipts bind the archive SHA-256, platform, source revision, clean tracked source state, SDK version and required checks. Outside Linux x64, the probes first require default production-policy rejection, then exercise real inference through the existing explicit experimental override and record `experimental_native_candidate`. The current production support scope stays unchanged.

Release artifact names use `release-bundle-*`, excluding `release-closeout-policy`. Candidate verification requires all four exact platforms and their matching receipts; missing platforms, wrong OS/architecture, stale revisions, skipped checks and altered archives are rejected. Publication repeats this verification before signing.

| Validation | Result |
| --- | --- |
| Local Release build, LLVM 18 / ONNX Runtime 1.30.0 | PASS on the development working tree |
| Relocated Linux x64 archive: installed CLIs, static/shared CPU outputs, core compilation, interpreter inference, missing-library negative | PASS locally; original build/SDK paths hidden |
| Candidate selection and receipt negative controls | PASS: policy report excluded; missing/unexpected platform, missing receipt, stale revision, wrong OS/architecture, dirty source, missing check and tampered digest rejected |
| Signed-release contract and existing closeout regression matrix | PASS locally, including 54 inherited closeout cases |
| CI status hygiene and platform contract guard | PASS locally |
| Native Linux x64 / macOS ARM64 / Linux ARM64 archive execution | PASS on head `dbd84bcfb507927cee865100120ed934f52d6384`, CI run 37808859561; macOS and ARM64 retain explicit experimental scope |
| Native Windows x64 archive execution | PASS on `a7fa4cfeb46dd31ac90736c23c41d03f19d956fb`, including app-local DLL identity, both consumers, interpreter and missing-runtime negative; combined receipt acceptance still requires the checkout correction below |
| Full compiled tensor source path on Windows/macOS | Separate release blocker; this PR tests interpreted ONNX execution and native runtime consumers, not unqualified MLIR portability |

The local build is a development tree based on the SHA above; its receipt is not a clean committed release qualification. Hosted results must be retained before marking the package PR ready. This change makes no latency, power, energy or GA claim. Full operational release qualification and source-to-native AI parity remain required as described in [cpu_release_packages.md](cpu_release_packages.md).

### PR121 first hosted run and fixes

[CI run 37761804858](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37761804858), on initial head `9aa4e03b3f58fe60fb1fc45e12f743a776dceb4f`, caught these integration issues:

| Failure | Root cause and correction |
| --- | --- |
| Evidence workflow | The required `Current PR evidence` heading was renamed. Restore the contracted heading; retain all required report sections. |
| Linux x64 archive | Ubuntu's LLVM 18 installation exposes `llc-18`; the consumer searched only for `llc`. Resolve the versioned binary first. |
| Windows archive | The MSYS2 shell did not expose Git, needed for provenance. Install Git explicitly with the package-job dependencies. |
| macOS/ARM64 inference | The production allowlist intentionally rejects non-Linux-x64 hosts. Test default rejection, then use the existing explicit experimental mode to collect native output evidence. Record the mode and require truthful telemetry; do not change the allowlist. |
| Enterprise OTLP/Prometheus and runtime-package installation tests | These three tests built only the previous installed targets. Build both newly installed CLIs before the complete install operation; the third case was found by auditing all `cmake --install` call sites. |
| CTest Makefile AI gate | A source assertion expected the former absolute ONNX link directory. Update it to the relocatable imported-target wiring, backed by the actual archive execution tests. |

The ordinary Windows/macOS/ARM64 compiler jobs, toolchain matrix, reproducibility, security and both MLIR lanes passed on that first head.

### PR121 second hosted run and fixes

[CI run 37808859561](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37808859561), on head `dbd84bcfb507927cee865100120ed934f52d6384`, passed the native Linux x64, Linux ARM64 and macOS ARM64 archive jobs. These jobs executed the installed interpreter, static/shared CPU consumers, compiled core source and missing-runtime negative from the relocated archives. The production guard passed before experimental inference on macOS/ARM64. Ordinary platform jobs, all four toolchain lanes, security, reproducibility, CTest parity, both MLIR lanes and latency/energy evidence also passed. Tooling and experiment-results workflows passed.

Two remaining failures are corrected in this revision:

- Windows built and staged the archive, but native CMake emitted a drive-letter checksum path that could not match the MSYS2 prefix removed by `sed`. Generate checksum entries from basenames and write LF endings explicitly. A regression fixture reproduces native Windows path and CRLF formatting; it fails with the previous producer and passes with the correction, while the archive-tamper and unsigned-publication negatives remain enforced.
- Ubuntu CTest passed 22 of 23 tests; `runtime_production_packaging` still omitted the newly installed CLIs from its partial build. Add both targets, matching the two adapter fixtures fixed earlier.

Windows execution, all-four candidate aggregation and the complete amended CI result remain pending. The archive scope does not include a minimal end-user Windows image rehearsal or full typed source-to-native AI parity.

#### Windows runtime selection follow-up

On `bf65dffd5aa5d657eec996d8b915f9b18e0d596b`, [Windows job 113731439956](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37903540967/job/113731439956) passed archive verification, installed CLI execution, consumer compilation and default production refusal. Both inference consumers then loaded the machine's ONNX 1.17.1 instead of the bundled 1.30.0 and failed with an API-version mismatch. PATH priority cannot override the Windows system-directory search order.

Consumers now stage the imported runtime DLLs beside their executables using CMake's runtime dependency list. Qualification checks the deployed ONNX hash against the extracted bundle and verifies the loaded module is app-local before inference. The package documentation includes the same deployment requirement and a CMake example. Native Windows verification remains required; no inference pass is inferred from compilation alone. macOS and Linux ARM64 archive execution passed on `bf65dff`.

#### Native package results and Windows receipt correction

Head `a7fa4cfeb46dd31ac90736c23c41d03f19d956fb` passed all four native archive jobs in [PR CI 37904344364](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37904344364) and [push CI 37904337310](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37904337310). Windows executed four exact outputs through both C ABI consumers, verified the loaded app-local ONNX DLL, checked default production refusal, executed the compiled core source and interpreter inference, and passed the missing-runtime negative. All 18 primary PR CI jobs passed, including Ubuntu core, CTest parity, both MLIR lanes, security, reproducibility and latency/energy evidence. Only candidate receipt aggregation and its dependent aggregate failed.

The retained [Windows archive receipt](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37904337310/artifacts/11603534143) has matching source revision, native architecture, ONNX version and archive digest, but `source_dirty:true`. Its installed source header and documentation contain CRLF and match the committed LF bytes after normalization. Windows Git performed the checkout using its automatic conversion policy; the later MSYS2 Git status used a different default. The workflow now disables automatic conversion before checkout and records that policy locally for both Git installations. Qualification checks the tracked source before and after execution and fails with changed paths instead of emitting a misleading pass receipt. A regression reproduces the CRLF mismatch and verifies that actual staged and unstaged edits are still rejected; untracked build output remains excluded.

The clean-source requirement and aggregate acceptance rules are unchanged. The corrected candidate receipts and final CI result still require hosted verification on the next revision. Native Windows/macOS experiments do not promote the production allowlist or establish full typed source-to-native AI parity.

#### Windows toolchain pin and deterministic cache recovery

Head `94b2e5ec19544d65a1882252189c2af0a65f5eef` passed the three non-Windows native archive jobs and every other primary PR job in [PR CI 38030955404](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38030955404). Both Windows lanes stopped at the existing LLVM-major guard because the rolling MSYS2 repository had advanced to 23.1.3-1. The clean Windows source receipt therefore remains unverified on that head. Both workflows now acquire the six matching Clang/LLVM 22.1.8-3 packages from official MSYS2 archive URLs, check committed SHA-256 hashes and upstream detached signatures, and install them in one transaction. The LLVM 22 requirement is unchanged; other prerequisites still follow their configured repositories.

[Push CI 38030953576](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38030953576) also exposed a scheduling assumption in the cache lifecycle test: all 4,000 worker calls could legitimately encounter a reset/registration gap, violating its unconditional requirement for at least one successful racing call. The PR invocation passed. The corrected test keeps 3,996 calls racing 64 resets, then requires each of the four workers to produce the exact final model output after registration completes. Reset-task exceptions reach waiting workers, and unexpected statuses, output mutation, torn results and post-reset cache recovery remain checked. This changes the test synchronization, not runtime behavior.

Local validation used the actual Linux x64 runtime archive from `a7fa4cf` and ONNX Runtime 1.30.0 with the amended C++ harness, compiled by GCC 13.3. Four retained complete runs passed the full cache boundary/lifecycle suite. The normal-scheduling run recorded 3,499 successes and 501 registration-gap rejections; three single-CPU runs recorded 4,000/0, 1,153/2,847 and 2,473/1,527. Each completed 4,000 calls and 64 resets. These are correctness counts, not latency measurements. Hosted sanitizer execution on the amended head remains required.

The signed-release contract, all 54 inherited closeout cases, platform contract, CI status hygiene, shell syntax and whitespace checks pass locally. All six pinned MSYS2 archive hashes and signatures verify locally against the upstream MSYS2 keyring; installation and native execution still require Windows CI. Final candidate aggregation and the mandatory CI aggregate remain pending on the amended head.

The first pinned-toolchain invocation on `70aee8f2c07583db3b144db0f38f4daddd85467d`, [push CI 38065501834](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/38065501834), exposed two installer integration issues. The ordinary Windows checkout converted the checksum manifest to CRLF, so its strict filename parser rejected the first entry. Explicit LF Git attributes now cover the installer and manifest. The CPU lane preserved LF and verified the first archive hash, but the hosted image's `pacman-key` helper rejected the command because of its keyring permission checks. Signature verification now runs inside the existing package-manager transaction, using a temporary configuration that explicitly requires trusted signatures for every local archive. The configuration must contain `LocalFileSigLevel = Required TrustedOnly` before installation proceeds; the global runner configuration and keyring remain unchanged. Download and hash checks still run for all six archives before installation. A local Git checkout regression with `core.autocrlf=true` confirms byte-identical installer/manifest files and successful parsing of all six entries. The amended cache harness passed the hosted application-boundary and prepared-cache sanitizer steps in the same push run; the complete profiling job was still in progress when this evidence was recorded. Native Windows verification of this correction remains pending.

### Energy and power result

Packaging and explicit experimental execution establish no energy or latency improvement. Physical energy and GA claims remain blocked by the existing evidence policies.

### Reproduction

Run `python3 tests/packaging/test_cpu_release_candidates.py`, `bash tests/ai_runtime/test_onnxruntime_backend_source.sh` and `bash scripts/check_signed_release_contract.sh`. Build/install the enabled CPU runtime with the pinned SDK, then run `scripts/prepare_release_bundle.sh` and `python3 scripts/check_installed_cpu_package.py ARCHIVE PLATFORM --sdk SDK --build BUILD --report REPORT`. `.github/workflows/cpu-packages.yml` contains the exact native OS commands and invokes the same archive verifier for CI and release candidates. Experimental-mode receipts must never be treated as production support promotion.

### Verified PR120 hosted results

PR120 head `69bc10e7084036986d099d3111d374a703898287` passed [CI](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37726222619), [tooling](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37726222201) and [extended fuzz/sanitizers](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37726222063). Seed 56 completed 1,129,345 parser, 635,611 module, 1,208,787 semantic and 1,188,143 lowering inputs, 181 seconds per stage. ASan/LSan/UBSan and mandatory runtime/training ThreadSanitizer passed. These hosted results close the local LeakSanitizer limitation recorded historically below.

Comparison base: `3417f8ed261c32aa59f09080cc4d325b316edc0e`

### Historical PR120 parser lifetime evidence

This PR addresses the post-PR119 audit's parser memory blocker. Parser allocations previously survived until process exit, including nodes from failed parses. The CLI now owns one `ParseSession` for its full module graph, while every fuzz input has its own session. Session destruction releases AST allocations, scanner tokens and source ranges; nested temporary parses preserve outer graphs and roots.

The public-release implementation sequence is recorded in [public_release_implementation_plan.md](public_release_implementation_plan.md). The requested CPU target scope includes Windows x64 and macOS ARM64 in addition to Linux x64. Native ONNX execution from installed packages is required in the packaging PR before expanding qualification claims.

Local validation on the working tree based on the exact merge above:

| Check | Observed result |
| --- | --- |
| Clang 18 Release CMake build with LLVM/MLIR 18 and ONNX Runtime 1.30.0 | PASS |
| Parser lifetime: 25,000 sessions / 100,000 valid and malformed parses | PASS; zero live parser allocations, scanner strings and source ranges after every session; nested module/function-name retention checked |
| Lifetime regression RSS, uninstrumented Linux | 1,556 KiB initial / 1,944 KiB peak; a repeat in the robustness gate peaked at 1,968 KiB |
| Earlier audited parser probe, 100,000 simple valid parses | 1,556 KiB initial / 243,444 KiB peak; this older workload differs from the new mixed regression and is not a paired performance comparison |
| Module resolution, deterministic locks and multi-file code generation | PASS |
| Interpreter/LLVM/native semantic differential execution | PASS |
| Function, scope, control-flow, deterministic-error and cleanup gate | PASS |
| Exact source diagnostic ranges and stable codes | PASS |
| Parser malformed-input/resource-bound matrix plus lifetime gate | PASS |
| Local ASan/LSan/UBSan lifetime attempt | BLOCKED by runner: LeakSanitizer cannot open `/proc/99/task`; mandatory leak detection was not disabled. The script failed before entering the extended random campaigns. |
| Hosted mandatory CI and seed-56 extended fuzz | Pending on the final PR revision; no hosted pass is claimed here. |

The extended workflow now runs on parser/AST/harness PR changes as well as nightly. PR runs use the formerly failing seed 56, four stages of 180 seconds each, the unchanged 2,048 MiB RSS limit and mandatory leak/undefined-behavior checks. Its deterministic lifetime regression runs under the same instrumentation. Full diagnostics remain retained as artifacts while console output is bounded. Fuzzing grows a temporary copy of the checked-in seeds.

### Energy and power result

This is a correctness and bounded-lifetime fix. RSS observations do not establish lower energy, power or end-to-end inference latency. No new physical-energy or production-readiness claim is made.

### Reproduction

Build the compiler with the normal CMake configuration, then run `bash scripts/check_parser_robustness.sh`, `bash scripts/check_module_resolution.sh`, `bash scripts/check_semantic_differential.sh`, `bash scripts/check_functions_control_error_semantics.sh` and `bash tests/diagnostics/test_source_diagnostics.sh`. Set `SHORTHAND_BIN` and `SHORTHAND_RUNTIME_LIB` to the built outputs when using an out-of-tree build. The standalone lifetime test is `bash scripts/check_parser_lifetime.sh`.

On a runner supporting LeakSanitizer process inspection, execute `SHORTHAND_FUZZ_SEED=56 SHORTHAND_FUZZ_MAX_TOTAL_TIME=180 SHORTHAND_FUZZ_RUNS=0 bash scripts/check_fuzz_sanitizers.sh`. Preserve `/tmp/shorthand_fuzz_*.out`, build logs and crash artifacts. Hosted PR and extended-fuzz runs retain these automatically. The comparison base is the merged PR119 SHA above.

### Historical PR119 implementation

PR119 comparison base: `9ef794f172423cebb6a83bfa75c06368da04ec72`.

GitHub PR119 implements the first optimization selected from PR118's measured phase attribution. The production runtime now caches the expensive hardware inventory and route behind a cheap hardware-generation token and a separate environment-policy signature. Repeated calls for the same model therefore reuse the qualified route; a changed policy invalidates routing immediately, and a changed hardware-generation token forces a fresh full probe. The model-content safety boundary is intentionally unchanged: self-contained cached ONNX models are still reread and byte-compared on every request, including replacements with preserved timestamps.

The compiled C bridge now calls `AIRuntime::inferCachedInto` with runtime-owned transactional output scratch. The ONNX prepared session binds that destination directly while retaining public finite/shape/dtype checks. Caller output is copied only after successful backend and output validation. Generic/oversized/external-data fallback paths preserve their prior semantics. Routine per-inference `stderr` formatting is disabled by default and can be restored with `SHORTHAND_RUNTIME_INFER_LOG=1`; structured runtime telemetry and counters remain available.

New/expanded regression coverage requires:
- one full hardware probe across repeated stable-route calls and a second probe after an injected generation change;
- a warm prepared call reporting `route_cache_hit=true`, `hardware_probes=1`, and `preallocated_output=true`;
- unchanged exact-content replacement detection, policy deny/recover, reset, external-weight, finite/capacity, lifecycle/concurrency and rollback tests;
- phase instrumentation on the new preallocated ORT path so optimized execution cannot disappear from diagnostic evidence;
- source guards requiring the preallocated compiled bridge path.

Hosted CI and same-run head/base measurements are pending for the final PR revision. No latency, power or energy reduction is claimed before those retained results exist.

### Energy and power result

No physical energy result is asserted by this source change. CI must continue to report the highest valid E1/E2/E3 class or explicit unavailability. A routing-cache or latency improvement is not converted into joules. Lower-energy or lower-power language remains blocked unless the existing comparison policy produces a supported result at equal quality and boundary.

### Reproduction

The required runtime-profile workflow builds the ordinary and diagnostic runtime archives with the pinned ONNX Runtime SDK, runs the extended compiled cache and hardware-routing tests, executes the generated three-cell comparison, replays phase/evidence bundles, and retains the generated Markdown/raw artifacts. The comparison base for this PR is the merged PR118 commit above. The final tested head SHA and hosted artifact links will be appended after required CI executes.

This report records the testing expansion and retained benchmark evidence for the prepared-runtime work. The historical PR115 and PR116 measurements use different comparison baselines. Their ratios must not be treated as repeats of the same experiment. The initial implementation is from [PR115](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/115), with the testing/reporting expansion in [PR116](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/116).

### Historical PR118 phase-attribution evidence

Comparison base: `6d12270c11bec28159fb38dca4e825c7dd7e2f8a`

This slice adds diagnostic attribution and a stronger explicit ORT control. It does not yet introduce a new performance optimization or establish an energy saving. The next optimization must follow the measured bottleneck.

The optional `shorthand_runtime_profiled` static archive records exclusive phases of the actual compiled `.short` entry: registration, facade lock acquisition, bridge validation and input copying, policy refresh, hardware probing/routing, complete model reread/comparison, preparation, backend work/ORT execution, output copying, runtime telemetry, bridge telemetry/logging, independent output verification and entry residual. It is a separate, non-installed target enabled with `SHORTHAND_BUILD_RUNTIME_PHASE_PROFILE=ON`. Normal targets preprocess the hooks away; the native isolation test checks that the ordinary runtime archive contains no diagnostic symbols. The public 25-symbol C ABI is unchanged.

CI captures 64 numerically verified warm calls per compiled cell (b1, b16, b32). Every raw phase partition must sum exactly to its instrumented total, contain the expected registration/inference/verification visits, and show no warm session preparation. Clock-pair calibration and clock-read counts are retained. Clock overhead is included, never subtracted. Diagnostic timings are excluded from latency ratios and metering runs.

The uninstrumented comparison retains ORT BASIC and adds separately named `direct_all` with `ORT_ENABLE_ALL`. Both use the same FP32 model and pinned SDK, one intra/inter thread, sequential execution, spinning disabled, prepared sessions and preallocated buffers, with identical independent output verification. ALL is one tuning candidate, not proof of globally optimal ORT tuning. Head/base still compare the same generated object linked to the exact two runtime revisions. The direct boundary omits ShortHand registration, routing, model rereads and telemetry; the report labels that difference. ORT documents these optimization levels and threading controls in its [graph optimization](https://onnxruntime.ai/docs/performance/model-optimizations/graph-optimizations.html) and [thread management](https://onnxruntime.ai/docs/performance/tune-performance/threading.html) documentation; their effects on this workload are measured, not assumed.

| Validation | Result before hosted CI |
|---|---|
| Real SDK native runtime and diagnostic archive | Built locally with Clang 18 / ORT 1.30.0 |
| Diagnostic execution | Passed cold/warm/reset, NaN rollback, deny/recover policy, 64 calls across two threads, nested-capture rejection and exception unwinding |
| Production isolation | Passed: no `runtime_profile` symbols in the ordinary static archive |
| Python evidence regression discovery | 57 tests passed locally, including mislabeled tuning, profile contamination, incomplete phases, incorrect partitions and tampered bundles |
| Compiled capture and replay | Passed three cells, 16 uninstrumented blocks per runner/cell plus 64 separate diagnostic calls/cell; local head/base comparison not run |
| Frozen ABI and thread-safety gate | Passed: 25 public symbols, consumer execution and serialized-state test |
| Hosted latency/energy comparison | Pending first run for this source revision; no new reduction claim |

All previous cache, sanitizer, application correctness and required CI gates remain. The evidence index now also requires the diagnostic execution/isolation log and replayed phase report. Every PR continues to include this committed document; each CI run additionally retains generated `BENCHMARKS.md`, `GENERATED.md`, raw profiles, settings, commands, sources, executable hashes and output-verification results.

Local diagnostic smoke capture (uncommitted development build, not a head/base claim) found hardware probing plus routing accounted for 47.3–60.0% of instrumented time; ORT execution accounted for 5.9–9.2%. This suggests investigating redundant discovery and route/telemetry construction while preserving request-time policy and device qualification. It does not justify skipping those checks. Local ALL/BASIC latency ratios were 1.1777, 0.9907 and 1.0405 for batches 1, 16 and 32: no consistent benefit. All nine separate local energy runs reported `amd_hwmon_socket_energy_unavailable`, with joules and watts `null`. Hosted observations below will be the citable result for a committed source revision.

### Historical PR117 implementation and evidence

Comparison base: `a5a3e020943ef2f9005b6a44f4c8024a8b58ef25`

This follow-up reuses the C bridge's runtime/backend registry and a single parsed declaration descriptor. The descriptor key includes model, input and output names; changed registrations and reset invalidate it. Environment policy is refreshed and hardware routing is still evaluated on every call. The prepared cache reuses its model-comparison storage but still reads and compares every model byte, including replacements with unchanged timestamps. Two bounded model buffers retain up to 32 MiB; reset releases them. This trades retained scratch memory for avoiding repeated comparison-buffer allocation and initialization.

The public C ABI, finite checks, output ownership and failure rollback remain intact. This slice does not implement borrowed tensor pointers or bind ONNX outputs directly into the caller's buffer. Such binding would require a transactional runtime-owned workspace to preserve the current failure guarantee.

| Validation | Current result |
|---|---|
| Python energy/evidence regression discovery | 54 tests passed locally |
| Committed-report CI contract | 6 tests passed locally: committed update, missing/worktree-only report, stale base, symlink, truncated report and invalid revision |
| Native runtime and qualification build | Passed locally with Clang 18 and the real ORT SDK |
| Real ONNX application | 42 boundary cases passed locally; all 1,797 real dataset rows checked (`live_onnx=1`) |
| Extended native cache and sanitizer tests | Passed locally: all cache boundaries, registration/policy regressions, lifecycle and workspace checks; ASan/UBSan passed. Local LSan could not inspect `/proc` and the script explicitly reran with leak detection disabled; hosted LSan remains required when supported |
| Same-run head/base and direct C++/ORT measurements | Captured in both hosted runs below; compiled ratios 0.9250–0.9440, with mixed resident results |
| Energy/power improvement | Unproven; local probe found no RAPL/AMD domains and no matching E2/E3 profile, so the highest available evidence class is `null` |

The new native regression alternates 100 calls across model/input/output names, independently tests mismatched input/output ranks with a fixed model name, rejects changed input shapes without touching the output, and repeats eight deny/recover policy cycles on the retained runtime. The rank test exposed a pre-existing uncached-fallback gap: an incompatible concrete output shape could still return success after preparation failed. This PR rejects that mismatch before copying any output. The test is retained in native and sanitizer CI. CI now requires this Markdown document to change in every PR and to identify that PR's exact base commit. The generated per-run report uses the selected base revision rather than assuming it reconstructs the session on every call.

#### PR117 retained hosted evidence

Tested implementation: `0112838166cc62542d008a226498946e127dd11c`. Both runs compare it against the base above, which already includes prepared-session reuse. Each artifact's finalized index reports `status=complete` and `missing=[]`; every indexed file hash and the ZIP SHA-256 were verified after download.

| Run | Retained artifact | Verified indexed files | ZIP SHA-256 |
|---|---|---:|---|
| [PR CI 37137644532](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37137644532) | [runtime-profile-37137644532-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37137644532/artifacts/11279420752) | 1,184 | `b3ba6e59562a17ed59d754fc91c25dd94124713965b518fcec6b2f704dd38950` |
| [Push CI 37137642095](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37137642095) | [runtime-profile-37137642095-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/37137642095/artifacts/11279176676) | 1,183 | `b5b7e78076d0fb6cdf5daaaf0cd9680d3bac061c0afe53d476f8ca772967feec` |

The evidence job passed in both runs, including native cache/descriptor tests, real-digit quality checks, generated-source oracle validation, evidence replay, and the instrumented cache/workspace ASan/LSan/UBSan tests. Hosted LeakSanitizer ran without the local `/proc` restriction. The PR run also passed the committed-report check. The one-file difference between the indexes is its `committed-report-check.txt` log.

Overall CI initially failed its enterprise source guards: two scripts still required `AIRuntime runtime;` and `runtime.inferCached(...)`, the obsolete per-call construction path. The follow-up updates those guards to require retained runtime construction, policy refresh, cached inference and reset. Its real missing-SDK and real-ONNX execution tests remain mandatory and passed locally after the correction. This guard/report follow-up changes no compiler or runtime implementation. The [PR checks](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/117/checks) show validation of the final follow-up revision; these measurements remain attributed to the tested implementation SHA above.

Compiled synthetic FP32 probes (one thread, 16 timed blocks per runner/cell):

| Cell | PR head ms/vector | PR head/base | PR head/direct C++/ORT | Push head ms/vector | Push head/base | Push head/direct C++/ORT |
|---|---:|---:|---:|---:|---:|---:|
| b1-t1 | 0.075471 | 0.9320 | 45.993× | 0.076594 | 0.9323 | 44.974× |
| b16-t1 | 0.004871 | 0.9418 | 28.869× | 0.004874 | 0.9250 | 28.382× |
| b32-t1 | 0.002484 | 0.9424 | 20.376× | 0.002493 | 0.9440 | 20.383× |

These observations correspond to 5.6–7.5% lower compiled-boundary mean latency than the already-cached baseline across the two runs. Each head/base comparison is within its own runner; the separate hosted runs are not paired with each other. The compiled path still takes 20.4–46.0 times the direct prepared C++/ORT time. This is descriptive evidence for reducing bridge overhead, not a confidence-bound speedup or superiority claim.

Resident real-digit comparison (all 1,797 images, six paired blocks per cell):

| Cell | PR head µs/image | PR base µs/image | PR paired median reduction | PR paired ratio range | Push paired median reduction | Push paired ratio range |
|---|---:|---:|---:|---|---:|---|
| b1-t1 | 3.112231 | 3.142349 | 1.13% | 0.8885–0.9993 | −0.11% | 0.9985–1.1045 |
| b16-t1 | 0.557288 | 0.555911 | −0.26% | 0.9949–1.0318 | −0.87% | 0.9924–1.0171 |
| b32-t1 | 0.469934 | 0.466901 | −0.53% | 0.9824–1.0111 | 0.60% | 0.9701–1.0163 |
| b16-t2 | 0.560034 | 0.555951 | −0.74% | 0.9949–1.1360 | 0.80% | 0.9887–1.0231 |
| b16-t4 | 0.559836 | 0.561574 | 0.31% | 0.9914–1.0357 | 0.14% | 0.9829–1.0108 |

Negative reductions are regressions. Some individual blocks exceed 5% regression; these diagnostics do not satisfy the plan's confidence-bound acceptance campaign. The resident path does not use the optimized compiled C bridge, and no consistent resident improvement is established.

Independent resident controls from the PR run (median µs/image, six blocks each):

| Cell | ShortHand native | C++/ORT | Python/ORT |
|---|---:|---:|---:|
| b1-t1 | 3.117480 | 2.230510 | 41.651034 |
| b16-t1 | 0.553882 | 0.386260 | 3.143956 |
| b32-t1 | 0.471010 | 0.323351 | 1.789468 |
| b16-t2 | 0.554538 | 0.383577 | 3.208634 |
| b16-t4 | 0.560229 | 0.387532 | 3.178721 |

Both runs reported energy unavailable: no readable CPU package/socket counters and no applicable E2/E3 runtime-feature calibration. There is no measured joules/task or watts result and no lower-energy or lower-power claim. The comparison buffer's increased retained memory is also not an energy measurement. The next practical experiments remain phase attribution, a separately identified tuned `ORT_ENABLE_ALL` control, and bounded fixed-shape AOT fusion under equal-quality checks.

#### Earlier prepared-runtime evidence

The authoritative pre-expansion hosted bundle is [runtime-profile-36963499649-1](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36963499649/artifacts/11209171583). Its ZIP SHA-256 is `4c0c86316bc608f77f2ca6b437583e2af87b3de012cb9978543e1965858792bd`. Under the evidence schema used by that run, `run-evidence.json` reported `status=complete`, `missing=[]`, and 1,180 indexed files. Every indexed file hash was verified after download. The expanded checks in this change are required for subsequent runs and are reported separately below.

The workflow keeps latency, power and energy claims disabled. The results below are evidence for the tested workload and boundary. They do not establish a language-wide result, a lower-power result, or superiority to optimized C++/ORT.

### Expanded tests

The follow-up test expansion adds checks at the C ABI, resident workspace, evidence parser and sanitizer boundaries. The tests are wired into `runtime-profile.yml`, which is required for every push and pull request.

| Area | Coverage added | Result |
|---|---|---|
| Real ONNX prepared cache | 2,000 sequential calls with idempotent re-registration; exact model-content replacement with preserved mtime; reset and re-registration invalidation; corruption/deletion/recovery; external-weight deletion and recovery | Pass locally and retained as a required CI gate |
| Concurrent runtime lifecycle | 4 worker threads perform 4,000 inference calls while a fifth thread performs 64 resets and registrations; successful calls and registration gaps are checked for valid statuses, counts and output canaries | Pass; all 4,000 calls accounted for |
| C ABI boundary matrix | Null/empty names, missing model/tensors, null buffers, invalid sizes/capacities, null output count, aliased buffers, spare-capacity canaries and extreme finite/nonfinite values | Pass |
| Bridge input boundary | 65,536-float retained scratch limit and 65,537-float temporary path, repeated three times with output canaries | Pass |
| Prepared snapshot bound | Exactly 16 MiB is cacheable; 16 MiB plus one byte uses the uncached path | Pass |
| Resident workspace | 2,000 alternating full/tail calls; all real slots rewritten and tail slots zeroed; 80 rejected-input rollback checks; concurrent independent workspaces | Pass |
| Evidence parser | 12 generated-evidence tests cover boolean/nonfinite/missing values, process and meter failure, energy denominator, wrong digest, invalid protocol, duplicates, unknown observations, unsafe paths and symlinks | Pass locally |
| Evidence index | Missing/failed evidence, unavailable energy, false authorization flags and malformed JSON are fail-closed | Pass locally |
| Markdown report | Report refuses unsuccessful reports and refuses numeric energy values when the meter is unavailable | Pass locally |
| Sanitizers | Prepared cache and workspace built with ASan/UBSan. LeakSanitizer runs when the runner permits `/proc` inspection; managed local execution reported its restriction and completed the ASan/UBSan rerun | Pass; local LSan unavailable due runner restriction |

The local evidence regression command ran 45 Python tests across the profile, baseline, energy, comparison, runtime-delta, energy-status, evidence-index, generated-evidence and Markdown-report suites. The existing native qualification suite also passed with the real ONNX SDK and `live_onnx=1`.

### Hosted compiled-runtime experiment

The compiled lane uses a real `.short` source program and a synthetic zero-input FP32 MatMul+Add graph. It validates every output vector against an independent pinned ONNX Runtime oracle. It measures one-thread batch sizes 1, 16 and 32. The timed boundary includes source initialization, registration, runtime validation, routing, telemetry/logging and output verification. The direct control uses a prepared C++/ORT invocation with matching output verification.

| Cell | Prepared head mean (ms/vector) | Head/base ratio | Head/direct prepared C++/ORT |
|---|---:|---:|---:|
| b1-t1 | 0.028256 | 0.1715 | 28.334× |
| b16-t1 | 0.001957 | 0.1947 | 17.746× |
| b32-t1 | 0.000939 | 0.1908 | 10.251× |

Against the repeated-session baseline, the prepared head ratio corresponds to 80.5–82.9% lower measured latency for these probes. The head path remains 10.3–28.3 times slower than direct prepared C++/ORT. This exposes runtime and generated-boundary overhead that remains to be reduced. The compiled lane is not a digit-accuracy benchmark and does not measure multi-threaded generated execution.

### Hosted resident real-digit experiment

The resident lane evaluates all 1,797 held-out Optdigits images. Head and base execute the same model, dataset, precision, backend, batch/thread cell and quality checks. Six balanced paired blocks are retained per cell.

| Cell | Head median (µs/image) | Base median (µs/image) | Paired median reduction | Paired ratio range |
|---|---:|---:|---:|---:|
| b1-t1 | 1.896141 | 1.892991 | 0.15% | 0.9515–1.0759 |
| b16-t1 | 0.401278 | 0.415128 | 5.83% | 0.8437–1.0916 |
| b32-t1 | 0.356938 | 0.366566 | 0.86% | 0.9189–1.0283 |
| b16-t2 | 0.390400 | 0.412192 | 6.18% | 0.9177–0.9781 |
| b16-t4 | 0.393680 | 0.416144 | 4.98% | 0.9420–1.0323 |

The paired result is descriptive. Individual blocks include regressions, especially on shared hosted runners. No confidence-bound latency claim is authorized.

### Resident controls

The same hosted bundle includes independent native C++/ORT and Python/ORT controls. Values are resident median microseconds per image from six blocks; they are not energy measurements.

| Cell | ShortHand native | C++/ORT control | Python/ORT control |
|---|---:|---:|---:|
| b1-t1 | 1.850874 | 1.282920 | 16.584142 |
| b16-t1 | 0.390342 | 0.290399 | 1.362800 |
| b32-t1 | 0.339801 | 0.268091 | 0.821868 |
| b16-t2 | 0.392837 | 0.292844 | 1.380353 |
| b16-t4 | 0.389666 | 0.293632 | 1.379824 |

ShortHand's resident host remains slower than the direct C++/ORT control in these cells. The optimization slice improves the host path relative to its previous ShortHand baseline while leaving a measurable control gap.

### Energy and power result

The hosted runner exposed no readable CPU package/socket energy counter and no matching E2/E3 calibration profile. The retained summary is:

```text
available=false
evidence_class=null
hardware_measured_joules=null
joules_per_task=null
average_component_watts=null
claim_authorized=false
```

The evidence pipeline records this as an unavailable measurement. It does not infer joules from latency, report zero joules, or authorize lower-power or lower-energy language. A future runner with a valid counter or matched calibration profile can populate the same schema without changing the benchmark protocol.

### Latest expanded hosted validation

The expanded checks passed in [PR116's required CI run](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36970175339) at revision `996550347dbd9bbfae4f79e55340ebf697ea2295`. The retained [runtime-profile artifact](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36970175339/artifacts/11211791482) has ZIP SHA-256 `a7493b45edba7a046c744138c0d6a5fbb21210eed9f66df4f9187a75c29e7edb`. Its finalized evidence index reports `status=complete`, `missing=[]`, and 1,183 indexed files; every indexed file hash was verified after download.

The expanded hosted run recorded the following additional pass evidence:

- Real ONNX cache tests passed with 2,000 sequential calls, 4,000 concurrent calls during 64 resets/registrations, both bridge boundaries, both snapshot limits, invalid-argument/output-canary checks, and external-weight recovery.
- The prepared cache and workspace ASan/UBSan step passed. LeakSanitizer was enabled by the hosted runner, so the local `/proc` restriction did not apply to this retained run.
- Evidence regression tests, report generation, application correctness, resident workspace checks and energy availability validation all passed.

The latest compiled probe was materially different from the earlier PR115 run on the shared hosted environment:

| Cell | Head mean ms/vector | Head/base | Head/direct prepared C++/ORT |
|---|---:|---:|---:|
| b1-t1 | 0.081271 | 0.9971 | 49.841× |
| b16-t1 | 0.005221 | 0.9940 | 31.649× |
| b32-t1 | 0.002642 | 0.9911 | 22.338× |

This PR116 run corresponds to 0.3–0.9% lower head latency than its selected base, `72cd42e91509aed4ad79d29fbc1a5bb9acd25c25`, while remaining 22.3–49.8 times slower than direct prepared C++/ORT. That base already includes PR115's session reuse. The earlier 80.5–82.9% observation compares against the pre-reuse implementation and measures a different change. The difference between these ratios must not be attributed solely to hosted-runner variance. Neither run authorizes a general speedup claim.

The latest resident paired median reductions were 0.21% (`b1-t1`), 0.44% (`b16-t1`), 3.06% (`b32-t1`), 0.27% (`b16-t2`) and −0.07% (`b16-t4`). Individual paired blocks included regressions. Energy remained unavailable with null joules and watts.

### Reproduction

The required workflow runs the following classes on every push and pull request:

```text
python3 experiments/energy/test_generated_infer_evidence.py
python3 experiments/energy/test_ci_run_evidence.py
python3 experiments/energy/test_render_benchmark_markdown.py
CXX=clang++-18 python3 tests/ai_application/test_compiled_cache.py build-profile build-profile/onnxruntime-1.30.0
scripts/check_prepared_cache_sanitizers.sh build-profile/onnxruntime-1.30.0 build-profile-sanitized
```

The full workflow additionally rebuilds the exact PR head and base runtime, captures the generated and resident comparisons, hashes source/model/data/compiler/SDK/binaries and raw logs, replays the generated and resident bundles, probes energy capability, renders `BENCHMARKS.md`, and finalizes `run-evidence.json`. Missing or altered evidence fails the required status.

The retained artifact expires after 90 days under the current workflow policy. Archive the cited bundle before expiry when the result must remain independently citable.

### Interpretation

The prepared-session and workspace changes are practically effective for the measured repeated-session and resident-host boundaries. The compiled path still has substantial overhead relative to direct C++/ORT, so the evidence supports continued runtime and generated-code optimization. The present data does not support a claim that ShortHand consumes less power or energy than C++, because no joules/task observation was available. Lower latency alone is not evidence of lower watts or joules.
