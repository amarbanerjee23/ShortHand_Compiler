#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SHORTHAND_MLIR_BUILD_DIR:-${ROOT_DIR}/build-mlir}"
for tool in cmake ninja python3; do
  command -v "${tool}" >/dev/null || { echo "error: MLIR qualification requires ${tool}" >&2; exit 1; }
done
LLVM_CONFIG="${LLVM_CONFIG:-$(command -v llvm-config-18 || command -v llvm-config || true)}"
[[ -x "${LLVM_CONFIG}" ]] || { echo "error: LLVM 18 llvm-config is mandatory" >&2; exit 1; }
[[ "$("${LLVM_CONFIG}" --version)" == 18.* ]] || { echo "error: LLVM/MLIR 18.x is mandatory" >&2; exit 1; }
LLVM_CMAKE_DIR="$("${LLVM_CONFIG}" --cmakedir)"
MLIR_CONFIG_DIR="${MLIR_DIR:-${LLVM_CMAKE_DIR%/llvm}/mlir}"
[[ -s "${MLIR_CONFIG_DIR}/MLIRConfig.cmake" ]] || { echo "error: MLIR 18 development package is mandatory" >&2; exit 1; }
cmake -S "${ROOT_DIR}/mlir" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMLIR_DIR="${MLIR_CONFIG_DIR}" \
  -DSHORTHAND_MLIR_TESTING=ON -DSHORTHAND_MLIR_SANITIZERS="${SHORTHAND_MLIR_SANITIZERS:-OFF}"
cmake --build "${BUILD_DIR}" --parallel 2
ctest --test-dir "${BUILD_DIR}" --output-on-failure
echo "PASS PR92 generated MLIR dialect lit verifiers roundtrip installed consumer and freshness gate"

bash "${ROOT_DIR}/scripts/check_mlir_lowering.sh"
