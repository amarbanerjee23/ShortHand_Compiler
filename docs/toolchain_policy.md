# Toolchain Policy

ShortHand intentionally does **not** vendor Flex, LLVM, `llvm-config`, `llc`, Clang, or other compiler toolchain binaries into this repository.

## Why the toolchain is not committed

* Flex and LLVM are large, platform-specific binary/toolchain dependencies.
* Committing them would make the repository harder to audit, update, and secure.
* Toolchain binaries differ across Linux distributions, macOS, CPU architectures, and LLVM versions.
* CI and developer machines should install toolchains through their operating-system package manager, a pinned container image, or an external cache outside the Git repository.

## Required tools

A full ShortHand compiler build requires:

* C++17 compiler (`g++` or compatible)
* `make`
* Flex
* Bison
* Flex runtime development library (`libfl-dev` on Ubuntu/Debian)
* LLVM development tools with `llvm-config`
* `llc`
* Clang
* CMake and Ninja for the CMake path

## Ubuntu/Debian example

```bash
sudo apt-get update
sudo apt-get install -y build-essential make g++ flex bison libfl-dev llvm llvm-dev clang clang-format cmake ninja-build
```

## CI recommendation

Use a CI image that already contains Flex, Bison, LLVM development headers/tools, `llc`, and Clang, or restore those tools from a CI-managed cache outside the repository. The cache may be permanent for CI performance, but the binaries should not be committed to source control.

## Offline or restricted environments

For restricted environments, mirror distribution packages or prebuilt LLVM/Flex archives in an organization-controlled artifact store, then install them into the build image or runner workspace before running `setup_build_infra.sh`. Keep those artifacts outside this Git repository.

## Optional AI SDK backends

ShortHand does not vendor ONNX Runtime, TensorRT, OpenVINO, LibTorch, llama.cpp, CUDA, OpenBLAS, or Eigen. Optional SDKs are disabled by default and must be supplied through local roots such as `ONNXRUNTIME_ROOT`, `TENSORRT_ROOT`, `CUDA_ROOT`, `OPENVINO_ROOT`, `LIBTORCH_ROOT`, `LLAMACPP_ROOT`, `OPENBLAS_ROOT`, or `EIGEN_ROOT`. If strict optional backend mode is off, missing roots produce unavailable backend capabilities and the deterministic fallback remains buildable. If strict mode is on, configuration fails clearly when a requested SDK root is absent.
