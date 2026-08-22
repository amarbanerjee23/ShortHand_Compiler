# TensorRT optional fixture

trt_optional_fixture_status: unavailable_path_proof_no_false_success
schema: shorthand.backend_tensorrt_optional_fixture.v1
production_claim_boundary: not production-executing yet
full_backend_matrix_claim: false

## Purpose

This document defines the PR #53 TensorRT backend fixture slice.

The current TensorRT backend is intentionally not a live execution backend yet. Even when the `SHORTHAND_HAS_TENSORRT` build macro is enabled, the backend reports unavailable until direct TensorRT engine loading and execution are implemented.

Therefore PR #53 adds an explicit unavailable-path proof rather than a false success fixture.

## Covered rows

| Backend row | Format | Current PR #53 behavior | Production claim boundary |
| --- | --- | --- | --- |
| `tensorrt` | `engine` | Compiled-hook bridge attempts the TensorRT backend and must return non-success with no output copied. | not production-executing yet. |
| `onnxruntime_tensorrt` | `onnx` | Matrix row remains skip-safe because ONNX Runtime TensorRT EP execution is not implemented as a separate live fixture yet. | not production-executing yet. |

## Required behavior

The TensorRT fixture gate must prove all of the following:

1. The bridge-enabled compiled hook accepts a TensorRT engine model declaration.
2. `short_ai_infer_f32` does not return `SHORTHAND_RUNTIME_OK` for TensorRT until real TensorRT execution exists.
3. No output values are copied on the unavailable path.
4. Runtime success counters remain zero.
5. Runtime telemetry records a non-success status.
6. The shared backend live SDK matrix records TensorRT rows as `negative_qualified`, not `live_success`.

## Default CI behavior

Default CI must remain dependency-light. The TensorRT gate compiles against the existing backend abstraction and stub implementation. It does not require TensorRT headers or libraries.

If `TENSORRT_ROOT` is set, this PR still does not claim live execution. The current backend reports that TensorRT SDK detection is not enough because direct execution is not enabled.

## Future upgrade path

A future PR may replace this unavailable-path proof with real TensorRT engine execution only when all of the following exist:

- a committed or generated tiny TensorRT engine fixture, or a deterministic build step for one,
- SDK include/library discovery,
- bridge-enabled `short_ai_infer_f32` execution through `AIRuntime::infer`,
- real output validation,
- no fallback or `not_executed` status during a claimed live run.

Until then, TensorRT must remain explicitly not production-executing yet.

## Evidence

- `tests/integration/test_tensorrt_optional_fixture.sh`
- `scripts/check_tensorrt_optional_fixture.sh`
- `tests/integration/test_backend_live_sdk_matrix.sh`
- `docs/backend_live_sdk_matrix.md`
