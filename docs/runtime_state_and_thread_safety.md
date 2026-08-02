# ShortHand runtime state and thread-safety contract

runtime_state_contract_version: 1.0.0
runtime_state_model: single_process_wide_default_context
runtime_thread_safety_status: serialized_public_abi
runtime_multi_tenant_isolation: process_boundary_required
production_claim_boundary: thread_safe_does_not_mean_multi_tenant_isolated

## Purpose

The runtime ABI v1 exposes a fixed set of process-level C functions. PR #60 defines how those functions behave when multiple threads call them concurrently without changing the frozen ABI v1 symbol set.

## State model

Runtime ABI v1 owns one process-wide default context containing:

- model registrations,
- tensor registrations,
- Green AI contracts,
- Green AI measurements,
- inference counters,
- last-inference status, backend, reason, telemetry and bridge evidence,
- generated observability, Prometheus and OTLP-like snapshots.

The context is intentionally shared by all threads in the process. It is not a tenant, request or session isolation boundary.

## Synchronization contract

The packaged `libshorthand_runtime.a` and CMake `shorthand_runtime` target expose the original 25 ABI v1 symbols through `RuntimeThreadSafeFacade.cpp`.

Every public ABI call:

1. acquires one process-wide recursive mutex,
2. executes the complete underlying operation while holding that mutex,
3. releases the mutex only after state and evidence updates are complete.

This makes public operations serialized and linearizable. Registration, reset, queries, inference attempts, counters and evidence snapshots cannot concurrently mutate the underlying containers or strings.

Backend execution invoked through the public runtime ABI is also serialized in ABI v1. Parallel backend execution is not claimed by this contract.

## String-returning functions

The underlying runtime stores mutable process-wide strings. The public façade copies each returned string into thread-local snapshot storage before releasing the lock.

For each string-returning function, the returned pointer:

- belongs to the calling thread,
- remains valid until the same function is called again on that thread or the thread exits,
- is not invalidated by another thread changing runtime state,
- must not be freed by the caller.

Callers should copy the value when it must outlive the next call on the same thread.

## Reset and lifecycle

`short_runtime_reset()` is an exclusive serialized operation. When it returns:

- all process-wide registrations and measurements are cleared,
- all inference counters are zero,
- last-inference state returns to the documented initial values,
- future calls observe the reset state.

A reset may run concurrently with other public calls, but the mutex defines a total order. It does not cancel an operation that already holds the runtime lock.

## Isolation boundary

ABI v1 does not expose context handles. Therefore:

- independent tenants must use separate processes,
- independent tests should call `short_runtime_reset()` before and after use,
- one thread cannot create a private registry invisible to another thread,
- a future handle-based context API would require an additive API design and separate versioned evidence.

This tested explicit limitation satisfies the current enterprise requirement without silently changing the frozen ABI.

## Compatibility design

`ShorthandRuntime.cpp` is compiled under private `shimpl_*` names only for the packaged runtime library. `RuntimeThreadSafeFacade.cpp` owns the frozen 25 public `short_*` symbols.

The private symbols are implementation details and are not part of ABI v1. Direct source-level test harnesses that compile `ShorthandRuntime.cpp` continue to work as before, while distributed runtime artifacts use the synchronized façade.

## CI evidence

- `tests/runtime/test_runtime_state_thread_safety.sh`
- `scripts/check_runtime_state_thread_safety.sh`
- `abi/runtime_public_symbols_v1.txt`
- `scripts/check_runtime_abi_api_stability.sh`

The test validates:

1. concurrent model, tensor, contract and measurement registration,
2. exact final counts after concurrent operations,
3. concurrent no-execution inference attempts with honest counters,
4. concurrent JSON, Prometheus and OTLP-like snapshot reads,
5. thread-local string snapshot stability while another thread changes last-inference state,
6. reset ordering and zero-state recovery,
7. unchanged ABI v1 public symbol count.

## Claim boundary

Passing this gate means the existing process-wide ABI is data-race protected and behaviorally serialized for calls made through the packaged runtime library.

It does not mean:

- tenant-level state isolation exists,
- backend inference executes in parallel,
- lock-free progress is guaranteed,
- cross-process shared state exists,
- the language is production-ready by itself.
