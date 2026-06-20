# Compiler Pipeline

1. Flex lexer tokenizes `.short` files from `scanner_parser/scanner.ll`.
2. Bison parser builds the AST from `scanner_parser/parser.yy` with conflicts treated as validation failures.
3. AST classes in `ast/` are visited by the printer, interpreter, and LLVM generator.
4. Semantic analysis is the next required hardening area; invalid programs must stop before LLVM generation.
5. The interpreter executes examples and builtins.
6. The LLVM backend emits IR/bitcode and invokes LLVM tools for native binaries. LLVM 14+ opaque-pointer-safe APIs are the compatibility target.
7. AI runtime code is C++ and optional: ONNX Runtime and LibTorch are used only when roots are supplied.
8. The Green AI tool is standalone C++ and does not require LLVM, Python, ONNX Runtime, or LibTorch.
