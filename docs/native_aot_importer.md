# Native bounded ONNX import

`aot/StaticAffineModel.cpp` is the native compiler foundation for explicit static
FP32 MatMul+Add AOT. It does not yet connect to a source compilation mode, emit a
kernel, change runtime routing or remove the runtime dependency from a ShortHand
program. Those integration and workspace qualifications remain in PR126.

The importer accepts one owned snapshot, performs no file access and requires
no Python, protobuf SDK or ONNX SDK. It reuses the existing immutable dependency
envelope check, then independently decodes the bounded graph and FP32 constants.
The accepted model retains its owned bytes, SHA-256 identity, dimensions and
weights behind const accessors. Mutating the caller's original bytes cannot
change the accepted artifact. This foundation does not replace the existing
runtime immutable handle or mutable-model content validation.

The supported subset follows the independent Python/ONNX AOT oracle:

- Standard-domain opset 13, ONNX IR 8 through 10, at most 16 MiB.
- One static rank-2 FP32 input and output, positive dimensions at most 1,024,
  and at most 65,536 elements in each input/output.
- Exactly MatMul followed by Add, with two unique named constants: FP32 weights
  `[inputs, outputs]` and bias `[outputs]`. Either Add operand order is accepted.
- At most 1,048,576 weights plus 1,024 bias values. Raw little-endian FP32 and
  protobuf float_data encodings retain their exact bits, including signed zero
  and finite subnormals. Constants must be finite and have exact element counts.
- Consistent graph wiring, intermediate signatures and unique value producers.

Malformed/truncated/overflowing protobuf, wrong wire types, duplicate singular
fields, unknown fields in decoded messages, invalid UTF-8, unresolved or cyclic
inputs, duplicate names and incorrect supported-affine signatures reject. The
dependency envelope rejects external/segmented tensors, custom operator domains,
training/function/device containers and unknown dependency-bearing fields.
Sparse/quantized representations and types/shapes outside this bounded contract
are rejected conservatively. The existing default runtime path is unaffected.

The result has three explicit dispositions:

| Disposition | Meaning and required consumer action |
|---|---|
| `Supported` | Fully inspected bounded MatMul+Add graph with owned constants; eligible for subsequent AOT lowering. |
| `RequiresBackendValidation` | A structurally checked, dependency-bounded static FP32 graph is outside the native operator/attribute subset. This is **not** proof of valid ONNX semantics or a successful fallback. A hybrid consumer must independently validate and prepare it with a qualified real backend before executing it. |
| `Rejected` | No accepted model is returned. A consumer must fail; it must not report a successful fallback. |

In particular, an unknown operator can produce `RequiresBackendValidation`,
while the independent ONNX checker rejects it. That case is deliberately tested
to prevent consumers from mistaking the intermediate disposition for success.
Graph attributes are dependency-checked by the shared envelope but are never
executed by this importer; their semantic validation belongs to the real backend.

`test_native_aot_importer.py` compares accepted dimensions and exact constant
byte hashes against the independent pinned Python/ONNX importer. It exercises
raw/float encodings, bias order, signed zero/subnormals, maximum shapes and byte
boundaries, metadata, ownership, malformed encodings, unsafe dependencies,
invalid graphs and valid/invalid unsupported operators. The native executable
links only this importer and the existing SHA-256 implementation. CI runs it
under ASan/LSan/UBSan and requires its success marker in the evidence index.

These are import-correctness checks. They do not establish execution speed,
energy savings, AOT source integration or broader production qualification.
