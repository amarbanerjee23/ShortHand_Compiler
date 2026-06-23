# Compiler Pipeline

The official pipeline is parser/scanner -> AST -> semantic analyzer -> interpreter, LLVM bitcode, native output, or evidence emitter. GreenAI declarations are compile-time/evidence metadata; inference calls use the C++ AI runtime abstraction and deterministic fallback when optional SDKs are absent.


## AI runtime selection

Parsed model/tensor/infer nodes are validated semantically, then the interpreter builds `shorthand::ai::ModelSpec` and `TensorBuffer` objects. `AIRuntime` owns a `BackendRegistry` that evaluates backend preferences in source order, skips unavailable or incompatible backends with reasons, and selects fallback when no real backend is available. Optional SDK headers stay isolated to backend `.cpp` files.
