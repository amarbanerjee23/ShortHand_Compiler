# OpenVINO optional fixture

openvino_optional_fixture_status: unavailable_path_proof_no_false_success
schema: shorthand.backend_openvino_optional_fixture.v1
full_backend_matrix_claim: false
production_claim_boundary: not production-executing yet

## Purpose

This document defines the PR #54 OpenVINO backend fixture slice.

The current OpenVINO backend is intentionally not a live execution backend yet. Even when the `SHORTHAND_HAS_OPENVINO` build macro is enabled, the backend reports unavailable until direct OpenVINO IR loading and execution are implemented.

Therefore PR #54 adds an explicit unavailable-path proof rather than a false success fixture.

## Covered row

| Backend row | Format | Current PR #54 behavior | Production claim boundary |
| --- | --- | --- | --- |
| `openvino` | `openvino_ir` | Compiled-hook bridge attempts the OpenVINO backend and must return non-success with no output copied. | not production-executing yet |

## Required behavior

The OpenVINO fixture gate must prove all of the following:

1. The bridge-enabled compiled hook accepts an OpenVINO IR model declaration.
2. `short_ai_infer_f32` does not return `SHORTHAND_RUNTIME_OK` for OpenVINO until real OpenVINO execution exists.
3. No output values are copied on the unavailable path.
4. Runtime success counters remain zero.
5. Runtime telemetry records a non-success status.
6. The shared backend live SDK matrix records the OpenVINO row as `negative_qualified`, not `live_success`.

## Default CI behavior

Default CI remains dependency-light. The OpenVINO gate compiles against the existing backend abstraction and stub implementation. It does not require OpenVINO headers or libraries.

If `OPENVINO_ROOT` is set, this PR still does not claim live execution. The current backend reports that SDK detection is not enough because direct execution is not enabled.

## Future upgrade path

A later PR may replace this unavailable-path proof with real OpenVINO execution only when all of the following exist:

- a committed or generated tiny OpenVINO IR fixture,
- SDK include and library discovery,
- bridge-enabled `short_ai_infer_f32` execution through `AIRuntime::infer`,
- real output validation,
- no fallback or `not_executed` status during a claimed live run.

Until then, OpenVINO must remain explicitly not production-executing yet.

## Evidence

- `tests/integration/test_openvino_optional_fixture.sh`
- `scripts/check_openvino_optional_fixture.sh`
- `tests/integration/test_backend_live_sdk_matrix.sh`
- `docs/backend_live_sdk_matrix.md`
