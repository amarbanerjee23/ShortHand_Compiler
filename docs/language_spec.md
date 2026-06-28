# ShortHand Language Specification

ShortHand is a C++/LLVM-first compiled GreenAI language. Python is not required for the official compiler, runtime, validation, tests, or evidence path.

New GreenAI syntax includes `tensor name element "shape"`, `model name { ... };`, `greenai_contract name { ... };`, `greenai_measure name { ... };`, and `infer model(input) -> output;`. Model declarations capture format, path, task, precision, input/output shapes, backend preference, compact-model intent, and quality guardrails.

Evidence mode is invoked as `short_hand program.short evidence [--output report.json]` and emits JSON with the mandatory disclaimer: `Evidence report only; this tool does not grant certification.`

## AI model, tensor, and infer syntax

A model declaration records the model format, path, task, precision, input/output shapes, ordered backend preferences, compact flag, and required quality guardrail. A tensor declaration records an element type and shape. Inference uses `infer model(input) -> output;`; the legacy `>` form is accepted by the current scanner as the same token. Backends are tried in declaration order, incompatible or unavailable backends are skipped, and fallback is used only when allowed or explicitly listed. The fallback result is deterministic and never reports success.
