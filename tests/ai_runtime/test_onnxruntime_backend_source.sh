#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BACKEND="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.cpp"
MAKEFILE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/Makefile"
CMAKE_FILE="${ROOT_DIR}/CMakeLists.txt"

for needle in \
  "Ort::Session session" \
  "session.Run" \
  "GetInputNameAllocated" \
  "GetOutputNameAllocated" \
  "CreateTensor<float>" \
  "InferenceStatus::Success" \
  "onnxruntime_exception" \
  "onnxruntime_cpu_currently_requires_float32_input"; do
  if ! grep -q "${needle}" "${BACKEND}"; then
    echo "expected ONNX Runtime backend implementation to contain: ${needle}" >&2
    exit 1
  fi
done

if grep -q "execution hook is built but model execution is not configured" "${BACKEND}"; then
  echo "old non-executing ONNX Runtime stub text is still present" >&2
  exit 1
fi

for needle in \
  'AI_CXXFLAGS += -I$(ONNXRUNTIME_ROOT)/include' \
  'AI_LDLIBS += -L$(ONNXRUNTIME_ROOT)/lib -lonnxruntime' \
  '$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) $(LLVM_CXXFLAGS)'; do
  if ! grep -Fq "${needle}" "${MAKEFILE}"; then
    echo "expected Makefile ONNX Runtime build wiring to contain: ${needle}" >&2
    exit 1
  fi
done

for needle in \
  "target_include_directories(short_hand PRIVATE \${ONNXRUNTIME_ROOT}/include)" \
  "target_link_directories(short_hand PRIVATE \${ONNXRUNTIME_ROOT}/lib)" \
  "target_link_libraries(short_hand PRIVATE onnxruntime)"; do
  if ! grep -Fq "${needle}" "${CMAKE_FILE}"; then
    echo "expected CMake ONNX Runtime SDK wiring to contain: ${needle}" >&2
    exit 1
  fi
done

if [[ -n "${ONNXRUNTIME_ROOT:-}" ]]; then
  if [[ ! -f "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" ]]; then
    echo "ONNXRUNTIME_ROOT is set but include/onnxruntime_cxx_api.h is missing" >&2
    exit 1
  fi
  make -C "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" clean
  make -C "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" short_hand ai_app
else
  echo "ONNXRUNTIME_ROOT not set; source-level implementation and build-wiring checks passed."
fi
