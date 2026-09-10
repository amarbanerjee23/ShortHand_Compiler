#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  fi
}

require_file mlir/README.md
require_file mlir/CMakeLists.txt
require_file mlir/lib/ShortHandDialect.cpp
require_file mlir/include/ShortHand/IR/ShortHandTypes.td
require_file mlir/include/ShortHand/IR/ShortHandAttributes.td
require_file scripts/check_mlir_dialect.sh
require_contains mlir/README.md 'mlir_contract: shorthand.mlir.v1'
require_contains mlir/README.md 'lowering_status: pending_pr93'
require_contains .github/workflows/ci.yml 'test "${{ needs.mlir.result }}" = "success"'
require_contains Compiler_new_ws/Short_Hand/src/Makefile 'test-mlir:'
require_contains tests/ctest_parity/expected_make_targets.txt 'test-mlir'

require_file mlir/include/ShortHand/IR/ShortHandDialect.td
require_file mlir/include/ShortHand/IR/ShortHandOps.td
require_file mlir/examples/ai_greenai_pipeline.mlir
require_file docs/mlir_lowering_plan.md

require_contains mlir/include/ShortHand/IR/ShortHandDialect.td 'def ShortHand_Dialect : Dialect'
require_contains mlir/include/ShortHand/IR/ShortHandDialect.td 'let name = "shorthand"'
require_contains mlir/include/ShortHand/IR/ShortHandDialect.td 'cppNamespace = "::shorthand::ir"'

require_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_ModelOp'
require_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_TensorOp'
require_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_InferOp'
require_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_GreenAIContractOp'
require_contains mlir/include/ShortHand/IR/ShortHandOps.td 'def ShortHand_GreenAIMeasureOp'

require_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.model"'
require_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.tensor"'
require_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.infer"'
require_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.greenai_contract"'
require_contains mlir/examples/ai_greenai_pipeline.mlir '"shorthand.greenai_measure"'

require_contains docs/mlir_lowering_plan.md 'ShortHand source'
require_contains docs/mlir_lowering_plan.md 'ShortHand semantic IR'
require_contains docs/mlir_lowering_plan.md 'ShortHand MLIR dialect'
require_contains docs/mlir_lowering_plan.md 'LLVM dialect'
require_contains docs/feature_implementation_status.md 'MLIR dialect scaffold'

if grep -R "production-ready" mlir docs/mlir_lowering_plan.md | grep -v 'does not claim production readiness' | grep -v 'not production-ready' >/tmp/shorthand_mlir_claims.out 2>&1; then
  echo "error: unsupported MLIR production-readiness claim found" >&2
  cat /tmp/shorthand_mlir_claims.out >&2
  exit 1
fi

echo "PASS MLIR foundation gate"
