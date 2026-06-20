# Known Limitations

This repository is **not yet a public release candidate**. Current hardening adds validation gates and documentation, but the legacy AST/interpreter still uses raw owning pointers and incomplete type semantics. Full semantic analysis, function return semantics, typed runtime values, complete `continue`/`return` parsing, and comprehensive LLVM/native equivalence tests remain required before external artifact review.

AI training is a C++ runtime demo only unless `LIBTORCH_ROOT` is set; the language does not yet implement full training syntax. ONNX inference is optional and must be enabled with `ONNXRUNTIME_ROOT`. No documentation may claim literal zero bugs; the target claim is no known bugs under full validation once the full suite passes.
