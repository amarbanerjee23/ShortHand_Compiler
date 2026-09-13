# ShortHand generated MLIR dialect

mlir_contract: shorthand.mlir.v1
qualified_dialect_scope: linux-x64-llvm18
lowering_status: implemented_linux_x64_llvm18
production_claim: false

PR92 provides a TableGen-generated dialect library and `shorthand-opt` driver.
The registered parser and verifier reject unknown ShortHand operations by default.
The C++ namespace is `shorthand::ir`, avoiding shadowing upstream `mlir` names.
Generated `.inc` files live only in the build tree; the `.td` files are authoritative.
The public textual contract is version 1.0.0 and replaces the uncompiled scaffold.
There was no released MLIR syntax or ABI to preserve from that scaffold.

## Build and qualify

The qualified toolchain is LLVM/MLIR 18.x on Linux x64, C++17, CMake 3.20 or newer,
Ninja, Python 3, llvm-lit and FileCheck from the same LLVM installation. Ubuntu
24.04 provides `llvm-18-dev llvm-18-tools libmlir-18-dev mlir-18-tools`.
Missing dependencies fail the mandatory gate.

```sh
LLVM_CONFIG=/usr/bin/llvm-config-18 bash scripts/check_mlir_dialect.sh
build-mlir/shorthand-opt mlir/examples/ai_greenai_pipeline.mlir
cmake --install build-mlir --prefix /path/to/sdk
```

A standalone build uses `cmake -S mlir -B build-mlir -G Ninja
-DMLIR_DIR=/usr/lib/llvm-18/lib/cmake/mlir`. The root build exposes
`-DSHORTHAND_BUILD_MLIR=ON`; the existing runtime SDK remains independently
buildable on its qualified platforms. Linux x64 release staging includes this
MLIR package. MLIR is not yet qualified on the other compiler platforms.
The installed package requires the exact LLVM/MLIR patch version it was built
against because upstream C++ ABIs are not stable across releases.

Downstream consumers use `find_package(ShortHandMLIR 1 CONFIG REQUIRED)` and
link `ShortHandMLIR::Dialect`. Public headers and TableGen sources are installed
under `include/ShortHand/IR`. LLVM's license is included with the installed SDK.
See `mlir/test/consumer` for generated operation builders, checked type and
attribute construction, registration and parsing. Pass `llvm::StringRef` to
attribute `getChecked` overloads, as required by the generated LLVM 18 API.

## Version 1 IR contract

| Construct | Representation and verification |
| --- | --- |
| `!shorthand.model<input, output>` | Static, unencoded, non-scalar ranked tensor signatures; positive dimensions; bounded int64 element and byte counts. |
| Tensor element types | `f32`, `f16`, `bf16`, signless `i4`, `i8`, `i32`; type support is an IR contract, not an execution claim. |
| `#shorthand.backend<name>` | One explicit backend policy from fallback, onnxruntime_cpu, onnxruntime_cuda, onnxruntime_tensorrt, tensorrt, openvino, libtorch, llamacpp. |
| `#shorthand.evidence<MQ, DQ, mode>` | Declared MQ0-MQ4/DQ0-DQ4 with `evidence_only` mode; these declarations do not establish evidence quality. |
| `shorthand.model` | Model symbol, typed signature, canonical format, path, task, quality guardrail and compatible backend. |
| `shorthand.tensor` | Defined dense tensor data and a matching ranked SSA result; no uninitialized data. |
| `shorthand.infer` | One SSA input and output; a model symbol resolved in the nearest symbol table; exact signature agreement. |
| `shorthand.greenai_contract` | Contract symbol, nonempty unit/criteria/guardrail, unique nonempty boundary components, evidence attribute, positive finite carbon factor and finite nonnegative budgets. |
| `shorthand.greenai_measure` | Resolved contract, backend, positive int64 inference count, finite positive watts/seconds and finite positive computed joules. |

Canonical model formats are `onnx`, `tensorrt_engine`, `torchscript`,
`openvino_ir`, and `gguf`. Source spelling aliases are normalized by the source
lowering bridge. Backend/format compatibility is checked, but availability is
an execution-time responsibility. No path is opened by verification.

Carbon factor units are gCO2e/kWh; budgets use joules and gCO2e. Zero budgets are
valid strict budgets. Boundary names are extensible declared components, not a
claim that the certification boundary is complete. Measurements are declared
activity, not instrument-backed workbook records. The PR88-PR91 evidence tools
remain responsible for typed profiles, provenance, independent replay and
candidate claim controls. No operation grants certification or authorizes a
comparative energy claim.

Model and contract symbols are retained. Inference and measurement operations
conservatively retain side effects so DCE and CSE cannot discard or merge them.
Dense tensor materialization is pure. Location information survives printing
and parsing. Unknown operations are rejected by the default driver; the upstream
expert `--allow-unregistered-dialect` option is never used in qualification.

## Mandatory evidence

The gate executes lit/FileCheck and expected diagnostics, positive/negative
parser boundaries, custom/generic/bytecode/location round trips, observable-effect
preservation, checked and unchecked C++ API verification, four standalone public
headers, a relocated installed CMake consumer, installed TableGen regeneration,
eight-file freshness comparison, four incremental definition mutations and
missing-dependency rejection. CI runs GCC plus Clang ASan/LSan/UBSan with leak
detection enabled. Make and CTest include the same gate. CodeQL compiles the
generated dialect and driver in addition to existing compiler/evidence targets.

## Executable lowering

GitHub PR94 implements original roadmap PR93; merged GitHub PR93 is the separate gap assessment. Source/SDK SemanticIR lowers through verified ShortHand/LLVM dialects to LLVM IR. `ShortHandMLIR::Lowering` exports the public verifier, conversion and emission API. The source commands are `emit-mlir` and `compile-mlir` when built with `SHORTHAND_BUILD_MLIR=ON`.

`!shorthand.value` represents nominal scalar-payload records, enums, option/result and borrowed numeric slices. `shorthand.value`, `shorthand.project` and `shorthand.update` check exact storage, nominal identity, tags and field indices. Inference lowering supports float32 buffers and checks every frozen runtime ABI status and output count. Other tensor formats remain IR-only for inference.

The mandatory gate adds source/module/composite differential execution, independent SDK and malformed-IR rejection, relocated lowering consumers, real ONNX CPU output, O0/O2 evidence retention and runtime fault cases. TST024 closes only for Linux x64/LLVM18. See the [lowering contract](../docs/mlir_lowering.md) for supported syntax, safety bounds, ownership restrictions and remaining release work. No certification or energy-superiority claim is implied.
