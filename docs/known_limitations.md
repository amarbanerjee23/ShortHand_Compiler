# Known Limitations

Optional ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, Eigen, and OpenBLAS SDKs are not vendored and are not required in CI. Real backend inference and real energy telemetry require those SDKs or external instrumentation. Fallback evidence reports `not_executed` and does not pretend inference occurred.

## AI runtime abstraction limitations

Default CI exercises the deterministic fallback backend. Fallback is always available but never claims successful inference; it reports `not_executed` with `backend_not_available`. Real SDK execution requires locally installed SDK roots and does not imply certification, zero-carbon validation, or production readiness. Evidence output is an evidence report only and does not grant certification.
