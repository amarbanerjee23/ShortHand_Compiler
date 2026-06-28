# Production Green AI language architecture

This document records the architecture decisions for making ShortHand a stronger C++/LLVM-first Green AI language artifact. It does not claim defect-freedom. The release target is: **no known bugs under full validation** after all mandatory gates pass.

## Architecture objective

ShortHand should let application developers express AI workloads in simple language constructs while the compiler and runtime enforce evidence, boundary, and sustainability controls. The language should remain simple at the source level and strict at the validation layer.

Core principles:

- C++/LLVM-first compiler and runtime path.
- No Python dependency for official build, test, validation, runtime, report generation, or release gates.
- Native backend abstraction for optional AI libraries.
- Deterministic fallback behavior when optional SDKs are unavailable.
- Evidence-first Green AI reporting.
- Conservative claims policy: evidence reports are not certificates.
- CI must fail when a PR weakens language validation, Green AI evidence, or public-claims controls.

## Main components

| Component | Responsibility |
| --- | --- |
| Parser and scanner | Accept ShortHand language syntax and reject ambiguous grammar changes through Bison conflict errors. |
| Semantic analyzer | Validate model, tensor, inference, GreenAI contracts, measurements, shapes, precision, guardrails, loop controls, and unsupported combinations. |
| Interpreter | Execute the supported language subset and route AI inference statements through the runtime abstraction. |
| LLVM IR generator | Produce LLVM IR, bitcode, and native outputs for the compiled language path. |
| AI runtime abstraction | Select optional C++ AI backends using backend preferences and capabilities. |
| Fallback backend | Always-buildable deterministic backend that reports `not_executed` honestly when real SDKs are unavailable. |
| Green AI evidence tool | Parse `.greenai` manifests, calculate energy, carbon, and resource metrics, check budgets, and produce evidence reports. |
| C3-ECO gate | Enforce certification-candidate evidence completeness, claims controls, and AI boundary rules before merge. |
| CI release gates | Build, validate, test, sanitize, configure CMake, run CTest, and run C3-ECO conformance gates. |

## AI language model

The language-level AI abstraction is intentionally compact:

```short_hand
model classifier {
    format onnx;
    path "models/classifier.onnx";
    precision int8;
    input_shape "1,3,224,224";
    output_shape "1,1000";
    backend_preference tensorrt, onnxruntime_cuda, onnxruntime_cpu, fallback;
    quality_guardrail accuracy >= 90;
};

tensor image float "1,3,224,224";

infer classifier(image) -> result;
```

The compiler/runtime layer maps this simple syntax to backend capabilities and evidence output. Developers should not need to write TensorRT, ONNX Runtime, OpenVINO, LibTorch, or llama.cpp-specific code in the basic language path.

## Backend decision model

1. Parse model format, precision, shape, and backend preference.
2. Validate model and tensor declarations at semantic-analysis time.
3. Create runtime `ModelSpec` and `TensorBuffer` values.
4. Iterate backend preferences in order.
5. Select the first backend that is available and supports the model format.
6. If no real backend is available, use fallback and report `not_executed` with a reason.
7. Never report successful inference unless a backend actually executed it.

## Green AI and C3-ECO evidence model

ShortHand separates three layers:

1. Language constructs: `model`, `tensor`, `infer`, GreenAI contracts, and GreenAI measurements.
2. Evidence manifests: `.greenai` files that describe functional unit, boundary, measurement quality, data quality, carbon factors, budgets, and AI-specific controls.
3. Certification-candidate gate: C++ and shell gates that check whether a manifest is sufficiently complete for internal certification review.

The C3-ECO gate currently enforces:

- System identity: product, version, owner, release date, software class, deployment mode, and certification scope.
- Functional unit and success criteria.
- Boundary declaration with included and excluded layers.
- Measurement-quality and data-quality classes.
- Carbon accounting method, region, grid factor, and offset exclusion from core score.
- Security, accessibility, safety, privacy, correctness, and quality floors.
- Materiality and cumulative-omissions controls.
- Evidence retention and recertification acceptance.
- Claims integrity.
- AI role separation, model necessity, smaller-model evaluation, precision, batch size, runtime, and inference-energy measurement.

## Claims policy

ShortHand reports evidence. It must not claim that a software product is certified unless an authorized certification body has reviewed and issued such a certificate.

Allowed wording:

- `C3-ECO AI candidate evidence report only`
- `Evidence report only; this tool does not grant certification.`
- `no known bugs under full validation`

## CI design

Every PR should run:

1. Toolchain verification.
2. `bash setup_build_infra.sh`.
3. Strict language validation.
4. Smoke tests.
5. Makefile test suite.
6. Sanitizer tests.
7. CMake configure/build.
8. CTest.
9. C3-ECO conformance gate.
10. Production-readiness gate.

A PR should be merged only if it is conflict-free and all required checks pass.

## Compiler development rules

- Keep generated parser and scanner artifacts synchronized with grammar changes.
- Keep Bison shift/reduce and reduce/reduce conflicts as hard failures.
- Avoid C++ exceptions in files compiled with LLVM `llvm-config --cxxflags`, because CI can include `-fno-exceptions`.
- Use no-throw parsing and validation helpers in compiler/runtime code paths.
- Keep optional backend macros explicit and default them to disabled.
- Treat fallback as evidence of non-execution, not as fake success.
- Pair every efficiency optimization with quality, correctness, security, privacy, and accessibility guardrails.

## Current production-readiness boundary

This branch improves internal engineering readiness and certification-candidate evidence. It does not turn the repository into a universally complete commercial compiler for every possible AI application. Any public release must still be scoped by version, workload, boundary, platform, backend configuration, and validation evidence.
