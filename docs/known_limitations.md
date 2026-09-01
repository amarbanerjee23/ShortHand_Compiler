# Known Limitations

known_limitations_version: 2026-09-01-pr88
current_maturity: controlled_beta
production_claim: false
production_backend_scope: linux-x64-cpu-v1

The active beta-0.7 contract retains beta-0.5 cross-mode execution for exact scalars, fixed numeric/boolean arrays, functions, recursion, lexical locals, structured loops/returns and same-block label transfers. Beta-0.6 adds composite ABI schemas and ownership-plan validation, not composite execution. Beta-0.7 adds a typed C3-ECO preparation profile, not measurement, scoring or certification. Owned string arrays, by-value composite FFI, nested package dependencies and a network registry remain unavailable. Package v2 is deliberately offline, the core library is bounded to its 1.0.0 C ABI, and qualified member dispatch is not yet syntax. Production MLIR/composite lowering, representative AI workloads and measured performance/energy evidence are also open.

ONNX Runtime CPU is mandatory live numerical evidence for the declared `linux-x64-cpu-v1` production backend scope on the inherited Linux x64 CI lane. TensorRT, OpenVINO, LibTorch, llama.cpp, GPU, TPU and NPU paths remain experimental or inventory-only. Their absent SDKs or devices are never counted as production execution evidence.

## AI runtime abstraction limitations

Fallback is always available for deterministic negative behavior but never claims successful inference; it reports `not_executed` with `backend_not_available`. A locally installed SDK or detected accelerator does not imply qualification. The only qualified v1 pair is ONNX Runtime CPU on Linux x64 CPU.

The `shorthand.serving.runtime.v1` scheduler provides process-scoped tenant isolation, bounded concurrency/admission, deadlines, cooperative cancellation, health, low-cardinality metrics and graceful drain. It intentionally has no public listener. Authentication, authorization, TLS, cross-tenant multiplexing and hard termination of a handler that ignores the cancellation token require an external host/process boundary and are not qualified. The legacy default runtime context and loopback metrics adapter remain separate and are not hardened public ingress. The signed-release workflow remains partial until a real protected tag publication produces cryptographically verified attestations.

C3-ECO outputs are candidate evidence only. A conformant `shorthand.c3eco.profile.v2` proves typed preparation metadata, not measured efficiency or approval. ShortHand is not officially certified and does not claim that a language, runtime, cloud or model is inherently green. Energy, carbon or electricity-cost claims require a declared functional unit/boundary, real measurement or transparent estimation, provenance, uncertainty and quality equivalence.
