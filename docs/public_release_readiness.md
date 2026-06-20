# Public Release Readiness Gate

A release candidate requires all mandatory commands to pass from a clean checkout:

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

Skipped optional checks must be disclosed with the missing dependency (`ONNXRUNTIME_ROOT`, `LIBTORCH_ROOT`, RAPL/NVML, or platform tools). Do not claim production readiness, IEEE TSE readiness, or literal zero bugs. The only acceptable reliability claim is evidence-based: no known bugs under the full validation suite, after that suite passes.
