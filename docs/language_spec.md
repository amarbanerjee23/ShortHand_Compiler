# ShortHand Language Specification

ShortHand source files use `.short`. Whitespace separates tokens. Comments are `// ...`, `# ...`, and implementation-dependent block comments when supported by the lexer. Identifiers begin with a letter or underscore and continue with letters, digits, or underscores. Literals include integers, strings with C-style escapes, booleans `true`/`false`, and planned float literals.

Types are `int`, `float`, `bool`, `string`, `void`, and fixed-size arrays (`int a[10]`). Tensor types are experimental and not part of the mandatory language.

Expressions include arithmetic (`+ - * / %`), comparisons, logical operators, unary minus, variables, array indexing, calls, and literals. Division/modulo by zero is a runtime error under validation.

Statements include declarations, assignment, blocks, `if`/`else`, `while`, `loop`, `break`, `continue` (specified), `return` (specified), `read`, `print`, retained labels/goto with validation restrictions, `greenai(...)`, and `ai_infer(...)`.

Functions use `def <type> name(<typed parameters>) { ... };`. Functions should bind parameters in a fresh call frame and return values compatible with the declared type. Recursion is intended to be allowed after semantic analysis is complete.

Errors must include file, line, column, and actionable diagnostics. Undeclared variables, redeclarations, type mismatches, invalid returns, invalid array indices, invalid Green AI calls, and unavailable AI runtimes must not segfault.

Compilation targets are `run`, `print`, `compile` (IR and bitcode), `compile-bc`, and `compile-native`. Existing examples are treated as compatibility tests.
