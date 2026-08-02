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

PR 65 turns compiler diagnostics into a versioned and testable contract. Human-readable messages may improve, but the stable code, stage, severity and source ownership provide an automation-safe interface for CI, editors and future language tooling.

## Code allocation

| Range | Stage | Purpose |
| --- | --- | --- |
| `SHD2001` to `SHD2999` | Parser | Syntax and parser-production failures |
| `SHD3001` to `SHD3999` | Semantic | Core language semantic failures |
| `SHD4001` to `SHD4999` | AI | Model, tensor, backend and inference validation |
| `SHD5001` to `SHD5999` | Green AI | Contract, measurement and claim-safety validation |
| `SHD6001` to `SHD6999` | Lowering | Pre-lowering and code-generation readiness failures |

The authoritative catalogue is `tests/diagnostics/diagnostics_coverage_matrix.tsv`. Every code declared in `DiagnosticCodes.h` must appear exactly once in that file. Duplicate codes and uncatalogued codes fail the guard.

## Output contract

A ranged error is rendered as:

```text
file.short:line:column: error: [SHDxxxx] message [range start_line:start_column-end_line:end_column]
```

Warnings use the same shape with `warning`. Warnings are printed even when the program remains valid, but they do not create a failing exit status. Errors remain non-zero and block interpretation, evidence generation or lowering.

## Lowering safety

Undefined function calls are validated before LLVM IR generation and emit `SHD6001`. This is intentionally a lowering-preflight diagnostic: the compiler rejects an unresolved call before the IR generator can continue with a null or incomplete value.

## Guarded evidence

The live diagnostic gate proves representative behavior for:

1. parser syntax failure,
2. core semantic failure,
3. AI warning delivery,
4. AI shape validation failure,
5. Green AI contract failure,
6. lowering-preflight failure.

Each error case must fail, each warning-only case must succeed, and all cases must carry the expected stable code and source range.

## Boundaries

This contract does not complete parser recovery, malformed-input fuzzing, diagnostic localization, Unicode display-column handling, fix-it suggestions, imported-module provenance or LSP publication. Parser robustness remains PR 67, while editor publication remains PR 74. ShortHand remains controlled beta until the complete production-readiness roadmap passes.
