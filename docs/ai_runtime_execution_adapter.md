# AI Runtime execution adapter

## Purpose

This document defines the narrow adapter contract between the public runtime-hook ABI in `runtime/ShorthandRuntime.*` and the SDK-oriented C++ execution types in `ai_runtime/AI_Runtime.*`.

The current status is:

`adapter_contract_status: compile_checked_mapping_only`

`bridge_link_status: runtime_adapter_ai_core_link_checked`

`compiled_hook_execution_status: bridge_enabled_ai_runtime_infer_attempt`

`compiled_hook_success_status: optional_onnxruntime_success_fixture`

This means the adapter compiles and maps runtime-hook inputs into `AI_Runtime` data structures, an isolated link-build gate proves that the runtime hook layer, adapter layer, and AI runtime core can be compiled together, bridge-enabled builds can route `short_ai_infer_f32` into `AIRuntime::infer`, and SDK-enabled environments can prove a real ONNX Runtime success path through the public compiled hook.

The default standalone runtime library remains dependency-light and keeps the existing pending behavior unless it is compiled with `SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1` and linked with the AI runtime core.

## What the adapter owns

The adapter in `runtime/AIRuntimeBridgeAdapter.*` owns only data conversion and status mapping:

- `RuntimeBridgeModelInput` to `shorthand::ai::ModelSpec`
- `RuntimeBridgeTensorInput` to `shorthand::ai::TensorSpec`
- float32 input pointer and count to `shorthand::ai::TensorBuffer`
- `shorthand::ai::InferenceStatus` to `ShortHandRuntimeStatus`
- execution-readiness validation before the actual call path is added

The adapter version marker is:

`shorthand.runtime.ai_runtime_execution_adapter.v1`

## Bridge-enabled execution path

PR #47 wires the bridge-enabled runtime build path as follows:

```text
short_ai_infer_f32
  -> runtime registry validation
  -> AIRuntimeBridgeAdapter mapping
  -> AIRuntime::infer
  -> public ShortHandRuntimeStatus mapping
  -> output copy only on successful backend execution
```

The path is enabled only when `ShorthandRuntime.cpp` is compiled with:

```text
SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1
```

When SDK backends are not enabled, `AIRuntime::infer` still falls back honestly to `NotExecuted`, so the public C ABI returns `SHORTHAND_RUNTIME_NOT_EXECUTED`, keeps `output_count` at `0`, and records `backend_not_available` instead of claiming inference succeeded.

## Optional ONNX Runtime success fixture

PR #48 adds `tests/integration/test_compiled_hook_onnxruntime_success.sh` and `scripts/check_compiled_hook_onnxruntime_success.sh`.

When `ONNXRUNTIME_ROOT` is available, this fixture compiles the bridge-enabled runtime with `SHORTHAND_HAS_ONNXRUNTIME=1`, runs the identity ONNX fixture through `short_ai_infer_f32`, and verifies:

- public status is `SHORTHAND_RUNTIME_OK`,
- backend is `onnxruntime_cpu`,
- output count is `1`,
- output value round-trips to `42`,
- bridge status is `ai_runtime_execution_succeeded`,
- telemetry is attached to the runtime hook result.

When `ONNXRUNTIME_ROOT` is absent, the fixture returns a clean skip so default CI remains dependency-light.

## Status mapping contract

The adapter maps C++ runtime statuses to public C ABI statuses as follows:

| AI runtime status | Public runtime status |
| --- | --- |
| `Success` | `SHORTHAND_RUNTIME_OK` |
| `NotExecuted` | `SHORTHAND_RUNTIME_NOT_EXECUTED` |
| `BackendUnavailable` | `SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE` |
| `InvalidInput` | `SHORTHAND_RUNTIME_INVALID_INPUT` |
| `RuntimeError` | `SHORTHAND_RUNTIME_RUNTIME_ERROR` |

## Execution readiness contract

A request is only ready to be passed to `AI_Runtime` when:

1. model name and model path are present,
2. model format is recognized,
3. input and output tensor element types are `float32`,
4. input and output shapes are valid,
5. the input buffer element count matches the input tensor shape,
6. output capacity is at least the declared output element count.

If any of these are false, the runtime execution path must not return `SHORTHAND_RUNTIME_OK`.

## Runtime AI bridge link build

PR #46 adds an isolated link-build gate that compiles these pieces into one probe binary:

- `runtime/ShorthandRuntime.cpp`,
- `runtime/AIRuntimeBridgeAdapter.cpp`,
- `ai_runtime/AI_Runtime.cpp`,
- `ai_runtime/AI_Types.cpp`,
- `ai_runtime/AI_Backend.cpp`,
- `ai_runtime/AI_Telemetry.cpp`,
- the fallback and optional backend source files.

The probe validates that:

- runtime-hook C ABI symbols and AI runtime C++ symbols can coexist,
- adapter readiness checks pass for a valid typed request,
- direct `AIRuntime::infer` fallback behavior remains `NotExecuted` when no SDK backend is enabled.

## Runtime AI bridge execution path

PR #47 adds `scripts/check_runtime_ai_bridge_execution_path.sh`, which runs `tests/codegen/test_runtime_ai_bridge_execution_path.sh`.

That probe compiles the same runtime bridge pieces with `SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE=1` and optional SDK macros disabled. It verifies that:

- `short_ai_infer_f32` reaches `AIRuntime::infer`,
- fallback behavior remains `SHORTHAND_RUNTIME_NOT_EXECUTED` when no SDK backend is enabled,
- no output values are fabricated,
- the bridge request records `ai_runtime_execution_attempted`,
- `SHORTHAND_RUNTIME_OK` remains reserved for real backend success.

## Validation

The gate `scripts/check_ai_runtime_execution_adapter.sh` checks the adapter contract and runs `tests/codegen/test_ai_runtime_bridge_adapter.sh`, which compiles the adapter with `AI_Types.cpp` and verifies model/tensor/status mapping.

The gate `scripts/check_runtime_ai_bridge_link_build.sh` runs `tests/codegen/test_runtime_ai_bridge_link_build.sh`, which compiles the runtime hook layer, adapter, and AI runtime core together with optional SDK macros disabled.

The gate `scripts/check_runtime_ai_bridge_execution_path.sh` runs the bridge-enabled execution probe and verifies the safe no-SDK fallback path.

The gate `scripts/check_compiled_hook_onnxruntime_success.sh` runs the optional SDK-backed compiled hook success fixture. It skips when `ONNXRUNTIME_ROOT` is absent and proves real `SHORTHAND_RUNTIME_OK` behavior when the SDK is configured.

## Next implementation step

The next runtime PR should move from hook-local JSON and telemetry fragments toward an exportable runtime observability path. It must continue returning `SHORTHAND_RUNTIME_NOT_EXECUTED` or `SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE` unless real backend execution succeeds and populates output values.
