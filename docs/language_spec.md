# ShortHand Language Specification

Language version: beta-0.3

Conformance contract: beta-0.3

Base grammar version: beta-0.2

language_contract_status: parser_accurate_executable_matrix

production_claim: false

Historical base marker retained for compatibility gates: Language version: beta-0.2

Historical stability marker retained for earlier gates: Language version: beta-0.1

ShortHand is a C++ and LLVM-first compiled Green AI language. Python is not required for the official compiler, runtime, validation, conformance, tests or evidence path.

## Beta-0.3 objective

Beta-0.3 is the active layered language contract. It combines the beta-0.2 base grammar, the beta-0.3 module/package extension and deterministic resolver, and the `shorthand.c3eco.language.v1` candidate-evidence declaration extension. The accepted base scanner/parser surface is documented in `docs/language_grammar_ebnf.md` and traced by `tests/conformance/grammar_matrix_beta_0_2.tsv`. Modules are traced by `tests/conformance/module_matrix_beta_0_3.tsv` and `scripts/check_module_resolution.sh`.

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
- first-class C3-ECO candidate-evidence declarations,
- explicit parser and execution boundaries that remain unsupported.

## Parser-only validation

```text
short_hand source.short parse
```

`parse` validates lexical and grammar acceptance and then exits before semantic analysis, interpretation or lowering. It exists so grammar conformance can be tested independently from model precision support, shape checks, backend compatibility and Green AI completeness.

Parser-only acceptance is not evidence that a program is semantically valid or executable. Normal `run`, `print`, `compile`, `compile-bc`, `compile-native` and evidence modes still apply semantic analysis.

## Program structure

A beta-0.3 source file contains an optional module/package preamble followed by the beta-0.2 base program body:

1. one or more primitive declaration statements,
2. zero or more function definitions,
3. one or more logic statements.

Primitive declarations and functions have an ordered top-level position. AI and Green AI constructs are logic statements. Blocks and the top-level logic section are non-empty.

Function parameter declarations are semicolon-terminated. Empty parameter lists are not accepted by the current parser and remain an explicit beta boundary.

## Modules, packages and deterministic resolution

Beta-0.3 accepts `module`, `import` and `package` preamble declarations. `shorthand.package.v1` and `shorthand.lock.v1` define the manifest and lock contracts. Resolution is confined to declared roots, locked by content digest, ordered deterministically, cycle-checked and verified by multi-file interpreter/bitcode/native tests. The extension does not yet provide the production package registry, standard library or stable FFI planned for PR86.

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

## Executable type boundary

`int` and `bool` are the complete cross-mode executable core currently covered by `docs/execution_semantics_beta_0_3.md`. The parser accepts additional primitive declarations such as `float`, `double` and `string`, but parser acceptance is not an executable representation. Unsupported executable types fail semantic analysis with the documented diagnostic rather than silently lowering to another type. PR84 owns the production type and memory model.

## Compatibility and boundaries

Beta-0.3 is additive over valid beta-0.2 and beta-0.1 fixtures. Existing accepted programs remain in the conformance manifest.

The following are explicitly outside the current grammar contract:

- empty programs,
- empty blocks,
- empty function parameter lists,
- `for` syntax,
- parsed `while` syntax,
- arbitrary expression arguments in function calls,
- string literals as general expressions,
- non-`>=` quality guardrail operators,
- precision and backend aliases that are not scanner keywords,
- production registry publication and remote dependency resolution,
- a stable standard library or general-purpose C/C++ FFI.

These are documented constraints, not production-readiness claims. Parser robustness, modules and deterministic multi-file resolution are implemented. PR84-PR86 own the remaining type, control-flow, package, standard-library and FFI boundaries.

## Runtime and ABI boundary

Beta-0.3 does not change the frozen runtime ABI, which remains version `1.0.0` with exactly 25 public `short_*` symbols.
