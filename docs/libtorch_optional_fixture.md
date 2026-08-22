# LibTorch optional fixture

libtorch_optional_fixture_status: unavailable_path_proof_no_false_success
schema: shorthand.backend_libtorch_optional_fixture.v1
full_backend_matrix_claim: false
production_claim_boundary: not production-executing yet

## Purpose

This document defines the PR #55 LibTorch backend fixture slice.

The current LibTorch backend is not a live execution backend yet. Even when `SHORTHAND_HAS_LIBTORCH` is enabled, the backend remains unavailable until direct TorchScript loading, device selection, tensor transfer, execution, and output copying are implemented.

PR #55 therefore adds an explicit unavailable-path proof rather than making a false live-execution claim.

## Covered row

| Backend row | Format | Current PR #55 behavior | Production claim boundary |
| --- | --- | --- | --- |
| `libtorch` | `torchscript` | The compiled-hook bridge attempts the LibTorch backend and must return non-success with no output copied. | not production-executing yet |

## Required behavior

The LibTorch fixture gate must prove all of the following:

1. The bridge-enabled compiled hook accepts a TorchScript model declaration.
2. `short_ai_infer_f32` does not return `SHORTHAND_RUNTIME_OK` until real LibTorch execution exists.
3. No output values are copied on the unavailable path.
4. The caller output buffer remains unchanged.
5. Runtime success counters remain zero.
6. Runtime telemetry records a non-success status.
7. The shared backend live SDK matrix records the LibTorch row as `negative_qualified`, not `live_success`.

## Default CI behavior

Default CI remains dependency-light. The fixture compiles against the existing backend abstraction and stub implementation and does not require LibTorch headers or libraries.

When `LIBTORCH_ROOT` is set, the fixture enables the `SHORTHAND_HAS_LIBTORCH` build macro only to verify the current SDK-detected but execution-disabled behavior. SDK discovery alone is not evidence of successful inference.

## Future upgrade path

A later PR may replace this unavailable-path proof with real TorchScript execution only when it includes:

- deterministic LibTorch include and library discovery,
- a committed or generated small TorchScript fixture,
- CPU and supported accelerator device selection,
- bridge-enabled execution through `AIRuntime::infer`,
- real output and shape validation,
- no fallback, unavailable, or not-executed status during a claimed live run,
- telemetry identifying the actual LibTorch device used.

Until then, LibTorch remains explicitly not production-executing yet.

## Evidence

- `tests/integration/test_libtorch_optional_fixture.sh`
- `scripts/check_libtorch_optional_fixture.sh`
- `tests/integration/test_backend_live_sdk_matrix.sh`
- `docs/backend_live_sdk_matrix.md`
