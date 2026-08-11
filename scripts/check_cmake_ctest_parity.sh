#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PARITY_DIR="${ROOT_DIR}/tests/ctest_parity"
MAKEFILE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/Makefile"
EXPECTED_FILE="${SHORTHAND_CTEST_EXPECTED_FILE:-${PARITY_DIR}/expected_make_targets.txt}"
BUILD_DIR="${SHORTHAND_CTEST_PARITY_BUILD_DIR:-${TMPDIR:-/tmp}/shorthand-ctest-parity}"
EXECUTE="${SHORTHAND_CTEST_PARITY_EXECUTE:-0}"

for tool in cmake ctest; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: CTest parity requires ${tool}" >&2; exit 1; }
done
[[ -s "${MAKEFILE}" ]] || { echo "error: Makefile missing" >&2; exit 1; }
[[ -s "${EXPECTED_FILE}" ]] || { echo "error: CTest parity target manifest missing: ${EXPECTED_FILE}" >&2; exit 1; }
[[ -s "${PARITY_DIR}/CMakeLists.txt" ]] || { echo "error: CTest parity project missing" >&2; exit 1; }

mapfile -t expected_targets < <(grep -Ev '^[[:space:]]*(#|$)' "${EXPECTED_FILE}")
[[ ${#expected_targets[@]} -gt 0 ]] || { echo "error: empty CTest parity target manifest" >&2; exit 1; }

make_test_line="$(grep -E '^test:[[:space:]]' "${MAKEFILE}" | head -n1 || true)"
[[ -n "${make_test_line}" ]] || { echo "error: Makefile aggregate test target is missing" >&2; exit 1; }

for target in "${expected_targets[@]}"; do
  grep -Eq "^${target}:" "${MAKEFILE}" || {
    echo "error: parity manifest target is not defined by Makefile: ${target}" >&2
    exit 1
  }
  if [[ " ${make_test_line#test:} " != *" ${target} "* ]]; then
    echo "error: parity target is not part of Makefile test aggregate: ${target}" >&2
    exit 1
  fi
done

aggregate_words="$(printf '%s\n' "${make_test_line#test:}" | xargs -n1 | sort -u)"
manifest_words="$(printf '%s\n' "${expected_targets[@]}" | sort -u)"
if [[ "${aggregate_words}" != "${manifest_words}" ]]; then
  echo "error: CTest parity manifest differs from Makefile test aggregate" >&2
  diff -u <(printf '%s\n' "${aggregate_words}") <(printf '%s\n' "${manifest_words}") >&2 || true
  exit 1
fi

rm -rf "${BUILD_DIR}"
cmake -S "${PARITY_DIR}" -B "${BUILD_DIR}" -DSHORTHAND_SOURCE_DIR="${ROOT_DIR}" >/tmp/shorthand_ctest_parity_configure.out
ctest_listing="$(ctest --test-dir "${BUILD_DIR}" -N)"

for target in "${expected_targets[@]}"; do
  suffix="${target//-/_}"
  grep -Fq "make_${suffix}" <<<"${ctest_listing}" || {
    echo "error: generated CTest project missing Makefile target mapping: ${target}" >&2
    exit 1
  }
done

registered="$(grep -c 'Test #' <<<"${ctest_listing}" || true)"
if [[ "${registered}" -ne "${#expected_targets[@]}" ]]; then
  echo "error: CTest registered ${registered} parity tests, expected ${#expected_targets[@]}" >&2
  exit 1
fi

if [[ "${EXECUTE}" == "1" ]]; then
  ctest --test-dir "${BUILD_DIR}" --output-on-failure -L make-parity -j1
elif [[ "${EXECUTE}" != "0" ]]; then
  echo "error: SHORTHAND_CTEST_PARITY_EXECUTE must be 0 or 1" >&2
  exit 1
fi

printf 'PASS CMake CTest parity gate targets=%d executed=%s\n' "${#expected_targets[@]}" "${EXECUTE}"
