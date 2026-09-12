#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIALECT_BUILD="${SHORTHAND_MLIR_BUILD_DIR:-${ROOT_DIR}/build-mlir}"
BUILD_DIR="${SHORTHAND_MLIR_SOURCE_BUILD_DIR:-${ROOT_DIR}/build-mlir-source}"
LLVM_CONFIG="${LLVM_CONFIG:-$(command -v llvm-config-18 || command -v llvm-config || true)}"
[[ -x "${LLVM_CONFIG}" && "$("${LLVM_CONFIG}" --version)" == 18.* ]] || { echo "error: LLVM 18 is mandatory" >&2; exit 1; }
LLVM_BIN="$("${LLVM_CONFIG}" --bindir)"
LLVM_CMAKE="$("${LLVM_CONFIG}" --cmakedir)"
export SHORTHAND_LLVM_CLANG="${LLVM_BIN}/clang++"
export SHORTHAND_LLVM_LLI="${LLVM_BIN}/lli"
for tool in cmake ninja python3 objcopy readelf "${SHORTHAND_LLVM_CLANG}" "${SHORTHAND_LLVM_LLI}"; do
  command -v "${tool}" >/dev/null || { echo "error: mandatory lowering tool missing: ${tool}" >&2; exit 1; }
done
if [[ -z "${ONNXRUNTIME_ROOT:-}" ]]; then
  ONNXRUNTIME_ROOT="${BUILD_DIR}/onnxruntime-1.20.1"
  bash "${ROOT_DIR}/scripts/install_ci_onnxruntime_cpu.sh" "${ONNXRUNTIME_ROOT}"
fi
[[ -s "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" ]] || { echo "error: real ONNX SDK is mandatory" >&2; exit 1; }
FLAGS=""
if [[ "${SHORTHAND_MLIR_SANITIZERS:-OFF}" == ON ]]; then FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"; fi
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DSHORTHAND_BUILD_TESTING=OFF -DSHORTHAND_BUILD_MLIR=ON -DSHORTHAND_MLIR_TESTING=OFF \
  -DMLIR_DIR="${LLVM_CMAKE%/llvm}/mlir" -DLLVM_CONFIG="${LLVM_CONFIG}" \
  -DSHORTHAND_ENABLE_ONNXRUNTIME=ON -DSHORTHAND_STRICT_OPTIONAL_BACKENDS=ON \
  -DONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" -DCMAKE_CXX_FLAGS="${FLAGS}" -DCMAKE_EXE_LINKER_FLAGS="${FLAGS}"
cmake --build "${BUILD_DIR}" --parallel 2 --target short_hand shorthand_runtime
python3 "${ROOT_DIR}/tests/mlir_lowering/test_source_lowering.py" "${ROOT_DIR}" "${BUILD_DIR}/short_hand" "${DIALECT_BUILD}/shorthand-opt" "${DIALECT_BUILD}/test/shorthand-lowering-probe"
python3 "${ROOT_DIR}/tests/mlir_lowering/test_runtime_lowering.py" "${ROOT_DIR}" "${BUILD_DIR}" "${DIALECT_BUILD}/test/shorthand-lowering-probe" "${ONNXRUNTIME_ROOT}"
echo "PASS verified SemanticIR MLIR LLVM composite execution and real runtime lowering gate"
