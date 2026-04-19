#!/usr/bin/env bash
set -euo pipefail

# Bootstraps third-party dependencies used by the ShortHand native AI framework.
# By default, downloads CPU artifacts into ./third_party.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY_DIR="${ROOT_DIR}/third_party"

LLVM_VERSION="${LLVM_VERSION:-17}"
ONNXRUNTIME_VERSION="${ONNXRUNTIME_VERSION:-1.18.1}"
LIBTORCH_VERSION="${LIBTORCH_VERSION:-2.3.1}"

mkdir -p "${THIRD_PARTY_DIR}"

echo "[1/4] Installing system build tooling (Ubuntu/Debian)..."
if command -v apt-get >/dev/null 2>&1; then
  sudo apt-get update
  sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    curl \
    wget \
    unzip \
    flex \
    bison \
    libfl-dev \
    clang \
    llvm \
    llvm-dev
else
  echo "apt-get not found; skipping system package install."
fi

echo "[2/4] Downloading ONNX Runtime ${ONNXRUNTIME_VERSION} (CPU)..."
ONNX_ARCHIVE="onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz"
ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNX_ARCHIVE}"
mkdir -p "${THIRD_PARTY_DIR}/onnxruntime"
if [ ! -d "${THIRD_PARTY_DIR}/onnxruntime/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}" ]; then
  wget -O "${THIRD_PARTY_DIR}/${ONNX_ARCHIVE}" "${ONNX_URL}"
  tar -xzf "${THIRD_PARTY_DIR}/${ONNX_ARCHIVE}" -C "${THIRD_PARTY_DIR}/onnxruntime"
fi

echo "[3/4] Downloading LibTorch ${LIBTORCH_VERSION} (CPU, CXX11 ABI)..."
LIBTORCH_ARCHIVE="libtorch-cxx11-abi-shared-with-deps-${LIBTORCH_VERSION}%2Bcpu.zip"
LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/${LIBTORCH_ARCHIVE}"
mkdir -p "${THIRD_PARTY_DIR}/libtorch"
if [ ! -d "${THIRD_PARTY_DIR}/libtorch/libtorch" ]; then
  wget -O "${THIRD_PARTY_DIR}/libtorch.zip" "${LIBTORCH_URL}"
  unzip -q "${THIRD_PARTY_DIR}/libtorch.zip" -d "${THIRD_PARTY_DIR}/libtorch"
fi

echo "[4/4] Finalizing environment exports..."
cat <<EOF
Add these exports to your shell profile:

export ONNXRUNTIME_ROOT="${THIRD_PARTY_DIR}/onnxruntime/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}"
export LIBTORCH_ROOT="${THIRD_PARTY_DIR}/libtorch/libtorch"
export LD_LIBRARY_PATH="\$ONNXRUNTIME_ROOT/lib:\$LIBTORCH_ROOT/lib:\$LD_LIBRARY_PATH"

Build commands:
  make -C Compiler_new_ws/Short_Hand/src
  make -C Compiler_new_ws/Short_Hand/src ai_app ONNXRUNTIME_ROOT="\$ONNXRUNTIME_ROOT"
  make -C Compiler_new_ws/Short_Hand/src ai_train LIBTORCH_ROOT="\$LIBTORCH_ROOT"
EOF
