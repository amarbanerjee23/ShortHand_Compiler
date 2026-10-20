# Review of the Gemini Argon proposals and bounded implementation

Review baseline: merged PR123, `44af710d7bf280a931b3d218299e11baf27a345c`.
Input: the supplied `Pasted text(3).txt` conversation, treated as design proposals rather than verified repository or measurement facts.
Status: opt-in CPU backend feasibility experiment and a component-counter validity fix. Normal ShortHand source lowering and runtime ABI are unchanged. All superiority and production claim flags remain false.

## Decisions

| Proposal | Assessment | Implementation or remaining work |
|---|---|---|
| Make energy/quality contracts universal | Useful as explicit policy; declarations do not themselves measure energy or prove quality. Requiring them for every old program would change compatibility. | Retain current contracts and defaults. Add no mandatory source annotation. |
| Separate cold initialization, resident inference and end-to-end application work | Correct, and phase/runtime evidence already exists. Timing instrumentation does not measure the joules of each phase. | New AOT experiment retains cold and resident timings; component energy is collected in a separate pass over the entire child process. |
| RAPL and external-meter calibration | Useful if coverage, counter range, timing and instrument uncertainty are qualified. A small positive counter delta can conceal a full wrap after a sampling stall. | Fix the CI RAPL sampler to reject ambiguous actual intervals, including sysfs read duration; add wrap/range/clock negatives. No calibration or meter is invented. |
| SHA-256 “signed”, immutable auditor bundles | Incorrect terminology. A digest checks bytes against a trusted digest; it is neither a signature nor proof that a measurement occurred. Local files remain mutable. | Retain hashed raw artifacts and verify all required files before replay; emit derived reports only after complete validation. Describe the trust and 90-day CI retention limits. |
| Whole-model ONNX-to-MLIR AOT | A sound direction with a closed operator/shape/precision contract. Embedding ONNX bytes alone is not operator compilation. A complete importer is not present merely because MLIR exists. | Implement a separate static FP32 MatMul+Add source generator and native C++17 benchmark. It compiles arithmetic and constants, but does not yet integrate ONNX into MLIR or `.short` lowering. |
| AOT always beats C++/ORT | Unsupported. Generated C++ is itself a C++ implementation; tuned kernels can beat simple generated loops. | Compare AOT, prepared ORT BASIC and ORT ALL under the same C++ host. Retain losing cells. Initial batch 1 benefit does not justify replacing the existing runtime. |
| Fused preprocessing, epilogues and arena memory | Potentially useful after source tensor semantics, dependency analysis and lifetime proofs. Borrowing alone does not establish safe buffer reuse. | The experiment uses an explicit per-call output workspace and immutable constants. General host/model fusion and liveness planning remain R3/R6 work. The fixed buffer size is structural metadata, not a measured allocation count. |
| Mathematically guaranteed quantization/pruning accuracy | Incorrect for arbitrary models and data. A threshold annotation is not a proof of task quality after approximation. | Preserve strict FP32 with contraction and fast-math disabled. Require disjoint calibration/selection/confirmation data and held-out task-quality qualification before approximations. |
| Automatically choose the lowest-energy hardware | Device discovery and backend availability do not establish this. Selection needs comparable valid measurements and uncertainty for the deployment boundary. | No device-ranking change. Missing hardware evidence remains unavailable; no TDP-times-time estimate is substituted. |
| GPU/TPU/NPU lowering with no runtime dependencies | Too broad. Device code generation does not eliminate drivers, launch runtimes, transfers or target-specific qualification. | CPU experiment only. No accelerator success or portable GPU installer claim. |
| Collapse the legacy workspace and all Makefiles | The repository already has root CMake, installed package exports and Make/CMake parity gates. Moving paths can break scripts and consumers independently of runtime performance. | Preserve build/package layout. Any later cleanup needs a path/consumer migration and build-time evidence, not an asserted inference saving. |
| Export only C ABI headers | A narrow frozen C ABI is useful, but removing existing C++ headers can break supported source consumers. | Preserve public exports and the frozen runtime symbol set. The new model header belongs only to the experimental generated artifact. |
| DLPack/Python buffer borrowing and GIL release everywhere | Useful with correct owner lifetime, dtype/strides/device checks, synchronization, deleter ownership and exception handling. GIL release alone does not remove the serialized runtime facade. | Defer to a bounded binding design. No new Python runtime dependency, implicit buffer ownership transfer or concurrency guarantee is added. |
| Differential tests and sanitizers for every change | Useful within the executable contract and available toolchains. Passing tests cannot guarantee absence of all bugs; MSan also requires suitably instrumented dependencies. | Add real native execution, independent numerical oracles, rejection/alias/rollback/concurrent-call tests, mandatory hosted ASan/LSan/UBSan and replay corruption tests. Retain the existing full language/package gates. |
| One giant three-part PR guarantees zero regressions | Unsupported. Combining unrelated API, telemetry, compiler and packaging changes enlarges the review surface. | One active PR with the bounded experiment, counter correction, tests and evidence. Deferred work remains explicitly sequenced in the existing roadmap. |

## Exact AOT contract

`experiments/aot/compile_model.py` reads one model snapshot of at most 16 MiB. It accepts ONNX IR 8–10, standard-domain opset 13, exactly one rank-2 FP32 input/output, ordered `MatMul` followed by `Add`, and exactly two finite inline constants: weights `[K,N]` and bias `[N]`. Add input order may be reversed. Dimensions are positive and at most 1024; input/output tensors are at most 65,536 elements and weights at most 1,048,576 elements. General broadcasting, dynamic shapes, external/segmented weights, custom operators/attributes, functions, training graphs, unknown protobuf fields and malformed encodings are rejected. The ONNX checker runs in addition to the stricter subset validator.

The tool emits `model.h`, `model.cpp`, the exact `model.onnx` snapshot and a manifest. Compiling the source embeds immutable FP32 constants and implements the arithmetic without linking ONNX Runtime. This is not a freestanding binary: ordinary C/C++ runtime libraries still apply. ONNX and NumPy are build/validation dependencies only, pinned with wheel hashes for the Linux x64/Python 3.12 CI environment.

The artifact-local `sh_aot_infer` accepts contiguous native-endian FP32 storage with exact element counts and alignment. Caller-owned workspace must be disjoint from input/output and separate for concurrent calls. Input/output may partially overlap. Every input and result must be finite; output is committed only after success and remains unchanged on rejection. Workspace contents after failure are unspecified. As with ordinary C interfaces, the caller must supply genuinely valid storage and prevent conflicting concurrent writes. No mutable global scratch is introduced.

Reduction order is preserved, with the bias applied after the dot product. Compile with `-fno-fast-math -ffp-contract=off`; the retained benchmark commands enforce these flags. This is numerical qualification within declared tolerances, not a claim of bit-identical outputs across all compilers, inputs or BLAS implementations. Unsupported models fail explicitly; the source generator does not silently substitute a fallback backend. Existing supported ORT execution remains available through the existing runtime.

## Benchmark and energy boundary

The capture uses a deterministic nonzero affine fixture, FP32 inputs/constants, batches 1/16/32, `K=64`, `N=10`, one thread, eight warmups and 1,024 timed calls per process. A NumPy float64 oracle rounded to FP32 supplies the expected outputs, with `atol=1e-5` and `rtol=1e-4`. All output elements are checked on every call. The same C++ probe performs input validation, invocation and output verification for all three runners. AOT also performs its own transactional buffer validation; that additional cost is included. ORT uses prepared tensors, sequential execution, one intra/inter-op thread and spinning disabled.

Six balanced permutations retain every AOT/BASIC/ALL observation. Reports use paired block ratios with median/minimum/maximum, not a confidence interval or a cherry-picked winner. The short runs are a feasibility/regression signal on a shared host, not an independent confirmation campaign or task-accuracy study. Cold timing excludes fixture loading and includes backend construction plus the first checked invocation. Resident timing excludes generation, compilation and startup. Model generation time, native object compilation time, binary sizes and all raw commands remain recorded separately.

The separate energy pass includes the entire child process: startup, file loading, preparation, first call, warmups, all checked timed calls and teardown. Its denominator includes every verified vector in that boundary. CPU component counters are not process-attributed or whole-system energy. Missing counters, failed samples and ambiguous wraps yield null joules and a reason. Local counter absence does not prevent correctness testing; it prevents an energy-saving claim. This PR does not apportion energy from phase duration or infer carbon, certification, or an energy break-even count.

For RAPL, each counter read is bracketed with a monotonic start/end. The maximum possible interval between consecutive readings is bounded by the previous start and current end. If that interval times the configured per-package power bound reaches the counter range, the observation is rejected even when the apparent delta is small and positive. Counter values must lie in `[0, range)`, and accumulated updates commit only after every contributing domain passes. The default 2,000 W per-package bound is a conservative assumption, not a calibration certificate or a substitute for hardware qualification. A reset within the range can still be observationally indistinguishable from a legitimate wrap; absolute measurement authenticity is not established by this guard.

## Reproduction

Use a fresh Linux x64/Python 3.12 environment and the verified ORT 1.30.0 SDK provisioned by `.github/workflows/runtime-profile.yml`:

```sh
python3 -m venv aot-venv
aot-venv/bin/python -m pip install --require-hashes --only-binary=:all: --no-deps \
  -r tests/ai_application/requirements-ai-baseline-linux-x64-py312.txt \
  -r experiments/aot/requirements-linux-x64-py312.txt
CXX=clang++-18 SHORTHAND_AOT_SANITIZERS=1 \
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  aot-venv/bin/python experiments/aot/test_compile_model.py
aot-venv/bin/python experiments/aot/benchmark.py \
  --sdk build-profile/onnxruntime-1.30.0 --cxx clang++-18 --output aot-output
python3 experiments/aot/benchmark.py --output aot-output \
  --replay-sha256 "$(cat aot-output/manifest.sha256)"
python3 experiments/aot/test_benchmark.py --bundle aot-output
python3 experiments/energy/test_ci_energy_evidence.py
```

To emit and compile a compatible model separately:

```sh
aot-venv/bin/python experiments/aot/compile_model.py model.onnx --output generated-model
clang++-18 -std=c++17 -O3 -fno-fast-math -ffp-contract=off \
  -c generated-model/model.cpp -o generated-model/model.o
```

Both tools require a fresh output directory. Inspect `manifest.json` for dimensions and workspace size, then link the object into a host using its generated header. This command does not enable AOT for `.short` inference.

## CI, releases and remaining integration

`runtime-profile.yml` runs the importer/native contract suite, hosted ASan/LSan/UBSan, native AOT/ORT comparisons and replay negatives. `ci_run_evidence.py` requires the AOT report and the successful sanitizer/test marker; missing evidence cannot satisfy that job. Raw model/input/oracle/source/object/binaries/commands/stdout/stderr are hashed and retained in the existing profile artifact. Hash replay verifies integrity relative to its root digest; it cannot certify the host or sign the campaign. CI artifacts expire after 90 days unless separately archived.

The existing trusted-successful-push release workflow and Windows x64, Linux x64/arm64 and macOS arm64 installer lifecycle gates remain enabled. This research tool is not added to installed language packages; the language installers continue to come from those existing release jobs. No failed build is represented as a successful release and no PR is auto-merged.

R2 immutable model capability and R3 source tensor semantics still precede language-integrated R5 AOT. The standalone experiment does not satisfy R5's same-artifact ShortHand/C++ comparison. Before default routing or public savings claims: integrate without changing mutable-model semantics; qualify the identical artifact through both hosts; retain an explicit ORT path; extend operators one at a time; validate native platforms; measure representative held-out tasks, longer independent sessions and compile/setup amortization. R6 fusion/arena, R7 approximations and R8 energy tuning remain separate qualification work.

## Primary references and repository evidence

- [ONNX MatMul semantics](https://onnx.ai/onnx/operators/onnx__MatMul.html) and [Add semantics](https://onnx.ai/onnx/operators/onnx__Add.html): the implemented rank/broadcast subset is deliberately narrower.
- [ONNX checker](https://onnx.ai/onnx/api/checker.html), [external-data security](https://onnx.ai/onnx/repo-docs/ExternalDataSecurity.html), and [ONNX 1.23.2 release](https://github.com/onnx/onnx/releases/tag/v1.23.2): source validation and the pinned build dependency.
- [MLIR bufferization](https://mlir.llvm.org/docs/Bufferization/): safe in-place reuse depends on alias/read-after-write analysis and ownership handling.
- [Linux powercap](https://docs.kernel.org/power/powercap/powercap.html): energy counter ranges and hierarchical domains.
- [Existing implementation sequence](post_pr122_energy_implementation_plan.md), [current benchmark record](latency_energy_test_and_benchmark_results.md), [runtime ABI](runtime_abi_api_stability.md), [installed packages](cpu_release_packages.md), and [energy contracts](energy_contracted_compilation.md).
