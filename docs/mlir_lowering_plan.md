# ShortHand MLIR lowering plan

mlir_contract: shorthand.mlir.v1

ShortHand source -> parser and AST -> semantic analyzer -> ShortHand semantic IR
-> ShortHand MLIR dialect -> LLVM dialect -> LLVM IR -> bitcode/native binary.

PR92 implements the generated dialect, parser/printer, type/attribute/operation
verifiers, `shorthand-opt`, installable SDK and mandatory executable qualification.
See [the dialect contract](../mlir/README.md) for versioned types, units, operations,
supported toolchain, checked builders and exact test coverage.

PR93 will populate the dialect from validated SemanticIR, preserve source ranges,
normalize source aliases, lower operations through verified passes, implement
composite execution and hand inference to the qualified runtime. Its gates must
cover invalid shapes/ops, differential execution, optimization preservation and
live backend equivalence. The current compiler still uses its existing LLVM path.

TST024 remains partial until those execution gates pass. PR94 qualifies realistic
AI workloads; PR95 provides performance and equivalent-quality measured-energy
evidence. No generated-dialect result alone establishes production readiness.
