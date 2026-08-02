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

PR 65 adds the stable diagnostics coverage matrix and coded parser, semantic, AI, Green AI and lowering-preflight diagnostics. Legacy anchor lookup remains available only as compatibility support for code outside the guarded matrix.

## Boundaries

Source ranges and the PR65 matrix do not claim complete parser recovery, Unicode display-column handling, macro expansion provenance, imported-module source ownership, localization, fix-it suggestions or LSP publication. Those remain later roadmap work.
