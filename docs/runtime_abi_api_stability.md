# Runtime ABI and API stability

runtime_abi_contract_status: frozen_v1_symbol_manifest
runtime_abi_version: 1.0.0
runtime_api_version: 1.0.0
runtime_external_symbol_count: 25
production_claim_boundary: abi_stability_gate_is_not_full_production_readiness

## Purpose

This contract freezes the public ShortHand runtime C ABI used by generated programs, native applications and integration adapters. It separates the stable external C surface from internal C++ implementation types.

ABI stability is necessary for enterprise usage, but it is not sufficient for a production-ready claim. Tenant isolation, packaging, deployment, security, observability exporters and release governance remain separate roadmap gates.

## Stable ABI surface

The authoritative ABI v1 artifacts are:

- `Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h`
- `abi/shorthand_runtime_abi_v1.h`
- `abi/runtime_public_symbols_v1.txt`
- `tests/abi/test_runtime_abi_api_stability.sh`
- `scripts/check_runtime_abi_api_stability.sh`

The frozen v1 manifest contains exactly 25 external `short_*` C symbols. Removing, renaming or changing the parameter or return type of any manifested symbol is an ABI-breaking change.

The current header exposes:

- `SHORTHAND_RUNTIME_ABI_VERSION_STRING` as `1.0.0`
- `SHORTHAND_RUNTIME_API_VERSION_STRING` as `1.0.0`
- header-level `short_runtime_abi_version()` and `short_runtime_api_version()` helpers
- header-level major/minor compatibility checks

The version helpers are `static inline` C API helpers. They intentionally do not add external binary symbols to ABI v1.

## Frozen status values

The numeric values of `ShortHandRuntimeStatus` are part of ABI v1:

| Name | Value |
| --- | ---: |
| `SHORTHAND_RUNTIME_OK` | 0 |
| `SHORTHAND_RUNTIME_INVALID_ARGUMENT` | 1 |
| `SHORTHAND_RUNTIME_MODEL_NOT_FOUND` | 2 |
| `SHORTHAND_RUNTIME_TENSOR_NOT_FOUND` | 3 |
| `SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND` | 4 |
| `SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE` | 5 |
| `SHORTHAND_RUNTIME_NOT_EXECUTED` | 6 |
| `SHORTHAND_RUNTIME_INVALID_INPUT` | 7 |
| `SHORTHAND_RUNTIME_RUNTIME_ERROR` | 8 |

Existing numeric values must never be reassigned. New values may only be appended during a compatible minor release.

## Versioning rules

### ABI major

Increment the ABI major version when any of the following occurs:

- a manifested symbol is removed or renamed,
- a manifested function signature or calling convention changes,
- an existing status numeric value changes meaning,
- ownership or lifetime rules change incompatibly,
- a public structure is introduced and later changes layout incompatibly.

A new ABI major must preserve the old ABI through a compatibility library or clearly end support through a documented major release boundary.

### ABI minor

Increment the ABI minor version for additive binary-compatible changes, such as new external functions or appended status values. Existing v1 symbols and behavior contracts must remain available. Any new external symbol must be added to a new reviewed symbol manifest.

### ABI patch

Increment the ABI patch version for implementation fixes that do not change external symbols, signatures, status values, calling conventions or ownership contracts.

### API version

The API version covers source-level header additions, macros and helper functions. Additive source-compatible helpers may increment the API minor version. Removing or incompatibly changing a public declaration requires an API major version.

## Compatibility negotiation

`short_runtime_is_abi_compatible(major, minor)` and `short_runtime_is_api_compatible(major, minor)` return true only when:

1. the requested major equals the current major, and
2. the requested minor is non-negative and no newer than the current minor.

Patch releases are compatible within the same major and minor contract.

## Deprecation policy

No ABI v1 symbol is deprecated in this release.

Future deprecation must:

1. mark the declaration with `SHORTHAND_RUNTIME_DEPRECATED`,
2. document the replacement and migration path,
3. retain the deprecated external symbol throughout the current ABI major,
4. keep the frozen consumer and symbol tests passing,
5. remove the symbol only at a reviewed ABI-major transition.

Deprecation is not permission to silently weaken fallback honesty, output-buffer safety, telemetry or evidence behavior.

## Pointer and ownership rules

Caller-provided input and output buffers remain owned by the caller. Failure paths must not copy outputs.

The synchronized ABI façade copies every string-returning query into thread-local storage. A returned pointer belongs to the calling thread and remains valid until the same query function is called again on that thread or the thread exits. Consumers should still copy returned text when it must be retained for longer.

The runtime does not expose C++ standard-library types in the public C ABI.

## Thread-safety boundary

The packaged ABI v1 library serializes all public calls through one process-wide recursive mutex. This protects the process-wide registries, counters, cached evidence and reset lifecycle from concurrent data races while preserving the exact 25-symbol ABI.

This contract does not provide tenant-level or session-level contexts. Independent tenants require separate processes. It also does not claim parallel backend execution because ABI v1 deliberately serializes complete public operations.

Detailed evidence is maintained in `docs/runtime_state_and_thread_safety.md` and `scripts/check_runtime_state_thread_safety.sh`.

## Gate behavior

The ABI/API gate:

1. builds `libshorthand_runtime.a`,
2. extracts external symbols with `nm`,
3. compares the exact `short_*` set with the frozen 25-symbol manifest,
4. compiles a C11 consumer against the frozen ABI v1 header,
5. links and runs that consumer against the current runtime library,
6. verifies every frozen status numeric value,
7. compiles a current-header consumer that tests ABI/API version negotiation.

The state/thread-safety gate additionally verifies concurrent registration, inference-failure accounting, evidence reads, reset ordering and thread-local string lifetime.

A symbol addition, removal or rename fails the gate until the versioning decision and manifest update are reviewed.
