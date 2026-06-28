#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"

required_files=(
  "README.md"
  "docs/production_green_ai_architecture.md"
  "docs/c3eco_conformance_matrix.md"
  "Compiler_new_ws/Short_Hand/src/green_ai/C3ECOGate.cpp"
  "examples/green_ai/image_classification.greenai"
  "examples/green_ai/c3eco_ai_certification_candidate.greenai"
  "tests/green_ai/test_c3eco_gate.sh"
)

for path in "${required_files[@]}"; do
  test -s "${ROOT_DIR}/${path}" || { echo "missing required production-readiness artifact: ${path}" >&2; exit 1; }
done

if grep -RInE 'zero bugs|ZERO bugs|production-ready without limitation|certified green ai|carbon-neutral software|zero-carbon software' \
  "${ROOT_DIR}/README.md" "${ROOT_DIR}/docs" "${ROOT_DIR}/examples/green_ai"; then
  echo "unsupported production, certification, or zero-bug claim found" >&2
  exit 1
fi

grep -q 'no known bugs under full validation' "${ROOT_DIR}/README.md"
grep -q 'C3-ECO-AI' "${ROOT_DIR}/examples/green_ai/c3eco_ai_certification_candidate.greenai"
grep -q 'offsets_allowed_in_core_score: false' "${ROOT_DIR}/examples/green_ai/c3eco_ai_certification_candidate.greenai"

make -C "${SRC_DIR}" c3eco_gate green_ai_tool
make -C "${SRC_DIR}" test-c3eco

echo "Production readiness gate completed successfully."
