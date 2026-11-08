# Bounded source tensor values

Source tensor initialization, flattened indexing and mutation are available for
static FP32 tensors with 1 through 65,536 elements. `float` and `fp32` both select
FP32 tensor storage. Ordinary `float` variables retain their existing Float64
semantics; reading a tensor element widens its stored FP32 value to Float64.

```text
int status;
tensor input fp32 "1,2" = [1.25, -2];
tensor output float "1,2";
input[1] = input[0] + 0.5;
print input[0], input[1], output[0];
```

The existing grammar's ordinary declaration section is retained (`int status;`
in this example). Initializers contain signed integer or decimal literals and
must supply exactly the flattened element count. Empty lists, nonfinite values,
values outside the finite FP32 range, nonpositive or overflowing dimensions,
dynamic shapes and other element types reject when this value surface is used.
Shape strings accept the same surrounding whitespace and optional positive sign
as existing runtime shape parsing. Exponent notation is not introduced.

An uninitialized declaration fills its storage with zeros. An initialized
declaration copies its literal values, rounded to FP32. Each execution of the
declaration resets its storage. Indices are ordinary integer expressions;
out-of-range indices, including negatives, report `SHD7002`. Assignments evaluate
the right-hand side before the index, matching ordinary array assignment.
Nonfinite or out-of-range stored values report `SHD7004` before the store.
Successful stores round to FP32; finite underflow is allowed.

Tensor names and storage follow the existing program-scoped AI declarations.
They are not per-call local arrays: a recursive call that executes the same
declaration resets the same program-owned storage. An access before the
declaration executes (for example, after a skipped conditional declaration)
reports `SHD7004`. Ordinary lexical scalar/array bindings continue to take
precedence over an identically named tensor. Tensor storage is not implicitly
copied by a function call. This contract applies to the interpreter, LLVM
bitcode backend and MLIR backend.

Direct `read tensor[index]` is rejected. Read into an ordinary `float` variable
and then assign that value to the tensor to obtain the same checked conversion.
Assigning a tensor read to an integer variable still requires the existing
language's compatible type rules; this surface adds no implicit narrowing to
integers.

## Real inference

When an inference's input or output uses initialization or indexed access, both
tensors and the model must have matching static FP32 shapes within the same
65,536-element bound. The compiler canonicalizes FP32 precision for these calls.
The interpreter uses actual tensor values, and generated programs pass their
owned buffers through the existing `short_ai_infer_f32` ABI. Successful inference
replaces the output tensor only after runtime validation. Aliased input/output
uses the runtime's existing private output scratch and transactional commit.

Both declarations must have executed before inference. A missing, malformed,
unavailable or policy-rejected model reports `SHD7030`; a successful status also
requires the exact output count. The existing platform/backend qualification
policy remains in force. This is real runtime execution, not AOT integration.
Legacy metadata-only source behavior is preserved when the new value surface
is unused; this addition does not change the frozen runtime ABI.

## Qualification

`tests/mlir_lowering/test_source_tensors.py` runs the same programs through the
interpreter and both native backends at `-O0` and `-O2`, including FP32 rounding,
large integer literals, declaration reexecution, recursion, shadowing, evaluation
order, bounds, rejected types/shapes and real nonzero ONNX Identity execution.
It also exercises 65,536-element initialization and inference to catch excessive
aggregate code generation. The mandatory lowering lane runs this suite, including
its sanitizer configuration. Installed-package tests check nonzero values at
both optimization levels on every supported native package target.

These checks establish correctness of the declared surface. They do not measure
energy, prove a general latency improvement or broaden production qualification.
