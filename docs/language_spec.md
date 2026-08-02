# ShortHand Language Specification

Language version: beta-0.2

Conformance contract: beta-0.2

language_contract_status: parser_accurate_executable_matrix

production_claim: false

Historical stability marker retained for earlier gates: Language version: beta-0.1

ShortHand is a C++ and LLVM-first compiled Green AI language. Python is not required for the official compiler, runtime, validation, conformance, tests or evidence path.

## Beta-0.2 objective

Beta-0.2 converts the earlier grammar draft into an executable language contract. The accepted scanner and parser surface is documented in `docs/language_grammar_ebnf.md` and traced by `tests/conformance/grammar_matrix_beta_0_2.tsv`.

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
- explicit parser boundaries that remain unsupported.

## Parser-only validation

```text
short_hand source.short parse
```

`parse` validates lexical and grammar acceptance and then exits before semantic analysis, interpretation or lowering. It exists so grammar conformance can be tested independently from model precision support, shape checks, backend compatibility and Green AI completeness.

Parser-only acceptance is not evidence that a program is semantically valid or executable. Normal `run`, `print`, `compile`, `compile-bc`, `compile-native` and evidence modes still apply semantic analysis.

## Program structure

A beta-0.2 source file contains:

1. one or more primitive declaration statements,
2. zero or more function definitions,
3. one or more logic statements.

Primitive declarations and functions have an ordered top-level position. AI and Green AI constructs are logic statements. Blocks and the top-level logic section are non-empty.

Function parameter declarations are semicolon-terminated. Empty parameter lists are not accepted by the current parser and remain an explicit beta boundary.

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

## Green AI syntax

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

## Compatibility and boundaries

Beta-0.2 is additive over valid beta-0.1 fixtures. Existing accepted programs remain in the conformance manifest.

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
- module, import and package syntax.

These are documented constraints, not production-readiness claims. PR67 covers parser robustness and malformed-input hardening. PR68 and PR69 cover modules, imports and packages.

## Runtime and ABI boundary

Beta-0.2 changes the compiler CLI and conformance contract only. It does not change the frozen runtime ABI, which remains version `1.0.0` with exactly 25 public `short_*` symbols.
