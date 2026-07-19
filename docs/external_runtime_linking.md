# External runtime linking transition

## Purpose

ShortHand AI and GreenAI constructs need two things in compiled output:

1. Static metadata so evidence tools can inspect the compiled artifact.
2. Runtime hook calls so native binaries can execute observable AI/GreenAI registration and inference intent.

Earlier compiler slices emitted hook calls, but the generated LLVM module also contained local no-op hook bodies. That made bitcode self-contained, but it prevented the native binary from proving that it can resolve symbols from the real `libshorthand_runtime.a` runtime library.

## Current implementation

The current runtime integration has two layers:

- `scripts/generate_external_runtime_ir_generator.sh` converts `IR_Generator.cpp` into an external-runtime variant.
- `tests/codegen/test_external_runtime_native.sh` builds that external-runtime compiler variant, compiles a small AI program to LLVM IR and native code, links against `libshorthand_runtime.a`, and executes the resulting binary.

The test verifies:

- LLVM IR contains `declare i32 @short_ai_register_model`.
- LLVM IR does not contain `define i32 @short_ai_register_model`.
- Native linking reports the runtime library.
- The executable emits runtime hook output from `libshorthand_runtime.a`.

The runtime library is no longer only a logging shim. It now owns in-process registries for:

- models,
- tensors,
- GreenAI contracts,
- GreenAI measurements.

Runtime hooks now return explicit status codes for important cases:

- success,
- invalid argument,
- model not found,
- input tensor not found,
- output tensor not found,
- backend unavailable,
- not executed.

`short_ai_infer()` validates that the referenced model, input tensor, and output tensor were registered before returning `SHORTHAND_RUNTIME_NOT_EXECUTED`. This is intentionally conservative: it proves compiled binaries use the runtime registry, but it does not yet claim real backend execution through the compiled hook path.

## Why this is still transitional

The external-runtime compiler path is tested, but the checked-in `IR_Generator.cpp` still needs a source-level cleanup so external hook declarations are emitted directly by the default compiler implementation.

That cleanup should be done as a focused compiler PR because it affects core LLVM code generation.

## Remaining work

- Replace the generated external-runtime variant with direct source-level hook declaration logic in `IR_Generator.cpp`.
- Make `compile-native` consistently locate or build `libshorthand_runtime.a` through documented build-system rules.
- Route `short_ai_infer()` into `AI_Runtime` for SDK-enabled ONNX Runtime CPU execution.
- Keep fallback and not-executed statuses explicit when SDKs are unavailable.
- Move this hook abstraction into the MLIR lowering plan so `model`, `tensor`, `infer`, and `greenai_measure` lower through typed compiler operations rather than string-only hooks.
