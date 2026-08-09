# ShortHand language versioning and conformance policy

## Current contract

`shorthand.language.version: beta-0.3`

`shorthand.conformance.contract: beta-0.3`

`shorthand.grammar.matrix: beta-0.2-base+beta-0.3-modules`

`production_claim: false`

Historical markers retained for compatibility and old-task stability:

- `shorthand.language.version: beta-0.2`
- `shorthand.conformance.contract: beta-0.2`
- `shorthand.grammar.matrix: beta-0.2`
- `shorthand.language.version: beta-0.1`
- `shorthand.conformance.contract: beta-0.1`

Beta-0.3 is the current executable language contract. Beta-0.2 remains the complete base-language compatibility matrix, and beta-0.3 adds the optional module, package and import preamble. Historical markers do not identify the active version.

## Purpose

The version marker identifies the public parser and semantic surface that the repository protects. It is not a claim that ShortHand is complete or production ready.

The beta-0.3 contract is grounded in:

- the beta-0.2 base grammar in `docs/language_grammar_ebnf.md`,
- the beta-0.2 base specification in `docs/language_spec.md`,
- `docs/module_import_package_syntax.md`,
- `tests/conformance/grammar_matrix_beta_0_2.tsv`,
- `tests/conformance/module_matrix_beta_0_3.tsv`,
- `tests/conformance/manifest.txt`,
- `scripts/check_grammar_conformance_matrix.sh`,
- `scripts/check_module_ast_scaffold.sh`,
- `scripts/check_language_versioning.sh`,
- parser, semantic, diagnostics, codegen, runtime and evidence tests.

## What changed from beta-0.2

Beta-0.3 is additive. It:

1. adds the `package`, `module`, `import` and `as` keywords,
2. adds dotted module paths,
3. adds an optional source-unit preamble before the unchanged beta-0.2 body,
4. records package, module, import, alias, source path and source range information in an AST scaffold,
5. adds stable `SHD2011` through `SHD2016` diagnostics,
6. adds the deterministic `module-info` inspection mode,
7. does not resolve files, bind imported symbols or perform multi-file code generation.

Every beta-0.2 accept fixture remains required and accepted. Runtime ABI 1.0.0 and the 25 public `short_*` symbols are unchanged.

## Conformance layers

### Base lexical and grammar conformance

The complete beta-0.2 grammar matrix remains mandatory. A source conforms to the base grammar when:

```text
short_hand source.short parse
```

returns zero.

### Beta-0.3 module extension conformance

A source using the module extension must also satisfy:

- the ordering and uniqueness rules in `docs/module_import_package_syntax.md`,
- every row in `tests/conformance/module_matrix_beta_0_3.tsv`,
- deterministic `module-info` output with `resolver_status: not_resolved`.

### Semantic conformance

A syntactically accepted source conforms semantically when a normal compiler mode completes semantic analysis without errors. Stable `SHDxxxx` diagnostics remain the rejection contract.

### Lowering and runtime conformance

Parser acceptance of imports does not imply resolution or execution. PR70 will define package manifests, deterministic resolution, lockfiles, symbol visibility and multi-file code generation.

### Production conformance

There is no production conformance level yet. ShortHand remains controlled beta with `production_claim: false`.

## Compatibility rules

A change is compatible with beta-0.3 when it:

1. keeps every beta-0.2 accept fixture accepted,
2. keeps every beta-0.2 reject fixture rejected unless a later language version intentionally changes it,
3. keeps every beta-0.3 module accept fixture accepted,
4. keeps module ordering, duplicate and malformed-path boundaries rejected with stable codes,
5. preserves stable diagnostics or documents a migration,
6. preserves runtime and evidence honesty,
7. updates the applicable grammar, extension matrix, manifest and gates when syntax changes.

A new language version is required when accepted syntax, top-level ordering, semantic meaning or a documented rejection boundary changes.

## Matrix rules

`tests/conformance/grammar_matrix_beta_0_2.tsv` remains the authoritative complete base matrix.

`tests/conformance/module_matrix_beta_0_3.tsv` is the authoritative additive extension matrix. Its rows contain:

```text
id | area | source | anchor | fixture | expectation | rationale
```

Both gates require stable IDs, implementation anchors, existing fixtures, explicit accept or reject expectations, and conflict-free Bison and Flex generation.

## Manifest rules

The active marker is:

`current-version | shorthand.language.version | beta-0.3 | Current executable language contract marker.`

Historical beta-0.1 and beta-0.2 rows are retained for compatibility. The manifest must cover the base grammar matrix, module extension matrix, parser-valid, parser-invalid, semantic-invalid, diagnostics, codegen, runtime and evidence categories.

## Review rule

Any PR that changes scanner keywords, parser productions, semantic meaning, module metadata, diagnostics, runtime hooks, evidence output or public examples must update the corresponding conformance evidence or explicitly prove why the beta-0.3 contract remains unchanged.
