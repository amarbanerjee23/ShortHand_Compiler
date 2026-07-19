# Compiled infer bridge request

## Purpose

This document describes the current compiled-inference runtime bridge contract for ShortHand.

The compiler already emits external runtime hooks for `model`, `tensor`, `greenai_contract`, `greenai_measure`, and `infer`. The `libshorthand_runtime.a` hook library turns a validated compiled `infer(model, input, output)` call into a structured bridge request that is ready to be connected to `AI_Runtime`.

## Current metadata-only behavior

When `short_ai_infer(model, input, output)` is called, the runtime:

1. Verifies that the model was registered.
2. Verifies that the input and output tensors were registered.
3. Validates the tensor shape strings.
4. Builds a candidate AI runtime bridge request JSON document.
5. Records the request in runtime observability.
6. Returns `SHORTHAND_RUNTIME_NOT_EXECUTED`.

The return value intentionally remains `not_executed` because this metadata-only compiled runtime hook does not yet receive a concrete input tensor buffer. The bridge request records this boundary with:

- `schema: shorthand.runtime.compiled_infer_bridge_request.v1`
- `runtime_target: AI_Runtime`
- `bridge_status: candidate_request_only`
- `execution_ready: false`
- `input_buffer_required: true`
- `reason: input_buffer_required_for_ai_runtime_execution`

## Typed tensor-buffer bridge

PR #42 adds a typed float32 bridge ABI for runtime callers that can provide concrete tensor buffers:

```c
int short_ai_infer_f32(const char *model_name,
                       const char *input_name,
                       const float *input_values,
                       int input_count,
                       const char *output_name,
                       float *output_values,
                       int output_capacity,
                       int *output_count);
```

The typed bridge validates:

- model, input tensor, and output tensor registration,
- non-null input and output pointers,
- positive input and output sizes,
- input element count against the registered input tensor shape,
- output capacity against the registered output tensor shape.

After validation, the runtime records a typed bridge request with:

- `schema: shorthand.runtime.typed_infer_buffer_bridge_request.v1`
- `parent_schema: shorthand.runtime.compiled_infer_bridge_request.v1`
- `bridge_status: typed_buffer_received_execution_pending`
- `input_buffer_available: true`
- `runtime_target: AI_Runtime`
- `reason: ai_runtime_typed_buffer_bridge_pending`

The function currently returns `SHORTHAND_RUNTIME_NOT_EXECUTED` and sets `output_count` to `0`. It does not fabricate output values.

## Backend matrix guardrail

The typed buffer bridge is now tied to the backend compatibility matrix gate. The gate verifies that:

- backend compatibility policy is documented separately from real execution,
- fallback paths remain `not_executed`,
- the ONNX Runtime SDK gate skips safely when `ONNXRUNTIME_ROOT` is absent,
- the ONNX Runtime SDK gate rejects fallback when a real SDK execution run is requested,
- and the compiled hook bridge is still marked as execution-pending until it is actually connected to `AI_Runtime`.

This protects the project from accidentally describing the typed bridge as real backend execution before the runtime link is implemented.

## AI_Runtime bridge linkage

The runtime-hook ABI owner is `runtime/ShorthandRuntime.*`. `AI_Runtime.cpp` owns SDK-backed C++ runtime behavior and must not export duplicate `extern "C"` compiled-hook symbols such as `short_ai_infer`.

The bridge linkage boundary is documented in `docs/ai_runtime_bridge_linkage.md` and validated by `scripts/check_ai_runtime_bridge_linkage.sh`.

## Public C ABI

The latest bridge request is exposed through:

```c
const char *short_runtime_infer_bridge_request_json(void);
```

After a successful validation path through `short_ai_infer`, this function returns the latest metadata-only bridge request. After a successful validation path through `short_ai_infer_f32`, it returns the latest typed tensor-buffer bridge request. For missing model/tensor and invalid input paths, it returns `{}` because no valid runtime execution request exists.

## Why this is not yet real execution

The typed bridge receives input and output buffers, but the runtime hook library is still intentionally separate from the SDK-backed `AI_Runtime` build graph. A real SDK-backed execution step must connect this validated request and buffer payload to `AI_Runtime::infer`, then return `SHORTHAND_RUNTIME_OK` only when backend execution succeeds.

Until that is implemented, ShortHand must not claim that compiled inference executed through ONNX Runtime, TensorRT, OpenVINO, LibTorch, or any other backend from this hook path.

## Next implementation step

The next step is to route the typed buffer bridge into `AI_Runtime` behind the existing optional backend gates. That future PR should preserve these rules:

1. Return `SHORTHAND_RUNTIME_OK` only when `AI_Runtime` returns successful inference.
2. Keep fallback and unavailable backends as `not_executed` or `backend_unavailable`.
3. Populate `output_values` and `output_count` only when execution succeeds.
4. Keep observability and bridge request JSON claim-safe.
5. Pass the backend compatibility matrix gate before changing public execution claims.
6. Preserve runtime-hook ABI ownership so the linked build has no duplicate C symbol.
