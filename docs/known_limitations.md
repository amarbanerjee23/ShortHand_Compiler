# Known Limitations

known_limitations_version: 2026-08-22-pr84
current_maturity: controlled_beta
production_claim: false
production_backend_scope: linux-x64-cpu-v1

The active beta-0.4 language has cross-mode execution for exact `int`, `bool`, binary64 `float`/`double`, immutable string scalars and fixed numeric/boolean arrays. The guarded descriptor and ownership model exists, but record, enum, slice, option/result and ownership source syntax, owned string arrays, general functions/control flow, the package ecosystem, standard library and stable FFI are not complete. Concurrent multi-tenant serving, production MLIR lowering, representative AI workloads and measured performance/energy evidence are also open.

ONNX Runtime CPU is mandatory live numerical evidence for the declared `linux-x64-cpu-v1` production backend scope on the inherited Linux x64 CI lane. TensorRT, OpenVINO, LibTorch, llama.cpp, GPU, TPU and NPU paths remain experimental or inventory-only. Their absent SDKs or devices are never counted as production execution evidence.

## AI runtime abstraction limitations

Fallback is always available for deterministic negative behavior but never claims successful inference; it reports `not_executed` with `backend_not_available`. A locally installed SDK or detected accelerator does not imply qualification. The only qualified v1 pair is ONNX Runtime CPU on Linux x64 CPU.

Runtime JSON, Prometheus and OTLP-shaped observability exports exist, but the default process-wide context is not multi-tenant isolation and the loopback metrics adapter is not hardened public ingress. The signed-release workflow remains partial until a real protected tag publication produces cryptographically verified attestations.

C3-ECO outputs are candidate evidence only. ShortHand is not officially certified and does not claim that a language, runtime, cloud or model is inherently green. Energy, carbon or electricity-cost claims require a declared functional unit/boundary, real measurement or transparent estimation, provenance, uncertainty and quality equivalence.
