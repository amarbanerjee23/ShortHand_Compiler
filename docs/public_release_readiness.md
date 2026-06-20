# Public release readiness

ShortHand should not be described as bug-free or production-ready solely because the repository builds in one environment. A public release needs repeatable evidence across the compiler, grammar, runtime, examples, and Green AI tooling.

## Release gate

Before opening ShortHand for broad public use, run the following gate on a machine with the full toolchain installed (`g++`, `bison`, LLVM development files with `llvm-config`, LLVM runtime libraries, and the Flex runtime library):

```bash
./scripts/validate_language.sh
./scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test-green
```

A release candidate is ready only when all commands complete successfully and no step is skipped because of a missing compiler/runtime dependency. If a dependency is unavailable, record that result as environment-limited rather than as a language pass.

## Current quality bar

The validation gate checks:

- the Bison grammar with shift/reduce and reduce/reduce conflicts treated as errors;
- syntax-only C++ compilation for parser, scanner, interpreter, AST printer, AI runtime, Green AI tool, and LLVM IR generator when LLVM is present;
- full ShortHand compiler build when LLVM is present;
- interpreted execution of representative ShortHand examples when the compiler binary is runnable;
- LLVM bitcode/native compilation of the GreenAI language example when the compiler binary is runnable;
- strict validation, reporting, and eco-regression behavior for Green AI manifests.

## Public communication guidance

Use evidence-based language such as "validated by the release gate" rather than absolute claims like "0 bugs." No software project can honestly guarantee zero defects. Known skipped checks, missing dependencies, unsupported platforms, or runtime limitations must be disclosed in release notes.
