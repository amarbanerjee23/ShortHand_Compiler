# ShortHand Language Specification

ShortHand is a C++/LLVM-first compiled GreenAI language. Python is not required for the official compiler, runtime, validation, tests, or evidence path.

New GreenAI syntax includes `tensor name element "shape"`, `model name { ... };`, `greenai_contract name { ... };`, `greenai_measure name { ... };`, and `infer model(input) -> output;`. Model declarations capture format, path, task, precision, input/output shapes, backend preference, compact-model intent, and quality guardrails.

Evidence mode is invoked as `short_hand program.short evidence [--output report.json]` and emits JSON with the mandatory disclaimer: `Evidence report only; this tool does not grant certification.`


## AI model/tensor/infer abstraction

The user-facing abstraction is intentionally small:

```short
model classifier {
  format onnx;
  path "models/classifier.onnx";
  task "image_classification";
  precision int8;
  input_shape "1,3,224,224";
  output_shape "1,1000";
  backend_preference tensorrt, onnxruntime_tensorrt, onnxruntime_cuda, openvino, onnxruntime_cpu, fallback;
  compact true;
  quality_guardrail accuracy >= 90;
};

tensor image float "1,3,224,224";
infer classifier(image) -> result;
```

`->` is the preferred inference arrow. The older `infer classifier(image) > result;` form remains accepted temporarily. The runtime hides ONNX Runtime, TensorRT, OpenVINO, LibTorch, CUDA, and llama.cpp details behind `backend_preference`; fallback is deterministic and never reports success.
