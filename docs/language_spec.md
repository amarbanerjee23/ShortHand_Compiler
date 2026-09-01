# ShortHand Language Specification

Language version: beta-0.7

Conformance contract: beta-0.7

Base grammar version: beta-0.2

language_contract_status: parser_accurate_executable_matrix

production_claim: false

Historical base marker retained for compatibility gates: Language version: beta-0.2

Historical stability marker retained for earlier gates: Language version: beta-0.1

ShortHand is a C++ and LLVM-first compiled Green AI language. Python is not required for the official compiler, runtime, validation, conformance, tests or evidence path.

## Beta-0.7 objective

Beta-0.7 is the active layered language contract. It combines the beta-0.2 base grammar, beta-0.3 modules, beta-0.4 executable types, beta-0.5 functions/control flow, beta-0.6 enterprise schemas/packages/core FFI, the `shorthand.c3eco.language.v1` candidate-evidence declarations, and the typed `shorthand.c3eco.profile.v2` link and migration surface.

The matrix covers:

- lexical tokens and comments,
- ordered program structure,
- primitive declarations and arrays,
- typed function definitions and calls,
- blocks, assignments, conditions, loops, labels, goto, read, print, break, continue and return,
- every binary operator and unary minus,
- tensor, model and inference syntax,
- legacy inference compatibility forms,
- Green AI contract, measurement and report syntax,
- beta-0.3 module, import and package declarations,
- beta-0.4 float, string and typed-array execution,
- beta-0.5 expression calls, lexical declarations, recursion, cleanup and resolved labels,
- beta-0.6 enterprise composite schemas, ownership plans, offline packages and safe FFI,
- beta-0.7 typed C3-ECO profile links, domain validation and deterministic migration,
- first-class C3-ECO candidate-evidence declarations,
- explicit parser and execution boundaries that remain unsupported.

## Parser-only validation

```text
short_hand source.short parse
```

`parse` validates lexical and grammar acceptance and then exits before semantic analysis, interpretation or lowering. It exists so grammar conformance can be tested independently from model precision support, shape checks, backend compatibility and Green AI completeness.

Parser-only acceptance is not evidence that a program is semantically valid or executable. Normal `run`, `print`, `compile`, `compile-bc`, `compile-native` and evidence modes still apply semantic analysis.

## Program structure

A beta-0.5 source file contains an optional beta-0.3 module/package preamble followed by the compatible base program body:

1. one or more primitive declaration statements,
2. zero or more function definitions,
3. one or more logic statements.

Primitive declarations and functions have an ordered top-level position. AI and Green AI constructs are logic statements. Blocks and the top-level logic section are non-empty.

Function parameter declarations remain semicolon-terminated for compatibility. Empty parameter lists are accepted. Calls take zero or more arbitrary expressions and evaluate them from left to right. Typed declarations inside function bodies and nested braced blocks have lexical scope; nested blocks may shadow outer names, while same-scope redeclaration is rejected.

## Modules, packages and deterministic resolution

Beta-0.3 accepts `module`, `import` and `package` preamble declarations. Legacy `shorthand.package.v1` and `shorthand.lock.v1` remain compatible. Beta-0.6 adds `shorthand.package.v2` and `shorthand.lock.v2` for exact semantic versions, allowlisted SPDX licenses, package namespaces, local vendored dependencies, SHA-256 module locks and SPDX dependency output. Resolution is offline-only and confined to declared roots. No registry client is implied.

## AI model, tensor and inference syntax

A tensor declaration records a canonical precision token and a shape string. A model declaration records format, path, task, precision, input and output shapes, ordered backend preferences, compact intent and an integer `>=` quality guardrail.

Canonical model formats are:

`onnx`, `engine`, `torchscript`, `openvino_ir`, `gguf`.

Canonical precision tokens are:

`float`, `fp16`, `fp32`, `bf16`, `fp64`, `int8`, `int4`.

Canonical backend tokens are:

`tensorrt`, `onnxruntime_tensorrt`, `onnxruntime_cuda`, `onnxruntime_cpu`, `openvino`, `libtorch`, `llamacpp`, `fallback`.

Aliases understood by lower-level runtime normalization are not automatically language keywords. For example, `float32`, `f32`, `onnxruntime`, and `llama.cpp` are not parser tokens in beta-0.2.

Inference uses:

```short
infer model_name(input_tensor) -> output_tensor;
```

The earlier `>` arrow remains accepted as a compatibility form. Legacy `ai_infer(...)` and `aiinfer(...)` calls remain accepted with three string arguments.

Semantic analysis validates model and tensor declarations, shapes, backend compatibility, required guardrails and inference references. Fallback never reports successful execution without a real backend.

## Green AI and C3-ECO syntax

Beta-0.2 includes:

- `greenai_contract name { ... };`
- `greenai_measure name { ... };`
- `greenai("workload", inferences, watts, seconds);`

Contract syntax supports functional unit, success criteria, boundary, MQ/DQ levels, location carbon factor, energy and carbon budgets, quality guardrail, evidence retention and `claims_mode evidence_only`.

Measurement syntax is intentionally permissive at parse time. Semantic and evidence stages interpret `inferences`, `watts`, `seconds` and `backend`.

Evidence mode remains:

```text
short_hand program.short evidence [--output report.json]
```

Evidence output includes the mandatory disclaimer: `Evidence report only; this tool does not grant certification.`

Beta-0.3 also provides the ten first-class `shorthand.c3eco.language.v1` declarations documented in `docs/c3eco_language_contract.md`: `certification`, `functional_unit`, `workload`, `boundary`, `measurement_plan`, `ai_lifecycle`, `rag_pipeline`, `token_budget`, `model_routing` and `guardrails`. These declarations preserve candidate metadata and reject unsafe self-certification fields. They do not calculate a certification score, issue a certificate or prove measured energy/carbon performance.

Beta-0.7 adds `certification_profile name { ... };`. It uses native string, identifier, integer, decimal and boolean field values to link one identity, functional unit, workload, boundary, AI lifecycle, guardrail set and validity window. Legacy v1 blocks remain accepted and `c3eco-migrate` emits a fail-closed review manifest instead of guessing typed values. The complete contract is `docs/c3eco_certification_profile.md`.

## Executable type boundary

`int`, `bool`, binary64 `float`/`double`, immutable `string` and fixed numeric or boolean arrays are covered by `docs/execution_semantics_beta_0_4.md`. `float` and `double` name the same binary64 type. Assignments, function arguments, returns, operators and indices are checked exactly, with no implicit narrowing. String literals are general expressions and string equality is content-based.

`shorthand.type_memory.v1` also defines deterministic descriptors for slices, records, enums, options and results plus a guarded ownership state machine. Beta-0.6 exposes these as ABI schemas and ownership plans through `enterprise-check`; it does not expose composite execution in the base parser, interpreter or LLVM lowering. Arrays of owned strings and by-value composite FFI remain rejected. Schema acceptance must not be presented as executable support.

## Enterprise schema and FFI surface

```text
short_hand schema.enterprise.short enterprise-check
```

This mode requires `language shorthand.enterprise_language.v1;`, a dotted namespace, at least one record/enum/slice/option/result declaration and balanced ownership operations. It emits deterministic JSON and never sets a production claim. The installable core library provides the separate `shorthand_core_ffi_v1.h` C ABI and `shorthand::core::v1` C++ wrapper described in `docs/enterprise_packages_stdlib_ffi.md`.

## Functions and structured control flow

`shorthand.control_flow.v1` defines expression calls, recursion, lexical local storage, structured returns and labels. Non-void functions must return on every structured path. `break` and `continue` remain loop-only. A `goto` target must be a unique label in the same statement block, and a jump cannot cross a local declaration boundary. These restrictions prevent jumps from bypassing initialization or lexical cleanup.

Interpreter frames and LLVM lexical symbol tables implement the same shadowing and lifetime rules. The guarded positive program must produce identical output in interpreter, `lli` and native modes. Stable SHD3017 through SHD3023 diagnostics cover duplicate or undefined labels, illegal scope transfer, missing returns, duplicate declarations, void-value consumption and undefined functions.

## Compatibility and boundaries

Beta-0.7 is additive over valid beta-0.6, beta-0.5, beta-0.4, beta-0.3, beta-0.2 and beta-0.1 fixtures. Existing accepted programs remain in the conformance manifest. The former empty-parameter-list rejection remains intentionally superseded by the beta-0.5 function contract.

The following are explicitly outside the current grammar contract:

- empty programs,
- empty blocks,
- `for` syntax,
- parsed `while` syntax,
- slice, record, enum, option/result and ownership syntax in the executable base grammar,
- implicit numeric conversions,
- non-`>=` quality guardrail operators,
- precision and backend aliases that are not scanner keywords,
- production registry publication and remote dependency resolution,
- general-purpose or by-value composite C/C++ FFI beyond core ABI 1.0.0.

These are documented constraints, not production-readiness claims. Parser robustness, deterministic multi-file resolution, the guarded beta-0.5 executable subset and the beta-0.6 enterprise schema/package/core contract are implemented. Composite execution remains a production-lowering boundary and must preserve or explicitly version `shorthand.type_memory.v1`.

## Runtime and ABI boundary

Beta-0.7 does not change the frozen runtime ABI, which remains version `1.0.0` with exactly 25 public `short_*` symbols. Core FFI ABI 1.0.0 is separately versioned and frozen.
