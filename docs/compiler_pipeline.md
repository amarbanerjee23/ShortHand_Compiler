# Compiler Pipeline

The official pipeline is parser/scanner -> AST -> semantic analyzer -> interpreter, LLVM bitcode, native output, or evidence emitter. GreenAI declarations are compile-time/evidence metadata; inference calls use the C++ AI runtime abstraction and deterministic fallback when optional SDKs are absent.
