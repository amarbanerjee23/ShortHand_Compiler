# PR110 — Native Runtime and Generated-Code Energy Optimization Plan

Status: implementation-ready plan only  
Target branch: `master`  
PR intent: optimize ShortHand generated/runtime execution without weakening correctness, safety, determinism, observability, evidence integrity, or the currently qualified `linux-x64-cpu-v1` scope.

## 1. Objective

PR110 will make the ShortHand native execution path materially more efficient while preserving exact workload semantics and enterprise safeguards.

The immediate performance target is to reduce the current AIRuntime overhead relative to the independent direct C++/ONNX Runtime control. The retained PR109 benchmark showed AIRuntime at roughly 1.79–1.84x the direct C++/ORT latency across the five declared FP32 runtime cells. PR110 must close that abstraction penalty before any comparative physical-energy claim is attempted.

The final PR110 implementation is successful only if:

1. all existing compiler/runtime/AI/evidence tests remain green;
2. predictions, top-k ordering and numerical tolerances remain unchanged;
3. no validation, security, quality, evidence or fail-closed contract is removed merely to improve timing;
4. the optimized path is measured against the existing independent C++/ORT baseline with identical model, precision, batch, threading and ONNX Runtime settings;
5. every optimization is evaluated during build/benchmark tasks and retained as evidence;
6. a regression in any declared correctness or supported-platform gate blocks the PR;
7. noisy hosted-runner timing is diagnostic evidence, not a deterministic correctness gate;
8. final runtime-performance acceptance is based on repeated paired observations with all five declared cells reported.

## 2. Scope

PR110 is one implementation PR composed of independently reviewable commits/stages. No stage may depend on an unsafe partial state from another stage. Each stage must compile and pass the mandatory test suite before the next stage is added.

The optimization stages are:

### A. Frozen baseline and profiler contract
- Freeze the PR109 five-cell AIRuntime vs independent C++/ORT comparison as the before baseline.
- Retain exact model, dataset, ORT version, compiler flags, precision, batch/thread matrix and numerical thresholds.
- Extend low-overhead profiling only where required to attribute:
  - preprocessing;
  - input validation;
  - tensor setup;
  - ORT `Run`;
  - output validation;
  - output copy/materialization;
  - telemetry;
  - top-k/postprocessing;
  - allocation count/bytes where practical.
- Profiling mode must remain opt-in and must not be used for primary uninstrumented latency results.

### B. Prepared-session hot-path specialization
Move invariant work out of repeated inference:
- backend selection remains in preparation;
- model/hash/shape/precision validation remains in preparation where invariant;
- cache input/output names and tensor specifications;
- cache CPU memory info and immutable execution metadata;
- create a versioned prepared execution plan;
- preserve per-call checks that depend on caller data.

No dynamic backend rediscovery, configuration parsing or model metadata discovery may remain in the steady-state hot path unless required for correctness.

### C. Per-worker reusable workspaces
Introduce bounded reusable workspaces owned by a prepared session or serving worker:
- preallocate normalized input storage;
- preallocate postprocessing scratch;
- reserve prediction/top-k output capacity;
- avoid repeated vector growth;
- prevent shared mutable buffers across concurrent workers;
- retain safe tail-batch zero padding;
- preserve memory bounds and cancellation semantics.

Acceptance:
- concurrency/TSan tests prove no data races;
- failure paths cannot leak stale data from an earlier request;
- repeated full/partial/tail batches remain numerically identical.

### D. Eliminate avoidable output copies
Where ownership/lifetime permits:
- consume ORT output through a bounded read-only view/span for validation and postprocessing;
- materialize owned output only at an API boundary that requires ownership;
- do not expose ORT-owned memory beyond its valid lifetime;
- preserve exact external API behavior.

Add negative lifetime tests and sanitizer coverage.

### E. Fuse validation and preprocessing
For the declared static FP32 CPU path:
- combine finite/range validation and normalization into one pass;
- write directly into the reusable input workspace;
- preserve exact error reasons and fail-closed behavior;
- keep generic fallback code for unsupported/dynamic configurations.

Compiler/runtime implementation should remain vectorization-friendly and must not enable unsafe fast-math.

### F. Specialized postprocessing
Replace generic postprocessing only when compile/preparation-time metadata proves specialization safe:
- specialize fixed class-count/top-k paths;
- use deterministic tie ordering identical to the current contract;
- preserve generic fallback for arbitrary supported shapes;
- compare scores/predictions/top-k bit-for-bit or within the existing score tolerance as applicable.

The independent C++ baseline must receive equivalent algorithmic tuning where required for a fair native comparison.

### G. Decouple telemetry serialization from inference
Keep enterprise observability while removing avoidable hot-path serialization:
- hot path records bounded numeric/status fields only;
- JSON/OTLP/Prometheus formatting occurs after the timed inference boundary or through existing aggregation/export paths;
- qualification mode may retain detailed timing/evidence;
- production and qualification modes must have identical model results and safety checks.

No evidence field may be fabricated or silently dropped.

### H. Static generated execution plan
Add a deterministic, versioned execution-plan object generated during preparation/compilation containing only validated immutable choices such as:
- model identity;
- backend/provider;
- precision;
- static/concrete shapes;
- batch;
- thread count;
- preprocessing contract;
- output contract;
- postprocessing strategy;
- telemetry mode.

Hash/bind the plan where the existing qualification/evidence contract requires identity. Stale or incompatible plans fail closed.

### I. ONNX Runtime configuration evaluation
Evaluate, rather than assume, the best equivalent ORT configuration:
- existing BASIC graph optimization;
- EXTENDED/ALL when valid for the pinned ORT version;
- offline optimized model where reproducible and hash-bound;
- thread count;
- batch size;
- spinning policy;
- I/O binding/preallocated output where it materially helps CPU execution.

Any configuration selected for ShortHand must be available to the C++ baseline under the same comparison track. Record every tested configuration and do not select based on one favorable observation.

### J. LTO/PGO and target specialization experiment
Add explicit build profiles:
- portable release profile;
- optional hardware-specialized experimental profile.

Evaluate:
- `-O3` where compatible with the existing correctness contract;
- LTO;
- profile-guided optimization;
- declared target features.

Do not make `-march=native` part of a portable release artifact. Target-specific artifacts must record the target and remain separately qualified.

The C++ baseline receives equivalent compiler optimization opportunities.

### K. AOT model compilation prototype
Add a bounded experimental AOT path for the existing pinned FP32 classifier:
- initially evaluate ONNX-MLIR or another explicitly pinned model compiler;
- compile model operators into a native callable artifact;
- use a versioned tensor ABI;
- no silent fallback from the declared AOT path to ORT;
- unsupported operators/shapes fail explicitly;
- preserve the same model/data/quality contract.

Required comparison:
1. ShortHand + ORT;
2. direct C++ + ORT;
3. ShortHand + AOT model;
4. direct C++ calling the same AOT model.

This separates language/runtime gains from backend/compiler gains.

### L. Tensor-level optimization prototype
On the bounded AOT path, evaluate:
- shape/type propagation;
- constant folding;
- safe operator fusion;
- buffer lifetime analysis;
- static/reusable allocation;
- dead temporary elimination;
- vectorization/tiling where supported.

Each optimization must have an ablation toggle so its contribution can be measured independently.

No precision change, quantization or fast-math is permitted in the primary FP32 parity track.

### M. Precision/energy-search preparation
Do not broaden the current FP32 production qualification in this PR unless all evidence exists. Instead prepare an experimental candidate-search interface capable of later comparing:
- FP32;
- BF16/FP16 where hardware/backend supports them;
- INT8 where an explicit quality guardrail is supplied.

The current FP32/no-quantization qualification remains authoritative unless a separately qualified path is completed.

## 3. Correctness and robustness invariants

Every implementation commit must preserve:

- existing language syntax/semantics unless explicitly versioned;
- parser/AST/SemanticIR/MLIR/native equivalence;
- exact supported ABI contracts;
- model hash and dataset identity checks;
- input finite/range validation;
- output shape/type/finite validation;
- deterministic top-k tie behavior;
- declared numerical tolerances;
- minimum accuracy guardrail;
- batch/tail-padding semantics;
- bounded memory behavior;
- concurrency isolation;
- fail-closed unavailable backend behavior;
- evidence/claim safety;
- zero mandatory-test skips;
- sanitizer cleanliness.

No optimization is accepted if it obtains speed by:
- removing checks only from ShortHand while keeping them in the baseline;
- using a different model/precision;
- changing accuracy/quality;
- changing functional units;
- excluding previously included application work;
- selectively dropping slow/failed trials;
- using a weaker baseline.

## 4. Build-task evaluation design

Build/CI tasks must evaluate whether the optimizations work while keeping performance separate from deterministic correctness.

### 4.1 Mandatory correctness gates
Every PR110 push must run the existing mandatory suite, including where configured:
- strict language validation;
- Make/CMake/CTest parity;
- LLVM/MLIR lowering and installed-consumer tests;
- real ONNX CPU qualification;
- AI application tests;
- benchmark contract tests;
- ASan/LSan/UBSan;
- TSan concurrency coverage;
- fuzz/security/reproducibility gates;
- production-truth/claims-safety gates.

A failure blocks PR110.

### 4.2 Optimization diagnostic build task
Add a dedicated PR110 diagnostic job that:
1. builds an uninstrumented release binary;
2. runs the fixed five-cell AIRuntime/C++-ORT comparison;
3. runs the attribution profiler separately;
4. records raw observations and environment metadata;
5. compares the current head against the frozen PR109 baseline;
6. emits per-cell ratios and deltas;
7. uploads all observations whether improved, neutral or regressed.

Hosted CI timing remains diagnostic because runner noise can create false failures.

### 4.3 Per-stage evaluation
Each optimization stage must record:
- before/after latency;
- completed functional units;
- prediction/score parity;
- allocation/memory change when available;
- profiling attribution;
- build flags/configuration;
- whether the result improved, regressed or was inconclusive.

An optimization that consistently regresses the intended metric without compensating measured benefit must be removed or disabled before merge.

### 4.4 Final performance acceptance
Before PR110 is considered implementation-complete, run the retained repeated comparison design across all five current cells.

Primary target:
- ShortHand AIRuntime mean steady-state application latency <= 1.03x the best equivalent direct C++/ORT implementation in every declared cell.

Secondary guardrail:
- no cell may exceed 1.05x after accounting for repeated-run variability unless the PR explicitly remains draft and documents the blocker.

This is a prospective engineering acceptance target, not a current result and not an energy claim.

## 5. Physical-energy readiness

PR110 should make the runtime ready for the physical campaign but must not invent a power claim.

After the optimized software is frozen:
- retain compiler/runtime/model/dataset hashes;
- use a dedicated isolated Linux host;
- use an independently logging calibrated whole-host AC meter;
- keep RAPL as secondary CPU-package diagnostics;
- use randomized AB/BA paired execution;
- collect at least 30 pairs per declared configuration/session where practical;
- repeat at least three independent sessions;
- retain raw power traces, failed trials and meter uncertainty;
- report watts, elapsed time, total joules, joules/completed-correct-task, throughput, p95 latency and quality;
- never derive joules from elapsed time or TDP.

Physical superiority is outside PR110's merge criterion unless real calibrated evidence is available. The software optimization PR must remain valid even when physical measurement is pending.

## 6. Fair baseline policy

PR110 comparisons must include the independent C++17/ONNX control and keep it genuinely optimized.

For any optimization available equally to C++/ORT, apply it to both sides before claiming a ShortHand advantage.

Separate tracks:
1. same-backend abstraction-overhead track;
2. best-tuned deployment track;
3. AOT/backend experiment track;
4. later whole-application physical-energy track.

Do not average incompatible precision/backend tracks.

## 7. Rollback and feature containment

Every new optimization must be:
- covered by tests;
- independently disableable during development where practical;
- fail-closed on unsupported configurations;
- backed by the existing generic/prepared execution path until the optimized path is proven.

If an optimization causes:
- sanitizer failure;
- TSan race;
- numerical regression;
- quality regression;
- unsupported-platform breakage;
- evidence inconsistency;
- persistent performance regression;

it is removed or disabled before merge.

## 8. Expected code areas

Likely implementation surfaces:
- `Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.*`
- `Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxPreparedSession.h`
- `Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp`
- `Compiler_new_ws/Short_Hand/src/ai_runtime/ApplicationQualification.*`
- execution-plan/runtime ABI code as needed;
- MLIR/tensor lowering for the bounded AOT prototype;
- `experiments/energy/*` comparison/profiling harnesses;
- tests under `tests/ai_application`, `tests/ai_energy`, `tests/ai_benchmark`;
- CMake/Make/CI only where required to build/test the new paths.

Avoid unrelated language/release-governance changes.

## 9. Commit sequence inside PR110

Recommended atomic sequence:

1. freeze PR109 baseline + PR110 acceptance manifest;
2. add/validate low-overhead attribution metrics;
3. prepared invariant hoisting;
4. per-worker reusable workspaces;
5. output-view/copy reduction;
6. fused validation + preprocessing;
7. deterministic postprocessing specialization;
8. telemetry hot-path decoupling;
9. static execution plan;
10. ORT optimization/configuration evaluation;
11. LTO/PGO build experiment;
12. bounded AOT model path;
13. tensor optimization/ablation framework;
14. final five-cell uninstrumented comparison;
15. claims/readiness documentation update based only on executed evidence.

Every commit is expected to be buildable and testable.

## 10. PR110 merge checklist

PR110 remains draft until implementation begins. It may be marked ready only when:

- [ ] no mandatory CI failures;
- [ ] no sanitizer/TSan/fuzz regression;
- [ ] exact application correctness/quality preserved;
- [ ] independent C++ baseline remains fair and executable;
- [ ] all five runtime cells have retained before/after observations;
- [ ] every enabled optimization has an attributable or justified benefit;
- [ ] no optimization depends on a synthetic energy estimate;
- [ ] AIRuntime parity target is met or remaining gap is explicitly retained as a blocker;
- [ ] production/energy claims remain false unless supported by separate qualified evidence;
- [ ] documentation matches the exact tested revision.

## 11. Non-goals

PR110 will not:
- claim lowest-power or lowest-carbon language status;
- infer energy savings from latency;
- broaden production qualification to GPU/TPU/NPU without real execution evidence;
- remove enterprise safety/evidence checks to win a benchmark;
- replace the physical-energy campaign with hosted CI timing;
- make a universal claim from the Optdigits workload.

## 12. Follow-on after PR110

Once the optimized implementation is frozen and native parity/improvement is demonstrated, run the calibrated physical-energy campaign using the repository's existing measurement framework. Only that retained physical evidence can support workload/hardware-specific joules-per-task claims.
