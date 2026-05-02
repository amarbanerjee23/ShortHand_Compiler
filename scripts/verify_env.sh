#!/usr/bin/env bash
set -euo pipefail

check_cmd() {
  local cmd="$1"
  if command -v "${cmd}" >/dev/null 2>&1; then
    echo "✅ ${cmd}: $(command -v "${cmd}")"
  else
    echo "❌ ${cmd}: missing"
  fi
}

echo "Checking core compiler toolchain..."
check_cmd g++
check_cmd llvm-config
check_cmd llc
check_cmd clang
check_cmd flex
check_cmd bison

echo
echo "Checking optional AI SDKs..."
if [ -n "${ONNXRUNTIME_ROOT:-}" ] && [ -d "${ONNXRUNTIME_ROOT}" ]; then
  echo "✅ ONNXRUNTIME_ROOT=${ONNXRUNTIME_ROOT}"
else
  echo "⚠️ ONNXRUNTIME_ROOT not set or invalid"
fi

if [ -n "${LIBTORCH_ROOT:-}" ] && [ -d "${LIBTORCH_ROOT}" ]; then
  echo "✅ LIBTORCH_ROOT=${LIBTORCH_ROOT}"
else
  echo "⚠️ LIBTORCH_ROOT not set or invalid"
fi
