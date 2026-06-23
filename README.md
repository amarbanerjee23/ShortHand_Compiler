# ShortHand Compiler

ShortHand is a C++/LLVM-first programming-language research artifact with a legacy interpreter, LLVM IR/bitcode/native back end, optional C++ AI runtime integration, and a standalone C++ Green AI evidence tool.

## What this repository is

* A compiled-language artifact centered on C++, Flex/Bison, and LLVM.
* A research platform for language implementation, AI-runtime integration, and Green AI evidence reporting.
* A work-in-progress artifact with explicit validation gates and known limitations.

## What this repository is not

* It is not a production-ready compiler.
* It does not claim literal “0 bugs.” The target release claim is **no known bugs under full validation** after every mandatory validation command passes.
* Python is not required for official compiler, runtime, Green AI, validation, report-generation, or test workflows. Deprecated Python helpers remain under `deprecated/` only for historical context.

## Quickstart

```bash
bash setup_build_infra.sh
source ./shorthand_env.sh
short_hand Compiler_new_ws/Short_Hand/examples/greenai_report.short run
```

Required tools are a C++17 compiler, `make`, Flex, Bison, LLVM with `llvm-config`, `llc`, and `clang` for native output.

ShortHand does not commit Flex/LLVM/`llc` binaries into the repository. See `docs/toolchain_policy.md` for the toolchain installation and CI caching policy.

## Build and test

```bash
make -C Compiler_new_ws/Short_Hand/src compiler green_ai_tool
bash scripts/validate_language.sh --strict
bash scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
```

CMake is also available as a modern entry point:

```bash
cmake -S . -B build
cmake --build build
```

## Optional AI runtimes

ONNX Runtime and LibTorch are optional. They are enabled only when roots are supplied:

```bash
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
export LIBTORCH_ROOT=/path/to/libtorch
make -C Compiler_new_ws/Short_Hand/src ai_app ai_train
```

Without ONNX Runtime, `ai_infer` paths must report clear fallback diagnostics. Without LibTorch, training remains a runtime demo and not a complete ShortHand language feature.

## Green AI workflow

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
Compiler_new_ws/Short_Hand/build/green_ai_tool validate examples/green_ai/image_classification.greenai --strict strict
Compiler_new_ws/Short_Hand/build/green_ai_tool report examples/green_ai/image_classification.greenai --output /tmp/short_hand_green_report.json --strict strict
Compiler_new_ws/Short_Hand/build/green_ai_tool check examples/green_ai/image_classification.greenai --baseline /tmp/short_hand_green_report.json --threshold-percent 10
```

Reports are evidence reports only; they do not grant certification.

## Documentation map

* `docs/architecture.md` — artifact map and source layout.
* `docs/language_spec.md` — language syntax and required semantics.
* `docs/semantics.md` — operational semantics and safety policy.
* `docs/compiler_pipeline.md` — compiler phases and runtime integration.
* `docs/green_ai_certification.md` — Green AI evidence workflow.
* `docs/reproducibility.md` — clean-checkout reproduction commands.
* `docs/evaluation_plan.md` — benchmark and measurement plan.
* `docs/known_limitations.md` — current limitations and non-overclaims.
* `docs/public_release_readiness.md` — mandatory release gate.

## Benchmarks

Benchmark scaffolding is under `benchmarks/`. Run:

```bash
bash scripts/run_benchmarks.sh
bash scripts/collect_energy.sh
```

Energy scripts report unavailable measurement tools honestly and never fabricate values.

## Current status

Ready for internal engineering review only. See `docs/known_limitations.md` before making external publication or release claims.

## GreenAI compiled language core

ShortHand now includes first-class model, tensor, inference, GreenAI contract, GreenAI measurement, semantic validation, deterministic fallback runtime, and C3-ECO-aligned evidence constructs. The official implementation path is C++/LLVM-first and does not require Python. Optional AI SDKs (ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, Eigen, OpenBLAS) are selected through build roots/options and are not vendored. When they are absent, fallback evidence reports `runtime_backend: fallback`, `inference_status: not_executed`, and `reason: backend_not_available`.

Evidence artifacts are developer evidence reports only and include: `Evidence report only; this tool does not grant certification.`

## AI library abstraction

ShortHand users write `model`, `tensor`, and `infer` declarations; the C++ runtime chooses TensorRT, ONNX Runtime, OpenVINO, LibTorch, llama.cpp, or deterministic fallback from the model `backend_preference`. The preferred inference syntax is `infer classifier(image) -> result;`; the legacy `>` form is temporarily accepted for compatibility. Optional SDKs are not vendored and are not required for the default build. Missing SDKs never claim inference success: fallback reports `runtime_backend=fallback`, `inference_status=not_executed`, and `reason=backend_not_available`.

Backend enablement is explicit: ONNX Runtime uses `ONNXRUNTIME_ROOT`, TensorRT uses `TENSORRT_ROOT` plus `CUDA_ROOT`, OpenVINO uses `OPENVINO_ROOT`, LibTorch uses `LIBTORCH_ROOT`, and llama.cpp uses `LLAMACPP_ROOT`. Evidence remains developer evidence only and does not grant certification.
