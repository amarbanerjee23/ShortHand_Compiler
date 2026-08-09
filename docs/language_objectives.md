# ShortHand language objectives

shorthand.language.objectives.version: 2026-08-09-v2
objective_status: active_controlled_beta
production_claim: false

Historical stability marker retained for old-task guards: shorthand.language.objectives.version: 2026-07-29-v1

## Mission

ShortHand aims to be a simple, compiled, C++/LLVM-first language for building portable AI applications with first-class runtime, hardware, operational, and sustainability evidence.

The language should let developers express the AI workload and its requirements without writing backend-specific integration code for every deployment target. The compiler and runtime remain responsible for validating the program, selecting only execution-ready paths, producing native artifacts, and recording honest evidence about what actually happened.

## Primary outcome

The desired outcome is an enterprise production usage ready language that combines:

1. simple language-level AI declarations,
2. compiled native execution and predictable packaging,
3. portable backend and hardware selection,
4. strict semantic and runtime safety,
5. observable and auditable inference behavior,
6. first-class Green AI and C3-ECO-aligned evidence,
7. stable language, runtime, and release contracts.

ShortHand is still a controlled beta. These objectives define the destination and must not be read as a current production-readiness claim.

## Target users and workloads

ShortHand is intended for teams that need to build or govern AI workloads while keeping deployment details, execution evidence, and efficiency requirements visible.

Target workloads include:

- compiled AI inference applications,
- enterprise model-serving and decision-support components,
- CPU and accelerator-portable AI programs,
- edge and constrained-runtime AI workloads,
- Green AI measurement and evidence workflows,
- auditable AI pipelines where fallback and execution status must be explicit.

## Core language objectives

### 1. Simple user-facing AI syntax

Users should express models, tensors, inference, quality requirements, hardware preferences, and sustainability constraints through concise language constructs. Backend SDK plumbing must remain behind the compiler/runtime abstraction.

### 2. Compiled C++/LLVM-first implementation

The official compiler, runtime, validation, testing, evidence, and release path must remain C++/LLVM-first. Python may be used experimentally outside the official path but must not become a mandatory production dependency.

### 3. Stable and versioned language contract

Supported syntax and semantics must be versioned, documented, covered by conformance tests, and changed through an explicit compatibility and deprecation policy. Parser, AST, semantic analysis, LLVM/MLIR lowering, runtime hooks, examples, and documentation must evolve together.

### 4. Strict semantic correctness

Invalid model formats, precisions, tensor shapes, output capacities, references, quality guardrails, backend preferences, and Green AI declarations should fail before unsafe execution. Diagnostics should identify the source file, location, relevant range, and corrective reason.

### 5. Honest execution and fallback

Only a backend that completed real inference may report success. Missing SDKs, inaccessible hardware, unsupported formats, failed probes, and fallback paths must report non-execution or controlled failure. Detection, compatibility, execution readiness, and successful execution are separate states.

### 6. Backend and hardware portability

The runtime should support a stable backend abstraction across ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, and future C++ runtimes. Hardware inventory should cover CPU, GPU, TPU, and NPU classes, but a device may be selected only when an available compatible backend confirms execution readiness for the workload.

### 7. Deterministic operator control

Enterprise operators must be able to set device/backend preference, explicit overrides, deny-lists, memory floors, and CPU fallback policy. Automated selection must remain deterministic and explainable.

### 8. First-class observability

Runtime outputs should expose status, reason, backend, provider, selected device, model and tensor context, latency, input/output sizes, and measurement availability. Prometheus and OTLP integrations should remain optional adapters around a dependency-light core runtime.

### 9. First-class Green AI evidence

Energy, carbon, cost, quality, workload, boundary, and functional-unit concepts should be language-visible and auditable. Unmeasured values must remain unavailable rather than fabricated. Generated evidence must remain candidate or evidence-only unless an external authority grants certification.

### 10. Enterprise-scale program structure

The language should support modules, imports, packages, deterministic dependency resolution, reusable libraries, and multi-file builds without weakening isolation or reproducibility.

### 11. Secure and reproducible delivery

A clean checkout should build and test through documented Make and CMake workflows. Production releases should include signed artifacts, provenance, SBOMs, vulnerability scanning, protected publication, container hardening, and deployment validation.

### 12. Extensible compiler architecture

Semantic IR and MLIR should provide typed intermediate representations and lowering paths for AI, hardware, and Green AI operations. Extensibility must not bypass language versioning, semantic validation, or claim-safety rules.

### 13. Practical developer experience

The project should provide useful diagnostics, examples, formatter and linter behavior, syntax highlighting, and an LSP path. Tooling should help developers understand the compiler/runtime decision rather than hide it.

## Priority order

When objectives conflict, use this order:

1. correctness and user safety,
2. honest execution and evidence,
3. language/runtime compatibility,
4. reproducibility and security,
5. operational reliability,
6. performance and energy efficiency,
7. convenience and syntax reduction.

Performance or energy improvements must not weaken quality, security, privacy, accessibility, correctness, or evidence integrity.

## Explicit non-goals

ShortHand does not aim to:

- replace general-purpose C++ or Python for every programming task,
- vendor large proprietary or third-party AI SDK binaries,
- claim that every detected accelerator is usable,
- silently execute through fallback while reporting success,
- fabricate energy, carbon, latency, quality, or cost values,
- grant Green AI or C3-ECO certification,
- promise zero defects or unrestricted production readiness before the release gates pass,
- expose backend-specific SDK complexity directly in the core language syntax.

## Production success criteria

The objectives are satisfied for enterprise production usage only when:

1. the language grammar, semantics, compatibility, conformance, and diagnostics are complete for the supported release,
2. native compiler/runtime packaging is reproducible,
3. public runtime ABI and state behavior are versioned and tested,
4. marketed backends have live success fixtures or are excluded from production support claims,
5. hardware routing is execution-ready and claim-safe,
6. failure modes, observability, security, deployment, and release gates pass,
7. modules and multi-file builds work predictably,
8. Green AI evidence is authority-ready and remains claim-safe,
9. MLIR lowering is integrated beyond scaffold status,
10. the final production release-candidate gate passes without unsupported claims.

## Objective-to-roadmap alignment

- Language diagnostics, conformance, robustness, module syntax, and deterministic multi-file resolution: PR64 through PR70.
- Cross-mode semantic correctness, continuous fuzzing, full sanitizers, and race hardening: PR71 through PR72.
- Cross-platform reproducibility, signed releases, vulnerability policy, and hardened deployment: PR73 through PR76.
- Developer tooling, formatter/linter, syntax highlighting, and LSP: PR77 through PR78.
- Production backend and CPU/GPU/NPU hardware qualification: PR79.
- C3-ECO language, measured scoring, and authority-ready evidence: PR80 through PR82.
- Generated MLIR dialect integration and Semantic IR lowering: PR83 through PR84.
- Measured performance, ShortHand-versus-Python energy evidence, and the production RC gate: PR85.

## Change-control rule

Any change that materially alters the mission, priority order, non-goals, supported workload boundary, production success criteria, or claim-safety rules must update this document, the language/versioning documentation, the production-readiness plan, and the relevant conformance or guardrail tests in the same PR.
