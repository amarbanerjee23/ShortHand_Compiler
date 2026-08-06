#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
VALID="${ROOT_DIR}/tests/modules/valid/module_preamble.short"
LEGACY="${ROOT_DIR}/tests/conformance/beta_0_2/core_declarations.short"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_module_ast_build.out 2>&1 || {
    cat /tmp/shorthand_module_ast_build.out >&2 || true
    exit 1
  }
fi

for file in \
  "${VALID}" \
  "${LEGACY}" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_package.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_alias.short" \
  "${ROOT_DIR}/tests/modules/invalid/duplicate_import_path.short" \
  "${ROOT_DIR}/tests/modules/invalid/import_before_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/package_without_module.short" \
  "${ROOT_DIR}/tests/modules/invalid/malformed_module_path.short"; do
  [[ -f "${file}" ]] || { echo "error: missing module fixture: ${file}" >&2; exit 1; }
done

"${SHORT}" "${VALID}" parse >"${WORK_DIR}/valid-parse.out" 2>&1
"${SHORT}" "${VALID}" module-info >"${WORK_DIR}/module-info.json" 2>"${WORK_DIR}/module-info.err"

grep -Fq '"schema":"shorthand.module.ast.v1"' "${WORK_DIR}/module-info.json"
grep -Fq '"language_version":"beta-0.3"' "${WORK_DIR}/module-info.json"
grep -Fq '"resolver_status":"not_resolved"' "${WORK_DIR}/module-info.json"
grep -Fq '"package":{"name":"acme.ai"' "${WORK_DIR}/module-info.json"
grep -Fq '"module":{"name":"acme.recommendation"' "${WORK_DIR}/module-info.json"
grep -Fq '"path":"acme.models","alias":"models"' "${WORK_DIR}/module-info.json"
grep -Fq '"path":"acme.telemetry","alias":null' "${WORK_DIR}/module-info.json"
grep -Fq '"range":{"begin":{"line":1,"column":1}' "${WORK_DIR}/module-info.json"
grep -Fq '"range":{"begin":{"line":2,"column":1}' "${WORK_DIR}/module-info.json"

"${SHORT}" "${LEGACY}" module-info >"${WORK_DIR}/legacy-info.json" 2>"${WORK_DIR}/legacy-info.err"
grep -Fq '"package":null' "${WORK_DIR}/legacy-info.json"
grep -Fq '"module":null' "${WORK_DIR}/legacy-info.json"
grep -Fq '"imports":[]' "${WORK_DIR}/legacy-info.json"

expect_failure() {
  local fixture="$1"
  local code="$2"
  local output="${WORK_DIR}/$(basename "${fixture}").out"
  if "${SHORT}" "${fixture}" parse >"${output}" 2>&1; then
    echo "error: invalid module fixture unexpectedly parsed: ${fixture}" >&2
    exit 1
  fi
  grep -Fq "[${code}]" "${output}" || {
    echo "error: ${fixture} did not emit ${code}" >&2
    cat "${output}" >&2 || true
    exit 1
  }
  grep -Fq '[range ' "${output}" || {
    echo "error: ${fixture} did not emit a source range" >&2
    cat "${output}" >&2 || true
    exit 1
  }
  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' "${output}"; then
    echo "error: sanitizer failure while rejecting ${fixture}" >&2
    cat "${output}" >&2 || true
    exit 1
  fi
}

expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_package.short" SHD2011
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_module.short" SHD2012
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_import_alias.short" SHD2013
expect_failure "${ROOT_DIR}/tests/modules/invalid/duplicate_import_path.short" SHD2014
expect_failure "${ROOT_DIR}/tests/modules/invalid/import_before_module.short" SHD2015
expect_failure "${ROOT_DIR}/tests/modules/invalid/package_without_module.short" SHD2016
expect_failure "${ROOT_DIR}/tests/modules/invalid/malformed_module_path.short" SHD2001

{
  printf 'module stress.generated;\n'
  for index in $(seq 1 512); do
    printf 'import stress.dependency_%s as dep_%s;\n' "${index}" "${index}"
  done
  printf 'int value;\nvalue = 1;\n'
} >"${WORK_DIR}/large-import-set.short"

timeout --signal=TERM --kill-after=2 10 \
  "${SHORT}" "${WORK_DIR}/large-import-set.short" parse \
  >"${WORK_DIR}/large-import-set.out" 2>&1

if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' \
  "${WORK_DIR}/large-import-set.out"; then
  echo "error: sanitizer failure in import-set stress fixture" >&2
  cat "${WORK_DIR}/large-import-set.out" >&2 || true
  exit 1
fi

printf 'PASS module import package syntax and AST scaffold gate\n'
