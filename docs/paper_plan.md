# Top-Conference Research Plan: Energy-Aware Compiled DSL for AI Training and Inference

## 1. Central Claim

**Central claim:** ShortHand can become an **energy-aware compiled DSL for AI training and inference** that lets programmers state tensor, precision, placement, ownership, and energy intent at the language level, while the compiler lowers those guarantees into deterministic native execution plans with measurable energy, memory, and latency benefits over conventional framework-centric implementations.

The paper should argue that AI systems need compiler-visible energy and deployment contracts, not only runtime heuristics. ShortHand's contribution is to make those contracts explicit in the source language, prove or check them statically where possible, and exploit them in IR generation and runtime scheduling.

## 2. Language Guarantees

ShortHand should evolve around a small set of enforceable guarantees that are meaningful to compiler implementation and evaluation.

### Static Tensor Shapes

* Tensor ranks and dimensions are declared or inferred before code generation.
* Shape constraints are checked at compile time for tensor algebra, model layers, batching, and dataset interfaces.
* Dynamic dimensions are permitted only when bounded by explicit symbolic limits, such as `batch <= 128`.
* Guarantee: generated code never performs shape discovery on the critical training or inference path unless the source explicitly marks a dimension as dynamic.

### Memory Ownership and Lifetime

* Tensor buffers, model parameters, activation tensors, and dataset batches have explicit ownership categories: owned, borrowed, view, parameter, gradient, and temporary.
* Aliasing rules prevent destructive updates when a value is still live in another view.
* Lifetimes are known to the compiler for temporaries and intermediate tensors.
* Guarantee: the compiler can statically allocate or reuse buffers for non-escaping tensor values and reject unsafe aliasing patterns.

### Precision Policies

* Precision is a source-level policy rather than an incidental backend choice.
* Policies include `fp32`, `fp16`, `bf16`, `int8`, mixed precision, and accuracy-guarded quantization.
* Users can attach tolerance contracts to model outputs or intermediate tensors, such as maximum absolute error, top-k stability, or loss drift bounds.
* Guarantee: precision lowering is explicit, auditable, and either statically validated against policy constraints or checked by generated benchmark harnesses.

### Device Placement

* Programs may declare device intent for operators, tensors, model stages, or training phases: CPU, GPU, accelerator, or portable.
* Placement constraints are part of type checking and IR metadata.
* Guarantee: the compiler emits deterministic placement decisions and flags unintended host-device movement.

### Energy Budgets

* Programs may declare energy budgets at function, epoch, inference batch, or deployment target granularity.
* Budgets can be hard constraints, optimization goals, or profiling assertions.
* Guarantee: generated plans expose predicted and measured energy cost hooks and can fail compilation or benchmarking when a declared budget is violated.

### Deterministic Native Deployment

* The compiler produces native binaries or native runtime plans whose dependencies, model artifacts, tensor layouts, and precision decisions are reproducible.
* Nondeterministic behavior, such as thread scheduling, random seeds, and reduction ordering, is controlled by explicit language options.
* Guarantee: deployment artifacts are repeatable across supported machines when given the same source, compiler version, model artifact, and runtime configuration.

## 3. Compiler Optimizations

The research claim should be tied to concrete compiler passes that become possible because the language makes AI intent explicit.

### Operator Fusion

Fuse adjacent tensor operations, activation functions, normalization steps, and loss computations when shape, precision, and placement metadata show that fusion is safe. The expected benefits are fewer kernel launches, fewer intermediate writes, and lower memory traffic.

### Static Allocation and Buffer Reuse

Use ownership and lifetime information to allocate temporary tensors once per execution region, reuse activation buffers, and avoid heap allocation inside hot loops. This should be evaluated through peak memory, allocation count, and energy per batch.

### Quantization Hints and Precision Lowering

Lower operations according to user precision policies and compiler-derived quantization opportunities. Hints should flow through the AST and IR so that backend code can select integer kernels, mixed-precision kernels, or fallback full-precision kernels.

### Batch Scheduling

Exploit static or bounded batch dimensions to choose batch sizes, split oversized batches, pipeline dataset loading, and minimize energy under throughput constraints. Scheduling should account for target hardware and energy budgets.

### Thread Tuning

Generate runtime configuration for CPU thread pools, OpenMP settings, or backend-specific execution knobs. Thread counts should be selected from source constraints, benchmarking feedback, and determinism requirements.

### Dead Data Movement Elimination

Analyze placement metadata and tensor liveness to remove redundant CPU-GPU transfers, avoid materializing host copies for device-only tensors, and collapse layout conversions when consumers accept the producer layout.

## 4. Contribution-to-Implementation Map

| Research contribution | Parser tasks | AST tasks | IR generation tasks | Runtime tasks | Benchmark tasks |
| --- | --- | --- | --- | --- | --- |
| Static tensor contract system | Add grammar for tensor types, symbolic dimensions, and bounded dynamic dimensions. | Add tensor type nodes, shape expressions, and source spans for diagnostics. | Emit shape metadata on tensor operations and reject incompatible operations before lowering. | Expose runtime assertions only for explicitly dynamic dimensions. | Add shape-heavy model kernels, invalid-program tests, and compile-time diagnostic tests. |
| Ownership-aware memory planning | Parse ownership annotations for tensors, views, parameters, gradients, and temporaries. | Represent ownership, borrow edges, mutation permissions, and lifetime scopes. | Build liveness intervals, alias sets, and buffer reuse plans. | Add arena allocation, reusable tensor pools, and deterministic deallocation hooks. | Measure peak memory, allocation count, cache behavior, and energy per batch. |
| Precision policy language | Parse precision declarations, mixed-precision regions, and tolerance contracts. | Attach precision policies to expressions, functions, layers, and model blocks. | Propagate precision metadata, insert casts, and select quantized or mixed kernels. | Implement precision-specific kernels or backend dispatch paths. | Compare accuracy, latency, energy, and model-quality drift against FP32 baselines. |
| Device placement guarantees | Parse placement annotations for tensors, functions, model stages, and deployment targets. | Store placement constraints and portability fallbacks. | Generate placement-aware IR and diagnose illegal movement or unsupported devices. | Manage device buffers, transfer scheduling, and backend initialization. | Measure transfer count, PCIe or host-device bytes, latency, throughput, and energy. |
| Energy-budgeted compilation | Parse energy budgets and optimization objectives. | Represent budgets as program contracts with scope and severity. | Annotate IR regions with budget metadata and estimated cost models. | Add measurement hooks for RAPL, NVML, power meters, or backend counters. | Report joules per inference, joules per epoch, energy-delay product, and budget violations. |
| Deterministic native deployment | Parse reproducibility modes, seed declarations, and deterministic reduction settings. | Track deterministic regions and nondeterminism permissions. | Emit stable reduction strategies, fixed scheduling metadata, and reproducible artifact manifests. | Control seeds, thread counts, kernel choices, and artifact loading. | Run repeated-trial variance tests across builds and machines. |
| Optimization pass suite | Parse optional optimization directives and debugging flags. | Attach optimization eligibility and user override metadata. | Implement fusion, static allocation, quantization lowering, batch scheduling, thread tuning, and dead movement elimination passes. | Provide tuned execution backends and profiling callbacks. | Use ablation studies to isolate each optimization's impact. |

## 5. Target Venues and Evaluation Standards

### Primary Compiler and Systems Venues

* **PLDI:** Strong fit if the paper emphasizes language design, static guarantees, IR design, formalized safety properties, and optimization correctness.
* **CGO:** Strong fit if the paper focuses on code generation, optimization passes, native deployment, and hardware-aware performance or energy gains.
* **OOPSLA:** Good fit if the language design and type or effect systems for ownership, placement, and precision are the central novelty.
* **ASPLOS:** Strong fit if the evaluation connects language/compiler design to architecture-level energy behavior on CPUs, GPUs, and accelerators.
* **OSDI or SOSP:** Possible only if runtime scheduling, deployment determinism, and large-scale system integration become the dominant contribution.

### AI Systems and ML Venues

* **MLSys:** Strong fit for end-to-end AI training and inference evaluation, especially if the compiler improves energy efficiency without degrading model quality.
* **NeurIPS Datasets and Benchmarks or main conference systems tracks:** Possible if the evaluation introduces robust energy benchmarks and reproducible AI deployment methodology.
* **ICML systems-oriented submissions:** Possible if the DSL contributes directly to efficient model training or inference workflows.

### Expected Evaluation Standards

A top-conference submission should include the following evidence.

1. **End-to-end workloads:** Include at least image classification, sequence modeling or transformer inference, matrix-heavy kernels, and small edge-inference deployments.
2. **Baselines:** Compare against idiomatic PyTorch or TensorFlow, TorchScript or `torch.compile`, TVM or MLIR-based flows where applicable, ONNX Runtime, and hand-tuned native kernels for selected microbenchmarks.
3. **Metrics:** Report latency, throughput, joules per inference, joules per training epoch, energy-delay product, peak memory, allocation count, transfer bytes, compile time, binary size, accuracy, and determinism variance.
4. **Ablations:** Disable operator fusion, static allocation, precision lowering, batch scheduling, thread tuning, and dead movement elimination independently.
5. **Correctness and guarantees:** Include static rejection tests, generated runtime assertion tests, precision error analysis, shape-safety tests, ownership violation tests, and deterministic replay tests.
6. **Hardware diversity:** Evaluate on at least one laptop or desktop CPU, one server CPU, one discrete GPU, and one edge-class device if the deployment claim is retained.
7. **Reproducibility:** Publish source programs, compiler flags, runtime configurations, power-measurement scripts, raw logs, and artifact manifests.
8. **Statistical rigor:** Use repeated runs, confidence intervals, warm-up policies, fixed seeds, and transparent handling of thermal throttling and background load.

## 6. Suggested Paper Structure

1. **Motivation:** AI energy costs are hard to control because tensor frameworks hide placement, memory, precision, and scheduling choices behind dynamic runtimes.
2. **Language design:** Present ShortHand's tensor, ownership, precision, placement, budget, and determinism constructs.
3. **Static semantics:** Define the shape, ownership, precision, and placement checks that establish compiler-visible guarantees.
4. **IR and optimization pipeline:** Show how guarantees become IR metadata and enable the optimization pass suite.
5. **Runtime system:** Describe native execution, energy instrumentation, backend dispatch, memory pools, and deterministic scheduling.
6. **Evaluation:** Demonstrate energy and performance gains, guarantee enforcement, optimization ablations, and deployment reproducibility.
7. **Limitations:** Discuss unsupported dynamic models, incomplete backend coverage, energy-measurement noise, and the tradeoff between determinism and peak throughput.
8. **Conclusion:** Argue that energy-aware AI compilation should be treated as a language and compiler contract, not as a post-hoc profiler setting.
