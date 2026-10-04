# Energy-Contracted Compilation

Status: implementation roadmap with PR119 foundation in progress  
Claim status: candidate engineering strategy only; no comparative-energy, lower-power, lowest-carbon or certification claim

## Objective

ShortHand should not treat energy as a number reported after execution. The compiler/runtime should treat energy as a constrained deployment objective:

> Generate only semantically valid execution/training candidates, reject candidates that fail quality/SLO/memory constraints, and promote an ECO plan only when the available energy-evidence class supports lower joules per completed functional unit than the accepted baseline.

Latency, average power and energy are separate quantities. A faster candidate is not automatically an energy winner. When energy is unavailable, ECO selection is unavailable rather than inferred from time, utilization, FLOPs or TDP.

## Existing foundation

The repository already provides the necessary control plane:

- typed model/tensor/quality declarations and verified SemanticIR/MLIR lowering;
- prepared ONNX CPU execution and a compiled C bridge;
- E0/E1/E2/E3 evidence separation, replay, hashes and fail-closed claim controls;
- equivalent native/Python/C++ comparison machinery;
- deterministic CPU training qualification with per-step/epoch/run energy boundaries;
- quality and production-scope guards.

PR118 added exclusive phase attribution and showed that the bounded compiled workload is dominated by host work rather than ORT execution. PR119 therefore starts with an execution-capability foundation instead of prematurely changing precision or model semantics.

## 1. Execution capability

A prepared execution capability is a process-owned object whose validity is bound to:

- model identity and content-validation policy;
- model/input/output declaration generation;
- hardware-generation token;
- routing-policy generation;
- backend/provider/runtime version;
- shape, dtype and output-capacity contract;
- preparation/tuning options;
- qualified target ISA and compatibility checks.

The steady-state path should approach:

```text
validated request
  -> capability lookup
  -> prepared/AOT execution
  -> output validation
  -> transactional commit
```

instead of re-running discovery, route construction and allocation work that is unchanged for the capability lifetime.

### PR119 foundation

PR119 implements two bounded parts without changing model replacement semantics:

1. Hardware inventory and route reuse behind a cheap hardware-generation key plus an independent environment-policy signature. Policy changes invalidate routing immediately; a changed hardware-generation token forces a new full probe.
2. Generic prepared output binding. The compiled bridge supplies runtime-owned scratch to `inferCachedInto`; the prepared ONNX path can write directly into that scratch while retaining public finite/shape/dtype checks. Caller memory is committed only after success.

The complete ONNX model is still reread and byte-compared for every cached request in PR119. Immutable registered-model semantics are a separate change and must have explicit lifecycle/versioning tests before removing that check.

## 2. Energy execution-plan IR

The next compiler slice should introduce a versioned internal artifact, initially named:

`shorthand.energy.execution_plan.v1`

It is not new source syntax. A plan records enough identity to reproduce and invalidate a deployment decision:

```text
model / graph hash
compiler + backend versions
hardware fingerprint + ISA
shape/layout/dtype
batch + threading
fusion and schedule decisions
arena/workspace bytes
packed-weight identity
quality contract and data split
latency / throughput / tails
energy evidence class/domain/profile
energy estimate or measurement + uncertainty
compile/tune cost
rejection reason
fallback plan
raw evidence hashes
```

The compiler produces a bounded candidate set; existing evidence runners evaluate it; a selector retains the Pareto frontier.

### Selection rule

For candidate `c` and accepted baseline `b`:

```text
eligible(c) =
    semantic_equivalence_or_declared_numerical_contract(c)
    && quality(c) >= quality_floor
    && latency_tail(c) <= SLO
    && memory(c) <= memory_budget
    && compatibility(c)
```

An ECO promotion additionally requires supported energy evidence. For physical/component measurement, the acceptance campaign should preserve the existing conservative rule: after sampling and measurement uncertainty, the upper confidence bound of `J(c)/J(b)` must be below 1. E2/E3 may drive an explicitly labelled estimated-energy decision only when their calibration/model validity requirements pass.

If no candidate passes, the baseline remains selected. A search is allowed to return no improvement.

## 3. AOT/static-memory candidate family

The first compiler-generated family should remain bounded to the existing qualified CPU scope:

- fixed FP32 MatMul + bias/compatible pointwise work;
- concrete batch shapes already used by the regression matrix;
- immutable packed constants;
- liveness-derived per-worker static arena;
- no heap allocation inside the accepted steady-state AOT compute region;
- cost-bounded fusion and vector/tile schedules;
- direct C++ caller of the identical AOT artifact;
- ORT FP32 retained as the generic fallback and control.

Required attribution cells remain:

1. ShortHand + ORT,
2. C++ + the same ORT configuration,
3. ShortHand + AOT,
4. C++ + the identical AOT artifact.

This prevents a backend/kernel improvement from being mislabeled as a language-only gain.

## 4. Energy-contracted training

The current `TrainingQualification.cpp` workload is a deterministic 354-parameter reference CNN, not a general autodiff/training compiler. Its present batch loop allocates per-sample gradient containers and creates/joins worker threads for every batch. The next training implementation should first remove that structural overhead before adding more model classes.

### TrainingExecutor

Introduce a reusable executor with:

```text
persistent bounded worker pool
per-worker activation arena
per-worker gradient arena
ordered reduction workspace
optimizer state/workspace
quality/evidence recorder
```

Workers are created once per training plan, not per optimizer step. Buffers are sized from checked liveness/shape facts and reused.

### TrainingPlan

A versioned training plan should bind:

- forward graph and trainable/frozen parameter sets;
- backward graph and saved-activation requirements;
- optimizer and accumulator dtype;
- batch size, worker count and deterministic reduction policy;
- activation/gradient workspace;
- mixed-precision policy when qualified;
- quality target, patience/max-work budget and validation split;
- energy/latency evidence and fallback.

The eventual compiler pipeline can reason across the complete step:

```text
forward
 -> saved-activation lifetime analysis
 -> backward
 -> gradient reduction
 -> optimizer update
```

This enables activation reuse, gradient-buffer reuse, dead-gradient elimination, optimizer fusion and bounded mixed precision while preserving deterministic/quality contracts.

## 5. Train-to-quality functional unit

Training efficiency should not be judged only by joules/epoch. The primary comparable functional unit should be declared useful work, for example:

```text
successful training run reaching validation_accuracy >= 0.95
```

Report at least:

- joules/successful training run when available;
- time to quality;
- samples and optimizer steps to quality;
- final quality/generalization metric;
- peak/reused workspace;
- failed or exhausted runs.

Early stopping is valid only when its quality rule and validation split are predeclared. A candidate cannot save energy by silently lowering the target or leaking held-out data into tuning.

## 6. Precision, sparsity and trainability

Only after the FP32/static-memory baseline is stable:

- evaluate FP32/INT8 and qualified BF16/FP16 operator groups;
- keep sensitive reductions/normalization in validated accumulator precision;
- include quantize/dequantize/packing cost;
- evaluate exact-zero structural sparsity only where a real efficient kernel exists;
- represent frozen/trainable/adapter regions so backward work is generated only where mathematically required.

Pruning, INT4, arbitrary early exits, request-time recompilation and accelerator frequency control remain out of the initial production scope.

## 7. Deployment policies

FAST/ECO/BALANCED are plan-selection labels, not unconditional compiler flags:

- **FAST**: minimum supported latency subject to quality/SLO/memory constraints.
- **ECO**: minimum supported measured or explicitly qualified estimated joules subject to the same constraints.
- **BALANCED**: a predeclared Pareto constraint/tie rule with compatible units, not an arbitrary weighted sum.

Every selected plan stores its parent baseline, evidence class, uncertainty, candidate set and fallback. Any model, hardware, backend, shape-distribution or SLO change invalidates the decision and requires revalidation.

## 8. CI and claim contract

Every implementation PR must:

- update `docs/latency_energy_test_and_benchmark_results.md`;
- name the exact comparison base and tested revision;
- retain correctness, output oracle, raw timings/counters and evidence hashes;
- run sanitizer/TSan/lifecycle/ABI tests applicable to the changed path;
- compare against an unchanged optimized control with equal tuning opportunity;
- preserve failed/inconclusive candidates;
- report energy as unavailable when no valid E1/E2/E3 source exists.

Deterministic correctness regressions block merge. Hosted timing remains diagnostic until the declared repeated campaign supports promotion. Energy promotion remains blocked without supported energy evidence.

## 9. Implementation sequence

| Slice | Scope | Status |
| --- | --- | --- |
| EC1 | Route/inventory generation cache + transactional prepared output | PR119 in progress |
| EC2 | Internal Energy Execution Plan v1 + bounded candidate database/Pareto selector | planned |
| EC3 | Fixed-shape FP32 AOT, static arena, fusion/schedule candidates and four-cell attribution | planned |
| EC4 | Persistent TrainingExecutor + static activation/gradient workspaces + train-to-quality evidence | planned |
| EC5 | Qualified mixed precision/trainability policies and broader real workloads | planned |

The sequence is evidence-directed. A candidate that fails quality, SLO, memory, portability or energy acceptance remains disabled; roadmap completion is not a reason to promote it.
