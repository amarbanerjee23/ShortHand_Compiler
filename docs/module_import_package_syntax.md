# ShortHand module, import and package syntax

module_syntax_contract_version: beta-0.3
module_ast_schema: shorthand.module.ast.v1
module_resolution_status: deterministic_resolver_available_via_pr70
multi_file_execution_status: llvm_native_graph_codegen_available_interpreter_calls_deferred_pr71
runtime_abi_change: none
production_claim: false

Historical PR69 scaffold markers retained for old-task stability:

- module_resolution_status: parser_and_ast_scaffold_only
- multi_file_execution_status: deferred_to_pr70

## Scope

PR69 adds an optional source-unit preamble. It gives a ShortHand file a package identity, a module identity and a list of declared imports. The parser records each declaration with its source file and one-based inclusive source range.

PR70 adds the separate deterministic package-resolution contract documented in `docs/module_resolution_and_lockfile.md`. The syntax and AST schema remain beta-0.3; no new source-language syntax is introduced by PR70.

## Grammar

```ebnf
programme          = module_preamble?, beta_0_2_programme ;
module_preamble    = { package_decl | module_decl | import_decl } ;
package_decl       = "package", module_path, ";" ;
module_decl        = "module", module_path, ";" ;
import_decl        = "import", module_path, [ "as", identifier ], ";" ;
module_path        = identifier, { ".", identifier } ;
```

The beta-0.2 declarations, functions, statements, AI constructs and Green AI constructs remain unchanged after the optional preamble.

## Ordering and uniqueness

1. A package declaration is optional at parser-only level and must appear before the module declaration.
2. A source unit using package or import syntax must contain exactly one module declaration.
3. The module declaration must precede all imports.
4. A module path may be imported only once.
5. A non-empty import alias must be unique within the source unit.
6. Empty path segments, leading dots and trailing dots are syntax errors.
7. For PR70 package resolution and executable multi-file modes, every resolved source must declare the package identity from `shorthand.package`.

## Example

```short
package acme.ai;
module acme.recommendation;
import acme.models as models;
import acme.telemetry;

int result;
result = 1;
```

## AST scaffold

`Compiler_new_ws/Short_Hand/src/ast/ModuleAST.h` defines:

- `AST_MODULE_PREAMBLE`,
- package and module identities,
- `AST_IMPORT_DECLARATION`,
- source-file provenance,
- source ranges,
- deterministic JSON inspection.

The local source scaffold can be inspected with:

```text
short_hand source.short module-info
```

The output schema is `shorthand.module.ast.v1`. It deliberately continues to report `resolver_status: not_resolved` because `module-info` is a single-source AST inspection command. It must not be confused with package resolution evidence.

PR70 package graph inspection uses:

```text
short_hand source.short module-graph
```

and the separate `shorthand.module.graph.v1` schema.

## Stable parser diagnostics

| Code | Meaning |
| --- | --- |
| `SHD2011` | Duplicate package declaration |
| `SHD2012` | Duplicate module declaration |
| `SHD2013` | Duplicate import alias |
| `SHD2014` | Duplicate import path |
| `SHD2015` | Invalid package, module or import ordering |
| `SHD2016` | Module declaration required for the preamble |

Malformed dotted paths continue to use the stable parser syntax code `SHD2001`. Resolver diagnostics `SHD2020` through `SHD2030` are documented separately in `docs/module_resolution_and_lockfile.md`.

## Compatibility

Every beta-0.2 accept fixture remains accepted. A source without a preamble produces an empty `shorthand.module.ast.v1` object containing null package and module values and an empty import list.

Beta-0.3 remains an additive language version. PR70 changes the package-resolution/build contract, not source grammar. It does not change the runtime ABI, backend selection, AI execution behavior or Green AI calculations.

## Test evidence

Syntax/AST evidence:

- `tests/conformance/module_matrix_beta_0_3.tsv`
- `tests/modules/valid/module_preamble.short`
- `tests/modules/invalid/`
- `scripts/check_module_ast_scaffold.sh`

Resolution/multi-file evidence:

- `tests/modules/resolver/valid_project/`
- `scripts/check_module_resolution.sh`
- normal, LLVM, negative and sanitizer-backed language suites
