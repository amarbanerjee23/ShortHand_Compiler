# PR110 — Native Runtime and Generated-Code Energy Optimization Plan

Status: historical proposal; remaining work superseded by the post-PR111 plan

Target branch: `master`  
PR intent: optimize ShortHand generated/runtime execution without weakening correctness, safety, determinism, observability, evidence integrity, or the currently qualified `linux-x64-cpu-v1` scope.

As of 2026-09-29, PR110 is merged and PR111 has delivered a consolidated subset
of the runtime and energy-evidence work, incorporating the retained PR112/113
changes. The future-tense stages and checklist below are preserved as the
original proposal, not a statement that all stages were implemented. Follow the
[current latency and energy PR plan](latency_energy_optimization_pr_plan.md) for
verified completion boundaries, the generated-program execution gap, and the
remaining four implementation batches. The PR109 timing ratio below is historical.

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
4. probes RAPL/NVML and required PMCs without failing correctness CI when unavailable;
5. produces the highest-qualified E1/E2/E3 energy evidence;
6. records raw observations, calibration identity and environment metadata;
7. compares the current head against the frozen PR109 baseline;
8. emits per-cell latency and energy ratios/deltas with uncertainty;
9. uploads all observations whether improved, neutral, regressed or unsupported.

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

## 5. Continuous calibrated energy-evidence pipeline

PR110 must add energy-related evidence to every relevant optimization run without misrepresenting modeled energy as physically measured whole-system energy.

### 5.1 Evidence classes

Every energy result must carry exactly one highest-supported evidence class:

- **E0 — physical whole-system measurement**: externally calibrated AC measurement around the complete host. This is the gold-standard system-energy class but is not required for ordinary PR110 CI.
- **E1 — hardware component measurement**: hardware-reported accumulated energy such as Linux RAPL `energy_uj` for CPU package/domain energy or NVML total-energy counters for supported GPUs.
- **E2 — calibrated counter model**: joules estimated from runtime performance counters using an architecture-specific, versioned calibration model with retained uncertainty and calibration provenance.
- **E3 — analytical Energy-Roofline estimate**: joules estimated from compiler/runtime operation counts plus memory-traffic estimates using versioned architecture coefficients.

The pipeline must never relabel E1/E2/E3 as E0. Reports must use distinct fields such as:
- `physical_system_joules`;
- `hardware_measured_joules`;
- `calibrated_joules_estimate`;
- `analytical_joules_estimate`.

### 5.2 Preferred evidence selection

For each benchmark cell, probe in this order:

1. use E1 when a supported hardware energy counter is readable and stable;
2. otherwise use E2 when all required PMCs are available and the runner CPU matches a validated calibration profile;
3. otherwise use E3 when a compatible analytical profile exists;
4. otherwise emit `energy_status=unsupported_hardware_model` and retain latency/correctness evidence only.

Missing counters, permission failures, unsupported CPUs, or incomplete coefficient sets must downgrade the evidence class instead of producing a guessed number.

### 5.3 Calibrated PMC model

The initial CPU model should support a form such as:

```text
E_est =
  beta_time * elapsed_time
+ beta_instr * instructions_retired
+ beta_cycles * cycles
+ beta_branch * branch_events
+ beta_llc * llc_misses
+ beta_mem * estimated_or_measured_memory_bytes
+ intercept
```

The exact feature set is calibration-profile specific. Coefficients must never be silently reused across incompatible CPU families or frequency regimes.

Each calibration profile must record:
- model version and SHA-256;
- CPU vendor/family/model/stepping or an explicitly broader validated class;
- operating-frequency/DVFS validity range;
- required PMCs;
- coefficient units;
- calibration dataset/source;
- fit/validation error;
- confidence/uncertainty bound;
- publication or retained calibration provenance;
- date and tool version.

A CI result is E2 only if its hardware identity and required counters satisfy the profile contract.

### 5.4 Compiler Energy-Roofline evidence

Independently of PMCs, the compiler/runtime should emit energy-relevant structural counters where derivable:

- FP/MAC operation counts;
- integer/vector operation counts where available;
- tensor bytes read/written;
- temporary tensor count and lifetime;
- estimated L1/L2/LLC/DRAM traffic when a validated model exists;
- allocation count/bytes;
- copies eliminated;
- operational intensity;
- static execution-plan identity.

The analytical model should follow the energy-roofline structure:

```text
E_analytical =
  sum(operation_count_i * energy_per_operation_i)
+ sum(bytes_at_memory_level_j * energy_per_byte_j)
+ static_power_term * elapsed_time
```

Published coefficients are priors/reference points, not universal constants. A historical process-node value such as a pJ/FLOP number must not be applied to unrelated modern CPUs without an explicit compatibility/calibration justification.

### 5.5 Relative energy efficiency

The primary continuous-CI energy statistic is a same-runner paired ratio:

```text
R_energy = ShortHand_joules_per_correct_task / baseline_joules_per_correct_task
```

and:

```text
energy_delta_percent = 100 * (1 - R_energy)
```

Report absolute modeled/measured joules with evidence class and uncertainty, but emphasize same-host paired ratios because common calibration/systematic error can cancel partially.

Comparisons must use:
- same runner;
- same model and dataset;
- same precision;
- same functional-unit definition;
- same quality threshold;
- same batch/thread/SLO cell;
- same measurement/model version.

### 5.6 CI output schema

Every relevant PR/push experiment should retain a machine-readable record containing at least:

```json
{
  "evidence_class": "E2",
  "method": "calibrated_pmc_v1",
  "physical_system_energy_measured": false,
  "component_energy_measured": false,
  "calibrated_joules_estimate": 0.0,
  "joules_per_completed_correct_task": 0.0,
  "model_uncertainty_percent": 0.0,
  "short_hand_to_cpp_energy_ratio": 0.0,
  "short_hand_to_python_energy_ratio": 0.0,
  "hardware_profile": "",
  "calibration_profile_sha256": "",
  "completed_correct_tasks": 0
}
```

Unavailable fields must be `null`, not zero or synthesized.

### 5.7 Continuous workflow policy

PR110 implementation must make the experiment pipeline run continuously:

**Relevant branch push (`agent/**`) and pull-request update**
- core correctness CI;
- tooling;
- fast five-cell uninstrumented latency comparison;
- runtime attribution/profile pass;
- hardware-energy-counter probe;
- PMC collection when permitted;
- E2/E3 energy estimate when qualified;
- ShortHand vs C++ and Python ratios;
- raw JSON/CSV + Markdown artifact upload.

**Push/merge to `master`**
- all of the above;
- exhaustive PR109-style paired software benchmark;
- retained historical comparison against the previous accepted master result;
- confidence intervals;
- durable energy-evidence summary.

The workflow definitions should be updated so runtime/energy experiments are not limited to pull-request path events only. Expensive duplicate runs should use GitHub concurrency cancellation, but the latest commit for every relevant PR must retain a complete evidence artifact.

### 5.8 Optimization effectiveness report

Every optimization stage should report, where the platform supports it:

- latency/task;
- throughput;
- cycles/task;
- instructions/task;
- LLC/cache events;
- allocation bytes/task;
- tensor/memory bytes/task;
- E1 hardware-measured J/task if available;
- E2 calibrated estimated J/task if qualified;
- E3 analytical estimated J/task if qualified;
- energy ratio to C++/ORT;
- energy ratio to Python/ORT;
- evidence class;
- uncertainty;
- correctness/quality result.

This lets PR110 demonstrate whether an optimization reduces host work and modeled/measured energy rather than relying on latency alone.

### 5.9 Scientific references and interpretation

The implementation should cite and align its methodology with established sources, including:

- Choi et al., **A Roofline Model of Energy**, IEEE IPDPS 2013 — operations, communication/memory traffic, concurrency, time and machine energy characteristics as the basis of an energy roofline;
- Horowitz, **Computing's Energy Problem (and what we can do about it)**, ISSCC 2014 — illustrative evidence that data movement can cost substantially more energy than arithmetic; values are historical reference points, not universal modern-CPU coefficients;
- Linux kernel **powercap/RAPL** interface documentation — accumulated hardware energy exposed through `energy_uj` where supported;
- NVIDIA **NVML** device-query API — accumulated total GPU energy on supported devices;
- peer-reviewed/validated PMC-based CPU energy-model literature used by the selected calibration profile.

The plan must preserve source/version metadata for any coefficients used so experimental results remain reproducible.

### 5.10 Physical-energy interpretation

PR110 does not require a dedicated physical meter runner. Continuous CI may therefore produce E1, E2 or E3 evidence depending on runner capabilities.

Only E0 is described as whole-system physical energy. E1 is component-level hardware measurement. E2/E3 are estimates.

The absence of E0 must not block software optimization work, but it continues to block unconditional claims such as:
- "lowest-power AI language";
- "X% lower whole-system energy" without a matching E0 study;
- universal hardware-independent energy superiority.

Workload/hardware-specific E1 results or explicitly labeled E2/E3 results may be reported according to their evidence class and uncertainty.

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
14. calibrated energy-evidence pipeline (E1/E2/E3) + architecture profiles;
15. continuous PR/push experiment workflows and retained artifacts;
16. final five-cell uninstrumented latency/energy comparison;
17. claims/readiness documentation update based only on executed evidence.

Every commit is expected to be buildable and testable.

## 10. PR110 merge checklist

PR110 remains draft until implementation begins. It may be marked ready only when:

- [ ] no mandatory CI failures;
- [ ] no sanitizer/TSan/fuzz regression;
- [ ] exact application correctness/quality preserved;
- [ ] independent C++ baseline remains fair and executable;
- [ ] all five runtime cells have retained before/after observations;
- [ ] every enabled optimization has an attributable or justified benefit;
- [ ] energy results carry explicit E0/E1/E2/E3 evidence class and uncertainty;
- [ ] unsupported hardware/counters downgrade evidence instead of fabricating joules;
- [ ] relevant PR/push CI retains latency plus highest-qualified energy evidence artifacts;
- [ ] no physical-energy claim depends on a modeled E2/E3 value;
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

After PR110, every relevant optimization PR should inherit the continuous latency + energy-evidence pipeline.

Where CI exposes RAPL/NVML, retain E1 hardware component joules/task. Where it does not, retain only qualified E2/E3 estimates with calibration/model uncertainty and provenance.

A later externally metered E0 study remains optional validation for whole-system energy claims, not a prerequisite for continuously tracking compiler/runtime energy efficiency.
