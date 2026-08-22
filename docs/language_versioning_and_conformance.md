# ShortHand language versioning and conformance policy

## Current contract

`shorthand.language.version: beta-0.4`

`shorthand.conformance.contract: beta-0.4`

`shorthand.grammar.matrix: beta-0.2-base+beta-0.3-modules+beta-0.4-types`

`shorthand.module.resolution.contract: shorthand.package.v1+shorthand.lock.v1`

`production_claim: false`

Historical markers retained for compatibility and old-task stability:

- `shorthand.language.version: beta-0.2`
- `shorthand.conformance.contract: beta-0.2`
- `shorthand.language.version: beta-0.3`
- `shorthand.conformance.contract: beta-0.3`
- `shorthand.grammar.matrix: beta-0.2`
- `shorthand.language.version: beta-0.1`
- `shorthand.conformance.contract: beta-0.1`

Beta-0.4 is the current executable language contract. Beta-0.2 remains the complete base-language compatibility matrix, beta-0.3 adds the optional module/package/import preamble, and beta-0.4 adds string expressions plus exact float, string and typed-array execution. PR70's deterministic package resolution remains unchanged. Historical markers do not identify the active version.

## Purpose

The version marker identifies the public parser and semantic surface that the repository protects. It is not a claim that ShortHand is complete or production ready.

The beta-0.4 contract is grounded in:

- the beta-0.2 base grammar in `docs/language_grammar_ebnf.md`,
- the beta-0.2 base specification in `docs/language_spec.md`,
- `docs/module_import_package_syntax.md`,
- `docs/module_resolution_and_lockfile.md`,
- `tests/conformance/grammar_matrix_beta_0_2.tsv`,
- `tests/conformance/module_matrix_beta_0_3.tsv`,
- `tests/conformance/type_matrix_beta_0_4.tsv`,
- `docs/production_type_memory_model.md`,
- `docs/execution_semantics_beta_0_4.md`,
- `tests/conformance/manifest.txt`,
- `scripts/check_grammar_conformance_matrix.sh`,
- `scripts/check_module_ast_scaffold.sh`,
- `scripts/check_module_resolution.sh`,
- `scripts/check_language_versioning.sh`,
- parser, module, semantic, diagnostics, codegen, runtime and evidence tests.

## What changed before beta-0.4

Beta-0.3 syntax is additive. It:

1. adds the `package`, `module`, `import` and `as` keywords,
2. adds dotted module paths,
3. adds an optional source-unit preamble before the unchanged beta-0.2 body,
4. records package, module, import, alias, source path and source range information in an AST scaffold,
5. adds stable `SHD2011` through `SHD2016` parser diagnostics,
6. adds the deterministic `module-info` local-source inspection mode.

PR70 keeps that syntax version but defines its package-resolution semantics:

1. `shorthand.package.v1` explicitly maps module identities to package-root-confined `.short` paths,
2. imports resolve only through that manifest,
3. resolved sources must match manifest package/module identity,
4. reachable dependency graphs are cycle-checked and deterministically ordered,
5. executable module modes require an exact `shorthand.lock.v1`,
6. direct imported functions are visible for LLVM/native lowering,
7. graph-wide linker-visible symbol collisions are rejected,
8. `module-graph` exposes deterministic resolution evidence,
9. imported function calls in legacy interpreter mode fail with `SHD2030` until PR71 closes semantic equivalence.

Every beta-0.2 accept fixture remains required and accepted. Runtime ABI 1.0.0 and the 25 public `short_*` symbols are unchanged.

## What changed from beta-0.3

Beta-0.4 changes source meaning and therefore advances the language version. It:

1. makes string literals general expression values;
2. executes `float` and `double` as one binary64 type;
3. executes immutable string scalars, value parameters and returns;
4. executes fixed arrays of numeric and boolean scalars;
5. enforces exact assignment, argument, return, operand and index types;
6. defines float and string operators and deterministic invalid-operator failures;
7. adds stable `SHD3010` through `SHD3016` type, storage, conversion and ownership diagnostics;
8. introduces `shorthand.type_memory.v1` descriptors and guarded ownership states without prematurely exposing composite syntax;
9. advances cross-mode evidence to `shorthand.semantic.differential.v2`.

## Conformance layers

### Base lexical and grammar conformance

The complete beta-0.2 grammar matrix remains mandatory. A source conforms to the base grammar when:

```text
short_hand source.short parse
```

returns zero.

### Beta-0.3 module syntax conformance

A source using the module extension must also satisfy:

- the ordering and uniqueness rules in `docs/module_import_package_syntax.md`,
- every row in `tests/conformance/module_matrix_beta_0_3.tsv`,
- deterministic `module-info` local-source output with `resolver_status: not_resolved`.

The `module-info` marker is intentionally local AST state, not a package-resolution claim.

### Beta-0.3 module resolution conformance

A package-resolved source must satisfy `docs/module_resolution_and_lockfile.md` and `scripts/check_module_resolution.sh`, including:

- an explicit valid `shorthand.package.v1`,
- package-root path confinement,
- exact package/module identity,
- deterministic transitive graph construction,
- cycle and ambiguity rejection,
- graph-wide linker-visible symbol uniqueness,
- deterministic lock generation and stale-lock rejection,
- multi-file LLVM/native binding.

### Semantic conformance

A syntactically accepted source conforms semantically when a normal compiler mode completes module-aware semantic analysis without errors. Stable `SHDxxxx` diagnostics remain the rejection contract. Full cross-mode equivalence remains PR71.

### Lowering and runtime conformance

For package programs, successful LLVM/native lowering requires a verified lockfile and valid graph. Imported module declarations/functions are lowered into the entry LLVM module before the entry program. Dependency top-level logic is not implicitly executed.

Direct imported function calls participate in the same interpreter/LLVM/native differential contract. Historical `SHD2030` remains reserved and must not be reused.

### Beta-0.4 type and memory conformance

A beta-0.4 implementation must satisfy every row in `tests/conformance/type_matrix_beta_0_4.tsv`, the descriptor and ownership unit gate, and exact interpreter/LLVM/native differential behavior. Unsupported owned string arrays and parser-unexposed composites remain fail-closed.

### Production conformance

There is no production conformance level yet. ShortHand remains controlled beta with `production_claim: false`.

## Compatibility rules

A change is compatible with beta-0.4 when it:

1. keeps every beta-0.2 accept fixture accepted,
2. keeps every beta-0.2 reject fixture rejected unless a later language version intentionally changes it,
3. keeps every beta-0.3 module syntax accept fixture accepted,
4. keeps every beta-0.4 type-matrix accept fixture accepted and every rejection boundary fail-closed,
5. keeps module ordering, duplicate and malformed-path boundaries rejected with stable codes,
6. preserves the PR70 deterministic resolver/lock contract or versions that contract explicitly,
7. preserves exact typing, checked storage and ownership-state rules or versions them explicitly,
8. preserves stable diagnostics or documents a migration,
9. preserves runtime and evidence honesty,
10. updates the applicable grammar, extension matrix, package-resolution evidence, manifest and gates when behavior changes.

A new source-language version is required when accepted syntax, top-level ordering or source-level semantic meaning changes. Package/lock and type/memory schema changes require their own explicit schema-version changes even if source grammar remains beta-0.4.

## Matrix rules

`tests/conformance/grammar_matrix_beta_0_2.tsv` remains the authoritative complete base matrix.

`tests/conformance/module_matrix_beta_0_3.tsv` is the authoritative additive syntax extension matrix. Its rows contain:

```text
id | area | source | anchor | fixture | expectation | rationale
```

Package-resolution behavior is executable evidence through `scripts/check_module_resolution.sh`, not a replacement grammar matrix.

`tests/conformance/type_matrix_beta_0_4.tsv` is the authoritative executable type extension matrix. It maps parser, semantic, interpreter, LLVM and reusable type-system obligations to positive or fail-closed fixtures.

## Manifest rules

The active marker is:

`current-version | shorthand.language.version | beta-0.4 | Current executable language contract marker.`

Historical beta-0.1, beta-0.2 and beta-0.3 rows are retained for compatibility. The conformance manifest must cover the base grammar matrix, module extension matrix, type matrix, module resolution, parser-valid, parser-invalid, semantic-invalid, diagnostics, codegen, runtime and evidence categories.

## Review rule

Any PR that changes scanner keywords, parser productions, semantic meaning, module metadata, resolver behavior, package/lock schema, type/memory behavior, diagnostics, runtime hooks, evidence output or public examples must update the corresponding conformance evidence or explicitly prove why the beta-0.4 contract remains unchanged.
