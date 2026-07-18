#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN=${SHORTHAND_BIN:-"${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand"}
EXAMPLE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/examples/ai_onnx_fallback.short"
WORK_DIR=$(mktemp -d)
trap 'rm -rf "${WORK_DIR}"' EXIT

pushd "${WORK_DIR}" >/dev/null
"${BIN}" "${EXAMPLE}" compile >/tmp/shorthand_ai_metadata_compile.out 2>&1
IR_FILE="ai_onnx_fallback.ir"

if [[ ! -s "${IR_FILE}" ]]; then
  echo "expected ${IR_FILE} to be generated" >&2
  cat /tmp/shorthand_ai_metadata_compile.out >&2 || true
  exit 1
fi

for needle in \
  "shorthand.model" \
  "shorthand.tensor" \
  "shorthand.greenai_contract" \
  "shorthand.greenai_measure" \
  "shorthand.infer" \
  "classifier" \
  "input_shape=1,3,224,224" \
  "compiled_metadata_only=true"; do
  if ! grep -q "${needle}" "${IR_FILE}"; then
    echo "expected LLVM IR metadata to contain: ${needle}" >&2
    cat "${IR_FILE}" >&2
    exit 1
  fi
done
popd >/dev/null
