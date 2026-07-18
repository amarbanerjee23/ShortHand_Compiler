# ShortHand Type and Shape Inference Direction

ShortHand currently uses a strict tensor-shape validation model rather than Hindley-Milner inference.

This is intentional for the current AI/compiler scope:

- Tensor declarations carry explicit element type and shape.
- Model declarations carry explicit input/output shapes.
- `infer model(input) -> output` validates input shape against the model input shape.
- If the output tensor is declared, the compiler validates output shape against the model output shape.
- Backend preferences are checked against the model format through `docs/backend_compatibility_matrix.md`.

## Why not Hindley-Milner now?

Hindley-Milner is useful for general-purpose functional type inference, but enterprise AI model compilation needs stronger domain checks:

1. Tensor rank and dimension compatibility.
2. Model input/output layer compatibility.
3. Backend-format compatibility.
4. Precision and quantization compatibility.
5. Evidence/measurement boundary validation.

The next production step is a typed tensor-shape inference engine with optional model introspection, not generic Hindley-Milner alone.

## Future model introspection

For SDK-enabled builds, the compiler/runtime should query model metadata where possible:

- ONNX Runtime: input/output names, element types, ranks and static dimensions.
- LibTorch: scripted model schema when available.
- OpenVINO: model input/output ports.
- TensorRT: engine bindings.
- llama.cpp/GGUF: context size and tokenizer/model metadata.

The compiler must keep fallback honest when a model cannot be introspected at compile time.
