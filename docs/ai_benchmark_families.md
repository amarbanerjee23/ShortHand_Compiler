# Realistic AI benchmark family qualification

benchmark_family_contract: shorthand.ai.benchmark_suite.v1
introduced_github_pr: 98
production_scope: linux-x64-cpu-v1
production_claim: false
comparative_energy_claim: false
official_certification_granted: false
lowest_carbon_language_claim: false
calibrated_physical_evidence: pending
full_standard_dataset_coverage: false
full_equivalent_baseline_coverage: false

PR98 broadens the AI evidence surface without broadening the production hardware boundary. ONNX Runtime CPU on Linux x64 remains the only production-qualified backend/device tuple. Accelerator inventory is not accelerator execution evidence. The existing `shorthand.ai.cpu_qualification.v1` measured-energy contract remains strict FP32/no-quantization; this PR does not weaken that contract to admit uncalibrated or differently bounded evidence.

## Family matrix

| Family | Executed evidence | Quality evidence | Equivalent baseline | Calibrated energy |
| --- | --- | --- | --- | --- |
| Classification | Native Optdigits application | 1,797 held-out UCI Optdigits rows, accuracy/top-k/confusion matrix | Optimized hash-pinned Python execution comparison | Pending controlled physical measurement |
| Detection | Tiny YOLO controlled registry plus native detection-shaped tensor fixture | Tensor execution agreement only; no mAP claim | Pending | Pending |
| Retrieval | Batched native similarity-tensor fixture | Deterministic finite output and prepared-session agreement | Pending | Pending |
| Training | Native reference CNN | Loss reduction, validation accuracy, gradient/thread determinism | Pending | Pending |
| Quantized inference | ONNX `QuantizeLinear`/`DequantizeLinear` fixture with UINT8 internal tensor | FP32 host-boundary finite output and quantization-error bound | Pending | Pending |
| Batched inference | Native Optdigits application at batch 1/16/32 | Prediction equivalence; padded tail excluded from functional units | Optimized Python comparison | Pending |
| Concurrent serving | Bounded application serving plus concurrent prepared-session probe | Prediction equivalence, tenant isolation, recovery and deterministic concurrent output | Optimized application comparison | Pending |

The checked manifest is `tests/ai_benchmark/benchmark_suite_v1.json`. `scripts/validate_ai_benchmark_suite.py` requires exactly these seven families, binds all evidence paths to repository files, rejects duplicate or missing families, and rejects unsupported production, energy, certification or lowest-carbon claims. Only entries explicitly backed by the real held-out Optdigits dataset may set `standard_dataset_quality: true`.

## Quantized execution boundary

The PR98 quantized fixture keeps the application tensor boundary FP32 while executing ONNX `QuantizeLinear` and `DequantizeLinear` internally with a UINT8 zero point. This tests an important production model shape without changing the PR95 energy qualifier's precision/math policy. The live ONNX lane checks deterministic repeated output, finite values and an error bound no larger than one quantization step. The SDK-off lane must fail preparation explicitly; it is not counted as live execution.

This is deliberately narrower than a general INT8 production qualification. Per-channel quantization, arbitrary quantized operators, calibration datasets, quantized training and task-accuracy equivalence are not claimed. A future extension must introduce a versioned measured-energy/math policy rather than silently reinterpreting the FP32 qualifier.

## Detection and retrieval boundaries

Detection and retrieval fixtures establish runtime mechanics and representative production tensor shapes, not end-task benchmark scores. The controlled registry still pins Tiny YOLO as a standard small detector artifact; its synthetic-input execution does not establish mAP. The retrieval MatMul fixture exercises batched vector-to-corpus scoring and concurrent prepared-session reuse; it does not establish recall@K on a public retrieval corpus.

These distinctions are machine-enforced in the suite manifest. `full_standard_dataset_coverage` and `full_equivalent_baseline_coverage` remain false until later evidence supplies task datasets and equivalent optimized baselines for the remaining families.

## CI and safety

`scripts/check_ai_energy_qualification.sh` compiles the family runtime probe with the same warnings-as-errors policy used by the native AI gate. In SDK-off jobs it proves explicit unavailable behavior. In the mandatory live ONNX lane it executes retrieval, detection-shaped and internally quantized models, then stresses concurrent reuse of one prepared retrieval session. Existing application, comparison, sanitizer, TSan, portability, security, CTest parity and zero-skip gates remain unchanged.

Python is used only to build deterministic test artifacts and validate repository evidence metadata. The deployed ShortHand runtime and the family execution probe are native C++ and do not require Python.

## Claims boundary

PR98 adds implementation and reproducible CI evidence; it does not establish that ShortHand is universally lower energy than Python or any other language. Comparative energy remains valid only for equivalent workloads measured within the existing calibrated boundary and replay policy. C3-ECO tooling continues to emit candidate evidence only; independent certification authority remains external.
