# ShortHand MLIR Lowering Plan

## Goal

ShortHand currently compiles through the existing AST/semantic analyzer and LLVM IR generator. The MLIR foundation introduces a typed intermediate layer for AI, tensor, inference, and GreenAI constructs without replacing the current compiler path yet.

## Target pipeline

```text
ShortHand source
  -> parser and AST
  -> semantic analyzer
  -> ShortHand semantic IR
  -> ShortHand MLIR dialect
  -> LLVM dialect
  -> LLVM IR
  -> bitcode/native binary
```

## Initial operations

The first dialect scaffold defines these operations:

| Operation | Purpose | Current source equivalent |
| --- | --- | --- |
| `shorthand.model` | Model declaration | `AST_MODEL_DECLARATION` / `ModelOp` |
| `shorthand.tensor` | Tensor declaration | `AST_TENSOR_DECLARATION` / `TensorOp` |
| `shorthand.infer` | Inference request | `AST_INFER_STATEMENT` / `InferOp` |
| `shorthand.greenai_contract` | C3-ECO evidence contract | `AST_GREENAI_CONTRACT` / `GreenAIContractOp` |
| `shorthand.greenai_measure` | GreenAI measurement | `AST_GREENAI_MEASUREMENT` / `GreenAIMeasurementOp` |

## Non-goals for this PR

This PR does not:

- require MLIR tools in CI,
- replace the current LLVM IR generator,
- lower MLIR to LLVM dialect,
- route compiled inference into the real SDK-backed runtime path,
- claim production readiness.

## Validation strategy

Until MLIR toolchain support is introduced, CI validates the dialect foundation using dependency-free static checks:

- dialect and operation TableGen files exist,
- all expected operations are present,
- the example module uses all expected op names,
- the lowering plan preserves the AST -> Semantic IR -> MLIR -> LLVM path,
- the feature tracker records MLIR as scaffolded rather than production-ready.

## Next implementation steps

1. Add optional MLIR toolchain discovery in CMake.
2. Generate dialect C++ headers/sources through TableGen when MLIR is available.
3. Add parser/printer tests gated behind MLIR availability.
4. Implement AST/SemanticIR to MLIR conversion for model/tensor/infer/GreenAI ops.
5. Lower `shorthand.infer` to runtime hooks or backend-specific execution calls.
6. Introduce LLVM dialect lowering once the MLIR path is stable.
