# Runtime infer observability bridge

## Purpose

Compiled ShortHand AI programs call runtime hooks for model registration, tensor registration, GreenAI contracts, GreenAI measurements, and infer requests. The runtime hook layer is intentionally conservative: it must expose execution intent and observability without claiming backend execution that did not happen.

## Current behavior

`short_ai_infer(model, input, output)` now records bridge-ready runtime state:

- total infer calls,
- successful infer calls,
- not-executed infer calls,
- backend-unavailable infer calls,
- invalid-input infer calls,
- last infer status,
- last infer backend,
- last infer reason,
- last infer telemetry JSON,
- aggregate observability JSON.

The public C ABI is in `Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h`:

- `short_runtime_infer_count()`
- `short_runtime_infer_success_count()`
- `short_runtime_infer_not_executed_count()`
- `short_runtime_infer_backend_unavailable_count()`
- `short_runtime_infer_invalid_input_count()`
- `short_runtime_last_infer_status()`
- `short_runtime_last_infer_backend()`
- `short_runtime_last_infer_reason()`
- `short_runtime_last_infer_telemetry_json()`
- `short_runtime_observability_json()`

## Claim boundary

The runtime hook currently returns `SHORTHAND_RUNTIME_NOT_EXECUTED` for registered model/tensor infer requests because the compiled-hook-to-`AI_Runtime` execution bridge is not enabled yet. This is deliberate.

The runtime must not claim real inference unless the hook path actually routes through an SDK-backed backend and receives a success result from that backend.

## Validation

`tests/codegen/test_runtime_library_build.sh` validates:

- exported runtime C ABI symbols compile and link,
- missing model/input/output cases return the expected status codes,
- registered infer returns `SHORTHAND_RUNTIME_NOT_EXECUTED`,
- last-infer telemetry JSON contains `shorthand.runtime.infer_telemetry.v1`,
- aggregate observability JSON contains `shorthand.runtime.observability.v1`,
- runtime logs preserve the source-level external native-linking behavior.

`tests/codegen/test_external_runtime_native.sh` continues validating that compiled native programs use the runtime library and emit runtime hook output.

## Next implementation slice

The next compatible slice is to route registered `onnx` models through `AI_Runtime` / `AIRuntime` when the SDK is configured, while preserving fallback and `not_executed` behavior when no real backend is available.
