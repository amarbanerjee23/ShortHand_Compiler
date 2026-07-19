# ShortHand MLIR Dialect Migration Plan

The current compiler preserves AI semantics in LLVM through metadata globals and runtime hook calls. This is useful for near-term evidence and backend execution, but it is not the final enterprise compiler architecture.

A production compiler should introduce a ShortHand MLIR dialect before final LLVM lowering so that AI, tensor, and GreenAI operations remain first-class during optimization.

## Proposed dialect operations

| Operation | Purpose |
| --- | --- |
| `shorthand.model` | Declare model format, path, precision, shapes, task and backend policy |
| `shorthand.tensor` | Declare tensor element type and shape |
| `shorthand.infer` | Represent model inference and output binding |
| `shorthand.greenai.contract` | Represent functional unit, boundary, MQ/DQ, budgets and claims mode |
| `shorthand.greenai.measure` | Represent workload/resource measurements |
| `shorthand.rag.retrieve` | Future RAG retrieval operation |
| `shorthand.route_model` | Future model routing operation |

## Migration stages

1. Keep current LLVM metadata and runtime hook lowering stable.
2. Add MLIR dialect definitions and parser/IR bridge behind a feature flag.
3. Lower AST AI nodes to MLIR instead of directly to LLVM metadata.
4. Add MLIR verification for tensor shapes, backend compatibility and GreenAI evidence rules.
5. Lower MLIR to LLVM runtime hooks and backend calls.
6. Add optimization passes for static tensor shapes, batching, cache-aware routing and energy/cost annotations.

## Current status

Not implemented yet. The near-term enterprise hardening work keeps semantics explicit and testable, while this plan prevents the project from mistaking raw LLVM metadata for the final production compiler design.
