# Backend live SDK matrix harness

backend_live_sdk_matrix_status: optional_matrix_harness
schema: shorthand.backend_live_sdk_matrix.v1
full_backend_matrix_claim: false

## Purpose

This document defines the backend live SDK matrix harness. The harness gives the project one repeatable place to record whether each marketed backend has a live SDK success fixture, a skip-safe optional SDK path, or only policy-compatible coverage.

The matrix is intentionally conservative. A backend row may report `live_success` only when a real fixture runs through the runtime path and returns expected output without fallback. All other rows must report `skip_safe` until a dedicated live fixture is implemented.

## Row statuses

| Status | Meaning | Claim boundary |
| --- | --- | --- |
| `live_success` | The backend ran a real SDK-backed fixture and returned expected output. | This can support a backend-specific claim for the tested configuration only. |
| `skip_safe` | The backend did not run because the SDK is absent, direct backend support is unavailable, or the dedicated live fixture is not implemented yet. | This is not proof of live backend support. It is safe default-CI behavior. |
| `policy_compatible_only` | The backend is understood by the compatibility policy, but has no live fixture yet. | Do not market this backend as production-supported. |
| `dedicated_fixture_planned` | The SDK path was detected or named, but the backend-specific live fixture belongs to a later PR. | Do not claim live support before that later PR lands. |
| `unavailable_path_proved` | A backend-specific fixture proved the backend path does not falsely report success while unavailable. | This is reliability evidence, not live support evidence. |

## Current matrix rows

| Backend | Model format | Current harness behavior | Dedicated fixture owner |
| --- | --- | --- | --- |
| `onnxruntime_cpu` | `onnx` | Runs `tests/integration/test_compiled_hook_onnxruntime_success.sh` when `ONNXRUNTIME_ROOT` is set; otherwise records `skip_safe`. | Existing ONNX Runtime fixture |
| `onnxruntime_cuda` | `onnx` | Records `skip_safe` and `policy_compatible_only` until the failure matrix or a dedicated live fixture is added. | PR58 decision or future split |
| `onnxruntime_tensorrt` | `onnx` | Uses the PR53 TensorRT optional fixture path and records `skip_safe` with no false success until ONNX Runtime TensorRT EP support exists. | PR53 unavailable-path proof; future live EP fixture still open |
| `tensorrt` | `engine` | Runs `tests/integration/test_tensorrt_optional_fixture.sh`; records `skip_safe` after proving the current TensorRT path is unavailable and does not copy outputs or report success. | PR53 TensorRT unavailable-path proof |
| `openvino` | `openvino_ir` | Runs `tests/integration/test_openvino_optional_fixture.sh`; records `skip_safe` after proving the current OpenVINO path is unavailable and does not copy outputs or report success. | PR54 OpenVINO unavailable-path proof |
| `libtorch` | `torchscript` | Runs `tests/integration/test_libtorch_optional_fixture.sh`; records `skip_safe` after proving the current LibTorch path is unavailable and does not copy outputs or report success. | PR55 LibTorch unavailable-path proof |
| `llamacpp` | `gguf` | Records `skip_safe` and `dedicated_fixture_planned`. | PR57 |

## TensorRT unavailable-path proof

PR #53 adds a TensorRT-specific fixture gate. The gate compiles the bridge-enabled runtime path, registers a TensorRT engine model, calls `short_ai_infer_f32`, and requires a non-success status with no output copy.

This proves claim safety for the current TensorRT path. It does not prove live TensorRT support.

## OpenVINO unavailable-path proof

PR #54 adds an OpenVINO-specific fixture gate. The gate compiles the bridge-enabled runtime path, registers an OpenVINO IR model, calls `short_ai_infer_f32`, and requires a non-success status with no output copy.

This proves claim safety for the current OpenVINO path. It does not prove live OpenVINO support, and the OpenVINO row remains not production-executing yet.

## LibTorch unavailable-path proof

PR #55 adds a LibTorch-specific fixture gate. The gate compiles the bridge-enabled runtime path, registers a TorchScript model, calls `short_ai_infer_f32`, and requires a non-success status with no output copy.

This proves claim safety for the current LibTorch path. It does not prove live LibTorch support, and the LibTorch row remains not production-executing yet.

## Hardware-aware routing boundary

The planned PR #56 will add hardware capability discovery and accelerator-aware routing for CPU, GPU, TPU, and NPU classes. Hardware presence alone must not create a `live_success` claim. A device may be selected only when a compatible execution backend confirms that the device is usable for the requested model format, precision, and workload.

## Default CI behavior

Default CI must remain dependency-light. Therefore the matrix harness may skip SDK-backed checks when optional SDK roots are not configured.

A skip must still be explicit and machine-readable. The test writes `/tmp/shorthand_backend_live_sdk_matrix.jsonl` with one row per backend.

## Claim-safety rule

The matrix harness must not claim `live_success` for TensorRT, OpenVINO, LibTorch, Llama.cpp, ONNX Runtime CUDA, or ONNX Runtime TensorRT until the dedicated backend fixture actually exists and passes real execution.

The harness may only claim `live_success` for `onnxruntime_cpu` when the existing compiled-hook ONNX Runtime success fixture passes with `ONNXRUNTIME_ROOT` configured.

## Evidence

- `tests/integration/test_backend_live_sdk_matrix.sh`
- `scripts/check_backend_live_sdk_matrix.sh`
- `tests/integration/test_compiled_hook_onnxruntime_success.sh`
- `tests/integration/test_tensorrt_optional_fixture.sh`
- `scripts/check_tensorrt_optional_fixture.sh`
- `docs/tensorrt_optional_fixture.md`
- `tests/integration/test_openvino_optional_fixture.sh`
- `scripts/check_openvino_optional_fixture.sh`
- `docs/openvino_optional_fixture.md`
- `tests/integration/test_libtorch_optional_fixture.sh`
- `scripts/check_libtorch_optional_fixture.sh`
- `docs/libtorch_optional_fixture.md`
- `docs/backend_compatibility_matrix.md`
