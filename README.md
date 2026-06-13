# ShortHand_Compiler

ShortHand is a C++/LLVM-first compiled programming language and toolchain for binary-first AI applications and Green AI certification-readiness evidence. Python is no longer required for the official Green AI path: parsing, validation, carbon estimation, report generation, eco-regression checks, inference direction, and training direction are handled by compiled C++ binaries.

## Compiler architecture

The core compiler is implemented in C++14 with Flex/Bison-generated parser sources and LLVM IR/native-code backends.

- Grammar: `Compiler_new_ws/Short_Hand/src/scanner_parser/parser.yy`
- Lexer: `Compiler_new_ws/Short_Hand/src/scanner_parser/scanner.ll`
- Generated parser/scanner: `Compiler_new_ws/Short_Hand/src/parser.tab.*`, `Compiler_new_ws/Short_Hand/src/lex.yy.c`
- AST: `Compiler_new_ws/Short_Hand/src/ast/`
- Visitors: `Compiler_new_ws/Short_Hand/src/visitors/`
- CLI: `Compiler_new_ws/Short_Hand/src/main.cpp`
- Optional AI runtimes: `Compiler_new_ws/Short_Hand/src/ai_runtime/`
- Compiled C++ Green AI tool: `Compiler_new_ws/Short_Hand/src/green_ai/GreenAITool.cpp`

## Build

```bash
# Core compiler; requires LLVM development tools.
make -C Compiler_new_ws/Short_Hand/src

# Compiled C++ Green AI evidence tool; does not require LLVM, Python, ONNX Runtime, or LibTorch.
make -C Compiler_new_ws/Short_Hand/src green_ai_tool

# C++ Green AI tests; no Python required.
make -C Compiler_new_ws/Short_Hand/src test-green
```

Build requirements for the core compiler are `g++`, `llvm-config`, LLVM libraries, and the Flex runtime library. Normal builds use committed generated parser files, so Flex/Bison executables are only needed for `make regen`.

## Running ShortHand programs

```bash
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> run
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> print
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile-bc
./Compiler_new_ws/Short_Hand/build/short_hand <file.short> compile-native
```

## In-language AI and Green AI primitives

ShortHand includes executable AI/GreenAI primitives in `.short` programs:

```short
int inferences, watts, seconds;
inferences = 1000;
watts = 50;
seconds = 10;
ai_infer("models/demo.onnx", "1,3", "0.1,0.2,0.3");
greenai("edge_model_v1", inferences, watts, seconds);
```

- `ai_infer(model_path, shape_csv, input_csv);` routes through the native C++ `AI_Runtime` abstraction. It uses ONNX Runtime C++ when built with `ONNXRUNTIME_ROOT`; otherwise it prints a clear fallback message.
- `greenai(name, inferences, watts, seconds);` computes `energy_j = watts * seconds` and integer `inf_per_j = inferences / energy_j` in interpreter mode and LLVM IR output.

Examples:

- `Compiler_new_ws/Short_Hand/examples/ai_infer.short`
- `Compiler_new_ws/Short_Hand/examples/greenai_report.short`
- `Compiler_new_ws/Short_Hand/examples/green_ai_compiled_pipeline.short`
- `Compiler_new_ws/Short_Hand/examples/ai_train.short`

## Native AI runtime builds

```bash
# ONNX Runtime C++ inference binary.
make -C Compiler_new_ws/Short_Hand/src ai_app ONNXRUNTIME_ROOT=$ONNXRUNTIME_ROOT
./Compiler_new_ws/Short_Hand/build/short_ai_app model.onnx 1,3 0.1,0.2,0.3

# LibTorch C++ training binary.
make -C Compiler_new_ws/Short_Hand/src ai_train LIBTORCH_ROOT=$LIBTORCH_ROOT
./Compiler_new_ws/Short_Hand/build/short_ai_train 200 0.01 trained_model.pt
```

Optional future backend extension points include OpenVINO, TensorRT, llama.cpp/GGML/GGUF, FAISS, OpenCV, Apache Arrow/Parquet, oneDNN/DNNL, XNNPACK, Eigen, SentencePiece, and C++ tokenizers. These remain optional: the core compiler and Green AI tool build without these SDKs.

## Compiled C++ Green AI evidence workflow

The official Green AI workflow is now a compiled native binary:

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
./Compiler_new_ws/Short_Hand/build/green_ai_tool validate examples/green_ai/image_classification.greenai --strict strict
./Compiler_new_ws/Short_Hand/build/green_ai_tool report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
./Compiler_new_ws/Short_Hand/build/green_ai_tool check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
```

Wrapper scripts are preserved and call the compiled C++ binary:

```bash
./scripts/green-report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
./scripts/green-check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
```

`.greenai` sidecar manifests remain useful for audit metadata that should not be hard-coded into executable logic: functional units, boundaries, carbon factors, model necessity, MQ/DQ classes, measurement assumptions, and regression thresholds. They are now parsed and validated by C++.

## Report safeguards

The C++ report generator implements:

- operational carbon: `energy_kwh * grid_factor_kgco2e_per_kwh * 1000.0`
- additive component carbon for separately declared embodied, network, storage, and third-party AI API carbon
- per-functional-unit carbon and energy
- structured budget results: `pass`, `fail`, `not_evaluated`, `unknown`
- strict/advisory/off validation modes
- MQ/DQ validation
- offset separation: offsets are never subtracted from core software carbon footprint
- no fake measurements: missing telemetry is reported as unknown or as user-declared assumptions
- no certification claim: reports state, “Evidence report only; this tool does not grant certification.”

## Why compiled C++ for Green AI?

Compiled C++/LLVM aligns with Green AI because it avoids a Python interpreter dependency, lowers runtime overhead, supports native binaries for edge and embedded deployment, enables LLVM optimization, integrates directly with ONNX Runtime C++ inference and LibTorch C++ training, and provides a software-stack-aware path for measuring and reporting AI energy/carbon behavior.

## Validation quickstart

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
make -C Compiler_new_ws/Short_Hand/src test-green
./scripts/validate_language.sh
./scripts/smoke_test.sh
```

## Documentation

See `docs/green_ai_certification.md` for the Green AI manifest syntax, report schema, validation rules, eco-regression checks, MQ/DQ classes, certification-readiness mapping, and no-greenwashing safeguards.
