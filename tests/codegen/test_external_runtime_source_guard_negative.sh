#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SOURCE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/IR_Generator.cpp"
GUARD="${ROOT_DIR}/scripts/apply_external_runtime_to_ir_source.sh"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

bash "${GUARD}" "${SOURCE}" >"${TMP_DIR}/positive.out"
grep -Fq 'PASS external runtime source lowering is canonical, target-aware and Python-free' "${TMP_DIR}/positive.out"

cp "${SOURCE}" "${TMP_DIR}/IR_Generator.cpp"
printf '\n// regression sentinel\n// BasicBlock *entry = BasicBlock::Create(ShortGlobalContext, "entry", fn);\n' >> "${TMP_DIR}/IR_Generator.cpp"

set +e
bash "${GUARD}" "${TMP_DIR}/IR_Generator.cpp" >"${TMP_DIR}/negative.out" 2>"${TMP_DIR}/negative.err"
status=$?
set -e

if [[ ${status} -eq 0 ]]; then
  echo "error: external runtime source guard accepted a forbidden local runtime stub" >&2
  exit 1
fi
grep -Fq 'source-level runtime lowering still contains forbidden local-stub/native-link text' "${TMP_DIR}/negative.err" || {
  cat "${TMP_DIR}/negative.err" >&2
  echo "error: external runtime source guard negative test failed for an unexpected reason" >&2
  exit 1
}

if grep -Eq 'python(3)?[[:space:]]+-' "${GUARD}"; then
  echo "error: external runtime source guard reintroduced a Python interpreter dependency" >&2
  exit 1
fi

echo "PASS external runtime source guard negative regression"
