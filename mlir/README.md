# ShortHand MLIR Foundation

This directory contains the first non-build MLIR foundation for ShortHand.

The current compiler still lowers executable programs through the existing AST and LLVM IR path. This MLIR foundation is intentionally introduced as a controlled scaffold so that future compiler work can move from string-oriented AI/GreenAI runtime hooks to typed operations.

## Intended lowering path

```text
ShortHand source
  -> parser and AST
  -> semantic analyzer
  -> ShortHand semantic IR
  -> ShortHand MLIR dialect
  -> LLVM dialect
  -> LLVM IR / bitcode / native binary
```

## Dialect scope

The initial dialect models the compiler-level operations that already exist in the beta language surface:

- `shorthand.model`
- `shorthand.tensor`
- `shorthand.infer`
- `shorthand.greenai_contract`
- `shorthand.greenai_measure`

These operations are not wired into the build yet. They define the typed contract for the next lowering phase.

## Claim boundary

This is a dialect scaffold, not a production MLIR pipeline. It does not replace the current LLVM codegen path yet and does not claim production readiness.
