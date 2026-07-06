# Language Feature Implementation Plan

This plan describes the next compiler-language changes needed to move ShortHand toward a usable AI software language.

## Goal

Make AI workload declarations safer and more useful across the compiler pipeline. The immediate implementation in this PR focuses on semantic correctness for `infer model(tensor) -> output;`.

## Current compiler path

1. Lexer recognizes AI and GreenAI keywords.
2. Parser builds AST nodes for tensor declarations, model declarations, GreenAI contracts, GreenAI measurements, and infer statements.
3. AST stores model and tensor metadata.
4. Semantic analyzer validates models, tensors, contracts, and infer references.
5. Interpreter executes fallback-aware AI runtime flow.
6. LLVM code generation currently preserves core program behavior and GreenAI reporting.
7. Evidence mode emits GreenAI and AI declaration metadata.

## Required language changes

### L1. AST metadata discipline

Requirement: keep model, tensor, contract, measurement, and infer metadata available to all visitors.

Current status: implemented through existing AST data structs and visitor access.

### L2. Syntax and parser stability

Requirement: keep beta syntax stable for `tensor`, `model`, `greenai_contract`, `greenai_measure`, and `infer`.

Current status: no new syntax is introduced in this PR. The change intentionally avoids lexer/parser churn and strengthens semantics over existing syntax.

### L3. Semantic validation for AI inference

Requirement: an infer statement should not be accepted when the input tensor is incompatible with the model input shape.

Implemented in this PR:

- If the model and tensor are known, the analyzer compares model `input_shape` and tensor shape.
- `dynamic` remains allowed as a compatibility escape hatch.
- Shape mismatch now becomes a semantic error before interpretation or code generation.

### L4. Negative tests

Requirement: invalid AI programs should fail before runtime.

Implemented in this PR:

- Added an invalid semantic test where the model expects `1,3,224,224` but the tensor declares `1,3,128,128`.
- The existing negative test runner automatically picks up the new test file.

### L5. Code generation follow-up

Requirement: compiled AI programs should eventually emit runtime metadata for model declarations, tensor declarations, and infer statements instead of no-op lowering.

Status: planned for the next implementation PR. This PR keeps the codegen surface unchanged to reduce merge risk while adding semantic correctness.

## Acceptance criteria for this PR

- Existing valid AI examples still pass.
- The new invalid shape-mismatch program fails semantic validation.
- CI passes on the latest PR head SHA.
- No unsupported production-readiness claim is introduced.
