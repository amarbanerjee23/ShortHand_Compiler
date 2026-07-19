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

## Compiler behavior

- A model with at least one compatible real backend is accepted.
- A model with no compatible real backend but an explicit `fallback` is accepted for candidate evidence and pilot workflows, but must not claim executed inference.
- A model with no compatible real backend and no fallback is rejected.
- Incompatible backend preferences are warnings when another compatible backend exists.

## Enterprise status

The ONNX Runtime CPU path is the first real backend execution path. Full enterprise release still requires:

1. SDK-enabled CI execution against the committed ONNX fixture.
2. Equivalent real execution gates for any backend marketed as supported.
3. Runtime telemetry for latency, input/output shape, backend, execution status, and energy source.
4. Certification evidence bundle linkage to measured execution.
