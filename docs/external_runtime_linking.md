# Source-level external runtime linking

## Purpose

ShortHand AI and GreenAI constructs need two things in compiled output:

1. Static metadata so evidence tools can inspect the compiled artifact.
2. Runtime hook calls so native binaries can execute observable AI/GreenAI registration and inference intent.

Earlier compiler slices emitted hook calls, but the generated LLVM module also contained local no-op hook bodies. That made bitcode self-contained, but it prevented native binaries from proving that they can resolve symbols from the real `libshorthand_runtime.a` runtime library.

## Current implementation

The default Makefile and CMake builds compile `Compiler_new_ws/Short_Hand/src/visitors/IR_Generator.cpp` directly.

The external-runtime lowering is now permanently represented in the checked-in C++ source. The compatibility targets remain:

- `scripts/apply_external_runtime_to_ir_source.sh`
- Makefile target: `runtime-source-lowering`
- CMake target: `shorthand_runtime_source_lowering`

The script is intentionally **validation-only and Python-free**. It verifies that the canonical external declarations and native runtime-linking helpers are present and rejects the historical local no-op runtime stubs. It does not rewrite the source during a build. This keeps older build/enterprise contracts stable while ensuring ShortHand compiler construction does not require Python merely to mutate checked-in C++.

The canonical source emits external declarations for hooks such as:

- `short_ai_register_model`
- `short_ai_register_tensor`
- `short_greenai_register_contract`
- `short_greenai_record_measurement`
- `short_ai_infer`
- `short_ai_infer_legacy`

The old generated file path `IR_Generator.default_runtime.cpp` has been removed from the default Makefile and CMake builds.

The native path resolves these declarations against `libshorthand_runtime.a` when `compile-native` is used. The linker can be overridden with `SHORTHAND_NATIVE_LINKER`; otherwise the external-runtime path uses a C++ linker so the C++ runtime library is resolved correctly.

## Validation

`tests/codegen/test_external_runtime_native.sh` exercises the default compiler. The test verifies that:

- the Makefile uses `runtime-source-lowering`,
- the Makefile no longer references `IR_Generator.default_runtime.cpp`,
- `IR_Generator.cpp` contains external runtime hook declarations,
- `IR_Generator.cpp` no longer contains the local runtime hook stub return,
- the default compiler generates LLVM IR containing `declare i32 @short_ai_register_model`,
- the generated LLVM IR does not contain `define i32 @short_ai_register_model`,
- `compile-native` links `libshorthand_runtime.a`,
- the native executable emits runtime hook output from the registry-backed runtime library.

`tests/codegen/test_external_runtime_source_guard_negative.sh` additionally injects the historical local-stub pattern into a temporary source copy and requires the Python-free source guard to reject it. The PR75 portability contract also fails if the guard reintroduces a Python interpreter invocation.

The enterprise hardening gate requires the source-level runtime-lowering test to pass.

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

- Route `short_ai_infer()` into `AI_Runtime` for SDK-enabled ONNX Runtime CPU execution.
- Keep fallback and not-executed statuses explicit when SDKs are unavailable.
- Move this hook abstraction into the MLIR lowering plan so `model`, `tensor`, `infer`, and `greenai_measure` lower through typed compiler operations rather than string-only hooks.