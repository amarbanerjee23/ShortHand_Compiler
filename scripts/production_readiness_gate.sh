#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

required_files=(
  "README.md"
  "docs/known_limitations.md"
  "docs/green_ai_certification.md"
  "docs/public_release_readiness.md"
  "examples/green_ai/image_classification.greenai"
)

for path in "${required_files[@]}"; do
  if [[ ! -f "${path}" ]]; then
    echo "error: required production-readiness evidence file is missing: ${path}" >&2
    exit 1
  fi
done

if ! grep -q 'no known bugs under full validation' README.md; then
  echo "error: README.md must retain the scoped reliability statement." >&2
  exit 1
fi

candidate_manifest="examples/green_ai/image_classification.greenai"
if ! grep -q 'C3-ECO-AI' "${candidate_manifest}"; then
  echo "error: candidate manifest must declare C3-ECO-AI." >&2
  exit 1
fi
if ! grep -q 'offsets_allowed_in_core_score: false' "${candidate_manifest}"; then
  echo "error: candidate manifest must keep offsets_allowed_in_core_score: false." >&2
  exit 1
fi

readiness_doc="docs/public_release_readiness.md"
for required_text in \
  'Skipped Optional Checks' \
  'ONNXRUNTIME_ROOT' \
  'LIBTORCH_ROOT' \
  'RAPL/NVML' \
  'platform-specific measurement tools'; do
  if ! grep -q "${required_text}" "${readiness_doc}"; then
    echo "error: ${readiness_doc} must mention ${required_text}." >&2
    exit 1
  fi
done

claim_scan_pattern='zero[[:space:]]+bugs|ZERO[[:space:]]+bugs|production-ready[[:space:]]+without[[:space:]]+limitation|certified[[:space:]]+green[[:space:]]+ai|carbon-neutral[[:space:]]+software|zero-carbon[[:space:]]+software|IEEE[[:space:]]+TSE[[:space:]]+readiness'
scan_targets=(README.md docs examples/green_ai)
if grep -RInE "${claim_scan_pattern}" "${scan_targets[@]}"; then
  echo "error: unsupported public-readiness, certification, carbon, or defect claim wording found." >&2
  exit 1
fi

make -C Compiler_new_ws/Short_Hand/src c3eco_gate green_ai_tool
make -C Compiler_new_ws/Short_Hand/src test-c3eco
