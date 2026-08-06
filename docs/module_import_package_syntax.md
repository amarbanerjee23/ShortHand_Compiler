# ShortHand module, import and package syntax

module_syntax_contract_version: beta-0.3
module_ast_schema: shorthand.module.ast.v1
module_resolution_status: parser_and_ast_scaffold_only
multi_file_execution_status: deferred_to_pr70
runtime_abi_change: none
production_claim: false

## Scope

PR69 adds an optional source-unit preamble. It gives a ShortHand file a package identity, a module identity and a list of declared imports. The parser records each declaration with its source file and one-based inclusive source range.

This PR does not locate files, load dependencies, build a package graph, bind imported symbols, create a lockfile or perform multi-file code generation. Those behaviors are reserved for PR70. The `module-info` CLI mode therefore reports `resolver_status: not_resolved`.

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

1. A package declaration is optional and must appear before the module declaration.
2. A source unit using package or import syntax must contain exactly one module declaration.
3. The module declaration must precede all imports.
4. A module path may be imported only once.
5. A non-empty import alias must be unique within the source unit.
6. Empty path segments, leading dots and trailing dots are syntax errors.

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

The scaffold is intentionally separate from symbol resolution. It can be inspected with:

```text
short_hand source.short module-info
```

The output schema is `shorthand.module.ast.v1` and must not claim that an import was found, loaded or executed.

## Stable diagnostics

| Code | Meaning |
| --- | --- |
| `SHD2011` | Duplicate package declaration |
| `SHD2012` | Duplicate module declaration |
| `SHD2013` | Duplicate import alias |
| `SHD2014` | Duplicate import path |
| `SHD2015` | Invalid package, module or import ordering |
| `SHD2016` | Module declaration required for the preamble |

Malformed dotted paths continue to use the stable parser syntax code `SHD2001`.

## Compatibility

Every beta-0.2 accept fixture remains accepted. A source without a preamble produces an empty `shorthand.module.ast.v1` object containing null package and module values and an empty import list.

Beta-0.3 is an additive language version. It does not change the runtime ABI, backend selection, AI execution behavior, Green AI calculations or existing statement semantics.

## Test evidence

- `tests/conformance/module_matrix_beta_0_3.tsv`
- `tests/modules/valid/module_preamble.short`
- `tests/modules/invalid/`
- `scripts/check_module_ast_scaffold.sh`
- normal and sanitizer-backed language suites
