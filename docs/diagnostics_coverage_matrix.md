# Diagnostics coverage matrix

diagnostics_coverage_contract_version: 1.1.0
diagnostics_coverage_status: stable_coded_stage_matrix_guarded
diagnostic_code_prefix: SHD
covered_stages: parser, module, semantic, ai, greenai, lowering
source_range_requirement: parser_ast_and_module_diagnostics
warning_delivery_status: printed_without_failing_successful_compilation
lowering_preflight_status: unresolved_calls_rejected_before_ir_generation
runtime_abi_change: none
production_claim_boundary: matrix_is_not_parser_recovery_or_localization_completion

PR65 turned compiler diagnostics into a versioned and testable contract. PR67 added bounded-input and lexical-failure codes. PR69 added module-preamble ordering, uniqueness and completeness diagnostics. PR70 adds a distinct module-resolution stage for manifest, path, graph, lockfile, package identity and multi-file symbol failures. Human-readable messages may improve, but stable codes, stages, severity and source ownership provide an automation-safe interface for CI, editors and future tooling.

## Code allocation

| Range | Stage | Purpose |
| --- | --- | --- |
| `SHD2001` to `SHD2016` | Parser | Syntax, lexical, module-preamble and parser-resource failures |
| `SHD2020` to `SHD2030` | Module | Package manifest, deterministic resolution, graph, lockfile and imported-execution boundary failures |
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

## Module-resolution allocation

| Code | Contract |
| --- | --- |
| `SHD2020` | Package manifest not found |
| `SHD2021` | Package manifest malformed or incomplete |
| `SHD2022` | Imported module absent from the manifest or source tree |
| `SHD2023` | Manifest or resolved module path escapes the package root |
| `SHD2024` | Manifest mapping and source module identity disagree |
| `SHD2025` | Reachable import cycle |
| `SHD2026` | Ambiguous or duplicate manifest mapping |
| `SHD2027` | Source package identity disagrees with the package manifest |
| `SHD2028` | Missing, stale or mismatched deterministic lockfile |
| `SHD2029` | Graph-wide linker-visible symbol collision |
| `SHD2030` | Imported function requested in interpreter mode before PR71 equivalence support |

These codes are exercised by `scripts/check_module_resolution.sh`, including sanitizer-backed paths. Resolver errors carry the initiating source path and a stable source-point range. `SHD2030` is an explicit honesty boundary: PR70 supports imported function binding for LLVM/native compilation but does not pretend the legacy interpreter executes imported calls correctly.

## Output contract

A ranged error is rendered as:

```text
file.short:line:column: error: [SHDxxxx] message [range start_line:start_column-end_line:end_column]
```

Warnings use the same shape with `warning`. Warnings are printed even when a program remains valid, but do not create a failing exit status. Errors remain non-zero and block interpretation, evidence generation or lowering.

## Lowering safety

Undefined function calls are validated before LLVM IR generation and emit `SHD6001`. The compiler rejects an unresolved call before the IR generator can continue with a null or incomplete value. PR70 additionally validates direct-import function visibility and graph-wide symbol uniqueness before multi-file lowering.

## Guarded evidence

The live diagnostics gate proves representative behavior for parser syntax, core semantics, AI warnings, AI shape validation, Green AI contracts and lowering preflight. Parser robustness proves deterministic failure for resource and lexical attacks. The module scaffold gate proves each PR69 parser code and source range. The PR70 resolver gate proves manifest, path, graph, lock and imported-execution failures without sanitizer findings or false-success behavior.

Each error case must fail, each warning-only case must succeed, and all cases must carry the expected stable code and required source provenance.

## Boundaries

The diagnostics contract does not provide IDE-style multi-error recovery, localization, Unicode display-column handling, fix-it suggestions or LSP publication. Cross-module LSP publication remains PR78. ShortHand remains controlled beta until the complete roadmap passes.
