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

PR 65 turned compiler diagnostics into a versioned and testable contract. PR 67 extends the parser allocation with bounded-input and lexical-failure codes. Human-readable messages may improve, but the stable code, stage, severity and source ownership provide an automation-safe interface for CI, editors and future language tooling.

## Code allocation

| Range | Stage | Purpose |
| --- | --- | --- |
| `SHD2001` to `SHD2999` | Parser | Syntax, lexical and parser-resource failures |
| `SHD3001` to `SHD3999` | Semantic | Core language semantic failures |
| `SHD4001` to `SHD4999` | AI | Model, tensor, backend and inference validation |
| `SHD5001` to `SHD5999` | Green AI | Contract, measurement and claim-safety validation |
| `SHD6001` to `SHD6999` | Lowering | Pre-lowering and code-generation readiness failures |

The authoritative catalogue is `tests/diagnostics/diagnostics_coverage_matrix.tsv`. Every code declared in `DiagnosticCodes.h` must appear exactly once in that file. Duplicate codes and uncatalogued codes fail the guard.

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

These codes are executed by `scripts/check_parser_robustness.sh`; they are not merely catalogue placeholders.

## Output contract

A ranged error is rendered as:

```text
file.short:line:column: error: [SHDxxxx] message [range start_line:start_column-end_line:end_column]
```

Warnings use the same shape with `warning`. Warnings are printed even when the program remains valid, but they do not create a failing exit status. Errors remain non-zero and block interpretation, evidence generation or lowering.

## Lowering safety

Undefined function calls are validated before LLVM IR generation and emit `SHD6001`. This is intentionally a lowering-preflight diagnostic: the compiler rejects an unresolved call before the IR generator can continue with a null or incomplete value.

## Guarded evidence

The live diagnostics gate proves representative behavior for parser syntax, core semantics, AI warnings, AI shape validation, Green AI contracts and lowering preflight. The parser robustness gate additionally proves bounded and deterministic behavior for all PR 67 parser and scanner codes.

Each error case must fail, each warning-only case must succeed, and all cases must carry the expected stable code and source range.

## Boundaries

The parser is now bounded and guarded by a malformed-input corpus, but the diagnostics contract does not provide IDE-style multi-error recovery, localization, Unicode display-column handling, fix-it suggestions, imported-module provenance or LSP publication. Editor publication remains PR 74. ShortHand remains controlled beta until the complete production-readiness roadmap passes.
