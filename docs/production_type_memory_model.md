# ShortHand production type and memory model

type_system_contract: shorthand.type_memory.v1
language_version: beta-0.4
implementation_pr: 84
current_maturity: controlled_beta
production_claim: false
c3eco_alignment: evidence_integrity_and_no_quality_degradation

## Purpose

This contract defines stable type identities, checked storage arithmetic and ownership-state rules for the production-language work. It also defines the beta-0.4 executable subset implemented across the interpreter, LLVM bitcode and native modes. The contract is evidence of a guarded implementation boundary, not a claim that the full planned language or product is production ready.

## Canonical types

`ProductionTypeSystem` provides descriptors for `void`, `bool`, signed 32-bit `int`, IEEE-754 binary64 `float`, UTF-8 byte-string values, fixed arrays, borrowed slices, records, enums, options and results. Composite descriptors reject empty identities, duplicate fields or variants, zero extents and unsupported payloads. Their canonical names are deterministic and exact type identity is required for assignment.

The beta-0.5 executable source surface directly executes scalar `int`, `bool`, `float`/`double`, `string` and fixed arrays of numeric or boolean scalars. Beta-0.6 exposes records, enums, option/result and slices as versioned ABI schemas plus ownership plans through `enterprise-check`. They are not yet executable parser/interpreter/LLVM values. This boundary prevents later lowering or FFI work from inventing incompatible identities or lifetime rules without presenting schema validation as composite execution.

Logical `bool` values have the canonical observable values 0 and 1. LLVM may use `i1` for transient predicates, but storage, function ABI and variadic calls use a zero-extended `i32` representation. Lowering performs this conversion only at exact boolean boundaries; it is not an implicit numeric source conversion.

## C3-ECO and sustainable AI alignment

Exact types, checked extents and deterministic failures protect the correctness, repeatability and evidence-integrity safeguards in the repository's C3-ECO candidate profile. A measurement, functional unit, quality result or carbon factor must not change meaning through silent narrowing, unchecked overflow, out-of-bounds access or use-after-move. These controls support G6 security, G7 safety/privacy/accessibility, G8 repeatability and G13 no-quality-degradation evidence, but they do not prove lower energy use or grant certification. Any efficiency claim still requires equivalent useful work, measured or transparently estimated energy/carbon data, provenance and uncertainty under the separate C3-ECO gates.

## Numeric and conversion rules

- Integer behavior remains the checked beta-0.3 contract.
- `float` and `double` are one source-level binary64 type in beta-0.4.
- Numeric arithmetic requires both operands to have the same type.
- Remainder is defined only for integers.
- Floating division by zero fails with `SHD7001` before an engine can diverge.
- Assignments, arguments and returns require exact types.
- `implicit_numeric_narrowing: forbidden`
- The checked float-to-int primitive rejects NaN, infinity and values outside the signed 32-bit range with `SHD3012`. No implicit source conversion syntax is enabled in beta-0.4.

## Strings

String literals are immutable expression values. String variables use observable value semantics in the interpreter and one pointer-sized immutable handle in LLVM. The handle points only to compiler-owned, process-lifetime storage; mutation, deallocation and user-owned buffers are not part of beta-0.4. Printing and equality are defined across all three execution modes. Ordered comparison, arithmetic and unbounded input into strings are rejected. Arrays of owned strings remain explicitly unsupported until element destruction and move behavior are connected to generated code.

## Arrays and slices

Fixed arrays retain their extent as part of their type. Array storage size uses checked multiplication and fails with `SHD3011` on overflow. Every index must be an integer. Negative or out-of-range indices fail with `SHD7002` in interpreter, bitcode and native execution.

Slices are non-owning `(owner, offset, length)` views. Range construction uses subtraction-safe bounds validation. `slice_bounds_failure: SHD7002`

## Ownership state machine

The reusable ownership checker has five states: `uninitialized`, `owned`, `shared_borrowed`, `mutably_borrowed` and `moved`. It enforces:

- reads only from initialized, non-moved values;
- any number of shared borrows or one exclusive mutable borrow;
- no move or assignment while borrowed;
- no owner access during an active mutable borrow;
- deterministic release transitions;
- reinitialization after move;
- `use_after_move: SHD3016`

The current scalar source language uses value semantics, so these state transitions are not exposed as new syntax. Owned composites introduced by later PRs must reuse this checker or version the contract.

## Stable diagnostics

| Code | Contract |
| --- | --- |
| `SHD3010` | Invalid type descriptor or source type construction. |
| `SHD3011` | Storage extent or layout overflow. |
| `SHD3012` | Invalid explicit conversion. |
| `SHD3013` | Assignment, argument, return or operand type mismatch. |
| `SHD3014` | Operator is not defined for the operand type. |
| `SHD3015` | Condition is not `bool` or `int`. |
| `SHD3016` | Ownership or lifetime state violation. |
| `SHD7001` | Integer or floating arithmetic-domain failure. |
| `SHD7002` | Fixed-array or slice bounds failure. |

## Evidence

`tests/types/test_production_type_memory_model.cpp` exercises descriptor validation, canonical identity, storage overflow, checked conversion, slice bounds, exclusive borrowing and move safety under strict C++ warnings with both GCC and Clang in the primary CI lane. `scripts/check_enterprise_packages_stdlib_ffi.sh` connects every guarded descriptor family and the ownership state machine to the beta-0.6 schema source. `scripts/check_semantic_differential.sh` keeps typed executable programs in exact interpreter, LLVM and native agreement. Sanitizer suites compile the same compiler sources, and CI treats missing tools as failure.

The frozen runtime ABI remains version 1.0.0 with its existing 25 public `short_*` symbols. Beta-0.6 adds a separate core FFI ABI 1.0.0 with its own exact symbol manifest; it does not add or rename a runtime ABI symbol.
