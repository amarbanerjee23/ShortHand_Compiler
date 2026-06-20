# Reproducibility

Supported validation platforms are Linux x64, Linux ARM64, macOS Intel, and macOS ARM64 when C++17, Flex, Bison, LLVM/`llvm-config`, `llc`, `clang`, CMake, and Ninja are installed.

Commands:

```bash
bash setup_build_infra.sh
source ./shorthand_env.sh
bash scripts/validate_language.sh --strict
bash scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional components must be disclosed: set `ONNXRUNTIME_ROOT` for ONNX inference and `LIBTORCH_ROOT` for training demos. If absent, optional tests are reported as environment-limited; mandatory checks fail in strict mode when required tools are missing.
