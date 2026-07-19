# Semantic IR and diagnostic span plan

Status: implementation scaffold and validation plan.

This document defines the next compiler-correctness layer between the current AST visitors and future MLIR lowering.

## Why this exists

The compiler currently performs useful work directly from AST visitors:

- semantic checks validate tensors, models, inference statements and GreenAI declarations;
- LLVM code generation emits metadata globals and runtime hook calls;
- evidence tooling walks the AST to generate reports.

That is good for the beta compiler, but it creates three long-term problems:

1. AI and GreenAI semantics are spread across visitors.
2. Optimizations cannot reason over typed operations before LLVM lowering.
3. Diagnostics still depend partly on anchor lookup instead of exact AST node ranges.

The semantic IR gives these constructs a typed middle layer.

## Initial operation set

The first semantic IR scaffold lives in `Compiler_new_ws/Short_Hand/src/semantic_ir/SemanticIR.h`.

It defines:

- `TensorOp`
- `ModelOp`
- `InferOp`
- `GreenAIContractOp`
- `GreenAIMeasurementOp`
- `ProgramIR`
- `SourceLocation`
- `SourceRange`

This header is intentionally dependency-light and header-only so it can be introduced without destabilizing parser, semantic analyzer or LLVM codegen.

## Intended compiler pipeline

```text
ShortHand source
  -> parser
  -> AST with source ranges
  -> semantic analyzer
  -> ShortHand semantic IR
  -> LLVM codegen today
  -> MLIR dialect later
  -> LLVM IR / bitcode / native
```

## Source range policy

Every AST node that can produce a user-visible diagnostic should eventually carry:

- file
- start line
- start column
- end line
- end column

Current diagnostics already print source-aware messages for important AI/GreenAI semantic anchors. The next implementation step is to move from anchor lookup to explicit AST node spans.

## Diagnostic quality bar

A semantic or parser error should eventually follow this style:

```text
examples/app.short:12:5: error: model classifier has invalid precision
  precision fp32;
  ^
hint: accepted precision values are float, float32, f32, float16, fp16, f16, bfloat16, bf16, int8, int4, int32 or int
```

## Current validation in this PR

This PR adds a language-correctness validation gate that checks:

- the semantic IR header compiles as a standalone C++17 include;
- the grammar document includes required AI, GreenAI and inference rules;
- conformance fixtures exist and are executable through the compiler;
- parser-invalid and semantic-invalid fixtures are rejected;
- source diagnostics still include file, line, column and caret output;
- candidate evidence and external runtime linking tests remain compatible.

## Out of scope for this PR

This PR does not yet:

- rewrite parser actions to populate `SourceRange` on every AST node;
- route all semantic analyzer checks through `ProgramIR`;
- remove direct AST visitor lowering;
- introduce MLIR TableGen files;
- make any production-readiness claim.

Those are follow-up slices after the semantic IR and conformance contract are stable in CI.
