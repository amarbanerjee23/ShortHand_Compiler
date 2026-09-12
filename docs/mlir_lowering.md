# Executable SemanticIR and MLIR lowering

mlir_lowering_contract: shorthand.mlir.lowering.v1
qualified_lowering_scope: linux-x64-llvm18
enterprise_execution_contract: shorthand.enterprise_language.v2
production_claim: false

GitHub PR94 implements original roadmap PR93. Merged GitHub PR93 is the separate [enterprise AI/C3-ECO audit](ENTERPRISE_AI_C3ECO_GAP_ASSESSMENT.md). Future roadmap IDs are tracked separately in [the release plan](production_readiness_pr_plan.md).

## Build and use

Build with LLVM/MLIR 18, Clang 18, Flex, Bison, OpenSSL and Ninja. The mandatory gate acquires the checksum-pinned real ONNX CPU SDK and runs every source, SDK and runtime qualification:

```sh
LLVM_CONFIG=/usr/bin/llvm-config-18 bash scripts/check_mlir_dialect.sh
build-mlir-source/short_hand tests/mlir_lowering/composites.short emit-mlir --output composites.mlir
build-mlir-source/short_hand tests/mlir_lowering/composites.short compile-mlir --output composites.ll
clang-18 composites.ll -O2 -o composites
./composites
build-mlir/shorthand-opt composites.mlir --lower-shorthand-to-llvm --canonicalize --cse
```

The root CMake option `SHORTHAND_BUILD_MLIR=ON` enables these additive commands. `emit-mlir` emits verified ShortHand/LLVM dialect IR with source locations; `compile-mlir` emits textual LLVM IR. Link programs using AI operations against the matching `shorthand_runtime` library and ONNX Runtime CPU SDK. Source-only scalar/composite programs need no AI runtime. Existing `compile`, `run`, package and evidence modes retain their contracts. A build without MLIR rejects these commands with SHD6002. Invalid input is buffered and rejected before opening the output artifact.

## Compiler and SDK contract

After existing parsing, module/lock resolution and semantic validation, `SemanticIRBuilder` copies the AST into an owned `ProgramIR`. Its public header depends only on the C++ standard library. The SDK independently validates user-constructed IR before lowering; it does not trust the source frontend to have run.

Installed C++ consumers use `find_package(ShortHandMLIR 1 CONFIG REQUIRED)` and link `ShortHandMLIR::Lowering`. Include `ShortHand/SemanticIR.h` and `ShortHand/Conversion/Lowering.h`. `verifySemanticIR`, `lowerSemanticIR`, `lowerToLLVM` and buffered `emitProgram` expose the stages. Installed packages require the exact LLVM/MLIR patch version used to build them. `mlir/test/consumer/lowering.cpp` exercises this API from a relocated install.

| Surface | Executable scope |
| --- | --- |
| Core source | Exact int32, bool, float64 and immutable strings; fixed numeric/bool arrays; globals/lexical locals; calls and recursion; conditionals, loops, break/continue, return, safe same-block goto, read and print. |
| Enterprise v2 source | Explicit language/namespace preamble, named records/enums/option/result/slices, typed constructors and fields, owned bindings, move, shared/mutable borrow, release, record update and global numeric-array views. |
| Composite SDK | Nominal by-value records, enum tags, option/result tagged scalar payloads and global-array borrowed slices; function parameters/results and record updates. |
| AI | Static initialized tensors and single-input/output float32 inference via checked frozen runtime ABI 1.0.0. Public SemanticIR supplies tensor data and TensorIndex. Legacy source tensor declarations remain zero-initialized and do not yet expose typed tensor access syntax. |
| Evidence | Model/contract metadata, units, boundaries, quality guardrails, budgets, declared activity and source identity survive conversion. `llvm.used` retains metadata through LLVM optimization. |

The v2 enterprise entry syntax is intentionally small. It does not combine legacy source function/control/AI syntax into one grammar; the public SemanticIR supports composing those values. The v1 `enterprise-check` schema language and external core/runtime ABIs are unchanged. Example:

```text
language shorthand.enterprise_language.v2;
namespace example;
record Evidence { float64 energy_j; bool quality_passed; };
owned Evidence initial = Evidence(12.5, true);
move initial to verified;
borrow mutable verified as writer;
set writer.energy_j = 10.25;
release writer;
print verified.energy_j, verified.quality_passed;
```

## Safety and optimization

Independent validation checks exact types and nominal identities, arity, symbols, storage, finite tensor initializers, shape overflow, scope, labels and runtime ABI conflicts. Input limits are 100000 semantic nodes, depth 256, 65536 elements per array/tensor and 256 composite fields/variants. Source parsing also retains the existing file/token/depth limits.

Int32 arithmetic wraps except guarded division; division by zero and MIN/-1 report SHD7001. Bounds failures report SHD7002, zero loop step SHD7003 and a non-void SDK function falling through SHD7004. Invalid tags and inactive option/result payloads report SHD7031. Slice construction checks offset/length without overflow; indexing checks range before access. Source borrows must be released and owners cannot move/mutate incompatibly. SDK slices can borrow only unshadowed global arrays, preventing stack-backed escape. Allocas are placed at function entry to avoid loop-driven stack growth.

The conversion pass checks legality and runtime ABI signatures before canonicalization/CSE. Unused unsupported operations cannot disappear into a false-success conversion. Every inference registration, execution status and output count is checked; missing/corrupt models and unavailable execution fail with SHD7030. Inference and declared measurement remain observable even when their numeric results are unused.

## Qualification and limits

Mandatory qualification includes source/module interpreter and LLVM/native equivalence, O0/O2 outputs, invalid ownership and bounds, SDK mutation rejection, malformed hand-written MLIR, runtime ABI spoofing, relocated installed consumers, actual ONNX CPU output 42, telemetry counters and optimized evidence retention. GCC and Clang ASan/LSan/UBSan lanes execute these checks. CodeQL includes the bridge and lowering; root CTest and the 21-target Make/CTest parity suite invoke the same required gate.

TST024 is implemented only for Linux x64/LLVM18. Other MLIR platforms, nested/heap-owning composites, owned string arrays, by-value composite FFI and stack-escaping slices remain unsupported. Low-level hand-written LLVM pointers remain a trusted SDK boundary; the language does not validate arbitrary foreign pointers. Preexisting `llvm.used` with ShortHand metadata is rejected to prevent ambiguous retention. Representative application integration is the next roadmap increment.

The identity model proves runtime handoff and numerical preservation, not representative AI performance or lowest carbon footprint. Declared watts/seconds and MQ/DQ attributes are not instrument-backed measurements. PR88-PR91 profile/workbook/assessment/auditor contracts remain responsible for evidence preparation. No lowering operation grants certification or a comparative energy claim. The PR93 audit's workload, measurement, lifecycle, independent-review and operational blockers remain in the roadmap.
