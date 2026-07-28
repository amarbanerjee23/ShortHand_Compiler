# ShortHand Backend Compatibility Matrix

This matrix defines which model formats may be routed to which runtime backends. The semantic analyzer uses this policy to warn about incompatible preferences and reject programs that have no compatible backend and no fallback path.

Matrix guardrail marker: `full_backend_matrix_claim: false`.

Backend live SDK matrix marker: `backend_live_sdk_matrix_status: optional_matrix_harness`.

TensorRT fixture marker: `trt_optional_fixture_status: unavailable_path_proof_no_false_success`.

OpenVINO fixture marker: `openvino_optional_fixture_status: unavailable_path_proof_no_false_success`.

## Format to backend compatibility

| Model format | Compatible production backends | Fallback allowed | Current execution status |
| --- | --- | --- | --- |
| ONNX | `onnxruntime_cpu`, `onnxruntime_cuda`, `onnxruntime_tensorrt`, `tensorrt` | Yes, but fallback must report `not_executed` | `onnxruntime_cpu` has SDK-backed execution when `ONNXRUNTIME_ROOT` is configured; the matrix harness records row-level status. `onnxruntime_tensorrt` is skip-safe until live TensorRT EP support exists. |
| TensorRT engine | `tensorrt`, `onnxruntime_tensorrt` | Yes, but fallback must report `not_executed` | PR #53 adds a TensorRT unavailable-path proof with no false success. This is not live TensorRT execution. |
| TorchScript | `libtorch` | Yes, but fallback must report `not_executed` | Policy-compatible only until PR #55 adds an optional live fixture or explicit unavailable-path proof. |
| OpenVINO IR | `openvino` | Yes, but fallback must report `not_executed` | PR #54 adds an OpenVINO unavailable-path proof with no false success. This is not live OpenVINO execution and remains not production-executing yet. |
| GGUF | `llamacpp` | Yes, but fallback must report `not_executed` | Policy-compatible only until PR #56 adds an optional live fixture or explicit unavailable-path proof. |

## Backend execution validation tiers

The backend matrix has four separate validation tiers. These tiers must not be collapsed into a single unsupported claim.

| Tier | Meaning | Current evidence | Claim boundary |
| --- | --- | --- | --- |
| `policy_compatible` | The language/runtime understands that a model format can be associated with a backend family. | `AI_Types.cpp` parses model formats and backend aliases, and `backendSupportsFormat` encodes compatibility. | This is compatibility policy, not proof of live backend execution. |
| `sdk_execution_optional` | A backend can execute when its SDK is present and configured. | `tests/integration/test_onnxruntime_sdk_gate.sh` runs the ONNX identity fixture only when `ONNXRUNTIME_ROOT` is set, and fails if fallback is used during that SDK-enabled run. | Default CI may skip SDK execution when the SDK is absent. |
| `backend_live_sdk_matrix_harness` | One matrix runner records a row for each marketed backend as `live_success`, `skip_safe`, `policy_compatible_only`, or `dedicated_fixture_planned`. | `tests/integration/test_backend_live_sdk_matrix.sh` writes `/tmp/shorthand_backend_live_sdk_matrix.jsonl` and runs backend row gates. | Only rows with real fixture execution may claim `live_success`. |
| `compiled_hook_bridge_pending` | Legacy compatibility tier name retained for no-SDK and fallback paths where compiled metadata and typed float32 buffers exist but no backend execution result is available. | `short_ai_infer_f32` records `shorthand.runtime.typed_infer_buffer_bridge_request.v1`; no-SDK and unsupported-backend paths return `SHORTHAND_RUNTIME_NOT_EXECUTED` or an honest unavailable/error status. | A bridge request is not a successful inference execution path unless backend execution returns success. |

## Backend live SDK matrix harness

PR #52 added the shared live SDK matrix harness. PR #53 added the TensorRT unavailable-path proof. PR #54 adds the equivalent OpenVINO unavailable-path proof.

Evidence:

- `docs/backend_live_sdk_matrix.md`
- `docs/tensorrt_optional_fixture.md`
- `docs/openvino_optional_fixture.md`
- `tests/integration/test_backend_live_sdk_matrix.sh`
- `tests/integration/test_tensorrt_optional_fixture.sh`
- `tests/integration/test_openvino_optional_fixture.sh`
- `scripts/check_backend_live_sdk_matrix.sh`
- `scripts/check_tensorrt_optional_fixture.sh`
- `scripts/check_openvino_optional_fixture.sh`

The harness records rows for:

- `onnxruntime_cpu`
- `onnxruntime_cuda`
- `onnxruntime_tensorrt`
- `tensorrt`
- `openvino`
- `libtorch`
- `llamacpp`

The harness must keep default CI skip-safe. It may report `live_success` only for `onnxruntime_cpu` when `ONNXRUNTIME_ROOT` is configured and the compiled-hook ONNX Runtime success fixture passes. TensorRT and OpenVINO rows currently prove unavailable-path honesty only; they must not be marketed as live execution support.

## Compiler behavior

- A model with at least one compatible real backend is accepted.
- A model with no compatible real backend but an explicit `fallback` is accepted for candidate evidence and pilot workflows, but fallback must report `not_executed`.
- A model with no compatible real backend and no fallback is rejected.
- Incompatible backend preferences are warnings when another compatible backend exists.
- The typed buffer bridge validates concrete float32 input/output buffers, but it must not claim success unless a backend execution path actually returns success.

## Enterprise status

The ONNX Runtime CPU path is the first real backend execution path. Full enterprise release still requires:

1. SDK-enabled CI or local execution against the committed ONNX fixture.
2. One shared backend live SDK matrix harness with skip-safe row reporting.
3. Equivalent real execution gates for any backend marketed as supported.
4. Runtime telemetry for latency, input/output shape, backend, execution status, and energy source.
5. Certification evidence bundle linkage to measured execution.
6. Compiled typed-buffer hook execution through `AI_Runtime`, returning success only when backend execution actually succeeds.
