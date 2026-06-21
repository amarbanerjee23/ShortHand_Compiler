# Known Limitations

Optional ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, Eigen, and OpenBLAS SDKs are not vendored and are not required in CI. Real backend inference and real energy telemetry require those SDKs or external instrumentation. Fallback evidence reports `not_executed` and does not pretend inference occurred.
