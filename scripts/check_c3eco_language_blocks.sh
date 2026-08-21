#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
VALID="${ROOT_DIR}/tests/c3eco/c3eco_all_blocks.short"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}" "${ROOT_DIR}/c3eco_all_blocks.bc"' EXIT

if [[ ! -x "${SHORT}" ]]; then
  make -C "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" short_hand >/tmp/shorthand_c3eco_build.out 2>&1 || {
    cat /tmp/shorthand_c3eco_build.out >&2 || true
    exit 1
  }
fi

"${SHORT}" "${VALID}" parse
"${SHORT}" "${VALID}" print >"${WORK_DIR}/ast.out"
for kind in certification functional_unit workload boundary measurement_plan ai_lifecycle rag_pipeline token_budget model_routing guardrails; do
  grep -Eq "^${kind} .*fields=" "${WORK_DIR}/ast.out" || {
    echo "error: AST printer missing C3-ECO declaration kind ${kind}" >&2
    exit 1
  }
done

"${SHORT}" "${VALID}" c3eco-report --output "${WORK_DIR}/report.json"
grep -Fq '"c3eco_language_contract": "shorthand.c3eco.language.v1"' "${WORK_DIR}/report.json"
grep -Fq '"official_certification_granted": false' "${WORK_DIR}/report.json"
for kind in certification functional_unit workload boundary measurement_plan ai_lifecycle rag_pipeline token_budget model_routing guardrails; do
  grep -Fq "\"kind\":\"${kind}\"" "${WORK_DIR}/report.json" || {
    echo "error: evidence report missing C3-ECO declaration kind ${kind}" >&2
    exit 1
  }
done

(
  cd "${ROOT_DIR}"
  rm -f c3eco_all_blocks.bc
  "${SHORT}" "${VALID}" compile-bc >/dev/null
  test -s c3eco_all_blocks.bc
  if command -v llvm-dis >/dev/null 2>&1; then
    llvm-dis c3eco_all_blocks.bc -o - | grep -Fq 'shorthand.c3eco_declaration'
  fi
)

expect_code() {
  local fixture="$1"
  local code="$2"
  if "${SHORT}" "${ROOT_DIR}/tests/c3eco/${fixture}" c3eco-report --output "${WORK_DIR}/${fixture}.json" >"${WORK_DIR}/${fixture}.out" 2>"${WORK_DIR}/${fixture}.err"; then
    echo "error: ${fixture} unexpectedly passed C3-ECO semantic validation" >&2
    exit 1
  fi
  grep -Fq "[${code}]" "${WORK_DIR}/${fixture}.err" || {
    cat "${WORK_DIR}/${fixture}.err" >&2 || true
    echo "error: ${fixture} missing expected ${code}" >&2
    exit 1
  }
}

expect_code c3eco_duplicate_declaration.short SHD5101
expect_code c3eco_missing_required_field.short SHD5102
expect_code c3eco_invalid_field.short SHD5103
expect_code c3eco_unsafe_claim.short SHD5104

echo "PASS C3-ECO first-class language blocks grammar AST semantics evidence and claim-safety gate"
