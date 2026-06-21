# ShortHand Language Specification

ShortHand is a C++/LLVM-first compiled GreenAI language. Python is not required for the official compiler, runtime, validation, tests, or evidence path.

New GreenAI syntax includes `tensor name element "shape"`, `model name { ... };`, `greenai_contract name { ... };`, `greenai_measure name { ... };`, and `infer model(input) -> output;`. Model declarations capture format, path, task, precision, input/output shapes, backend preference, compact-model intent, and quality guardrails.

Evidence mode is invoked as `short_hand program.short evidence [--output report.json]` and emits JSON with the mandatory disclaimer: `Evidence report only; this tool does not grant certification.`
