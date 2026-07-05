# Public Release Readiness Gate

A release candidate requires all mandatory commands to pass from a clean checkout:

```bash
bash setup_build_infra.sh
source ./shorthand_env.sh
bash scripts/validate_language.sh --strict
bash scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
```

## Skipped Optional Checks

Default CI may skip optional checks when their local dependencies or host measurement facilities are not present. Skipped items and the missing dependency or facility must be disclosed for:

- `ONNXRUNTIME_ROOT`
- `LIBTORCH_ROOT`
- `RAPL/NVML`
- platform-specific measurement tools

These optional checks must be enabled, run, and backed by retained evidence before any scoped public release or external certification claim that depends on ONNX Runtime execution, LibTorch execution, hardware energy telemetry, GPU telemetry, or host-specific measurement data.

## Claims Policy

Do not make unsupported production, certification, external publication, or absolute defect-freedom claims. The only acceptable reliability statement is evidence-based: no known bugs under the full validation suite, after that suite passes.
