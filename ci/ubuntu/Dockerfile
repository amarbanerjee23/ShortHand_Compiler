FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       make \
       g++ \
       flex \
       bison \
       libfl-dev \
       llvm \
       llvm-dev \
       clang \
       clang-format \
       cmake \
       ninja-build \
       curl \
       ca-certificates \
       git \
       jq \
    && rm -rf /var/lib/apt/lists/*

COPY scripts/install_ci_onnxruntime_cpu.sh /usr/local/share/shorthand/install_ci_onnxruntime_cpu.sh
RUN bash /usr/local/share/shorthand/install_ci_onnxruntime_cpu.sh /opt/shorthand/onnxruntime-1.20.1

ENV ONNXRUNTIME_ROOT=/opt/shorthand/onnxruntime-1.20.1
ENV LD_LIBRARY_PATH=/opt/shorthand/onnxruntime-1.20.1/lib

RUN set -euo pipefail; \
    command -v make; command -v g++; command -v flex; command -v bison; \
    command -v llvm-config; command -v lli; command -v llc; command -v clang; \
    command -v cmake; command -v ninja; \
    test -s "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h"; \
    test -d "${ONNXRUNTIME_ROOT}/lib"

LABEL org.opencontainers.image.source="https://github.com/amarbanerjee23/ShortHand_Compiler"
LABEL org.opencontainers.image.description="Pinned Ubuntu 24.04 ShortHand compiler and ONNX Runtime CPU CI qualification environment"
