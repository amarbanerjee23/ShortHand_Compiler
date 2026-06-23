# Known Limitations

Optional ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, Eigen, and OpenBLAS SDKs are not vendored and are not required in CI. Real backend inference and real energy telemetry require those SDKs or external instrumentation. Fallback evidence reports `not_executed` and does not pretend inference occurred.


## Optional backend limitations

Default builds use deterministic fallback because CI does not provide ONNX Runtime, TensorRT/CUDA, OpenVINO, LibTorch, or llama.cpp SDK roots. ONNX Runtime CPU execution is compiled only with `ONNXRUNTIME_ROOT`; TensorRT requires `TENSORRT_ROOT` and `CUDA_ROOT`; OpenVINO, LibTorch, and llama.cpp require their respective roots. TensorRT/OpenVINO/LibTorch/llama.cpp execution paths are guarded stubs unless the SDK integration is enabled and completed locally. No backend may claim success unless real inference ran.
