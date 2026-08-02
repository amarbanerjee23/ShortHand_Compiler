# Llama.cpp optional fixture

llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success
schema: shorthand.backend_llamacpp_optional_fixture.v1
full_backend_matrix_claim: false
production_claim_boundary: not production-executing yet

## Purpose

This document defines the PR #57 Llama.cpp backend fixture slice for GGUF workloads.

The current Llama.cpp backend is not a live execution backend. Even when `SHORTHAND_HAS_LLAMACPP` is enabled, direct GGUF model loading, context creation, tokenization, execution, output handling, device offload, and telemetry are not implemented in the backend.

PR #57 therefore proves the unavailable path is honest instead of claiming live Llama.cpp inference.

## Covered row

| Backend row | Format | Current behavior | Production claim boundary |
| --- | --- | --- | --- |
| `llamacpp` | `gguf` | The compiled-hook bridge attempts hardware/backend routing and must return non-success with no output copied. | not production-executing yet |

## Required behavior

The fixture must prove:

1. The bridge accepts a GGUF model declaration and float32 input/output buffers.
2. `short_ai_infer_f32` does not return `SHORTHAND_RUNTIME_OK` until real Llama.cpp execution exists.
3. No output values are copied on the unavailable path.
4. The caller output buffer remains unchanged.
5. Runtime success counters remain zero.
6. Hardware inventory may detect CPU, GPU, TPU, or NPU, but no device may be treated as execution-ready without an available compatible Llama.cpp backend.
7. Runtime telemetry and bridge evidence remain non-success.
8. The shared backend matrix records `llamacpp` as `skip_safe`, never `live_success`.

## Default CI behavior

Default CI remains dependency-light. The proof compiles the existing runtime and backend abstraction without requiring Llama.cpp headers, libraries, GGUF weights, or accelerator hardware.

When `LLAMACPP_ROOT` is set, the fixture enables the `SHORTHAND_HAS_LLAMACPP` macro only to verify the current SDK-detected but direct-execution-disabled behavior. SDK discovery is not evidence of inference success.

## Future live-fixture requirements

A future change may replace this proof with a real Llama.cpp fixture only when it provides:

- deterministic include and library discovery,
- a legally redistributable tiny GGUF fixture or deterministic generated equivalent,
- explicit CPU and optional GPU offload configuration,
- tokenizer and context initialization,
- deterministic prompt/input and output assertions,
- bridge-enabled execution through `AIRuntime::infer`,
- no fallback, unavailable, or not-executed status during the claimed live run,
- selected hardware, backend, latency, token, memory, and measurement telemetry.

Until those conditions are satisfied, Llama.cpp remains explicitly not production-executing yet.

## Evidence

- `tests/integration/test_llamacpp_optional_fixture.sh`
- `scripts/check_llamacpp_optional_fixture.sh`
- `tests/integration/test_backend_live_sdk_matrix.sh`
- `docs/backend_live_sdk_matrix.md`
- `docs/hardware_capability_routing.md`
