# ShortHand Backend Compatibility Matrix

This matrix defines which model formats may be routed to which runtime backends. The semantic analyzer uses this policy to warn about incompatible preferences and reject programs that have no compatible backend and no fallback path.

## Format to backend compatibility

| Model format | Compatible production backends | Fallback allowed | Current execution status |
| --- | --- | --- | --- |
| ONNX | `onnxruntime_cpu`, `onnxruntime_cuda`, `onnxruntime_tensorrt`, `tensorrt` | Yes, but fallback must report `not_executed` | `onnxruntime_cpu` has SDK-backed execution when `ONNXRUNTIME_ROOT` is configured |
| TensorRT engine | `tensorrt`, `onnxruntime_tensorrt` | Yes, but fallback must report `not_executed` | Stubbed/unavailable unless SDK support is added |
| TorchScript | `libtorch` | Yes, but fallback must report `not_executed` | Stubbed/unavailable unless SDK support is added |
| OpenVINO IR | `openvino` | Yes, but fallback must report `not_executed` | Stubbed/unavailable unless SDK support is added |
| GGUF | `llamacpp` | Yes, but fallback must report `not_executed` | Stubbed/unavailable unless SDK support is added |

## Backend execution validation tiers

The backend matrix has three separate validation tiers. These tiers must not be collapsed into a single unsupported claim.

| Tier | Meaning | Current evidence | Claim boundary |
| --- | --- | --- | --- |
| `policy_compatible` | The language/runtime understands that a model format can be associated with a backend family. | `AI_Types.cpp` parses model formats and backend aliases, and `backendSupportsFormat` encodes compatibility. | This is compatibility policy, not proof of live backend execution. |
| `sdk_execution_optional` | A backend can execute when its SDK is present and configured. | `tests/integration/test_onnxruntime_sdk_gate.sh` runs the ONNX identity fixture only when `ONNXRUNTIME_ROOT` is set, and fails if fallback is used during that SDK-enabled run. | Default CI may skip SDK execution when the SDK is absent. |
| `compiled_hook_bridge_pending` | Compiled code has validated runtime metadata and typed float32 buffers, but the hook is not yet linked into SDK-backed execution. | `short_ai_infer_f32` records `shorthand.runtime.typed_infer_buffer_bridge_request.v1` and returns `SHORTHAND_RUNTIME_NOT_EXECUTED`. | The typed buffer bridge is not a successful inference execution path yet. |

Matrix guardrail marker: `full_backend_matrix_claim: false`.

## Compiler behavior

- A model with at least one compatible real backend is accepted.
- A model with no compatible real backend but an explicit `fallback` is accepted for candidate evidence and pilot workflows, but fallback must report `not_executed`.
- A model with no compatible real backend and no fallback is rejected.
- Incompatible backend preferences are warnings when another compatible backend exists.
- The typed buffer bridge validates concrete float32 input/output buffers, but it must still return `not_executed` until a future PR safely routes the request into `AI_Runtime` and receives a real backend success result.

## Enterprise status

The ONNX Runtime CPU path is the first real backend execution path. Full enterprise release still requires:

1. SDK-enabled CI execution against the committed ONNX fixture.
2. Equivalent real execution gates for any backend marketed as supported.
3. Runtime telemetry for latency, input/output shape, backend, execution status, and energy source.
4. Certification evidence bundle linkage to measured execution.
5. Compiled typed-buffer hook execution through `AI_Runtime`, returning success only when backend execution actually succeeds.
