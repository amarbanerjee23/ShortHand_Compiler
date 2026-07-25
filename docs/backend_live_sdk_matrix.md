# Backend live SDK matrix harness

backend_live_sdk_matrix_status: optional_matrix_harness
schema: shorthand.backend_live_sdk_matrix.v1
full_backend_matrix_claim: false

## Purpose

This document defines the PR #52 backend live SDK matrix harness. The harness gives the project one repeatable place to record whether each marketed backend has a live SDK success fixture, a skip-safe optional SDK path, or only policy-compatible coverage.

The matrix is intentionally conservative. A backend row may report `live_success` only when a real fixture executes through the runtime path and returns successful output without fallback. All other rows must report `skip_safe` until a dedicated fixture is implemented.

## Row statuses

| Status | Meaning | Claim boundary |
| --- | --- | --- |
| `live_success` | The backend executed a real SDK-backed fixture and returned expected output. | This can support a backend-specific execution claim for the tested configuration only. |
| `skip_safe` | The backend did not execute because the SDK is absent or the dedicated fixture is not implemented yet. | This is not proof of execution. It is a safe default-CI behavior. |
| `policy_compatible_only` | The backend is understood by the compatibility policy, but has no live execution fixture yet. | Do not market this backend as production-executing. |
| `dedicated_fixture_planned` | The SDK path was detected or named, but the backend-specific fixture belongs to a later PR. | Do not claim live execution before that later PR lands. |

## Current matrix rows

| Backend | Model format | Current PR #52 harness behavior | Dedicated fixture owner |
| --- | --- | --- | --- |
| `onnxruntime_cpu` | `onnx` | Runs `tests/integration/test_compiled_hook_onnxruntime_success.sh` when `ONNXRUNTIME_ROOT` is set; otherwise records `skip_safe`. | Existing ONNX Runtime fixture |
| `onnxruntime_cuda` | `onnx` | Records `skip_safe` and `policy_compatible_only` until the matrix is finalized or a fixture is added. | PR57 decision or future split |
| `onnxruntime_tensorrt` | `onnx` | Records `skip_safe` and `dedicated_fixture_planned`. | PR53 |
| `tensorrt` | `engine` | Records `skip_safe` and `dedicated_fixture_planned`. | PR53 |
| `openvino` | `openvino_ir` | Records `skip_safe` and `dedicated_fixture_planned`. | PR54 |
| `libtorch` | `torchscript` | Records `skip_safe` and `dedicated_fixture_planned`. | PR55 |
| `llamacpp` | `gguf` | Records `skip_safe` and `dedicated_fixture_planned`. | PR56 |

## Default CI behavior

Default CI must remain dependency-light. Therefore the matrix harness may skip SDK-backed execution when optional SDK roots are not configured.

A skip must still be explicit and machine-readable. The test writes `/tmp/shorthand_backend_live_sdk_matrix.jsonl` with one row per backend.

## Claim-safety rule

The matrix harness must not claim `live_success` for TensorRT, OpenVINO, LibTorch, Llama.cpp, ONNX Runtime CUDA, or ONNX Runtime TensorRT until the dedicated backend fixture actually exists and executes successfully.

The harness may only claim `live_success` for `onnxruntime_cpu` when the existing compiled-hook ONNX Runtime success fixture passes with `ONNXRUNTIME_ROOT` configured.

## Evidence

- `tests/integration/test_backend_live_sdk_matrix.sh`
- `scripts/check_backend_live_sdk_matrix.sh`
- `tests/integration/test_compiled_hook_onnxruntime_success.sh`
- `docs/backend_compatibility_matrix.md`
