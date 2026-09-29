# Latency and energy optimization PR plan

Plan version: 2026-09-29, revision 2 — feasibility review of the earlier 15 proposals

Implementation baseline: `master` at `9140e12d6495237e8a4daface805f556a137d126`

Status: planning deliverable; all four implementation batches below are planned

Planning PR: [GitHub PR114](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/114)

Scope: compiled ShortHand applications, the native runtime, CPU model execution, and reproducible evidence

## 1. Delivery decision

Deliver the remaining work in **four sequential implementation PRs**, with one active implementation PR at a time. Each PR contains reviewable, buildable commits, tests, and its own before/after evidence. This document and its [tracking table](latency_energy_optimization_pr_plan.tsv) are delivered together in one planning PR. Planning does not demonstrate an optimization or authorize a performance claim.

Use stable work IDs **LE1–LE4**, not guessed GitHub PR numbers. Record the actual URL when each PR is opened. Branches target the latest merged `master`; avoid stacked branches, duplicate implementation PRs, and branches left without a PR. Reuse an existing open PR for the same work. Close a superseded PR only after confirming that its required changes and evidence are retained.

This plan supersedes the *unimplemented sequence* in the [PR110 proposal](pr110_runtime_energy_optimization_plan.md). PR111 already consolidated the retained PR112/113 changes; do not implement those again. The earlier PR83–PR102 release roadmap remains closed. These new optimization batches address runtime/performance evidence gaps; they do not change release qualification, certification, or the production truth ledger merely by being planned.

Priorities are: make actual generated programs reuse prepared execution; remove remaining host work; tune execution under latency and energy constraints; then explore compiler-level improvements. Reducing latency, average power in watts, and energy per task in joules are separate objectives. A faster run can draw more power. Report each supported quantity separately.

## 2. Verified starting point and remaining gaps

The September 22–27 recommendations called for prepared sessions, reusable buffers, fused preprocessing, fewer copies, deferred telemetry, static plans, ORT tuning, LTO/PGO, AOT model compilation, tensor fusion/memory planning/vectorization, and later quality-constrained precision search. This plan reconciles those recommendations with the merged source rather than treating them all as new work.

| Area | Observed at the baseline | Action |
| --- | --- | --- |
| Repository status | [PR111](https://github.com/amarbanerjee23/ShortHand_Compiler/pull/111) merged on 2026-09-28; PR112 incorporated into that branch, PR113 closed as superseded | Start from merged `master`; retain the consolidation |
| Existing validation | Master [CI](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36418803737), [experiments](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36418803739), and [runtime profile](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/36418803824) completed successfully | These are baseline workflow results, not evidence that this new plan is implemented |
| Prepared ONNX runtime | `OnnxPreparedSession.h` caches a session, tensor specifications/names and CPU memory metadata; the application path can bind a caller-provided output buffer | Reuse and extend this implementation |
| Classification host | `ApplicationQualification.cpp` fuses input validation/normalization, validates padded outputs, appends results transactionally, and uses bounded stack scratch for deterministic top-k | Preserve these gains and failure semantics |
| Remaining host allocations | `classifyBatchAppend` creates a `TensorBuffer`, copies its spec, and allocates/zeroes normalized input per batch; resident qualification constructs row vectors and repeat output containers | Add reusable worker/request workspaces and input views; measure allocation scope explicitly |
| Generated-language path | `mlir/lib/LowerToLLVM.cpp` emits `short_ai_infer_f32`; `runtime/ShorthandRuntime.cpp` builds an adapter buffer and calls `AIRuntime::infer`; `OnnxRuntimeBackend::infer` constructs an `Ort::Session` for that invocation | The prepared application benchmark does not establish prepared execution for generated programs. Address this gap in LE1 |
| Runtime synchronization | `RuntimeThreadSafeFacade.cpp` serializes the legacy process state using a recursive mutex | Preserve its semantics; measure lock cost, and design any parallel prepared handles with explicit ownership |
| ORT settings | Prepared CPU sessions use BASIC graph optimization, sequential execution, explicit thread counts, and disabled intra/inter-op spinning | Evaluate alternatives; disabling spinning is already implemented |
| Energy pipeline | `ci_energy_evidence.py`, `ci_energy_compare.py`, and the workflows provide capability probing, E1 collection and E2/E3 profile consumption | Extend the existing pipeline; do not create a second energy engine |
| Calibration coverage | No default CPU calibration profile is checked in; profile validation/matching is intentionally bounded | Strengthen validity checks and accept a real profile only when its evidence and hardware match |
| Compiler | MLIR lowers verified language operations and checked runtime calls; this is not a complete model-operator AOT optimization pipeline | Add a bounded, optional model-compilation path in LE3 |

The retained PR109 result of approximately **1.79–1.84 times direct C++/ORT latency** is historical, pre-PR111 evidence. It is not the current runtime ratio. LE1 must capture fresh same-runner comparisons of the merged baseline and candidate before reporting any new gain. A successful workflow alone does not supply that ratio or establish joules/task.

The first measured workload remains the pinned FP32 UCI Optdigits classifier. Broader AI families require real workloads, quality metrics and equivalent controls before generalization. The native application remains usable without Python; Python is permitted for existing build-time experiments, test oracles, and comparison tooling.

### 2.1 Feasibility assessment of the earlier recommendations

**The central direction is practical, but simultaneous latency and energy reductions are a hypothesis to test per workload and hardware.** AOT compilation, static memory planning, fusion, specialization and constrained tuning have established implementations in other compiler systems. ShortHand can integrate them with its quality and evidence contracts. This does not establish that the combination is novel research or that it will outperform every optimized C++ implementation.

The earlier response's example milliseconds, millijoules, accuracy values and optimization certificate were illustrative, not ShortHand measurements. Its GPU/NPU research results do not forecast gains on the currently qualified CPU backend. In particular, the EDA paper concerns an NPU; CPU caches do not provide the same explicitly managed on-chip storage. The previously quoted early-exit/DVFS percentages were not independently substantiated in this review and are not used as targets.

| Earlier proposal | Practical assessment and expected mechanism | Decision within this plan |
| --- | --- | --- |
| 1. Energy as a compiler objective | Feasible as a constrained optimization policy; choosing fewer cycles or lower latency alone cannot establish lower joules | Accept in LE2/LE4 using measured or explicitly qualified estimated energy |
| 2. Remove ORT from the hot path | Feasible for a supported, fixed model graph; native code still needs validation, ABI handling and possibly native kernel libraries | Accept bounded CPU AOT in LE3; retain ORT for the general supported path |
| 3. Whole-model / temporal fusion | Fewer intermediate materializations can help, but the largest fused region can spill registers, repeat work or lose a tuned library kernel | Replace unrestricted super-fusion with cost-bounded region fusion in LE3 |
| 4. Inter-layer memory scheduling | Plausible for compatible producer/consumer tiles; dependencies, reductions and cache pressure limit legal reuse | Accept one CPU multi-layer pilot in LE3; no NPU speedup/energy numbers transferred to CPU |
| 5. Static tensor memory compiler | Highly practical for static shapes; reuse non-overlapping lifetimes in bounded per-worker scratch | Accept an explicit arena plan in LE3 and reusable host buffers in LE1 |
| 6. Accuracy-aware compilation | Feasible with suitable kernels and representative calibration/held-out data; reduced precision can also slow execution | Accept bounded operator-group mixed precision in LE4; defer INT4 until a qualified workload/kernel needs it |
| 7. AI semantic type system | Shape/layout/ownership facts can enable safe specialization; observed ranges and accuracy are not mathematical guarantees | Accept verified internal IR/plan facts in LE3 first; defer the proposed new source syntax |
| 8. Compile sparsity | Immutable structured zeros can avoid arithmetic; indexing/packing overhead can outweigh savings | Conditional LE4 experiment with existing exact-zero weights; defer activation sparsity, pruning and MoE routing |
| 9. Conditional inference / early exit | Requires a model with trained/calibrated intermediate exit heads or an explicitly validated cascade; confidence is not guaranteed correctness | Defer from LE1–LE4 implementation; revisit only with a suitable model, full-path fallback and quality evidence |
| 10. Hardware-specific schedules | Feasible on qualified CPUs with ISA/OS checks; wider vectors, unrolling and more threads are not always beneficial | Accept bounded CPU schedule/variant selection in LE2/LE3 |
| 11. Energy autotuner / ECO optimizer | Practical when it reuses the existing evidence engine and limits compile/measurement cost | Accept budgeted offline search and a versioned Pareto database in LE2/LE4; no mandatory thousands-of-schedules search |
| 12. Direct DVFS control | System policy and shared frequency domains complicate safe per-model control; hosted environments often cannot expose it | Defer frequency writes; record observable frequency/governor state and tune threads/batching instead |
| 13. CPU/GPU/NPU partitioning | Technically possible but depends on live-qualified device backends, transfer costs and device-specific energy data | Defer until accelerator execution and end-to-end transfer evidence exist |
| 14. Traffic-guided runtime specialization | Practical as a small offline AOT variant set with explicit shape guards and a supported fallback | Accept at most three CPU variants initially in LE3; no request-time compilation or automatic production mutation |
| 15. Proof-carrying optimization | A replayable empirical report is feasible; hashes and signatures do not prove correctness or energy superiority | Accept an optimization evidence manifest using existing auditor primitives; call it evidence, not certification |

The highest expected near-term impact comes from LE1's repeated-session elimination and buffer reuse, followed by bounded AOT and memory/fusion work. Confidence in reducing repeated work is higher than confidence in a particular end-to-end speedup, and no percentage energy saving is presently established. Autotuning chooses among valid trade-offs; an evidence manifest validates provenance rather than speeding execution itself.

### 2.2 Concrete model opportunity and experiment boundary

`scripts/create_digit_application_fixture.py` emits an ONNX opset-13 graph containing **MatMul followed by Add**, with weights `[64,10]`, bias `[10]`, and a symbolic batch input. Application qualification supplies concrete batch shapes 1/16/32. This is a manageable first AOT target: specialize the *validated concrete shape*, reuse packed immutable weights, fuse a legal bias epilogue, and compare direct native execution with the same optimized ORT settings.

Preserve range/finite validation, normalization, FP32 score tolerance, all required scores and deterministic top-k. Folding normalization into weights/bias can change rounding and must not be enabled as an exact rewrite without proving the numerical contract; measure it separately if allowed under an explicit error budget. Avoid dead-output elimination when the application API still requires those outputs.

The `digits.short` emitted by that fixture is explicitly a **float64 correctness oracle**, not the equivalent FP32 inference benchmark. LE1 must add a compiled `.short` loop using the FP32 `infer` runtime boundary for the same-backend comparison. Do not compare different precisions or claim generated-program gains from the resident qualification executable alone. The two-operator graph is also insufficient to validate inter-layer scheduling: use a separate supported, licensed multi-layer workload for that experiment.

## 3. Consolidated PR sequence

Effort ranges are planning estimates for an engineer familiar with this repository, excluding CI queue time, unavailable dependencies, and external calibration-data acquisition. They are not delivery dates.

| ID / priority | PR scope | Depends on | Primary owner role | Estimate | Completion outcome |
| --- | --- | --- | --- | --- | --- |
| LE1 / P0 | Prepared execution in generated programs, reusable host workspaces, and fresh comparisons | Merged PR111 | Runtime/compiler engineer | 8–12 engineer-days | Actual `.short` programs reuse prepared sessions; less repeated allocation/copying with preserved behavior |
| LE2 / P0 | Bounded ORT/serving tuning and stronger continuous energy evidence | LE1 | Runtime/performance engineer | 7–12 engineer-days | Reproducible latency/power/energy trade-offs and validated configuration selection |
| LE3 / P1 | Verified semantic plans, bounded AOT, static arenas, region fusion, CPU variants, LTO/PGO | LE2 evidence contract and LE1 runtime | Compiler engineer | 20–35 engineer-days | End-to-end compiled applications with attributable, independently switchable model/compiler optimizations |
| LE4 / P1 | Mixed-precision/conditional sparsity experiments, constrained selection and broader qualification | LE2 and LE3 accepted baseline | Compiler/performance engineer | 10–18 engineer-days | Per-workload Pareto choices and an auditable report of wins, ties, losses and remaining blockers |

Energy collection and output/quality checks accompany **every** batch. LE2 improves their coverage; LE1 does not wait for it to preserve the current E0–E3 evidence rules. AOT or precision experiments may conclude with no winning candidate; do not enable a losing path to satisfy a roadmap checkbox. If dependency or review size requires a split, update this table and explain the concrete reason before opening another PR.

The revised estimates include the bounded additions below, not a general tensor compiler, a new language-wide dependent type system, arbitrary-model mixed precision, early exits, DVFS control or accelerator partitioning. Conditional experiments have a finite evaluation budget and may close as deferred with an explicit reason. They do not require extra PRs or prevent accepted core work from completing.

## 4. LE1 — prepared generated execution and reusable host memory

Suggested branch: `agent/latency-energy-runtime-reuse`.

**Problem and hypothesis.** Prepared resident inference and compiled-language inference currently use different paths. Session construction, metadata rebuilding, buffer allocation/copying and synchronous reporting can overwhelm small models. Reusing validated state should reduce repeated work; the size of the benefit must be measured.

**Commit sequence**

1. Freeze a baseline manifest and run two separate lanes: the existing five-cell resident application comparison and a real compiled `.short` program that repeatedly executes inference. Retain source, generated IR/native artifacts, model/data hashes, binary hashes, counters, output checks and environment. Profile session creation, bridge work, lock time, allocations, copies, normalization, ORT execution, output validation, top-k and telemetry separately from uninstrumented timings.
2. Add a bounded prepared execution context for the compiled runtime. Reuse `AIRuntime::prepare`/`PreparedInference` and the existing ONNX implementation. Preserve the existing C ABI through an adapter; add a versioned handle API only if needed, with installed-consumer tests. Bind cache identity to model contents, shapes/dtypes, provider/version, preparation options, and registration generation. Define reset, re-registration, shutdown, eviction and model replacement behavior. Identical repeated registrations generated inside an inference loop must be idempotent and preserve valid preparation; changed definitions invalidate it. A mutable path alone is not a cache key. Keep the old path for unsupported configurations with explicit routing and evidence.
3. Keep model replacement semantics safe: establish an explicit immutable prepared-model lifetime, or detect/revalidate changes before cache reuse. Do not silently stop observing model changes that the old per-call loader would have observed. Concurrent reset/re-registration must not destroy an in-flight context; allocation failure must leave the previous valid state intact.
4. Add bounded worker/request-owned scratch for normalized input and internal output/postprocessing storage. Accept a pointer-and-length/read-only view internally under C++17; do not require a language-standard upgrade. Remove avoidable `rows(offset)` copies, spec copies, full-batch zeroing and repeat container growth. Initialize all real inputs on every call, zero every tail slot before inference, and keep externally owned result lifetimes unchanged.
5. Reuse output binding already delivered by PR111. Keep reusable `Ort::Value` wrappers only where input/output storage addresses and ownership are stable; otherwise recreate wrappers safely. Do not expose an ORT-owned view beyond its lifetime. Retain the existing stack top-k scratch; specialize top-1/small-k only when profiling justifies it and tie ordering is unchanged.
6. Move avoidable legacy-path JSON/log formatting to the existing observation/export boundary while retaining counters, error status and observable telemetry content. Bound deferred buffers and define flush, overflow and shutdown behavior. Benchmark normal telemetry-enabled service operation as well as resident compute; moving export work out of one timed region must not hide its total cost.
7. Apply equivalent algorithmic/container improvements to the independent C++ control. Capture final head/base and native-control results, with ablations for prepared reuse, scratch reuse and reporting changes.

**Implementation surfaces.** `Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.cpp`, `RuntimeThreadSafeFacade.cpp`, `AIRuntimeBridgeAdapter.*`; `src/ai_runtime/AI_Runtime.*`, `AI_Backend.h`, `AI_Telemetry.*`, `ApplicationQualification.*`, `backends/OnnxPreparedSession.h`; `mlir/lib/LowerToLLVM.cpp` if generated handoff changes; `experiments/energy/cpp_onnx_baseline.cpp`, `pr_runtime_delta.py`, `profile_report.py` and their existing tests. Here and below, `src/` abbreviates `Compiler_new_ws/Short_Hand/src/`.

**Required verification.** Repeated generated inference through the installed runtime; reset, re-registration and changed-model identity; failed preparation; full/tail/alternating batch shapes; NaN/Inf/range/overflow errors; output-capacity errors; deterministic ties; failure followed by success without stale data; concurrent calls/reset/shutdown; bounded memory under soak. Extend the relevant existing tests under `tests/ai_application`, `tests/runtime`, `tests/integration` and `tests/mlir_lowering`. Run ASan/LSan/UBSan and TSan where those paths are exercised.

**Exit criteria.** One preparation per unchanged prepared context while it remains resident, proven by a test counter; no per-batch allocation for ShortHand-owned normalized-input/internal scratch after warmup for fixed shapes; explicit separate counts for ORT/internal-library allocations and caller-owned result materialization. Every output/quality and ABI check passes. Report both generated-program and resident-host deltas. Use the latency targets in section 8; unresolved parity is retained as a blocker for parity claims. No unmeasured percentage saving is promised.

**Rollback.** Disable the new prepared adapter/workspace path and retain the validated generic path. Keep regression tests and comparison artifacts. Preserve the legacy global lock until a separately tested ownership design makes its relaxation safe; a shared mutable scratch buffer is not a concurrency optimization.

## 5. LE2 — execution tuning and defensible energy evidence

Suggested branch: `agent/latency-energy-runtime-tuning`.

**Problem and hypothesis.** Threading, graph optimization, batching and queue behavior trade latency against CPU activity and energy. An energy model cannot infer device-independent joules from a computation count. The current evidence machinery needs broader validation before a fitted model can guide tuning reliably.

**Commit sequence**

1. Add explicit, bounded preparation options for ORT BASIC/EXTENDED/ALL, supported sequential/parallel configurations, thread counts, memory-pattern/arena choices and offline optimized-model artifacts. Keep current defaults initially. Hash the source and optimized model plus ORT version, CPU/ISA and options. Reject incompatible or stale artifacts. Evaluate CPU I/O binding against the existing preallocated-output implementation instead of assuming another gain.
2. Run a predeclared offline tuning grid with the same search budget on ShortHand and C++/ORT. Include intra-op threads 1/2/4 subject to actual CPU limits, the current five batch/thread cells, bounded worker counts, and spinning disabled versus explicitly tested alternatives. Do not enable newer spinning options unless supported by the pinned ORT headers/runtime. Prevent worker-count times intra-op-count oversubscription. Keep the current blocking serving queue as the starting point.
3. Add sparse, bursty and sustained load profiles with fixed offered load, deadlines and queue limits. Include idle/inter-arrival intervals in the service energy window. Batch within a declared maximum wait; record queue delay, request p95/p99, rejection/timeout rate and throughput. Reject energy-saving candidates that miss the SLO. Record total energy and completed tasks at the same load, not only a saturated throughput result.
4. Extend `ci_energy_evidence.py` and `calibration_profiles` with versioned validation of feature units, CPU identity, applicable stepping/ISA, CPU quota/topology, kernel/perf event definitions, frequency/DVFS validity, measurement boundary, calibration source hash, held-out error, sample domain and profile expiry. Preserve v1 compatibility for already supported consumers; new eligibility rules must not silently promote weak profiles. Reject missing/ambiguous matches, out-of-domain inputs, stale provenance, impossible energy and unavailable required counters. A boolean `validated_calibration` or descriptive provenance string alone is insufficient evidence for a new accepted E2 profile.
5. Harden workload/counter alignment: include all workload threads/children, record perf enabled/running times and multiplexing quality, timestamp energy sampling windows, handle counter wraps, and avoid overlapping package/domain sums. Compare like measurement domains; CPU package energy is not whole-host energy. Keep observer overhead and background activity diagnostics. Do not subtract guessed idle energy from the primary result.
6. Add a reproducible calibration import/fit-and-validation path using existing-host E1 observations when accessible or a compatible retained external dataset. Split by workload/run, hold out validation workloads, version coefficients/units and report prediction uncertainty. Published coefficients qualify only for their validated hardware/conditions. If no suitable observations exist, ship the capability and explicit unavailable result without a default profile.
7. Extend paired reports to include watts over a declared common service interval when energy is actually available, joules/task, cycles/instructions, allocations, copies and uncertainty. Keep E1/E2/E3 results separate. Select and freeze a candidate on tuning data, then evaluate on independent repetitions; retain the full search, including failures and losing settings.

**Implementation surfaces.** `src/ai_runtime/AI_Types.*`, `backends/OnnxPreparedSession.h`, `ApplicationQualification.*`, `src/serving/ServingRuntime.*`; `experiments/energy/ci_energy_evidence.py`, `ci_energy_compare.py`, `calibration_profiles`, `runtime_state_of_practice.py`, `cpp_onnx_baseline.cpp`; `.github/workflows/runtime-profile.yml` and `experiment-results.yml` only as required for new coverage. Extend existing contracts rather than adding duplicate workflows.

**Required verification.** Unsupported option/version, stale optimized model, mismatched profile/units/domain, missing counters, counter wrap, multiplexing, nonfinite/negative estimates, timeout and nonzero child exit, changed CPU quota, concurrency, deadline and shutdown tests. Use synthetic profiles only in validator tests and label them synthetic. Real fitted coefficients require retained measured training/validation evidence. Test that missing energy remains `null`/unavailable and cannot become zero or an energy-saving claim.

**Exit criteria.** A reproducible candidate-selection manifest and complete same-runner comparison. Every enabled setting meets quality/SLO/memory constraints. E2 acceptance requires independently held-out validation and a predeclared error threshold appropriate to the intended effect size; report prediction intervals, not just training fit. A difference smaller than combined uncertainty is inconclusive. With no valid energy evidence, selection is latency/work-based and explicitly not an energy optimum. The plan does not require a dedicated physical runner.

**Budgeted tuner foundation.** Add one shared candidate-record contract around the existing runners: source/model/graph and shape signatures, settings, target/ISA, toolchain/backend, workload/SLO, quality split, energy class/domain/profile, uncertainty, compile cost, raw evidence hashes and rejection reason. Start with a deterministic grid or staged elimination, including the unchanged baseline. Use a configurable initial cap of 32 candidates per model/target plus a separate held-out confirmation budget; record actual time/energy spent tuning and stop at the declared budget. A learned cost model is optional later work, not a prerequisite. Cache results only under complete identity keys, reject stale entries, and rerun deployment validation before reuse. LE3 adds kernel schedules to this same contract; LE4 adds precision choices. Do not create three independent autotuners.

**Rollback.** Restore the existing BASIC/sequential/no-spin options and worker configuration. Reject an invalid profile without disabling correctness/latency reporting. A calibration failure cannot relax the strict measured-energy release gate.

## 6. LE3 — static plans, AOT and tensor/compiler optimization

Suggested branch: `agent/latency-energy-aot-tensor`.

**Problem and hypothesis.** Removing wrapper overhead can approach the same native backend, but cannot establish universal superiority over optimized C++. A potential application-level advantage comes from using known model shapes, constants, buffer lifetimes and preprocessing/postprocessing together to eliminate work and data movement.

**Commit sequence**

1. Specify a deterministic static execution-plan artifact. Bind model/data contract, concrete shapes/dtypes, backend/compiler versions, target ISA, thread/batch options, preprocessing, numerical policy, postprocessing and telemetry mode. Validate hashes at load and reject stale or unsupported plans. Build on LE1 prepared-context identity rather than introducing a second cache.
2. Add one optional, pinned AOT route for the existing FP32 classifier at validated concrete batch shapes, using ONNX-MLIR or a documented alternative selected after an operator/toolchain compatibility spike. Start with the verified MatMul+Add graph described in section 2.2. A bounded MLIR emitter is an alternative only for that validated operator subset; do not claim an arbitrary ONNX importer or hand-code a different model. Select one initial route. Verify pinned LLVM18 compatibility, and isolate an external AOT toolchain if it needs another LLVM version. Pin licenses, downloads and hashes; do not require a new compiler backend for ordinary builds.
3. Expose a versioned native tensor ABI with shape, byte size, alignment, ownership and error contracts. Execute it from a compiled `.short` program, not only a benchmark utility. Unsupported operators/shapes fail explicitly; the declared AOT track must not silently execute ORT. Keep a direct C++ caller of the *same AOT artifact*.
4. Implement a bounded pass pipeline: shape/type propagation and legal constant folding; fusion of supported pointwise/pre/postprocessing operations; liveness-based buffer reuse and dead temporary removal; tiling/vectorization when the target supports them. Use proven alias/ownership analysis and explicit deallocation. Verify operator support before extending it. Do not fuse or reorder floating-point reductions outside the established numerical contract.
5. Add per-pass switches and optimization remarks with operation counts, tensor traffic bounds, copies/materializations, peak live scratch, code size and compilation time. Logical tensor bytes are not measured DRAM bytes or joules. Report unknown traffic levels as unknown. Validate structural counts against small known graphs.
6. Evaluate portable `-O3`, LTO, PGO with representative training inputs, and separately labeled target-specific builds. Give the direct C++ control the same compiler opportunities. Keep held-out measurement inputs independent of PGO/tuning inputs; no unsafe fast-math or undeclared `-march=native` in portable release artifacts. Record startup/compilation cost and the break-even task count.

**Implementation surfaces.** `mlir/lib/SemanticLowering.cpp`, `LowerToLLVM.cpp`, `mlir/include/ShortHand`, new optional pass/backend files, `src/runtime` and `src/ai_runtime` adapters, `CMakeLists.txt`, the compiler Makefile, and existing experiment harnesses. Use existing MLIR differential, installed SDK, AI integration, sanitizer and reproducibility test registration.

**Required verification.** Source-to-native and direct-artifact parity; per-pass positive/negative IR tests; unsupported operators/dynamic shapes; alignment, aliasing and use-after-free; corrupted/stale plan/model/artifact; portable ISA behavior; reproducible builds; numerical/quality parity; disabled-feature builds. Treat FMA contraction/reassociation as a numerical-policy decision and test it, not an invisible speed flag.

**Exit criteria.** Report all four attribution cells: ShortHand+ORT, C+++ORT, ShortHand+AOT and C+++the same AOT. Each enabled pass has an ablation, correctness evidence and a justified benefit with no material memory/latency regression. As an aspirational experiment target, seek at least 10% application latency reduction versus the best equivalent ORT configuration on the bounded workload; this is not a promised result or a language-only claim. Promote AOT only on supporting evidence; otherwise retain it explicitly experimental or defer it. Report energy only at the class actually obtained.

**Rollback.** Optional AOT/pass/build flags default off until accepted. ORT and portable FP32 remain available. Reverting an experimental pass must not alter source semantics, installed ABI or evidence readers.

### 6.1 Required bounded compiler additions

These are reviewable stages inside LE3, not new PRs. Complete the AOT/arena baseline before enabling fusion or schedule search.

| Addition | Implementation boundary | Acceptance and stop condition |
| --- | --- | --- |
| Semantic facts with provenance | Extend internal plan/MLIR attributes for shape, dtype, layout/strides, alignment, ownership/aliasing, immutable constants and permitted numerical error. Mark facts as statically verified, runtime-guarded or merely observed. Reuse `mlir/include/ShortHand/IR` and the current model signatures | Reject contradictions and test propagation/invalidation across views and calls. Only established facts authorize check elimination. Dataset ranges and statistical accuracy do not prove overflow freedom or per-input error bounds. No new annotation syntax is required |
| Explicit static arena | Plan tensor birth/last use, checked byte size, alignment and reusable offsets after legal bufferization. Share read-only packed weights; allocate bounded scratch per concurrent worker, not one mutable process-global arena. Emit total and per-worker peak bytes | Verify no live aliases overlap and integer size arithmetic cannot overflow. Test alignment, tails, error cleanup, concurrent calls and memory caps. Require zero steady-state heap calls in the bounded generated AOT compute region; report adapter/telemetry/library allocations separately. MLIR bufferization alone is not an allocation-free guarantee |
| Cost-bounded region fusion | Begin with a MatMul bias epilogue and compatible pointwise work. Add one producer/consumer tiling experiment on a separate multi-layer graph. Include live footprint, strides/layout conversion, register pressure, recomputation and code size in candidate rejection | Compare unfused, epilogue-fused and tiled regions with identical arithmetic/quality contracts. Preserve reductions and all API-visible outputs. Do not promise cache residency or elimination of DRAM traffic without appropriate evidence. Stop or split a region when spills/recomputation or a lost tuned kernel erase the gain |
| CPU schedule selection | Add bounded tile size, vector width, loop order and thread choices to LE2's candidate contract. Reuse native/MLIR kernels; benchmark library calls as candidates instead of replacing every optimized GEMM | Keep an unchanged schedule control and equal C++ tuning budget. Validate ISA and required OS state before execution; retain the portable path. Record instruction/cache counters when available, scratch, code size and compile cost. No automatic wider-vector or full-unroll preference |
| Small AOT variant set | Start with at most three declared hot shapes, selected from a recorded workload distribution; current batch 1/16/32 can be the initial experiment. Dispatch with shape/layout/ISA guards and bind every variant to the model/plan identity | Include dispatch and padding in end-to-end measurements; test mixed and unseen shapes. Use an explicitly supported generic path, or fail closed if no such shape is supported. A fallback must be identified in evidence and excluded from an AOT-only result. Cap aggregate code/arena size and keep request-time compilation disabled |

Add an ablation row for each stage and for the full combination. Structural reductions in allocations or materializations are useful causal evidence, but are not joule estimates. Include a model large enough to stress memory before asserting a memory-traffic benefit; the tiny centroid classifier can be dominated by host overhead and already fit in cache.

**No wholesale ORT removal.** ORT can already optimize graphs and memory and call tuned kernels. Native AOT code may still use support libraries and allocate unless the chosen ABI/pass pipeline prevents it. LE3 succeeds by demonstrating an accepted bounded path; replacing a mature general runtime or outperforming its best kernels is not an assumption.

## 7. LE4 — constrained precision and broader application qualification

Suggested branch: `agent/latency-energy-quality-search`.

**Problem and hypothesis.** Some workloads benefit more from reduced precision, tensor layout, batching or different schedules than from wrapper tuning. Benefits are workload/hardware-dependent and valid only at equivalent declared quality and service requirements.

**Commit sequence**

1. Extend the candidate interface to FP32 and separately qualified INT8/BF16/FP16 only when the actual CPU/backend supports them. Reuse existing bounded INT8 family tests as a starting point, not as proof of full-model quality. Keep FP32 parity as an independent track. Any model/operator substitution, quantization, pruning or sparsity experiment needs explicit model identity, quality limits and a matched optimized baseline.
2. Implement bounded offline constrained selection: minimize available joules/task subject to quality, request p95/p99, throughput, memory and compatibility constraints; also retain the latency/energy Pareto frontier. When energy is unavailable, expose that fact and select only on supported metrics. Do not label instruction-count or latency winners as energy winners.
3. Use separate calibration/tuning/held-out evaluation splits. Include model/task metrics and predeclared acceptable degradation; a single accuracy number is not sufficient for retrieval or generation. Freeze budgets and thresholds before measuring candidates. Revalidate deployment when model, input-shape distribution, hardware or SLO changes.
4. Extend the existing benchmark-family manifest with real, licensed retrieval/embedding and a convolutional/vision workload before broad claims; add transformer/generative workloads only when their backend/operator support exists. Report current fixture-only families as bounded tests. Include generated ShortHand programs, application preprocessing, inference, postprocessing, data handling and normal telemetry. Maintain the original Optdigits cells for regression continuity.
5. Run the existing optimized native and resident Python controls. Extend C/Rust/Java, PyTorch eager/compile, or other state-of-practice tracks only where executable, pinned and quality-equivalent; record dependency/hardware unavailability and never count it as a win. Provide same-backend and best-tuned comparisons separately.
6. Freeze the candidate revision, rerun a retained campaign, document supported workloads/hardware, and update capability/coverage evidence only for executed functionality. Preserve raw files, hashes, scripts, source/model/data/license references, failed observations and environment manifests beyond temporary CI-artifact retention. Update release truth only when a qualified gate actually changes.

**Implementation surfaces.** Existing qualification/precision configuration and plan readers, `experiments/energy/state_of_practice.py`, `runtime_state_of_practice.py`, `campaign.py`, `report_results.py`, `run_verified.py`, `tests/ai_benchmark/benchmark_suite_v1.json`, family fixtures, quality tests and associated documentation.

**Required verification.** Accuracy/quality threshold rejection; no held-out leakage; unsupported dtype/hardware; quantization saturation/outlier cases; deterministic selection/ties; cache invalidation; overload, tail latency and timeouts; precision-matched baselines; full reproducibility and retained-evidence replay. Higher measured error cannot be concealed by averaging workloads.

**Exit criteria.** Every accepted deployment has a quality/SLO/memory/evidence manifest and a reproducible fallback. The final report covers all declared cells and labels improvements, equivalence, regressions, inconclusive and unsupported cases. Lower-power and lower-energy conclusions require the corresponding measurements or explicitly labeled compatible estimates. No universal C++ superiority, lowest-carbon, certification or GA claim follows from this plan.

**Rollback.** Restore the last accepted FP32 plan and configuration without a source rewrite. Preserve tuned/quantized artifacts and rejected-candidate evidence for reproducibility; remove them from automatic selection when validity expires.

### 7.1 Practical extensions to the quality/energy optimizer

1. **Operator-group mixed precision.** Begin with a small supported set of FP32/INT8 groups, then BF16/FP16 only with qualified native kernels. Keep sensitive reductions/normalization in the validated precision where applicable, specify accumulator dtype and quantize/dequantize boundaries, and count conversion/packing overhead. Treat each candidate as a new numerical plan, with layer-level deviation diagnostics and held-out task-quality evaluation. Reject unsupported kernel emulation or quality/SLO failures; the smallest dtype is not automatically the fastest. INT4, arbitrary per-operator combinatorial search and model retraining remain deferred.
2. **Exact-zero structured sparsity, conditional.** Inspect immutable weights for real structural zeros and a supported efficient CPU sparse kernel. If either is absent, record `not_applicable` and retain dense code. If present, compare dense and sparse execution on the same model with packing, indexes, branches and memory overhead included. Preserve floating-point/finite-value semantics; multiplying zero by NaN/Inf cannot be silently treated as zero. Test a dense counterexample and choose sparse only above an empirically supported crossover. Do not prune weights, assume an activation distribution, or infer transformer/MoE support from this experiment.
3. **FAST/ECO/BALANCED as deployment policy labels.** These are optional manifest selections over validated plans, not newly implemented language syntax. FAST minimizes observed latency subject to quality/memory/SLO constraints. ECO minimizes compatible energy evidence under the same constraints; when only E2/E3 is available its objective must say *estimated energy*. BALANCED uses a predeclared energy/latency constraint or tie rule, not an arbitrary weighted sum with incompatible units. With no valid energy data, ECO is unavailable; none of the labels denotes a global optimum. Select once at preparation/deployment, and include any allowed switching overhead in evaluation.
4. **Replayable optimization evidence.** Extend the candidate record with pass/variant/precision decisions, parent baseline, graph/model/compiler/target identities, dataset splits, quality and tail-latency distributions, memory/energy domains and uncertainty, raw artifact hashes and the acceptance decision. Reuse the existing candidate-auditor manifest/signature/replay infrastructure where applicable. Tamper tests must reject mismatched model, target, output, baseline or missing evidence. A signature proves integrity under its trust policy; it is not a mathematical proof, an authenticated physical measurement or C3-ECO certification. A report with missing energy must remain useful and must not invent it.

The concrete research hypothesis is that retaining AI semantic facts plus quality/SLO constraints lets ShortHand **automatically select and justify** more efficient application plans than the declared controls. To assess an integration advantage, compare fixed baseline, each optimization alone, the combined compiler without tuning, and the combined compiler with equal-budget tuning. Share backend artifacts/optimization opportunities with C++ controls so a kernel improvement is not mislabeled as a language-only win. Formal novelty and publication claims need a separate prior-art evaluation.

## 8. Common experiment and acceptance contract

### Comparison boundaries

| Track | Required comparison | What it can establish |
| --- | --- | --- |
| Same-backend resident application | ShortHand and independent C++/ORT, same model/settings/work | Runtime/host abstraction cost |
| Compiled-language application | Actual compiled `.short` against independent native application, same work and backend | Generated-program end-to-end effect |
| PR delta | Candidate and its actual merged base, on the same runner | Incremental software change |
| Backend/compiler | Both languages against the identical ORT and identical AOT artifacts | Backend gains separated from language/host gains |
| Deployment/precision | Equal quality/SLO constraints and equal tuning opportunity | Best declared deployment trade-off, with precision/model changes disclosed |

The fixed regression cells are **(batch, threads) = (1,1), (16,1), (32,1), (16,2), (16,4)**. Keep model, data, ORT version, preprocessing, top-k, seed, compiler flags, CPU quota and functional unit matched. Never compare a prepared steady-state ShortHand path with a C++ cold start, or an instrumented build with an uninstrumented one.

Resident timing includes validation, normalization, inference, output checks, postprocessing and required result materialization. Report load/compile/preparation, warmup, steady state, serving with queue/telemetry, and shutdown separately; also report end-to-end cost. A batch's latency divided by rows is amortized latency per task, not a request-latency percentile. Tail latency requires actual request timestamps including queueing.

Functional units are completed tasks satisfying the execution/output contract at the declared model-quality threshold. Do not discard valid but misclassified samples from the energy denominator. Report offered, completed, failed, timed-out and rejected tasks, plus classification/task quality. Exclude padded tail slots from completed units while including their computation in energy/time.

### Sampling and retained evidence

- Use balanced randomized AB/BA blocks on one host per paired campaign, plus paired direct-control observations. Capture clocks, CPU identity/quota, affinity/frequency settings where observable, OS/kernel, SDK/tool versions and process/thread counts. Hosted jobs on different machines are not paired observations.
- Use a pilot to set repetitions long enough to exceed timer/counter resolution. Predeclare the final repetition count and a minimum of 30 independent paired blocks for acceptance campaigns; extend the sample budget based on pilot variance and the intended effect size. Warmups and repetitions within one process are not independent host runs.
- Keep inexpensive PR diagnostics separate from acceptance campaigns. For the latter, repeat across at least three sessions and report each session/hardware stratum. Use a paired bootstrap or equivalent justified interval at the independent block/session level; do not pool thousands of inner iterations as independent samples. Report estimator, interval method/seed and sample count.
- Retain all trials and predefined exclusion reasons, including failures. Do not rerun solely until a favorable result appears. Freeze tuning choices before held-out acceptance. Publish per-cell arithmetic-mean latency and paired ratios with uncertainty; medians and tails are additional metrics. Do not average away a losing cell.
- Retain a manifest with base/head/source and binary hashes, generated-source path, dataset/model/plan hashes, settings, raw CSV/JSON, output and quality checks, stage profiles, allocation/copy counts, relevant counter traces, evidence class/domain, calibration hash and uncertainty. Every claim must point to the exact tested artifact.

### Prospective targets and decision rules

1. **Safety gate:** zero semantic/quality/ABI regressions, sanitizer/race findings or mandatory-test skips. Preserve finite/shape/dtype/range checks, deterministic ties, tail handling, transactional failures, memory bounds and fail-closed unavailable backends. The application-only validated-input token must not become an unchecked public path.
2. **Runtime parity goal:** ShortHand's mean resident application latency at most **1.03 times** the equivalent direct C++/ORT control in all five cells. Treat a 95% upper ratio bound at or below 1.03 as supported parity under this margin. If uncertainty crosses the margin, mark it inconclusive. A persistent ratio above **1.05** in any declared cell blocks the parity milestone. These are inherited engineering targets, not current results.
3. **Incremental promotion:** compare candidate/base separately. A 95% lower ratio bound above 1.05 is a material regression requiring rollback or an explicitly separate, opt-in trade-off profile. Borderline/inconclusive hosted results require further controlled evidence before promotion. A runtime improvement may merge with parity still open only if its scoped acceptance passes and the unresolved milestone stays explicit; do not mark the whole roadmap complete.
4. **Energy goal:** reduce same-boundary joules per completed task while satisfying the same quality/SLO. Support a lower-energy statement only when the upper bound of the appropriate energy ratio is below 1 after sampling and calibration/measurement uncertainty are considered. E2/E3 support a statement about the estimate, not a physical measurement. Systematic errors need not cancel just because two values share a profile.
5. **Power goal:** compare average power over equal offered-load/service intervals with the same successful-work/SLO requirements; identify the measured domain and idle policy. Lower joules/task alone is not proof of lower average or peak watts. Peak power requires adequate sensor resolution and its own reporting.
6. **No dedicated runner dependency:** unavailable energy or PMCs do not stop correctness/software work. They leave the energy outcome unavailable/inconclusive and energy promotion blocked. Do not lower the existing release qualification standard or introduce a synthetic power constant.

Report compilation/tuning amortization as well: `total_cost(N) = build_and_tune_cost + N * execution_cost` for both alternatives at the same boundary. Compute break-even only when the candidate has a lower per-task cost and the needed costs are available. Do not convert compilation time into compilation joules without energy evidence.

## 9. Energy interpretation and calibration policy

| Class | Quantity | Allowed interpretation |
| --- | --- | --- |
| E0 | Calibrated external whole-system energy | Whole-system joules/task within the measured study boundary |
| E1 | Hardware-reported component energy, e.g. package RAPL | Component joules/task; record domains, counter behavior and uncertainty |
| E2 | Hardware/conditions-matched calibrated counter model | Estimated joules/task, with retained measured calibration and prediction uncertainty |
| E3 | Compatible analytical operation/memory model | Analytical estimated joules/task, with declared assumptions and uncertainty |
| Unavailable | No valid counter/profile | `null` energy fields and a reason; latency and structural work metrics remain useful |

E0 is optional external validation and is **not** a new dedicated-runner deliverable. E1 may be available on an existing machine without installing a physical meter. E2 still needs valid measured calibration data somewhere; software alone cannot create that ground truth. E3 needs compatible coefficients and a defensible traffic model. Generic TDP, utilization, elapsed time or historical pJ/FLOP constants must not become measured joules.

For a bounded analytical model, account for operation types, memory-level traffic and static-time terms with explicit units. A count of FLOPs omits data movement, vectorization, caching, idle power and scheduling. Without those calibrated terms, report counts and bytes as optimization proxies. Keep the strict PR95 measurement/qualification contract and production/energy/certification claim flags unchanged unless their own requirements are met.

## 10. CI, review and closeout

Each implementation PR must include scoped unit, integration, negative/boundary, differential, sanitizer/TSan, portability, installed-consumer and performance/evidence coverage. Register new executable tests through the existing Make/CTest/CI parity mechanism. Extend workflow path filters when compiler/build changes start affecting experiments. Keep push and PR concurrency groups independent so one event does not cancel the other's required result.

CI has two decisions: deterministic correctness/contract failures block the PR; noisy hosted performance is diagnostic until a retained repeated campaign supports promotion. Optional hardware absence must be reported as unavailable, not converted into a successful mandatory execution test. Both final-head contexts `ci / ubuntu (push)` and `ci / ubuntu (pull_request)` must be successful before merge, alongside all other applicable mandatory checks. A documentation-only planning PR does not need invented runtime tests or performance results.

Reviewers should require:

- actual PR URL, base/head revisions, scope, dependencies, owner and completed commit checklist;
- generated-language and independent-control evidence, not only a faster qualification utility;
- all five regression cells, matched workload settings, output/quality results and uncertainty;
- explicit allocation/copy/tensor-work changes, normal telemetry cost, and energy availability/class;
- tested rollback and compatibility behavior, and remaining unsupported cases;
- documentation/coverage updates reflecting executed capabilities; no change to claims by assertion.

After each merge, update the tracking table with the actual PR URL and accepted evidence location, freeze a new baseline, and open only the next batch. Do not reopen PR112/113 or overwrite historical benchmark evidence. Implementation is complete only when every enabled path has met its declared exit criteria; a benchmark loss or unavailable calibration remains visible in the final report.

## 11. Technical references

These sources motivate experiments; they do not establish ShortHand speedups or supply universal energy coefficients. API options must be checked against the repository's pinned runtime/compiler versions.

1. [ONNX Runtime thread management](https://onnxruntime.ai/docs/performance/tune-performance/threading.html): threading, pool sharing and spinning trade-offs; supports the LE2 tuning design.
2. [ONNX Runtime I/O binding](https://onnxruntime.ai/docs/performance/tune-performance/iobinding.html): explicit input/output placement and preallocation; CPU benefit must be measured against PR111's existing path.
3. [MLIR bufferization](https://mlir.llvm.org/docs/Bufferization/): buffer reuse analysis and the need to manage deallocation; informs LE3 ownership requirements.
4. [Linux powercap framework](https://docs.kernel.org/power/powercap/powercap.html): energy counters, domains and maximum ranges; informs E1 counter handling.
5. [Choi et al., A Roofline Model of Energy, IPDPS 2013](https://jeewhanchoi.github.io/publication/pdf/energy_roofline.pdf): computation, communication and time-dependent energy terms; motivates calibrated operation/traffic modeling.
6. [ONNX-MLIR](https://onnx.ai/onnx-mlir/): established native model compilation and C/C++ runtime interfaces; supports AOT feasibility, not an automatic zero-allocation or performance claim.
7. [TVM MetaSchedule](https://tvm.apache.org/docs/deep_dive/tensor_ir/tutorials/meta_schedule.html): bounded search, hardware evaluation and a reusable tuning database; a methodological reference, not a required new ShortHand runtime dependency.
8. [OpenXLA GPU architecture](https://openxla.org/xla/gpu_architecture): fusion, buffer assignment, layout and library/code-generation choices. GPU memory behavior is not evidence of CPU cache residency or ShortHand savings.
9. [MLIR sparse tensor dialect](https://mlir.llvm.org/docs/Dialects/SparseTensorOps/): sparse encodings and generated iteration; performance remains format, workload and hardware dependent.
10. [ONNX Runtime quantization](https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html): hardware/operator support, quality checks, saturation and conversion overhead; reduced precision does not guarantee faster inference.
11. [BranchyNet, Teerapittayanon et al.](https://arxiv.org/abs/1709.01686): early-exit classifiers are model architecture/training work, supporting the decision to defer automatic early exits on arbitrary models.
12. [Linux CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html): shared policy domains, governors and operating-point controls; supports recording environment state before any later opt-in DVFS study.
13. [EDA, Pao et al., HPCA 2025 — authors' university record](https://scholar.nycu.edu.tw/en/publications/eda-energy-efficient-inter-layer-model-compilation-for-edge-dnn-i/): inter-layer scheduling on an NPU. Its reported device-specific results are not ShortHand CPU acceptance targets.

Repository sources: [experiment protocol](../experiments/energy/README.md), [calibration policy](../experiments/energy/calibration_profiles/README.md), [historical optimization proposal](pr110_runtime_energy_optimization_plan.md), [production truth](production_truth.md), and [AI family contract](ai_benchmark_families.md). Online references reviewed on 2026-09-29.
