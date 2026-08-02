# Backend failure-mode matrix

backend_failure_mode_matrix_status: finalized_v1
schema: shorthand.backend_failure_mode_matrix.v1
production_claim_boundary: failure_evidence_is_not_backend_success_evidence
false_success_allowed: false

## Purpose

This document defines the required failure behavior for the ShortHand AI runtime, compiled typed-buffer hook, hardware router, and optional backend boundary.

A production-grade runtime must be predictable when execution cannot proceed. Failure paths must preserve caller buffers, return a controlled status and reason, emit machine-readable evidence, and never increment an inference-success counter.

The deterministic matrix is implemented by:

- `tests/integration/test_backend_failure_mode_matrix.sh`
- `scripts/check_backend_failure_mode_matrix.sh`

The test writes `/tmp/shorthand_backend_failure_mode_matrix.jsonl` with schema `shorthand.backend_failure_mode_matrix.v1`.

## Required failure cases

| Case | Layer | Required result | Output rule | Evidence rule |
| --- | --- | --- | --- | --- |
| `invalid_model_format` | Compiled typed-buffer hook and bridge adapter | `invalid_input` | Output count remains zero and caller buffer is unchanged. | Reason identifies that the adapter request is not execution-ready. |
| `missing_sdk` | Compiled hook through `AIRuntime` | `not_executed` through honest fallback | No output copy. | Backend is `fallback`, hardware selection is false, and no success counter increments. |
| `input_shape_mismatch` | Compiled typed-buffer hook | `invalid_input` | No output copy. | Reason is `typed_buffer_shape_or_capacity_mismatch`. |
| `unsupported_precision` | Hardware/backend compatibility router | No execution-ready route | No backend invocation. | Inventory reports `backend_compatible: false`; selection remains false. |
| `output_capacity_mismatch` | Compiled typed-buffer hook | `invalid_input` | Output count remains zero and caller buffer is unchanged. | Reason is `typed_buffer_shape_or_capacity_mismatch`. |
| `inaccessible_hardware` | Hardware router | No execution-ready route | No backend invocation. | Hardware may be detected and format-compatible, but `accessible` and `execution_ready` remain false. |
| `hardware_probe_empty` | `AIRuntime` with injectable probe | `not_executed` fallback | No output data. | Inventory contains an empty device list and selection is false. |
| `fallback_honesty` | `AIRuntime` fallback | `not_executed` | No output data. | Backend is `fallback`, reason is `backend_not_available`, and telemetry does not claim success. |

## Status contract

Failure rows use controlled outcomes:

- `invalid_input` for malformed runtime requests, incompatible buffer sizes, invalid model format, or other pre-execution contract violations.
- `not_executed` when execution is intentionally not attempted because no SDK-backed, accessible, compatible and execution-ready backend is available and fallback is allowed.
- `backend_unavailable` when fallback is not allowed and no usable backend/device pair exists.
- `runtime_error` only after an execution-ready backend is selected and an execution attempt fails internally.

A failure case must never use `success`, `live_success`, or an equivalent success label.

## Cross-layer invariants

1. Validation happens before output copying.
2. Output count is zero on every non-success status.
3. Caller-provided output memory remains unchanged on every pre-execution failure.
4. Hardware detection does not imply accessibility.
5. Accessibility does not imply backend compatibility.
6. Backend compatibility does not imply execution readiness.
7. Execution readiness does not imply successful execution until the backend returns success.
8. Missing optional SDKs remain dependency-light and skip-safe in default CI.
9. Fallback always reports `not_executed` and never returns inferred values.
10. Machine-readable evidence preserves the observed status and reason.

## Matrix scope and backend claims

The matrix finalizes the common failure contract across ONNX Runtime, TensorRT, OpenVINO, LibTorch, Llama.cpp and future backends. It does not prove that every backend executes successfully.

Backend-specific production support still requires a real SDK-backed success fixture for each marketed configuration. At present, only the tested ONNX Runtime CPU fixture may provide backend-specific live-execution evidence when its SDK is configured and the fixture passes.

Unsupported or unavailable backend families must remain marked as non-production-executing or policy-compatible only.

## Relationship to hardware routing

The matrix depends on `docs/hardware_capability_routing.md` and `HardwareDiscovery.h`.

CPU, GPU, TPU and NPU entries must preserve four distinct states:

- detected
- accessible
- backend-compatible
- execution-ready

The `inaccessible_hardware`, `unsupported_precision`, and `hardware_probe_empty` rows prove that the router does not collapse these states into a false execution claim.

## Production boundary

This matrix is required reliability evidence, but it is not sufficient for a production-ready language claim.

Runtime ABI versioning, state isolation/thread-safety, production packaging, operations exporters, diagnostics, parser robustness, modules, release security, deployment, developer tooling, C3-ECO completion, MLIR lowering and the final production release-candidate gate remain separate roadmap requirements.
