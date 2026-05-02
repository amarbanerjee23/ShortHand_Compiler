#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"

echo "[1/3] Building optional AI binaries..."
make -C "${SRC_DIR}" ai_app ai_train

echo "[2/3] Verifying graceful runtime guidance without optional SDKs..."
APP_OUTPUT="$("${BUILD_DIR}/short_ai_app" fake.onnx 1,3 0.1,0.2,0.3 2>&1 || true)"
if echo "${APP_OUTPUT}" | grep -q "ONNX Runtime support not enabled"; then
  echo "short_ai_app fallback message OK"
else
  echo "short_ai_app fallback message missing"
  exit 1
fi

TRAIN_OUTPUT="$("${BUILD_DIR}/short_ai_train" 10 0.01 out.pt 2>&1 || true)"
if echo "${TRAIN_OUTPUT}" | grep -q "LibTorch support not enabled"; then
  echo "short_ai_train fallback message OK"
else
  echo "short_ai_train fallback message missing"
  exit 1
fi

echo "[3/3] Verifying environment checker script..."
"${ROOT_DIR}/scripts/verify_env.sh" >/dev/null

echo "Smoke tests completed successfully."
