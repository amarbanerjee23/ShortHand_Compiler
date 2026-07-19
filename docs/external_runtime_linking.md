# Default external runtime linking

## Purpose

ShortHand AI and GreenAI constructs need two things in compiled output:

1. Static metadata so evidence tools can inspect the compiled artifact.
2. Runtime hook calls so native binaries can execute observable AI/GreenAI registration and inference intent.

Earlier compiler slices emitted hook calls, but the generated LLVM module also contained local no-op hook bodies. That made bitcode self-contained, but it prevented native binaries from proving that they can resolve symbols from the real `libshorthand_runtime.a` runtime library.

## Current implementation

The default Makefile and CMake compiler builds now compile a generated external-runtime IR generator source:

- Makefile output: `Compiler_new_ws/Short_Hand/build/IR_Generator.default_runtime.cpp`
- CMake output: `${CMAKE_BINARY_DIR}/generated/IR_Generator.default_runtime.cpp`
- Source transform: `scripts/generate_external_runtime_ir_generator.sh`

That generated source is derived from `visitors/IR_Generator.cpp`, but it removes local runtime hook stub bodies and emits external declarations for hooks such as:

- `short_ai_register_model`
- `short_ai_register_tensor`
- `short_greenai_register_contract`
- `short_greenai_record_measurement`
- `short_ai_infer`
- `short_ai_infer_legacy`

The default native path now resolves those declarations against `libshorthand_runtime.a` when `compile-native` is used. The linker can be overridden with `SHORTHAND_NATIVE_LINKER`; otherwise the generated path uses a C++ linker so the C++ runtime library is resolved correctly.

## Validation

`tests/codegen/test_external_runtime_native.sh` now exercises the default compiler instead of building a separate external-runtime compiler variant. The test verifies that:

- the default compiler generates LLVM IR containing `declare i32 @short_ai_register_model`,
- the generated LLVM IR does not contain `define i32 @short_ai_register_model`,
- `compile-native` links `libshorthand_runtime.a`,
- the native executable emits runtime hook output from the registry-backed runtime library.

The enterprise hardening gate requires this default runtime-lowering test to pass.

## Runtime behavior

The runtime library owns in-process registries for:

- models,
- tensors,
- GreenAI contracts,
- GreenAI measurements.

Runtime hooks return explicit status codes for important cases:

- success,
- invalid argument,
- model not found,
- input tensor not found,
- output tensor not found,
- backend unavailable,
- not executed.

`short_ai_infer()` validates that the referenced model, input tensor, and output tensor were registered before returning `SHORTHAND_RUNTIME_NOT_EXECUTED`. This is intentionally conservative: it proves compiled binaries use the runtime registry, but it does not yet claim real backend execution through the compiled hook path.

## Remaining work

- Replace the generated default source with direct source-level external hook declaration logic in `IR_Generator.cpp`.
- Route `short_ai_infer()` into `AI_Runtime` for SDK-enabled ONNX Runtime CPU execution.
- Keep fallback and not-executed statuses explicit when SDKs are unavailable.
- Move this hook abstraction into the MLIR lowering plan so `model`, `tensor`, `infer`, and `greenai_measure` lower through typed compiler operations rather than string-only hooks.
