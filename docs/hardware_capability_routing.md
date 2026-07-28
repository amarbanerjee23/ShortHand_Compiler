# Hardware capability discovery and accelerator-aware routing

hardware_capability_routing_status: inventory_and_execution_ready_selection
inventory_schema: shorthand.hardware.inventory.v1
selection_schema: shorthand.hardware.selection.v1
production_claim_boundary: detection_is_not_execution_readiness

## Purpose

PR #56 adds a common runtime contract for discovering CPU, GPU, TPU, and NPU capability classes and selecting hardware only when a compatible backend is execution-ready for the requested model.

The implementation lives in `Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h` and is used by `AIRuntime::infer`.

## Hardware state model

Each inventory row separates four states:

| State | Meaning |
| --- | --- |
| `detected` | A non-destructive system probe found a hardware or platform signal. |
| `accessible` | The process has a usable device node or an explicit operator access signal. |
| `backend_compatible` | At least one permitted backend supports the device class, model format, and precision. This does not require the backend to be built. |
| `execution_ready` | The device is detected and accessible, policy allows it, and a compatible backend reports itself available. |

Hardware detection alone must never be reported as successful accelerator execution.

## Automatic probes

`SystemHardwareProbe` always records one row for each device class:

- CPU: host CPU is detected and accessible.
- GPU: checks non-destructive NVIDIA, AMD, DRM, and container visibility signals.
- TPU: checks TPU device and environment signals.
- NPU: checks accelerator device nodes, explicit operator signals, and Apple Silicon detection without assuming backend access.

The probe does not execute vendor commands, modify devices, download drivers, or treat SDK installation as execution success.

## Backend and device mapping

The router currently applies these compatibility families:

| Backend | Eligible device classes |
| --- | --- |
| `onnxruntime_cpu` | CPU |
| `onnxruntime_cuda`, `onnxruntime_tensorrt`, `tensorrt` | GPU |
| `openvino` | CPU, GPU, NPU |
| `libtorch` | CPU, GPU |
| `llamacpp` | CPU, GPU |
| `fallback` | No execution-ready device claim |

There is currently no production TPU backend in the registry. A detected TPU therefore remains visible in inventory but is not selected.

## Routing policy

Default device preference is:

`gpu,npu,tpu,cpu`

A device is selected only after model format, precision, backend availability, device accessibility, memory policy, deny-list, and model backend preference checks pass.

Operator controls:

- `SHORTHAND_DEVICE_PREFERENCE`: comma-separated device order.
- `SHORTHAND_DEVICE_OVERRIDE`: strict first device choice, with optional CPU fallback.
- `SHORTHAND_DEVICE_DENY`: comma-separated deny-list.
- `SHORTHAND_ALLOW_CPU_FALLBACK`: enables or disables automatic CPU fallback.
- `SHORTHAND_MIN_DEVICE_MEMORY_MB`: rejects devices below the declared memory floor. Unknown memory does not pass a non-zero floor.

## Deterministic testing

`StaticHardwareProbe` provides injectable fake inventory for CI. The integration test covers:

- GPU selection when an ONNX CUDA backend is execution-ready.
- CPU fallback after GPU denial or inaccessibility.
- NPU override with an execution-ready OpenVINO backend.
- TPU detection without an eligible backend.
- no-selection behavior when CPU fallback is disabled.
- runtime telemetry when detected hardware has no execution-ready backend.

## Telemetry

`InferenceResult` now carries:

- `hardware_inventory_json`
- `hardware_selection_json`
- `selected_device_class`
- `selected_device_id`

`telemetry_json_fragment` uses `shorthand.ai_runtime.telemetry.v2` and embeds both hardware schemas. A selected device means the router found a compatible available backend. It does not by itself mean the inference later returned success; final inference status remains separate.

## Claim boundary

PR #56 provides hardware inventory and execution-ready routing logic. It does not add a TPU backend, make unavailable GPU/NPU backends live, or prove accelerator execution on hardware absent from CI. Backend-specific live execution claims still require their dedicated SDK fixtures.

## Evidence

- `Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h`
- `Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp`
- `Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h`
- `tests/integration/test_hardware_capability_routing.sh`
- `scripts/check_hardware_capability_routing.sh`
