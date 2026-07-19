# Compiled infer bridge request

## Purpose

This document describes the current compiled-inference runtime bridge contract for ShortHand.

The compiler already emits external runtime hooks for `model`, `tensor`, `greenai_contract`, `greenai_measure`, and `infer`. The `libshorthand_runtime.a` hook library now turns a validated compiled `infer(model, input, output)` call into a structured bridge request that is ready to be connected to `AI_Runtime`.

## Current behavior

When `short_ai_infer(model, input, output)` is called, the runtime:

1. Verifies that the model was registered.
2. Verifies that the input and output tensors were registered.
3. Validates the tensor shape strings.
4. Builds a candidate AI runtime bridge request JSON document.
5. Records the request in runtime observability.
6. Returns `SHORTHAND_RUNTIME_NOT_EXECUTED`.

The return value intentionally remains `not_executed` because the compiled runtime hook does not yet receive a concrete input tensor buffer. The bridge request records this boundary with:

- `schema: shorthand.runtime.compiled_infer_bridge_request.v1`
- `runtime_target: AI_Runtime`
- `bridge_status: candidate_request_only`
- `execution_ready: false`
- `input_buffer_required: true`
- `reason: input_buffer_required_for_ai_runtime_execution`

## Public C ABI

The bridge request is exposed through:

```c
const char *short_runtime_infer_bridge_request_json(void);
```

After a successful validation path through `short_ai_infer`, this function returns the latest bridge request. For missing model/tensor and invalid input paths, it returns `{}` because no valid runtime execution request exists.

## Why this is not yet real execution

The compiled hook currently knows symbolic names and metadata, but not actual tensor data. A real SDK-backed call into `AI_Runtime` requires a typed tensor buffer or a compiled data binding that maps runtime variables into `TensorBuffer`.

Until that is implemented, ShortHand must not claim that compiled inference executed through ONNX Runtime, TensorRT, OpenVINO, LibTorch, or any other backend from this hook path.

## Next implementation step

The next step is to add a typed tensor payload bridge, such as one of the following:

- a C ABI that accepts a float32 input pointer and element count,
- compiler lowering that passes concrete tensor data into the runtime hook,
- or a runtime-managed tensor buffer registry that stores data separately from tensor metadata.

Once that exists, the runtime hook can safely route into `AI_Runtime` and return `SHORTHAND_RUNTIME_OK` only when backend execution actually succeeds.
