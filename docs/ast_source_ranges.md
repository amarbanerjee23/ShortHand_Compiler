# AST source ranges

ast_source_range_contract_version: 1.0.0
ast_source_range_status: parser_propagated_line_column_ranges
source_range_coordinate_system: one_based_inclusive
semantic_diagnostics_status: exact_ast_node_range_preferred
runtime_abi_change: none
production_claim_boundary: source_ranges_do_not_complete_diagnostics_coverage

PR 64 gives parser-produced AST objects stable source ownership without changing the runtime ABI.

## Contract

`SourceRange` contains an inclusive one-based begin and end position. Invalid or synthetic nodes may carry an unknown range. Parser reductions assign ranges to program, declaration, function, block, statement, control-flow, AI, GreenAI, variable, literal, unary and binary expression objects. Aggregate list nodes expand to the complete reduction range when children are appended.

Ranges are stored in a compiler-process registry keyed by AST object identity. The registry is intentionally outside the public runtime ABI because source ownership is a compiler concern. It is synchronized so diagnostic reads remain deterministic if later compiler passes become parallel.

## Lexer and parser behavior

Flex tracks line and column positions across whitespace, comments and multiline input. Bison `%locations` combines token positions into production ranges. Syntax errors print the current token range.

The end coordinate is inclusive. For example, `break;` on line 2 occupies `2:1-2:6`.

## Diagnostic behavior

Semantic diagnostics use `errorAtNode` and `warningAtNode`. This prevents the earlier anchor-search ambiguity where repeated names could point to the first textual occurrence instead of the AST node that caused the diagnostic.

Legacy anchor lookup remains available for code not yet migrated, but PR 65 must finish the diagnostics coverage matrix and remove remaining avoidable anchor-based reporting.

## Boundaries

This PR does not claim complete parser recovery, Unicode display-column handling, macro expansion provenance, imported-module source ownership or full diagnostics coverage. Those remain later roadmap work.
