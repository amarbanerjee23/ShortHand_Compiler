# ShortHand language versioning and conformance policy

## Current contract

`shorthand.language.version: beta-0.2`

`shorthand.conformance.contract: beta-0.2`

`shorthand.grammar.matrix: beta-0.2`

`production_claim: false`

Historical markers retained for old-task stability:

- `shorthand.language.version: beta-0.1`
- `shorthand.conformance.contract: beta-0.1`

Beta-0.2 is the current executable language contract. The historical beta-0.1 markers do not identify the active version.

## Purpose

The version marker identifies the public parser and semantic surface that the repository protects. It is not a claim that ShortHand is a complete or production-ready language.

The beta-0.2 contract is grounded in:

- `docs/language_grammar_ebnf.md`,
- `docs/language_spec.md`,
- `tests/conformance/grammar_matrix_beta_0_2.tsv`,
- `tests/conformance/manifest.txt`,
- `scripts/check_grammar_conformance_matrix.sh`,
- `scripts/check_language_versioning.sh`,
- parser, semantic, diagnostics, codegen, runtime and evidence tests.

## What changed from beta-0.1

Beta-0.2 does not introduce a breaking syntax removal. It:

1. replaces an aspirational grammar draft with a parser-accurate grammar,
2. adds a stable parser-only `parse` mode,
3. creates an implementation-linked grammar coverage matrix,
4. adds positive fixtures for core, AI and Green AI syntax,
5. records unsupported grammar boundaries as executable negative fixtures,
6. distinguishes parser acceptance from semantic and execution readiness.

Existing valid beta-0.1 fixtures remain required.

## Conformance layers

### Lexical and grammar conformance

A source conforms syntactically when:

```text
short_hand source.short parse
```

returns zero. This mode stops after parser construction and does not perform semantic validation.

### Semantic conformance

A syntactically accepted source conforms semantically when a normal compiler mode completes semantic analysis without errors. Stable `SHDxxxx` diagnostics remain the semantic rejection contract.

### Lowering and runtime conformance

Semantic acceptance does not imply successful lowering, linking or inference. Codegen, runtime and evidence categories remain separate in the manifest.

### Production conformance

There is no production conformance level yet. Modules, parser robustness, release security, deployment, developer tooling, C3-ECO completion and MLIR lowering remain open roadmap items.

## Compatibility rules

A change is compatible with beta-0.2 when it:

1. keeps every `accept` grammar-matrix fixture accepted by `parse`,
2. keeps every `reject` grammar-matrix fixture rejected unless the language version is intentionally advanced,
3. keeps existing semantic-invalid fixtures rejected,
4. preserves stable diagnostic codes or follows a documented diagnostic migration,
5. preserves runtime and evidence honesty,
6. updates the grammar, specification, matrix, manifest and relevant gates when syntax changes.

A change is breaking when it:

1. removes or renames an accepted keyword or production,
2. changes top-level ordering or statement meaning,
3. changes canonical inference semantics,
4. converts a documented rejected boundary into acceptance without versioning and tests,
5. weakens model, tensor, backend, Green AI or fallback validation,
6. removes a conformance row without replacement and rationale.

Breaking changes require a new language version marker.

## Grammar matrix rules

Every non-header row in `tests/conformance/grammar_matrix_beta_0_2.tsv` has seven tab-separated fields:

```text
id | area | source | anchor | fixture | expectation | rationale
```

The gate requires:

- unique stable IDs,
- all required language areas,
- a scanner, parser or CLI implementation anchor,
- an existing fixture,
- an `accept` or `reject` expectation,
- stable `SHD2001` diagnostics for rejected syntax,
- successful Bison and Flex regeneration with conflicts treated as errors.

The required areas are lexical, program, declarations, functions, statements, expressions, AI, Green AI, compatibility and boundary.

## Manifest rules

Every non-comment manifest entry uses:

```text
category | path | expectation | rationale
```

The active marker is:

`current-version | shorthand.language.version | beta-0.2 | Current executable language contract marker.`

A historical beta-0.1 version row is retained solely because older stability gates assert its exact text. It is not the active version.

The manifest must cover:

- current-version,
- version history,
- grammar-matrix,
- parser-valid,
- parser-invalid,
- semantic-invalid,
- diagnostics,
- codegen,
- runtime,
- evidence.

## Review rule

Any PR that changes scanner keywords, parser productions, semantic meaning, diagnostic contracts, runtime hooks, evidence output or public examples must update the corresponding conformance evidence or explicitly prove why beta-0.2 remains unchanged.
