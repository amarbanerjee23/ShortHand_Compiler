# AI Runtime bridge linkage

## Purpose

This document records the build-graph boundary between the compiled runtime hook library and the SDK-backed AI runtime.

ShortHand currently has two related but separate pieces:

- `runtime/ShorthandRuntime.*` owns the public compiled-hook C ABI used by generated/native code.
- `ai_runtime/AI_Runtime.*` owns SDK-backed C++ inference routing through the backend registry.

## runtime-hook ABI ownership

The public hook symbols must have a single owner. In the current design, that owner is `runtime/ShorthandRuntime.*`.

That file family exports the compiled-hook C ABI, including:

- `short_ai_register_model`,
- `short_ai_register_tensor`,
- `short_ai_infer`,
- `short_ai_infer_f32`,
- `short_runtime_infer_bridge_request_json`,
- runtime observability helpers.

`AI_Runtime.cpp` must not export a second `extern "C" short_ai_infer` implementation. Keeping only one owner avoids a duplicate C symbol when the runtime hook library and SDK-backed AI runtime are linked together later.

## Current bridge boundary

The typed buffer bridge already validates a concrete float32 input/output buffer request and records `shorthand.runtime.typed_infer_buffer_bridge_request.v1`.

The bridge still returns `SHORTHAND_RUNTIME_NOT_EXECUTED` because the SDK-backed call path has not been linked into the hook library yet.

## Why this PR is separate from real execution

The next execution PR must do more than call `AI_Runtime` directly. It must also make sure that:

1. there is one public C ABI owner,
2. the runtime hook library can link the required AI runtime sources without duplicate symbols,
3. fallback still returns `not_executed`,
4. `SHORTHAND_RUNTIME_OK` is returned only when a real backend execution succeeds,
5. output buffers are populated only on success.

This PR solves item 1 and adds a gate for it. It does not claim real compiled-hook backend execution.

## Validation

`scripts/check_ai_runtime_bridge_linkage.sh` verifies that:

- `AI_Runtime.cpp` no longer exports legacy compiled-hook C symbols,
- `ShorthandRuntime.*` remains the public hook ABI owner,
- Makefile and CMake still keep the standalone hook library separate,
- the compiled-infer bridge docs preserve the execution boundary.
