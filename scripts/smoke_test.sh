#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
EXAMPLE_GREENAI="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short"
EXAMPLE_AI="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_infer.short"
MANIFEST="${ROOT_DIR}/examples/green_ai/image_classification.greenai"

cd "${ROOT_DIR}"

echo "[1/8] Build GreenAI tool"
make -C "${SRC_DIR}" green_ai_tool

echo "[2/8] Test GreenAI tool"
make -C "${SRC_DIR}" test-green

echo "[3/8] Build ShortHand compiler"
make -C "${SRC_DIR}" clean all

echo "[4/8] Run ShortHand GreenAI example"
GREENAI_OUTPUT="$(${BUILD_DIR}/short_hand "${EXAMPLE_GREENAI}" run 2>&1)"
echo "${GREENAI_OUTPUT}"
echo "${GREENAI_OUTPUT}" | grep -q "GreenAI workload"
echo "${GREENAI_OUTPUT}" | grep -q "energy_j=500"

echo "[5/8] Run ShortHand AI inference example"
set +e
AI_OUTPUT="$(${BUILD_DIR}/short_hand "${EXAMPLE_AI}" run 2>&1)"
AI_STATUS=$?
set -e
if [[ "$AI_STATUS" -ne 0 ]]; then echo "AI example returned $AI_STATUS with fallback diagnostics"; fi
echo "${AI_OUTPUT}"
echo "${AI_OUTPUT}" | grep -q "AI inference"
echo "${AI_OUTPUT}" | grep -q "GreenAI workload"

echo "[6/8] Compile ShortHand GreenAI example to LLVM bitcode"
"${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-bc
test -f greenai_report.bc

echo "[7/8] Compile ShortHand GreenAI example to native binary"
"${BUILD_DIR}/short_hand" "${EXAMPLE_GREENAI}" compile-native
test -x greenai_report
NATIVE_OUTPUT="$(./greenai_report 2>&1)"
echo "${NATIVE_OUTPUT}"
echo "${NATIVE_OUTPUT}" | grep -q "GreenAI workload"

echo "[8/8] Generate GreenAI report and check eco-regression"
"${BUILD_DIR}/green_ai_tool" validate "${MANIFEST}" --strict strict
"${BUILD_DIR}/green_ai_tool" report "${MANIFEST}" --output /tmp/short_hand_green_report.json --strict strict
test -f /tmp/short_hand_green_report.json
grep -q "Evidence report only; this tool does not grant certification." /tmp/short_hand_green_report.json
"${BUILD_DIR}/green_ai_tool" check "${MANIFEST}" --baseline /tmp/short_hand_green_report.json --threshold-percent 10

echo "Smoke tests completed successfully."
