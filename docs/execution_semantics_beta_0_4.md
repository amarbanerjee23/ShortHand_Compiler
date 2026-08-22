# ShortHand beta-0.4 executable semantics

execution_semantics_contract: beta-0.4-pr84-v1
language_syntax_version: beta-0.4
semantic_differential_schema: shorthand.semantic.differential.v2
type_system_contract: shorthand.type_memory.v1
production_claim: false

Beta-0.4 preserves the complete beta-0.3 execution contract and adds exact typed execution for binary64 floating-point values, immutable strings and fixed numeric or boolean arrays. This document defines observable behavior shared by the interpreter, LLVM bitcode and native engines. `docs/production_type_memory_model.md` is authoritative for descriptors, storage arithmetic and ownership states.

## Values and exact typing

The executable scalar types are signed 32-bit `int`, logical `bool`, binary64 `float`/`double` and immutable `string`. `float` and `double` are aliases for one binary64 source type. Variables are zero or empty initialized. Assignments, function parameters and function returns require exact type identity. There are no implicit numeric conversions.

LLVM predicates use `i1` only while evaluating conditions. A `bool` crossing a storage, return, argument or variadic-print boundary is canonically zero-extended to `i32` with value 0 or 1. This explicit boundary rule avoids target-ABI-dependent variadic behavior and is exercised by the Windows, Linux and macOS differential lanes.

Integer behavior, control flow, module visibility and runtime failure behavior remain as specified by beta-0.3. Fixed arrays preserve their element type and extent. Arrays of owned strings are not executable until generated destruction and move semantics exist.

## Floating-point behavior

Floating arithmetic uses LLVM/C++ binary64 operations for addition, subtraction, multiplication, division and unary negation. Relational and equality operators return logical values. Remainder is rejected for floats. Division by positive or negative zero fails with `SHD7001` before the arithmetic instruction executes.

Observable printing uses sufficient decimal precision to round-trip a binary64 value. The differential contract compares the exact checked-in output across interpreter, `lli` and native execution.

## String behavior

String literals are expression values in beta-0.4. Variables and value parameters may hold them. Printing emits their byte content. Equality and inequality compare content. Arithmetic, ordering, logical operators and string input are rejected with a stable semantic diagnostic. The scanner's historical no-escape string boundary is unchanged.

## Conditions and indices

Conditions accept `bool` or `int`; zero is false and non-zero is true. A string or float condition is rejected with `SHD3015`, avoiding engine-specific truthiness. Array indices and counted-loop controls require `int`. Array bounds failures remain `SHD7002` for every element type.

## Differential evidence

`scripts/check_semantic_differential.sh` emits `shorthand.semantic.differential.v2` and requires:

- two positive golden programs in all three execution modes;
- exact float, string, typed-array, argument and return results;
- semantic negatives for unsupported ownership, mismatched types, invalid operators, invalid conditions, invalid indices and existing control/function boundaries;
- exact integer Green AI measurement operands so energy evidence cannot be silently narrowed;
- runtime negatives for integer and floating arithmetic domains, integer and floating array bounds and zero loop steps;
- no sanitizer, undefined-behavior or signal-crash markers.

At the beta-0.4 boundary, record, enum, slice, option/result and ownership syntax, a conversion expression, general function/control-flow syntax, a package registry, a standard library, FFI and any production-readiness claim were unavailable. Beta-0.5 supersedes only the general function/control-flow boundary through `shorthand.control_flow.v1`; the other limitations remain fail-closed for PR86 and later work.
