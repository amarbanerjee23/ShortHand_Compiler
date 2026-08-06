# Diagnostics coverage matrix

diagnostics_coverage_contract_version: 1.0.0
diagnostics_coverage_status: stable_coded_stage_matrix_guarded
diagnostic_code_prefix: SHD
covered_stages: parser, semantic, ai, greenai, lowering
source_range_requirement: parser_and_ast_diagnostics
warning_delivery_status: printed_without_failing_successful_compilation
lowering_preflight_status: unresolved_calls_rejected_before_ir_generation
runtime_abi_change: none
production_claim_boundary: matrix_is_not_parser_recovery_or_localization_completion

PR65 turned compiler diagnostics into a versioned and testable contract. PR67 added bounded-input and lexical-failure codes. PR69 adds module-preamble ordering, uniqueness and completeness diagnostics. Human-readable messages may improve, but stable codes, stages, severity and source ownership provide an automation-safe interface for CI, editors and future tooling.

## Code allocation

| Range | Stage | Purpose |
| --- | --- | --- |
| `SHD2001` to `SHD2999` | Parser | Syntax, lexical, module-preamble and parser-resource failures |
| `SHD3001` to `SHD3999` | Semantic | Core language semantic failures |
| `SHD4001` to `SHD4999` | AI | Model, tensor, backend and inference validation |
| `SHD5001` to `SHD5999` | Green AI | Contract, measurement and claim-safety validation |
| `SHD6001` to `SHD6999` | Lowering | Pre-lowering and code-generation readiness failures |

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

## Module-preamble allocation

| Code | Contract |
| --- | --- |
| `SHD2011` | Duplicate package declaration |
| `SHD2012` | Duplicate module declaration |
| `SHD2013` | Duplicate import alias |
| `SHD2014` | Duplicate import path |
| `SHD2015` | Invalid package, module or import ordering |
| `SHD2016` | Module declaration required for package/import use |

These codes are executed by `scripts/check_module_ast_scaffold.sh` in both normal and sanitizer-backed test paths. Malformed dotted paths use `SHD2001`.

## Output contract

A ranged error is rendered as:

```text
file.short:line:column: error: [SHDxxxx] message [range start_line:start_column-end_line:end_column]
```

Warnings use the same shape with `warning`. Warnings are printed even when a program remains valid, but do not create a failing exit status. Errors remain non-zero and block interpretation, evidence generation or lowering.

## Lowering safety

Undefined function calls are validated before LLVM IR generation and emit `SHD6001`. The compiler rejects an unresolved call before the IR generator can continue with a null or incomplete value.

## Guarded evidence

The live diagnostics gate proves representative behavior for parser syntax, core semantics, AI warnings, AI shape validation, Green AI contracts and lowering preflight. Parser robustness proves deterministic failure for resource and lexical attacks. The module scaffold gate proves each PR69 parser code and source range.

Each error case must fail, each warning-only case must succeed, and all cases must carry the expected stable code and source range.

## Boundaries

The diagnostics contract does not provide IDE-style multi-error recovery, localization, Unicode display-column handling, fix-it suggestions or LSP publication. PR69 records imported-source provenance in the AST scaffold, but file resolution and cross-file diagnostic propagation remain PR70. ShortHand remains controlled beta until the complete roadmap passes.
