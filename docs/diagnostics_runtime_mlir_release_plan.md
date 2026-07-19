# Diagnostics, Runtime Linking, MLIR, and Enterprise Release Plan

This document records the next compiler and enterprise engineering sequence after the backend/telemetry hardening work.

## Current PR scope

This PR intentionally implements a safe, CI-friendly slice:

1. Source-aware semantic diagnostics for AI/GreenAI anchors.
2. A linkable ShortHand runtime hook library.
3. CI checks for diagnostics and runtime symbol resolution.
4. A clear plan for MLIR and enterprise release hardening.

It does not claim that ShortHand is fully production-ready.

## Source-aware diagnostics

Current implementation:

- `Diagnostics` can load the source file.
- Semantic diagnostics can be anchored to key language constructs such as `model`, `tensor`, `infer`, `greenai_contract`, and `greenai_measure`.
- Output includes file, line, column, source text, and a caret.

Example shape:

```text
examples/app.short:30:1: error: infer input tensor shape 1,4 does not match model classifier input_shape 1,3,224,224
  infer classifier(input) -> output;
  ^
```

Remaining work:

- Add true AST source spans to every node.
- Carry exact begin/end locations from scanner/parser into AST construction.
- Add range-aware diagnostics for parser errors as well as semantic errors.
- Add diagnostic codes, for example `SHC-E-AI-001`.
- Add snapshot tests for diagnostics so user-facing messages stay stable.

## Runtime library linking

Current implementation:

- `runtime/ShorthandRuntime.h` declares the runtime hook ABI.
- `runtime/ShorthandRuntime.cpp` provides default hook implementations.
- Makefile builds `libshorthand_runtime.a`.
- CMake builds `shorthand_runtime`.
- A test verifies exported hook symbols resolve when linked from an external C++ probe.

Remaining compiler work:

- Switch `IR_Generator.cpp` from in-module hook stubs to external declarations.
- Make `compile-native` locate and link `libshorthand_runtime.a` or a shared runtime library.
- Add a native executable test that compiles an AI `.short` program and confirms runtime hooks execute from the external library.
- Later, replace logging-only hook behavior with real runtime registry and telemetry dispatch.

## MLIR preparation

Recommended next architecture:

```text
ShortHand AST
  -> ShortHand semantic IR
  -> ShortHand MLIR dialect
  -> ShortHand optimization passes
  -> LLVM dialect
  -> LLVM IR / bitcode / native binary
```

Initial MLIR dialect operations should be:

- `short.model`
- `short.tensor`
- `short.infer`
- `short.greenai.contract`
- `short.greenai.measure`
- `short.certification.boundary`
- `short.certification.functional_unit`

First optimization passes should be conservative:

- Static tensor-shape verification.
- Dead model declaration elimination only when safe.
- Backend compatibility checking.
- Precision policy validation.
- Evidence metadata preservation.

## Enterprise release preparation

After diagnostics and runtime linking, enterprise release hardening should focus on:

1. SBOM generation.
2. Dependency and license scanning.
3. Release artifact signing.
4. Container image signing.
5. OTLP/Prometheus export.
6. Kubernetes readiness/liveness probes.
7. C3-ECO schema validation.
8. Eco-regression checks.
9. Formal language grammar and conformance suite.
10. Governance/RFC workflow.
