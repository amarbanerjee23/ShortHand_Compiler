# Diagnostics coverage matrix

diagnostics_coverage_contract_version: 1.4.0
diagnostics_coverage_status: stable_coded_stage_matrix_guarded
diagnostic_code_prefix: SHD
covered_stages: parser, module, semantic, ai, greenai, lowering, runtime
source_range_requirement: parser_ast_module_and_semantic_diagnostics
warning_delivery_status: printed_without_failing_successful_compilation
lowering_preflight_status: unresolved_calls_rejected_before_ir_generation
runtime_failure_status: stable_code_cross_mode_differential_guarded
runtime_abi_change: none
production_claim_boundary: matrix_is_not_parser_recovery_or_localization_completion

PR65 turned compiler diagnostics into a versioned and testable contract. PR67 added bounded-input and lexical-failure codes. PR69 added module-preamble ordering, uniqueness and completeness diagnostics. PR70 added manifest, graph, lockfile and multi-file module diagnostics. PR72 extended the contract to executable core semantics and deterministic runtime failures. PR84 added exact type, storage, conversion, operator, condition and ownership failures. PR85 adds deterministic function, scope, return and label-resolution failures.

## Code allocation

| Range | Stage | Purpose |
| --- | --- | --- |
| `SHD2001` to `SHD2016` | Parser | Syntax, lexical, module-preamble and parser-resource failures |
| `SHD2020` to `SHD2030` | Module | Package manifest, deterministic resolution, graph, lockfile and historical imported-execution boundary failures |
| `SHD3001` to `SHD3999` | Semantic | Core executable-language semantic failures |
| `SHD4001` to `SHD4999` | AI | Model, tensor, backend and inference validation |
| `SHD5001` to `SHD5999` | Green AI | Contract, measurement and claim-safety validation |
| `SHD6001` to `SHD6999` | Lowering | Pre-lowering and code-generation readiness failures |
| `SHD7001` to `SHD7999` | Runtime | Deterministic execution-domain and runtime-invariant failures |

The authoritative catalogue is `tests/diagnostics/diagnostics_coverage_matrix.tsv`. Every code declared in `DiagnosticCodes.h` must appear exactly once. Duplicate codes and uncatalogued codes fail the guard.

## Parser robustness allocation

| Code | Contract |
| --- | --- |
| `SHD2004` | Source-size ceiling exceeded |
| `SHD2005` | Individual token-size ceiling exceeded |
| `SHD2006` | Scanner-work ceiling exceeded |
| `SHD2007` | Delimiter-nesting ceiling exceeded |
| `SHD2008` | Unexpected source byte |
| `SHD2009` | Unterminated block comment |
| `SHD2010` | Unterminated string literal |

These codes are executed by `scripts/check_parser_robustness.sh`.

## Module allocation

`SHD2011` through `SHD2016` cover module-preamble uniqueness and ordering. `SHD2020` through `SHD2029` cover the PR70 package manifest, resolver, graph, lockfile, package identity and graph-wide symbol contracts.

`SHD2030` is retained as a historical compatibility code because PR70 intentionally rejected imported interpreter calls before semantic equivalence existed. PR72 does not delete or reuse the code. The PR70 resolver fixture is instead upgraded to require imported-call interpreter/native equivalence, and the new semantic differential gate compares interpreter, `lli` and native execution.

## PR72 semantic allocation

| Code | Contract |
| --- | --- |
| `SHD3003` | Parser-valid type use remains outside the beta-0.4 executable boundary, including owned string arrays and array parameters |
| `SHD3004` | Function call arity differs from the declared signature |
| `SHD3005` | `return` appears outside a function |
| `SHD3006` | Historical beta-0.4 code for the former unsupported-`goto` boundary; retained and never reused |
| `SHD3007` | Function return usage violates its declared return contract |
| `SHD3008` | Scalar or array reference is undeclared |
| `SHD3009` | Duplicate function definition or conflicting imported function identity |

These codes fail before interpretation or lowering and retain source-aware ranges.

## Runtime allocation

| Code | Contract |
| --- | --- |
| `SHD7001` | Integer divide/remainder domain failure or floating division by zero |
| `SHD7002` | Fixed-array index or slice range outside declared owner bounds |
| `SHD7003` | Counted loop step is zero |
| `SHD7004` | Internal execution invariant failed after semantic validation |

Runtime errors are deterministic language failures, not sanitizer crashes. `scripts/check_semantic_differential.sh` requires the same stable runtime code from interpreter, LLVM bitcode executed with `lli`, and the native binary for the guarded cases.

## PR84 type and memory allocation

| Code | Contract |
| --- | --- |
| `SHD3010` | Type construction is invalid. |
| `SHD3011` | Storage-size calculation overflows or has an invalid extent. |
| `SHD3012` | Explicit checked conversion is outside its defined domain. |
| `SHD3013` | Assignment, argument, return, index or binary operands have incompatible types. |
| `SHD3014` | An operator is undefined for the operand type. |
| `SHD3015` | A branch or condition loop uses a value other than `bool` or `int`. |
| `SHD3016` | An ownership-state transition, borrow or value access violates lifetime rules. |

## PR85 function and control-flow allocation

| Code | Contract |
| --- | --- |
| `SHD3017` | A lexical statement block contains a duplicate label. |
| `SHD3018` | A `goto` target is undefined in the current lexical block. |
| `SHD3019` | A `goto` attempts to leave a lexical block or cross a declaration lifetime boundary. |
| `SHD3020` | A non-void function does not return a value on every structured path. |
| `SHD3021` | A declaration duplicates a name in the same lexical scope. |
| `SHD3022` | A void function result is consumed by a value-requiring context. |
| `SHD3023` | A function call cannot be resolved during semantic analysis. |

## Output contract

A ranged compile-time error is rendered as:

```text
file.short:line:column: error: [SHDxxxx] message [range start_line:start_column-end_line:end_column]
```

Warnings use the same shape with `warning`. Warnings are printed even when a program remains valid, but do not create a failing exit status. Errors remain non-zero and block interpretation, evidence generation or lowering.

Runtime implementations may include engine-specific source rendering, but differential normalization requires the same stable code, non-zero exit category and no crash/sanitizer marker. This keeps diagnostic identity stable without pretending interpreter stack state and generated native machine state have identical presentation internals.

## Lowering safety

Undefined function calls are validated before LLVM IR generation and emit `SHD3023`. `SHD6001` remains the stable lowering-invariant code for an unresolved function that reaches lowering despite semantic validation. PR70 validates direct-import visibility and graph-wide symbol uniqueness before multi-file lowering. PR72 validates function signatures and executable type boundaries, and PR85 completes source-level call and label resolution before execution or lowering.

## Guarded evidence

The live diagnostics gate proves representative behavior for parser syntax, core semantics, undefined functions, AI warnings, AI shape validation and Green AI contracts. Parser robustness proves deterministic resource and lexical failures. Module gates prove package and resolver failures. The PR72 differential gate adds semantic negatives and runtime-domain failures across interpreter, `lli` and native execution. The PR85 control-flow gate covers SHD3017 through SHD3023 in run, bitcode and native compilation modes.

Each error case must fail, each warning-only case must succeed, and every catalogued code must remain unique and stage-owned.

## Boundaries

The diagnostics contract does not provide IDE-style multi-error recovery, localization, Unicode display-column handling, fix-it suggestions or LSP publication. Cross-module LSP publication remains PR79. ShortHand remains controlled beta until the complete production roadmap passes.
