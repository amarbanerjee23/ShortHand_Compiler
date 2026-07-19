# External runtime linking transition

## Purpose

ShortHand AI and GreenAI constructs currently need two things in compiled output:

1. Static metadata so evidence tools can inspect the compiled artifact.
2. Runtime hook calls so native binaries can execute observable AI/GreenAI registration and inference intent.

Earlier compiler slices emitted hook calls, but the generated LLVM module also contained local no-op hook bodies. That made bitcode self-contained, but it prevented the native binary from proving that it can resolve symbols from the real `libshorthand_runtime.a` runtime library.

## Current implementation in this slice

This slice adds a deterministic generated-code path:

- `scripts/generate_external_runtime_ir_generator.sh` converts `IR_Generator.cpp` into an external-runtime variant.
- The generated variant declares hooks such as `short_ai_register_model` and `short_ai_infer` as external LLVM functions, without local no-op bodies.
- `tests/codegen/test_external_runtime_native.sh` builds that external-runtime compiler variant, compiles a small AI program to LLVM IR and native code, links against `libshorthand_runtime.a`, and executes the resulting binary.

The test verifies:

- LLVM IR contains `declare i32 @short_ai_register_model`.
- LLVM IR does not contain `define i32 @short_ai_register_model`.
- Native linking reports the runtime library.
- The executable emits runtime hook output from `libshorthand_runtime.a`.

## Why this is transitional

This is intentionally not the final compiler architecture.

The next source-level cleanup should update `IR_Generator.cpp` directly so external hook declarations are the default behavior, then remove the generated transformation script. That direct source cleanup should be done as a focused compiler PR because it affects core LLVM code generation.

## Remaining work

- Replace the generated external-runtime variant with direct source-level hook declaration logic.
- Make `compile-native` consistently locate or build `libshorthand_runtime.a` through documented build-system rules.
- Extend the runtime hook library from logging hooks to real runtime dispatch where appropriate.
- Move this hook abstraction into the MLIR lowering plan so `model`, `tensor`, `infer`, and `greenai_measure` lower through typed compiler operations rather than string-only hooks.
