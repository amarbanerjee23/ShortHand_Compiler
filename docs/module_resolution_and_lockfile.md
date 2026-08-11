# Deterministic module resolution, package manifest and lockfile

module_resolution_contract_version: 1.0.0
package_manifest_schema: shorthand.package.v1
package_lock_schema: shorthand.lock.v1
module_graph_schema: shorthand.module.graph.v1
language_version: beta-0.3
resolution_status: deterministic_manifest_locked_multi_file_codegen
interpreter_imported_call_status: deferred_to_pr71_with_stable_rejection
production_claim: false

## Purpose

PR70 turns the PR69 module/import/package AST scaffold into a deterministic package graph. A ShortHand package does not search the host filesystem, user home directory, environment variables or language-specific global package paths to guess where an import lives. Every resolvable module is explicitly mapped by a package manifest, every resolved source must remain under that package root, and executable module modes require an exact lockfile for the reachable graph.

This is a build and language-integrity contract. It does not change the frozen runtime ABI and it does not make ShortHand production ready by itself.

## Package root and manifest discovery

For a source file that uses a package/module preamble, the compiler walks parent directories from the entry source until it finds the nearest file named:

```text
shorthand.package
```

The directory containing that manifest is the package root. Resolution never continues above that selected package root and does not consult ambient search paths.

## `shorthand.package.v1`

The manifest is intentionally small and deterministic:

```text
format shorthand.package.v1
package acme.demo
module acme.demo.app src/app.short
module acme.demo.lib src/lib.short
```

Rules:

1. exactly one `format shorthand.package.v1` record is required,
2. exactly one package identity is required,
3. at least one module mapping is required,
4. each module name appears at most once,
5. two module names may not map to the same canonical source file,
6. module paths must be relative `.short` paths,
7. absolute paths and `..` traversal are rejected,
8. canonicalized module paths must remain inside the package root,
9. every manifest module must be inside the declared package namespace,
10. a resolved source must declare the same package identity and the exact module identity associated with its manifest path.

The manifest is declarative. It does not execute scripts, interpolate environment variables or download dependencies.

## Resolution algorithm

Starting from the entry module, the compiler:

1. loads the nearest package manifest,
2. validates the entry package/module identity against the manifest,
3. resolves every import by exact module-name lookup,
4. parses each newly reached source through the real ShortHand parser,
5. validates each resolved package/module identity,
6. recursively resolves transitive imports,
7. rejects cycles,
8. computes a deterministic dependency-first topological order,
9. validates graph-wide linker-visible symbol uniqueness,
10. performs module-aware semantic validation before execution or lowering.

Import traversal is deterministic because dependency names are sorted before graph ordering.

## Symbol visibility

PR70 uses an intentionally conservative symbol model while the language is still beta:

- a module's own functions are visible inside that module,
- functions from directly imported modules are visible to that module,
- transitive imports are not silently promoted into the importing module's direct namespace,
- graph-wide function/global name collisions are rejected before LLVM lowering because the current backend uses one linker-visible symbol namespace.

A later language version may add explicit export/private or qualified-name syntax. PR70 does not invent unversioned visibility syntax.

## Lockfile

Generate the lock with:

```text
short_hand path/to/entry.short lock
```

The compiler writes `shorthand.lock` in the package root. Its schema is deterministic:

```text
format shorthand.lock.v1
package acme.demo
entry acme.demo.app
module acme.demo.app src/app.short fnv1a64:...
module acme.demo.lib src/lib.short fnv1a64:...
```

Rows are ordered by module name and include only the graph reachable from the selected entry module. The source fingerprint detects source changes and stale locks. FNV-1a is used here as a deterministic content fingerprint, not as a cryptographic signature or supply-chain authenticity proof. Cryptographic signing, release attestations and protected publication remain PR74.

All executable module modes require the current reachable graph to match `shorthand.lock` exactly. Missing or changed source files, changed reachable modules, changed package identity or a stale fingerprint reject execution/lowering with `SHD2028`.

## Graph inspection

```text
short_hand path/to/entry.short module-graph
```

emits `shorthand.module.graph.v1` JSON containing:

- package identity,
- canonical manifest path,
- entry module,
- lock status,
- resolved modules and canonical source paths,
- direct imports,
- deterministic dependency-first order.

`module-info` remains the PR69 single-source AST inspection mode and continues to report its local scaffold as `resolver_status: not_resolved`; it is not the package graph command.

## Multi-file execution and code generation

### Root `run`

A module program may be run after its full graph and lockfile are verified. PR70 validates every reachable source and executes the entry module's top-level logic. Imported module top-level logic is not implicitly executed.

The legacy interpreter does not yet implement correct callable function semantics. Therefore an entry module that calls an imported function is rejected with `SHD2030` in `run` rather than reporting a false success. PR71 owns interpreter, LLVM and native semantic equivalence, including imported function calls.

### LLVM, bitcode and native

For `compile`, `compile-bc` and `compile-native`, reachable dependency modules are lowered before the entry module. Their global declarations and functions are added to the same LLVM module without generating dependency `main` functions. The entry module alone provides the executable top-level `main` path.

This allows an entry module to bind direct imported functions in generated LLVM/native code while preserving deterministic graph validation and collision checks.

## Stable diagnostics

| Code | Meaning |
| --- | --- |
| `SHD2020` | Package manifest not found |
| `SHD2021` | Invalid package manifest |
| `SHD2022` | Module missing from the manifest or source tree |
| `SHD2023` | Module path escapes the package root |
| `SHD2024` | Manifest/source module identity mismatch |
| `SHD2025` | Import cycle |
| `SHD2026` | Ambiguous manifest mapping |
| `SHD2027` | Source/manifest package mismatch |
| `SHD2028` | Missing or stale lockfile |
| `SHD2029` | Graph-wide symbol collision |
| `SHD2030` | Imported function call requested in interpreter mode before PR71 equivalence support |

## Security properties

The resolver is designed around untrusted package input:

- no implicit host search path,
- no environment-variable path expansion,
- no absolute source paths in the manifest,
- canonical root-confinement checks,
- no `..` traversal,
- no manifest execution hooks,
- deterministic graph traversal,
- bounded parser limits still apply to every resolved source,
- cycle and collision rejection occurs before lowering,
- stale lockfiles fail closed.

## Test evidence

`scripts/check_module_resolution.sh` covers:

- deterministic lock regeneration,
- graph JSON and topological order,
- multi-file root run,
- bitcode generation,
- imported-function native binding and execution,
- honest interpreter rejection for imported calls,
- stale lock rejection,
- missing manifest,
- missing import,
- path escape,
- module identity mismatch,
- package mismatch,
- ambiguous mapping,
- import cycles,
- graph-wide symbol collisions,
- a generated 128-module dependency chain,
- sanitizer-marker rejection.

The same gate is included in compiler, negative, LLVM and sanitizer test paths.

## Boundary after PR70

PR70 closes deterministic resolution, lockfile and multi-file LLVM/native graph construction. It does not close language-wide semantic equivalence, full interpreter function semantics, continuous fuzzing, cross-platform reproducibility, signed publication, production backend qualification, C3-ECO completion, MLIR lowering or measured energy evidence. Those remain explicit PR71 through PR85 work.
