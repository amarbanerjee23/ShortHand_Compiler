# AI Runtime execution adapter

## Purpose

This document defines the narrow adapter contract between the public runtime-hook ABI in `runtime/ShorthandRuntime.*` and the SDK-oriented C++ execution types in `ai_runtime/AI_Runtime.*`.

The current status is:

`adapter_contract_status: compile_checked_mapping_only`

`bridge_link_status: runtime_adapter_ai_core_link_checked`

This means the adapter compiles and maps runtime-hook inputs into `AI_Runtime` data structures, and an isolated link-build gate proves that the runtime hook layer, adapter layer, and AI runtime core can be compiled together. The public C ABI still does not call `AIRuntime::infer` yet.

## What the adapter owns

The adapter in `runtime/AIRuntimeBridgeAdapter.*` owns only data conversion and status mapping:

- `RuntimeBridgeModelInput` to `shorthand::ai::ModelSpec`
- `RuntimeBridgeTensorInput` to `shorthand::ai::TensorSpec`
- float32 input pointer and count to `shorthand::ai::TensorBuffer`
- `shorthand::ai::InferenceStatus` to `ShortHandRuntimeStatus`
- execution-readiness validation before the actual call path is added

The adapter version marker is:

`shorthand.runtime.ai_runtime_execution_adapter.v1`

## What the adapter does not own yet

The adapter does not yet perform SDK execution from the public runtime hook. It intentionally does not change `short_ai_infer_f32` return behavior.

That boundary keeps the adapter and link-build steps separate from the backend execution PR. A later runtime PR can use this adapter to route `short_ai_infer_f32` into `AI_Runtime` behind optional backend gates.

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

If any of these are false, the future runtime execution path must not return `SHORTHAND_RUNTIME_OK`.

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
- `short_ai_infer_f32` still returns `SHORTHAND_RUNTIME_NOT_EXECUTED`,
- adapter readiness checks pass for a valid typed request,
- direct `AIRuntime::infer` fallback behavior remains `NotExecuted` when no SDK backend is enabled.

This is a link-readiness check, not a public execution-behavior change.

## Validation

The gate `scripts/check_ai_runtime_execution_adapter.sh` checks the adapter contract and runs `tests/codegen/test_ai_runtime_bridge_adapter.sh`, which compiles the adapter with `AI_Types.cpp` and verifies model/tensor/status mapping.

The gate `scripts/check_runtime_ai_bridge_link_build.sh` runs `tests/codegen/test_runtime_ai_bridge_link_build.sh`, which compiles the runtime hook layer, adapter, and AI runtime core together with optional SDK macros disabled.

## Next implementation step

The next PR should connect `short_ai_infer_f32` to this adapter and then to `AIRuntime::infer` behind existing optional backend gates. It must continue returning `SHORTHAND_RUNTIME_NOT_EXECUTED` or `SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE` unless real backend execution succeeds and populates output values.