# Post-PR122 runtime optimization and energy evidence plan

Baseline: merged PR122, `72142acd9750fe32b1d0e1f70b8dd06e25ba7430`.
Status: R1 merged in PR123; bounded AOT feasibility merged in PR124; R2 owned runtime capability is implemented in PR125. Its initial implementation passed hosted qualification and published four native installers; evidence-validation/documentation follow-ups await their own final CI. See the [recorded hosted results](benchmark_results/immutable-model-hosted.md). R3–R8 remain separately sequenced.
Claim status: no measured energy reduction, general language superiority, GA or certification claim.

## Objective and scope

Reduce work per correct AI result, then measure whether that reduces elapsed time and energy under a declared deployment boundary. A useful result is a reproducible improvement for specified workloads, hardware, model quality and baselines. A finite benchmark campaign cannot establish that a language consumes the least energy for every AI program or outperform every possible C++ implementation.

The supplied proposal is a design input, not a verified experiment. Its 20.4–46.0x ratios describe historical PR117 observations; they are not a measurement of the merged PR122 runtime. Its predictions of 1.05–1.15x overhead and 35–70% energy savings have no retained evidence and are not acceptance targets. Current head/base results must come from the same-runner CI comparison.

## Audit decisions

| Proposed change | Decision and required correction |
|---|---|
| Remove the C bridge's input-vector copy | Implement a synchronous read-only FP32 borrow. Validate pointer/count/shape before backend use; release the pointer on every return and exception. Preserve owning adapter semantics. |
| Defer bridge JSON formatting | Implement owned snapshots of request descriptors and result strings. All exporters must observe the original call even if registrations change before the first query. Measure both infrequent and per-call observation. |
| Forward the address behind any LLVM load | Defer until alias/lifetime analysis proves the address still represents the SSA value at inference. An intervening write can otherwise change program semantics. Inspect generated code at O0/O2 before counting eliminated copies. |
| Replace full model reads with stat/mtime checks | Reject for the existing mutable-path API. Same-size replacements with preserved timestamps must still invalidate sessions. Do not enable stronger validation only in tests. |
| Bind an arbitrary pointer plus a digest string as an embedded model | Reject the supplied hook. It neither checks the digest nor establishes ownership/immutability, and bypasses the serialized facade. A const pointer is not an immutable-artifact contract. |
| Hide new exports by using a different symbol prefix | Reject as an ABI strategy. Passing a prefix-filtered symbol test does not prove encapsulation. Use versioned/internal interfaces with actual visibility and lifecycle tests. |
| Set `steady_state_heap_allocations = 0` | Reject as evidence. Count allocations in a precisely bounded region, including worker threads, and demonstrate that the counter detects an injected allocation. |
| Add the supplied persistent training pool | Defer to the training slice. Handle partial thread construction, cancellation, exceptions and shutdown; a constructor failure with joinable workers must not terminate the process. |
| Add a self-hosted energy job | Do not add a dependency on unavailable hardware. The supplied job only probes capabilities; it does not execute a metered comparison. Keep unavailable measurements explicit. |
| Attribute fused AOT versus unfused C++ entirely to the language | Reject that attribution. A C++ caller must run the identical AOT artifact; add a fused C++ control when investigating host/model fusion. |

## First slice: changes in this PR

`TensorBuffer` gains an explicit borrowed FP32 view. `short_ai_infer_f32` binds caller input only for the synchronous call, and both prepared and path-based ONNX execution consume the view. Existing vector-owning callers and `buildInputTensorBuffer` retain ownership. The resident descriptor releases the external pointer through RAII, including error paths. Consumers of C++ headers must rebuild; this is not a promise of binary compatibility for internal C++ structure layouts.

Output still goes to runtime-owned scratch and is copied to caller memory only on success. Input/output aliasing, including partial overlap, remains supported. Finite/shape checks, mutable-model rereads, route/policy invalidation and the frozen 25-symbol public C ABI remain in force. This removes one bridge input copy; it does not establish a zero-copy end-to-end pipeline or zero heap allocation.

The bridge retains owned observation state and materializes request JSON and the outer inference-telemetry envelope on demand. Backend telemetry is still produced by the existing runtime. Observability and OTLP exporters trigger the same materialization, and repeated queries reuse the result. Reset and failed inference clear old request state. No model embedding, compiler alias rewrite, registration hoisting, training optimization or precision relaxation is included in this slice.

The compiled evidence runner additionally measures head/base with all four JSON exporters consumed on every timed inference. This makes deferred formatting cost visible. These controls have separate raw files and report rows; they never enter the inference-only series or direct-ORT comparisons. Replay rejects incomplete, duplicate, unhashed, misordered or incorrectly labelled observer samples. The existing 17-phase diagnostic partition remains separate from uninstrumented performance measurements.

## Post-PR123 AOT feasibility update

The [Argon proposal review](argon_aot_energy_proposal_review.md) adds a bounded, opt-in static FP32 MatMul+Add backend experiment and corrects ambiguous RAPL counter-wrap handling. The experiment emits C++17 source and compares its native artifact against prepared ORT; it is not ONNX-to-MLIR or `.short` lowering. R2/R3 remain prerequisites for integrated R5, including the identical-artifact ShortHand/C++ comparison. Initial observations include slower AOT cells at batches 16 and 32; no default backend, language superiority or energy claim changes.

## Sequenced implementation work

These are dependency-ordered slices, not reserved GitHub PR numbers. Combine adjacent work only when it shares a compatibility boundary and can be reviewed and tested together. Keep one active implementation PR and update the benchmark Markdown with every PR.

| Slice | Dependencies | Deliverable | Required exit evidence |
|---|---|---|---|
| R1: borrowed input and lazy bridge observations | PR122 | This PR's bounded runtime changes and observer controls | Exact C ABI; owning/borrowed input checks; null/count/shape rejection; alias/rollback; lazy snapshot/reset/exporter consistency; sanitizer and native package gates; paired head/base observations |
| R2: explicit immutable model capability | R1 | Verified self-contained artifact ownership, preparation once, versioned internal binding and reset lifecycle | Digest mismatch rejection; owned byte lifetime; external-data rejection or complete dependency hashing; concurrent reset; stale handle rejection; mutable-path behavior unchanged; zero model-file reads in the *immutable* warm region measured by tracing |
| R3: source tensor operations and safe buffer lowering | R1, stable semantic contracts | Source initialization/indexing with bounds checks, compatible grammar composition, pointer views proven safe by analysis | Interpreter/LLVM/MLIR differential tests at O0/O2; invalid index/shape diagnostics; aliasing and mutation tests; existing programs unchanged; native Windows/macOS/Linux execution |
| R4: persistent training executor | Independent after R1 | Bounded reusable workers, per-worker activation/gradient workspace, fixed sample-order reduction | Gradients versus the original implementation; finite differences; identical seeded optimizer updates; partial construction and exception shutdown; TSan; measured allocation/thread counts; time and energy to equal held-out quality |
| R5: bounded model AOT import | R2/R3 | Static FP32 MatMul + Add first, then individually qualified operators | Explicit supported ops/opsets/dtypes/shapes; reject unsupported graphs before execution; ONNX differential outputs; exact same AOT library called from ShortHand and C++; ORT fallback retained; compiler time and binary-size evidence |
| R6: host/model fusion and arena planning | R3/R5 | Dependence-safe normalization/epilogue fusion and per-worker liveness-based workspace | Unfused/fused ablations; C++ same-artifact and fused controls; overlap/alignment/overflow/escape/recursion tests; numerical tolerances; measured peak workspace and allocations; no shared mutable scratch across workers |
| R7: quality-constrained approximations | R5/R6 | Explicit opt-in mixed precision and declared early-exit candidates | Disjoint train/calibration/test data; pinned thresholds before test evaluation; subgroup and tail-quality checks; numerical tolerance plus task-quality floor; rejection and fallback; no silent FP32 semantic changes |
| R8: constrained energy tuning and release assessment | Qualified earlier candidates | Bounded schedule search with energy/latency/memory constraints and a replayable selected plan | Independent selection/confirmation data; energy-domain validity and uncertainty; held-out superiority tests; compilation/tuning amortization; fallback when no candidate qualifies; synchronized governance documents |

### R2 implementation details

The current [internal v1 capability](immutable_model_capability.md) owns verified self-contained bytes and one prepared CPU session, checks current routing policy, serializes reset/replacement, and rejects stale handles. It does not install a new public header or change the C ABI. CI now requires traced zero model-path I/O with live positive controls, native lifecycle/sanitizer tests and installed static consumers on all four targets. Compiler-emitted sections, registration hoisting and source syntax remain future integration work; this ownership layer alone is not R5 AOT.

Introduce a capability that owns a validated artifact or refers to a compiler-emitted immutable section with a documented lifetime. Validate format, byte length, SHA-256 and external dependencies before preparation; retain a generation identifier across model changes and reset. Do not reuse the current path identity as an implicit immutable promise. The mutable API must continue detecting content replacement without relying on metadata. Registration hoisting must preserve declaration order, error reporting, thread safety and behavior after `short_runtime_reset`; a global constructor alone is insufficient.

Model embedding loads ONNX bytes into an executable. It does not compile model operators. Report the two features separately. Avoid portable-code assumptions about ELF `.rodata`: test PE/COFF and Mach-O mappings and relocation as part of installed-package qualification.

### R3/R5/R6 implementation details

Before exposing tensor mutation, define aliasing, bounds, lifetime, ownership and diagnostics in the language contract. Do not mechanically merge parser productions or change existing language-version interpretation. Keep a reference lowering as the differential oracle. Bufferization must preserve SSA reads in the presence of writes; borrow syntax alone is not a complete alias proof.

The AOT importer starts with a deliberately small self-contained static graph subset, bounded graph/constant sizes and explicit operator semantics. Handle broadcasting, layouts and overflow; reject malformed/external/dynamic graphs unless supported. Compare outputs using predeclared absolute/relative tolerances, including zeros, extreme values and nonfinite rejection. Do not enable fast-math silently. Extend operators only after equivalent tests exist.

Arena metadata records alignment, offsets, live intervals, peak bytes and scope. It is a compiler claim checked by analysis and runtime tests, not a measurement obtained by writing a constant. Include per-worker storage, recursion/escape behavior and the committed output boundary. Keep the simple unfused candidate available for ablation and fallback.

### R4 implementation details

Retain deterministic sample-ordered gradient accumulation and optimizer/seed/data behavior. Allocate the worker set, activation scratch and batch gradient storage before the defined compute region. Reserve telemetry/result vectors separately and include their costs in the full training boundary. Count worker and main-thread allocations inside the chosen region; report startup and per-batch costs separately. The current two-convolution qualification workload is not a general training engine or evidence for transformer training.

### R7/R8 implementation details

Quantization and early exit are explicit quality-constrained changes, not exact optimizations. Split tuning and confirmation runs to avoid selecting and reporting the same lucky minimum. A selection record identifies model/data/compiler/backend/hardware hashes, ISA, shape, precision, thread/spinning settings, candidate list, rejected reasons, quality, latency tails, energy domain/class, uncertainty, memory and compile/tune cost. Revalidate on any relevant identity change; use the accepted baseline when no candidate passes.

## Experiment protocol and permitted claims

### Functional units and comparison boundaries

Pre-register the model and dataset hashes, task, precision policy, quality threshold, batch/concurrency distribution, warmup policy and hardware before collecting confirmation data. Use a correct inference/request for inference; use time and energy to a fixed held-out quality for training. Report cold startup, resident inference and end-to-end application/service operation separately. Include failures in completion/reliability accounting; never count failed or unchecked outputs as useful work.

The current generated `.short` workload uses zero-initialized inputs and a synthetic MatMul+Add graph. Its numerical oracle checks outputs, but it does not establish held-out task accuracy or complete source-language preprocessing/postprocessing. The separate real-digit application comparison remains a distinct boundary. Generated compiled cells are b1-t1, b16-t1 and b32-t1; b16-t2/b16-t4 remain unsupported by the current compiled ABI, not silently passed.

| Attribution comparison | What it can establish |
|---|---|
| ShortHand + ORT versus C++ + matching ORT options | Overhead of the declared application/runtime boundary; document additional validation, mutable-file checks and telemetry |
| ShortHand + AOT versus C++ + the identical AOT artifact | Host/runtime overhead with the model implementation held fixed |
| AOT versus ORT under the same host and quality | Backend/compiler benefit, not a language-only advantage |
| Fused versus unfused under both hosts | Contribution of fusion; include a fused C++ implementation or expose the same fused artifact |
| Exact versus quantized/early-exit candidates | Quality-constrained system tradeoff, with both quality and resource results disclosed |
| Python/ORT and other framework baselines | Deployment comparisons with explicit pinned versions, tuning budgets and included work |

Use a tuned but bounded common search budget for batch sizes, threads, graph optimization and spinning. Record all candidates, not just the best result. Label missing framework/device support as unavailable and narrow the claim. Start with digits/tabular, then held-out vision, retrieval and sequence workloads with appropriate quality metrics. Small deterministic fixtures test mechanics; they cannot substitute for these tasks.

### Energy evidence

| Available evidence | Report | Allowed interpretation |
|---|---|---|
| E0 external calibrated meter | Integrated energy and uncertainty for the stated system boundary | Measured system energy for that boundary and campaign |
| E1 CPU/device counters | Counter domains, time window, resolution, wrap handling and joules per verified unit | Measured component-counter energy; not automatically process-attributed or whole-system energy |
| E2 valid calibration | Hardware-matched profile, provenance, validation residuals, uncertainty and estimated joules | Calibrated estimate, explicitly labelled; not a fresh physical measurement |
| E3 justified analytical model | Model assumptions, operation/traffic counts, parameter provenance, sensitivity and estimated joules | Analytical estimate within its validity limits; not a physical comparison |
| None | Null energy, reason unavailable, latency and structural observations | No energy reduction claim |

Do not manufacture E2/E3 values when their inputs or validated profiles are absent. Do not turn elapsed time, CPU utilization, FLOPs or nominal TDP into measured joules. RAPL domains can overlap: avoid summing a package and its contained subdomains. Record other host activity, counter coverage, sampling cadence and wrap constraints. Carbon requires additional location/time-specific factors and lifecycle boundaries; lower runtime joules alone is not a lowest-carbon or certification result.

No dedicated physical runner is assumed. Existing hosted CI probes available evidence and retains truthful unavailability. A future metered campaign may run on an accessible qualified machine or supplied trace; it must execute and replay actual workloads. Do not schedule untrusted PR code on a privileged self-hosted runner or create a permanently queued release dependency.

### Confirmation statistics

Use a pilot to choose run duration and sample size before confirmation. The proposed minimum is 30 randomized paired AB/BA blocks per independent session over at least three sessions; this is a starting protocol, not a guarantee of statistical power. Randomize with a recorded seed; retain order, thermal state, hardware settings, rejected runs and a predeclared exclusion policy. Never remove slow runs after observing results.

Treat the paired block/session as the sampling unit. Repeated calls within one process are not independent experiments. Use paired log ratios and a session-aware bootstrap or a justified equivalent. Report point estimates and confidence bounds for latency and joules, plus p50/p95/p99, throughput, quality and memory where supported by enough samples. Shared-runner CI ratios are descriptive regression signals; they do not authorize broad performance claims.

For a specified cell, a lower-energy claim needs a conservative upper confidence bound for `J(candidate)/J(baseline)` below 1 after instrument/model uncertainty. Use a declared practical improvement margin and adjust for multiple predeclared cells/baselines. Do not double-count uncertainty already included in the interval. A separate faster claim needs its own latency confidence bound below 1; meeting a latency SLO is a different result. Report losing and inconclusive cells. A global geometric mean cannot hide a failed cell behind an unqualified universal claim.

For compilation/tuning amortization, report incremental preparation energy `Delta_E_setup` and per-unit saving `Delta_E_unit`. Only when the latter is positive is an energy break-even count meaningful: `max(0, Delta_E_setup) / Delta_E_unit`. Include deployment lifetime and cache reuse assumptions. Measure both terms or label unavailable; binary embedding alone does not imply a positive break-even result.

Permitted final wording is scoped, for example: “On [hardware], revision [SHA] used [estimate/measurement, domain] fewer joules per correct [task] than [pinned baseline], at [quality/SLO], over [campaign], with [confidence interval].” Populate it only after the corresponding evidence and claim policy pass. Do not replace it with “the world's lowest-energy AI language.”

## CI, retained evidence and releases

`.github/workflows/ci.yml` already calls `runtime-profile.yml` on every push and PR. Keep its real ONNX tests, normal/profiled isolation, exact base build, numerical oracle, prepared-cache ASan/UBSan, energy probe, generated/resident comparisons, Markdown generation and manifest replay mandatory. This PR broadens the additional `experiment-results.yml` path filters to compiler/MLIR/benchmark changes. Existing PR122 successful trusted-push releases and four native installer lifecycle gates remain enabled.

Each run retains head/base SHAs, source diff digest, compiler/SDK identity, machine metadata, commands, model/data/oracle/executable hashes, raw stdout/stderr, invalid outcomes, inference-only and observer-inclusive timings, separate diagnostic phases, energy availability, generated Markdown and replay receipts. Artifact retention is currently 90 days, not permanent archival. Before relying on a campaign publicly, copy its accepted manifest and underlying evidence to a durable versioned evidence location, verify hashes again, and record the URI/digest in the committed benchmark report. A hash proves integrity relative to a trusted digest; it is not a signature or proof of external authenticity.

Do not auto-commit reports with a privileged PR workflow. Keep source PR evidence in `docs/latency_energy_test_and_benchmark_results.md`, link final run/artifact identities in the PR, and preserve accepted result snapshots when promoting a claim. Failed builds retain diagnostic evidence and cannot publish a successful development release. Packages remain unsigned development installers; optimization work does not expand the production allowlist.

The dated PR102/103 identifiers in `production_truth.tsv` and its coupled validation scripts are historical governance debt. Reconcile those files together with their linked coverage/release documents in a dedicated closeout slice, distinguishing implementation status from external qualification. Do not simply replace a PR number or flip a claim flag to make a gate pass.

## Reproduction and acceptance

Use the dependency installation and normal/profiled build commands in `.github/workflows/runtime-profile.yml`; use `.github/workflows/cpu-packages.yml` for native targets. The supplied proposal's unimplemented `--ci-metered` command is not a supported reproduction command.

Run the existing gates and these targeted checks against a real ORT-enabled build:

```sh
bash tests/abi/test_runtime_abi_api_stability.sh
bash scripts/check_runtime_ai_bridge_link_build.sh
ONNXRUNTIME_ROOT="$PWD/build-profile/onnxruntime-1.30.0" bash scripts/check_runtime_ai_bridge_execution_path.sh
CXX=clang++-18 python3 tests/ai_application/test_compiled_cache.py build-profile build-profile/onnxruntime-1.30.0
CXX=clang++-18 python3 tests/ai_application/test_runtime_phase_profile.py build-profile build-profile/onnxruntime-1.30.0
python3 experiments/energy/test_generated_infer_evidence.py
python3 experiments/energy/test_ci_run_evidence.py
python3 experiments/energy/test_render_benchmark_markdown.py
```

CI builds the exact base archive and invokes `generated_infer_evidence.py` with `--build`, `--base-build`, `--sdk`, both SHAs and `--output`. Observer controls are captured automatically when a base build is supplied. Replay with `--output DIRECTORY --replay-sha256 DIGEST` from the retained `manifest.sha256`. Keep full native matrices and sanitizers as release gates; local host-only tests do not establish Windows/macOS behavior.

## Primary technical references

- [ONNX Runtime user-buffer tensor API](https://onnxruntime.ai/docs/api/c/struct_ort_1_1_value.html): supports tensor creation over caller-supplied storage; the application must supply a valid lifetime.
- [MLIR bufferization](https://mlir.llvm.org/docs/Bufferization/): alias and read-after-write analysis governs safe conversion from tensor values to buffers.
- [Linux powercap framework](https://cdn.kernel.org/doc/html/latest/power/powercap/powercap.html): documents energy counters, ranges and hierarchical domains used by component metering.
- Repository contracts: [energy-contracted compilation](energy_contracted_compilation.md), [optimization roadmap](latency_energy_optimization_pr_plan.md), [benchmark results](latency_energy_test_and_benchmark_results.md), and [CPU package qualification](cpu_release_packages.md).
