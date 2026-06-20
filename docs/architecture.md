# ShortHand Artifact Map

ShortHand is a C++/LLVM-first compiled-language research artifact. Canonical compiler sources live in `Compiler_new_ws/Short_Hand/src`; the canonical grammar is `scanner_parser/parser.yy` and the canonical lexer is `scanner_parser/scanner.ll`. Generated parser/scanner files are kept only as a bootstrap convenience for environments that lack Flex during initial inspection; release validation regenerates/checks the canonical grammar.

Pipeline: lexer/parser -> AST -> semantic checks (current coverage documented in `known_limitations.md`) -> interpreter or LLVM IR/bitcode/native generation -> optional C++ AI runtime shim -> Green AI evidence tooling.
