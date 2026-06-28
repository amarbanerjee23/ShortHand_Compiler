# ShortHand Compiler

ShortHand is a C++/LLVM-first programming-language research artifact. It combines a legacy interpreter, LLVM IR/bitcode/native code generation, first-class Green AI evidence constructs, and an optional C++ AI runtime abstraction for model/tensor/inference workloads.

The implementation goal is to keep the language syntax simple for end users while allowing advanced C/C++ AI libraries such as ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, Eigen, and OpenBLAS to be integrated behind a stable compiler/runtime abstraction.

## What this repository is

- A compiled-language artifact centered on C++17, Flex, Bison, Make/CMake, and LLVM.
- A research platform for language implementation, AI-runtime integration, and Green AI evidence reporting.
- A C++/LLVM-first codebase. Python is not required for official compiler, runtime, Green AI, validation, report-generation, or test workflows.
- A work-in-progress artifact with explicit validation gates and known limitations.

## What this repository is not

- It is not a production-ready compiler.
- It does not claim literal "0 bugs." The target release claim is **no known bugs under full validation** after every mandatory validation command passes.
- It does not vendor large third-party AI SDK binaries into the repository.
- It does not grant Green AI certification. It emits developer evidence reports only.

## Repository layout

```text
.
├── Compiler_new_ws/Short_Hand/src/        # compiler, parser, visitors, AI runtime, Green AI tool
├── Compiler_new_ws/Short_Hand/examples/   # ShortHand examples
├── docs/                                  # architecture, language, semantics, release notes
├── scripts/                               # validation, smoke tests, benchmark helpers
├── tests/                                 # language, AI runtime, evidence, regression tests
├── benchmarks/                            # benchmark scaffolding
├── setup_build_infra.sh                   # one-command local/CI setup validation
└── CMakeLists.txt                         # modern CMake entry point
```

## Required toolchain

The official build path requires:

- Linux or another POSIX-like development environment
- C++17 compiler, tested with `g++`
- `make`
- Flex and the Flex runtime library (`libfl-dev` on Ubuntu)
- Bison
- LLVM with `llvm-config`
- `llc`
- `clang`
- CMake and Ninja for the CMake workflow

On Ubuntu 24.04, the CI installs the expected tools with:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential make g++ flex bison libfl-dev \
  llvm llvm-dev clang clang-format \
  cmake ninja-build
```

ShortHand does not commit Flex, LLVM, `llc`, compiler, or AI SDK binaries into the repository. See `docs/toolchain_policy.md` for the toolchain installation and CI caching policy.

## Quickstart

From a clean checkout:

```bash
bash setup_build_infra.sh
source ./shorthand_env.sh
short_hand Compiler_new_ws/Short_Hand/examples/greenai_report.short run
```

The setup script verifies required tools, checks Flex runtime linkage, builds mandatory compiler/Green AI targets, runs strict language validation, and executes smoke tests.

## Build and test

Use these commands before opening or merging a PR:

```bash
make -C Compiler_new_ws/Short_Hand/src compiler green_ai_tool
bash scripts/validate_language.sh --strict
bash scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
```

CMake is also available:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## CI validation gates

The GitHub Actions CI is expected to run the following gates:

1. Install required dependencies.
2. Verify required tools are on `PATH`.
3. Run `bash setup_build_infra.sh`.
4. Run strict language validation.
5. Run smoke tests.
6. Run the Makefile test suite.
7. Run sanitizer tests.
8. Configure and build through CMake.
9. Run CTest.
10. Upload CI artifacts when configured.

A PR should not be considered merge-ready until these gates pass on the PR branch.

## AI runtime backend abstraction

ShortHand supports simple AI workload declarations using `model`, `tensor`, and `infer` constructs. The user-facing syntax hides backend-specific details while the compiler/runtime layer selects the available C++ backend.

Example:

```short_hand
model classifier {
    format onnx;
    path "models/classifier.onnx";
    task "image_classification";
    precision int8;
    input_shape "1,3,224,224";
    output_shape "1,1000";
    backend_preference tensorrt, onnxruntime_tensorrt, onnxruntime_cuda, openvino, onnxruntime_cpu, fallback;
    compact true;
    quality_guardrail accuracy >= 90;
};

tensor image float "1,3,224,224";

infer classifier(image) -> result;
```

The abstraction is implemented around shared AI runtime types, model/tensor specifications, backend capabilities, backend selection, and inference result metadata. This keeps language-level syntax stable while allowing optional native AI libraries to be wired in behind the runtime layer.

## Supported AI concepts

ShortHand currently models these AI concepts at the language/runtime boundary:

- Model declaration: model name, format, path, task, precision, input/output shapes, compact mode, backend preference, and quality guardrail.
- Tensor declaration: tensor name, element type, and shape.
- Inference statement: `infer model(input) -> output;`.
- Backend preference ordering.
- Runtime backend reporting.
- Inference status reporting.
- Fallback behavior when optional SDKs are unavailable.
- Evidence output for AI-related runtime decisions.

## Optional AI backends

Optional AI SDKs are disabled by default. The fallback backend always builds without external SDKs.

| Backend family | Purpose | Default behavior |
| --- | --- | --- |
| TensorRT | GPU inference through TensorRT engines or ONNX Runtime TensorRT EP | Disabled unless SDK/root is configured |
| ONNX Runtime CPU/CUDA/TensorRT | ONNX model execution | Disabled unless `ONNXRUNTIME_ROOT` is configured |
| OpenVINO | OpenVINO IR execution | Disabled unless OpenVINO root/options are configured |
| LibTorch | TorchScript/C++ tensor runtime experiments | Disabled unless `LIBTORCH_ROOT` is configured |
| llama.cpp | GGUF/LLM runtime experiments | Disabled unless llama.cpp root/options are configured |
| Eigen/OpenBLAS | Optional numerical kernels | Disabled unless configured |
| Fallback | Deterministic no-SDK path for validation/evidence | Always available |

Example local configuration:

```bash
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
export LIBTORCH_ROOT=/path/to/libtorch
make -C Compiler_new_ws/Short_Hand/src ai_app ai_train
```

Default builds define optional backend feature macros as disabled, for example:

```text
SHORTHAND_HAS_ONNXRUNTIME=0
SHORTHAND_HAS_TENSORRT=0
SHORTHAND_HAS_OPENVINO=0
SHORTHAND_HAS_LIBTORCH=0
SHORTHAND_HAS_LLAMACPP=0
SHORTHAND_HAS_OPENBLAS=0
SHORTHAND_HAS_EIGEN=0
```

When optional SDKs are absent, the runtime must fail honestly with fallback diagnostics instead of pretending that real inference happened.

Expected fallback evidence includes values such as:

```json
{
  "runtime_backend": "fallback",
  "inference_status": "not_executed",
  "reason": "backend_not_available"
}
```

## Green AI workflow

Build the standalone Green AI evidence tool:

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
```

Run validation/report/check workflows:

```bash
Compiler_new_ws/Short_Hand/build/green_ai_tool validate examples/green_ai/image_classification.greenai --strict strict
Compiler_new_ws/Short_Hand/build/green_ai_tool report examples/green_ai/image_classification.greenai --output /tmp/short_hand_green_report.json --strict strict
Compiler_new_ws/Short_Hand/build/green_ai_tool check examples/green_ai/image_classification.greenai --baseline /tmp/short_hand_green_report.json --threshold-percent 10
```

Reports are evidence artifacts only. They include the disclaimer:

```text
Evidence report only; this tool does not grant certification.
```

## Evidence and claims policy

ShortHand evidence reporting follows an evidence-only policy:

- Report measured or declared values honestly.
- Do not fabricate energy, carbon, accuracy, latency, or runtime values.
- Report unavailable measurement tools as unavailable.
- Preserve the difference between executed inference and fallback/non-executed inference.
- Keep certification language out of generated reports unless an external certification body grants it.

## Benchmarks

Benchmark scaffolding is under `benchmarks/`:

```bash
bash scripts/run_benchmarks.sh
bash scripts/collect_energy.sh
```

Energy scripts must report unavailable measurement tools honestly and must not fabricate values.

## Developer rules for compiler/runtime changes

- Keep official compiler/runtime code C++/LLVM-first.
- Do not add Python as a required build, validation, runtime, or report-generation dependency.
- Do not vendor large third-party SDKs or binaries into this repository.
- Preserve deterministic fallback behavior when optional AI SDKs are absent.
- Keep semantic validation strict enough to catch invalid model formats, invalid precision, invalid shapes, missing quality guardrails, and incompatible backend preferences.
- Keep parser, scanner, generated parser artifacts, Makefile, and CMake wiring consistent when changing language syntax.
- Use clear diagnostics instead of silent fallback when user input is invalid.
- Avoid C++ exceptions in compiler/runtime paths compiled with LLVM flags, because `llvm-config --cxxflags` can inject `-fno-exceptions` in CI. Use no-throw validation helpers for parser/semantic/runtime validation code.

## Important examples

```bash
# Green AI report example
short_hand Compiler_new_ws/Short_Hand/examples/greenai_report.short run

# AI backend abstraction example
short_hand Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short run
```

The AI abstraction example is allowed to use fallback diagnostics when real SDKs are not configured.

## Documentation map

- `docs/architecture.md` — artifact map and source layout.
- `docs/language_spec.md` — language syntax and required semantics.
- `docs/semantics.md` — operational semantics and safety policy.
- `docs/compiler_pipeline.md` — compiler phases and runtime integration.
- `docs/green_ai_certification.md` — Green AI evidence workflow.
- `docs/reproducibility.md` — clean-checkout reproduction commands.
- `docs/evaluation_plan.md` — benchmark and measurement plan.
- `docs/known_limitations.md` — current limitations and non-overclaims.
- `docs/public_release_readiness.md` — mandatory release gate.
- `docs/toolchain_policy.md` — external toolchain and dependency policy.

## Current status

ShortHand is ready for internal engineering review only. See `docs/known_limitations.md` before making external publication, certification, or production-readiness claims.
