# ShortHand_Compiler
A compiler for a Short Hand Programme Language

> **Status:** compiler + optional native AI extensions (ONNX Runtime inference, LibTorch training).

## Language objectives implemented

The ShortHand language in this repository is designed around:

- **Structured programming primitives**: variable declarations, assignments, arithmetic/logical expressions, and statement blocks.
- **Control flow coverage**: `if`, `if/else`, `loop` (range-style and condition-style), labels, and `goto`.
- **I/O support**: `read` and `print` statements.
- **Functions**: function definitions (`def`) with typed parameters and calls.
- **Execution targets**:
  - AST interpretation (`run`)
  - AST pretty-print (`print`)
  - LLVM IR generation (`compile`)
  - LLVM bitcode generation (`compile-bc`)
  - Native binary generation via LLVM toolchain (`compile-native`)

These objectives are encoded in the grammar under `src/scanner_parser/parser.yy` and implemented in AST visitors in `src/visitors/`.

## Build

From `Compiler_new_ws/Short_Hand/src`:

```bash
make
```

### Build requirements

- `g++` with C++14 support
- `llvm-config` available in `PATH` (LLVM development package)
- Flex library (`-lfl`)
- `llc` and `clang` in `PATH` if using `compile-native`

> Note: the default build uses committed/generated parser sources (`parser.tab.cc`, `lex.yy.c`) so Flex/Bison binaries are not required for normal builds.

### Regenerate scanner/parser sources (optional)

```bash
make regen
```

This requires both `bison` and `flex` executables installed.

## Running

```bash
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> run
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> print
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile-bc
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile-native
```

## AI-programming direction (binary-first)

Yes — this can be evolved into a language for AI-assisted systems programming without a Python runtime dependency.

- The compiler already has an LLVM backend foundation, which is the right base for binary-first workflows.
- With `compile-native`, `.short` programs can be turned into native executables directly.
- Next recommended steps:
  - add typed arrays/strings and richer function return types,
  - introduce module imports and standard library primitives,
  - expose a stable CLI/JSON diagnostics format so AI agents can compile, inspect errors, and auto-fix code loops.

## GreenAI language primitive

ShortHand now includes a first end-to-end GreenAI reporting primitive in the language itself:

```short
int inferences, watts, seconds;
inferences = 1000;
watts = 50;
seconds = 10;
greenai("edge_model_v1", inferences, watts, seconds);
```

`greenai(name, inferences, watts, seconds);` is available to the interpreter and LLVM IR backend. It reports total energy in joules (`watts * seconds`) and integer inference efficiency (`inferences / energy_j`). See `Compiler_new_ws/Short_Hand/examples/greenai_report.short` for a runnable example.

ShortHand also exposes native model inference through `ai_infer(model_path, shape_csv, input_csv);`. The interpreter routes this statement through the C++ `AI_Runtime` abstraction, so ONNX Runtime-backed inference works when the compiler is built with `ONNXRUNTIME_ROOT`; otherwise it emits the same clear fallback guidance as the standalone `short_ai_app`. See `Compiler_new_ws/Short_Hand/examples/ai_infer.short` for an end-to-end AI + GreenAI program.

## Green AI evidence and eco-regression tooling

ShortHand now includes a dependency-free Green AI manifest workflow for C3-ECO-style evidence generation. Green manifests are sidecar `.greenai` DSL files that declare functional units, system boundaries, carbon factors, energy/carbon budgets, MQ/DQ classes, model metadata, routing/cascade controls, hardware targets, data movement controls, and measured or estimated resource use. Existing `.short` programs remain backward-compatible; green validation is opt-in and controlled by `green_mode: "off" | "advisory" | "strict"`.

```bash
./tools/green_ai_tool.py validate examples/green_ai/image_classification.greenai --strict strict
./scripts/green-report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
./scripts/green-check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
python3 tests/test_green_ai_tool.py
```

See `docs/green_ai_certification.md` and the examples in `examples/green_ai/` for image inference, LLM/RAG inference, and training pipeline manifests.

## State-of-the-art AI runtime integration

This repository now includes optional native integrations for AI workloads:

- **ONNX Runtime inference** (`short_ai_app`) for high-performance model serving.
- **LibTorch training** (`short_ai_train`) for native training workflows.

These are optional and enabled at build time by setting library roots.

### Build AI binaries

From `Compiler_new_ws/Short_Hand/src`:

```bash
# ONNX Runtime inference binary
make ai_app ONNXRUNTIME_ROOT=/path/to/onnxruntime

# LibTorch training binary
make ai_train LIBTORCH_ROOT=/path/to/libtorch
```

### Run AI binaries

```bash
# Inference: <model.onnx> <shape_csv> <input_csv>
./Compiler_new_ws/Short_Hand/build/short_ai_app model.onnx 1,3 0.1,0.2,0.3

# Training: <epochs> <learning_rate> <output_model.pt>
./Compiler_new_ws/Short_Hand/build/short_ai_train 200 0.01 trained_model.pt
```

If dependencies are not enabled, the binaries fail with a clear message explaining which root variable must be set.

## Full framework setup (resources + links)

This section centralizes all resources needed to install and run the framework end-to-end.

### Official dependency links

- LLVM project: https://llvm.org/
- `llvm-config` docs: https://llvm.org/docs/CommandGuide/llvm-config.html
- Flex: https://github.com/westes/flex
- Bison: https://www.gnu.org/software/bison/
- ONNX Runtime releases: https://github.com/microsoft/onnxruntime/releases
- ONNX Runtime C/C++ docs: https://onnxruntime.ai/docs/get-started/with-cpp.html
- LibTorch (PyTorch C++ API): https://pytorch.org/cppdocs/
- LibTorch downloads: https://pytorch.org/get-started/locally/

### One-command bootstrap (Linux)

From repo root:

```bash
chmod +x scripts/bootstrap_framework.sh scripts/verify_env.sh
./scripts/bootstrap_framework.sh
./scripts/verify_env.sh
./scripts/smoke_test.sh
```

This script:
1. Installs core packages (`g++`, `clang/llvm`, `flex`, `bison`, etc.) where `apt-get` is available.
2. Downloads ONNX Runtime CPU binaries into `third_party/onnxruntime/`.
3. Downloads LibTorch CPU binaries into `third_party/libtorch/`.
4. Prints environment variables and build commands.
5. `smoke_test.sh` validates optional AI binaries and fallback behavior.

### Manual setup (if you prefer controlled versions)

1. Install build toolchain:
   - `g++`, `cmake`, `clang`, `llvm`, `llvm-config`, `flex`, `bison`, `libfl-dev`.
2. Download ONNX Runtime and set:
   - `export ONNXRUNTIME_ROOT=/absolute/path/to/onnxruntime`
3. Download LibTorch and set:
   - `export LIBTORCH_ROOT=/absolute/path/to/libtorch`
4. Ensure runtime loader paths:
   - `export LD_LIBRARY_PATH=$ONNXRUNTIME_ROOT/lib:$LIBTORCH_ROOT/lib:$LD_LIBRARY_PATH`

### Build matrix

```bash
# Core language compiler
make -C Compiler_new_ws/Short_Hand/src

# AI inference binary (ONNX Runtime)
make -C Compiler_new_ws/Short_Hand/src ai_app ONNXRUNTIME_ROOT=$ONNXRUNTIME_ROOT

# AI training binary (LibTorch)
make -C Compiler_new_ws/Short_Hand/src ai_train LIBTORCH_ROOT=$LIBTORCH_ROOT
```

### Usage matrix

```bash
# Language execution
./Compiler_new_ws/Short_Hand/build/short_hand Compiler_new_ws/Short_Hand/examples/all_features.short run
./Compiler_new_ws/Short_Hand/build/short_hand Compiler_new_ws/Short_Hand/examples/all_features.short print
./Compiler_new_ws/Short_Hand/build/short_hand Compiler_new_ws/Short_Hand/examples/all_features.short compile
./Compiler_new_ws/Short_Hand/build/short_hand Compiler_new_ws/Short_Hand/examples/all_features.short compile-bc
./Compiler_new_ws/Short_Hand/build/short_hand Compiler_new_ws/Short_Hand/examples/all_features.short compile-native

# AI inference
./Compiler_new_ws/Short_Hand/build/short_ai_app model.onnx 1,3 0.1,0.2,0.3

# AI training
./Compiler_new_ws/Short_Hand/build/short_ai_train 200 0.01 trained_model.pt
```

### Notes on “fully implement the language”

The repository currently implements the grammar + interpreter + LLVM IR backend with native/bitcode workflows and optional native AI integrations. “Full implementation” for production-grade use would additionally require:

- complete type-system parity across parser/interpreter/IR (including strict float/bool/string semantic checks),
- module/package system,
- standard library + FFI,
- conformance tests and CI matrix across OS/toolchain versions,
- optimizer and diagnostics hardening.

This README and scripts now provide the complete resource links and installation path needed to build and run the current framework immediately.

## Validation quickstart

From repo root:

```bash
./scripts/verify_env.sh
./scripts/smoke_test.sh
```

The smoke test intentionally validates that missing optional SDKs are handled with clear runtime messages, and that build targets still complete.
