# Compiler Pipeline

The official pipeline is parser/scanner -> AST -> semantic analyzer -> interpreter, LLVM bitcode, native output, or evidence emitter. GreenAI declarations are compile-time/evidence metadata; inference calls use the C++ AI runtime abstraction and deterministic fallback when optional SDKs are absent.

## AI runtime path

The interpreter records model and tensor declarations, converts them to shared AI runtime types, and sends `infer` statements through `AIRuntime`. Evidence mode includes backend preference and fallback status fields so downstream reports distinguish declared intent from executed inference.
