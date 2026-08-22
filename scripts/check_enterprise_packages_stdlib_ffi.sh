#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
ENTERPRISE_TESTS="${ROOT_DIR}/tests/enterprise"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ "${SHORT}" != /* ]]; then
  short_dir="$(cd "$(dirname "${SHORT}")" 2>/dev/null && pwd)" || {
    echo "error: unable to resolve SHORTHAND_BIN: ${SHORT}" >&2
    exit 1
  }
  SHORT="${short_dir}/$(basename "${SHORT}")"
fi

CXX_BIN="${CXX:-g++}"
CC_BIN="${CC:-cc}"
COMMON=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -I"${ROOT_DIR}" -I"${SRC_DIR}")

command -v jq >/dev/null 2>&1 || {
  echo "error: jq is required for mandatory SPDX JSON structure validation" >&2
  exit 1
}

"${CXX_BIN}" "${COMMON[@]}" \
  "${ENTERPRISE_TESTS}/test_enterprise_language.cpp" \
  "${SRC_DIR}/enterprise/EnterpriseLanguage.cpp" \
  "${SRC_DIR}/type_system/ProductionTypeSystem.cpp" \
  -o "${WORK_DIR}/enterprise-language-test"
"${WORK_DIR}/enterprise-language-test" "${ENTERPRISE_TESTS}" "${WORK_DIR}"

"${CXX_BIN}" "${COMMON[@]}" \
  "${ENTERPRISE_TESTS}/test_sha256.cpp" "${SRC_DIR}/module/Sha256.cpp" \
  -o "${WORK_DIR}/sha256-test"
"${WORK_DIR}/sha256-test" "${WORK_DIR}/sha256-million-a.bin"

"${CXX_BIN}" "${COMMON[@]}" \
  "${ENTERPRISE_TESTS}/test_package_v2.cpp" \
  "${SRC_DIR}/module/ModuleResolver.cpp" "${SRC_DIR}/module/Sha256.cpp" \
  -o "${WORK_DIR}/package-v2-test"
"${WORK_DIR}/package-v2-test" "${WORK_DIR}"

"${CXX_BIN}" "${COMMON[@]}" \
  "${ENTERPRISE_TESTS}/test_core_ffi.cpp" "${SRC_DIR}/core/ShorthandCore.cpp" \
  -o "${WORK_DIR}/core-ffi-test"
"${WORK_DIR}/core-ffi-test"

"${CC_BIN}" -std=c11 -Wall -Wextra -Wpedantic -Werror -I"${ROOT_DIR}" \
  -c "${ENTERPRISE_TESTS}/test_core_ffi_c.c" \
  -o "${WORK_DIR}/core-c-consumer.o"
"${CXX_BIN}" "${WORK_DIR}/core-c-consumer.o" "${SRC_DIR}/core/ShorthandCore.cpp" \
  -I"${ROOT_DIR}" -o "${WORK_DIR}/core-c-consumer"
"${WORK_DIR}/core-c-consumer"

if [[ "$(uname -s)" == "Linux" ]]; then
  "${CXX_BIN}" "${COMMON[@]}" -fPIC -shared \
    -DSHORTHAND_CORE_SHARED=1 -DSHORTHAND_CORE_BUILDING_LIBRARY=1 \
    "${SRC_DIR}/core/ShorthandCore.cpp" -o "${WORK_DIR}/libshorthand_core.so"
  nm -D --defined-only "${WORK_DIR}/libshorthand_core.so" | awk '{print $3}' | \
    grep '^short_core_' | LC_ALL=C sort >"${WORK_DIR}/actual-symbols.txt"
  cmp -s "${ROOT_DIR}/abi/core_ffi_public_symbols_v1.txt" "${WORK_DIR}/actual-symbols.txt" || {
    echo "error: core FFI public symbol set changed" >&2
    diff -u "${ROOT_DIR}/abi/core_ffi_public_symbols_v1.txt" "${WORK_DIR}/actual-symbols.txt" >&2 || true
    exit 1
  }
fi

"${CXX_BIN}" "${COMMON[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  "${ENTERPRISE_TESTS}/test_core_ffi.cpp" "${SRC_DIR}/core/ShorthandCore.cpp" \
  -o "${WORK_DIR}/core-ffi-sanitized"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  "${WORK_DIR}/core-ffi-sanitized"

if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >"${WORK_DIR}/compiler-build.out" 2>&1 || {
    cat "${WORK_DIR}/compiler-build.out" >&2 || true
    exit 1
  }
fi

expect_failure() {
  local expected="$1"
  shift
  local output="${WORK_DIR}/failure-${RANDOM}.out"
  if "$@" >"${output}" 2>&1; then
    echo "error: command unexpectedly succeeded: $*" >&2
    exit 1
  fi
  grep -Fq "[${expected}]" "${output}" || {
    echo "error: expected ${expected}: $*" >&2
    cat "${output}" >&2 || true
    exit 1
  }
  grep -Fq '[range ' "${output}" || {
    echo "error: ${expected} lacks stable source provenance" >&2
    cat "${output}" >&2 || true
    exit 1
  }
}

"${SHORT}" "${ENTERPRISE_TESTS}/valid.enterprise.short" enterprise-check >"${WORK_DIR}/enterprise.json"
"${SHORT}" "${ENTERPRISE_TESTS}/valid.enterprise.short" enterprise-check \
  --output "${WORK_DIR}/enterprise-file.json"
cmp -s "${WORK_DIR}/enterprise.json" "${WORK_DIR}/enterprise-file.json" || {
  echo "error: enterprise-check stdout and --output evidence differ" >&2
  exit 1
}
grep -Fq '"schema":"shorthand.enterprise_language.v1"' "${WORK_DIR}/enterprise.json"
grep -Fq '"namespace":"policyclub.sustainable_ai"' "${WORK_DIR}/enterprise.json"
grep -Fq '"production_claim":false' "${WORK_DIR}/enterprise.json"
expect_failure SHD3016 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_use_after_move.enterprise.short" enterprise-check
expect_failure SHD3016 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_borrow_conflict.enterprise.short" enterprise-check
expect_failure SHD3010 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_duplicate_field.enterprise.short" enterprise-check
expect_failure SHD3016 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_unreleased_borrow.enterprise.short" enterprise-check
expect_failure SHD3024 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_enterprise_syntax.enterprise.short" enterprise-check
expect_failure SHD3025 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_duplicate_type.enterprise.short" enterprise-check
expect_failure SHD3024 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_stray_close.enterprise.short" enterprise-check
expect_failure SHD3025 "${SHORT}" "${ENTERPRISE_TESTS}/invalid_identity_collision.enterprise.short" enterprise-check

PACKAGE="${WORK_DIR}/package"
mkdir -p "${PACKAGE}/src" "${PACKAGE}/vendor/acme.math/src"
cat >"${PACKAGE}/vendor/acme.math/shorthand.package" <<'EOF'
format shorthand.package.v2
package acme.math
version 2.1.0
license MIT
module acme.math.ops src/ops.short
EOF
cat >"${PACKAGE}/vendor/acme.math/src/ops.short" <<'EOF'
package acme.math;
module acme.math.ops;
int initialized;
def int seven() { return 7; };
initialized = 1;
EOF
dependency_sha="$(sha256sum "${PACKAGE}/vendor/acme.math/shorthand.package" | awk '{print $1}')"
cat >"${PACKAGE}/shorthand.package" <<EOF
format shorthand.package.v2
package acme.app
version 1.4.0
license Apache-2.0
module acme.app.main src/main.short
dependency acme.math 2.1.0 vendor/acme.math sha256:${dependency_sha} MIT
EOF
cat >"${PACKAGE}/src/main.short" <<'EOF'
package acme.app;
module acme.app.main;
import acme.math.ops as math;
int result;
result = seven();
print "enterprise package", result;
EOF

"${SHORT}" "${PACKAGE}/src/main.short" lock >"${WORK_DIR}/lock.out"
cp "${PACKAGE}/shorthand.lock" "${WORK_DIR}/first.lock"
"${SHORT}" "${PACKAGE}/src/main.short" lock >"${WORK_DIR}/lock-repeat.out"
cmp -s "${WORK_DIR}/first.lock" "${PACKAGE}/shorthand.lock" || {
  echo "error: package v2 lock regeneration is not deterministic" >&2
  exit 1
}
grep -Fq 'format shorthand.lock.v2' "${PACKAGE}/shorthand.lock"
grep -Fq "dependency acme.math 2.1.0 vendor/acme.math sha256:${dependency_sha} MIT" "${PACKAGE}/shorthand.lock"
grep -Eq '^module acme.app.main src/main.short sha256:[0-9a-f]{64}$' "${PACKAGE}/shorthand.lock"
grep -Eq '^module acme.math.ops vendor/acme.math/src/ops.short sha256:[0-9a-f]{64}$' "${PACKAGE}/shorthand.lock"
"${SHORT}" "${PACKAGE}/src/main.short" module-graph >"${WORK_DIR}/graph.json"
grep -Fq '"schema":"shorthand.module.graph.v2"' "${WORK_DIR}/graph.json"
grep -Fq '"offline_only":true' "${WORK_DIR}/graph.json"
expect_failure SHD2034 env -u SOURCE_DATE_EPOCH "${SHORT}" "${PACKAGE}/src/main.short" package-sbom
expect_failure SHD2034 env SOURCE_DATE_EPOCH=-1 "${SHORT}" "${PACKAGE}/src/main.short" package-sbom
SOURCE_DATE_EPOCH=1787356800 "${SHORT}" "${PACKAGE}/src/main.short" package-sbom >"${WORK_DIR}/package.spdx.json"
SOURCE_DATE_EPOCH=1787356800 "${SHORT}" "${PACKAGE}/src/main.short" package-sbom >"${WORK_DIR}/package-repeat.spdx.json"
SOURCE_DATE_EPOCH=1787356800 "${SHORT}" "${PACKAGE}/src/main.short" package-sbom \
  --output "${WORK_DIR}/package-file.spdx.json"
cmp -s "${WORK_DIR}/package.spdx.json" "${WORK_DIR}/package-repeat.spdx.json" || {
  echo "error: package SPDX output is not deterministic" >&2
  exit 1
}
cmp -s "${WORK_DIR}/package.spdx.json" "${WORK_DIR}/package-file.spdx.json" || {
  echo "error: package SPDX stdout and --output evidence differ" >&2
  exit 1
}
grep -Fq '"spdxVersion":"SPDX-2.3"' "${WORK_DIR}/package.spdx.json"
grep -Fq '"creationInfo":{"created":"2026-08-22T00:00:00Z"' "${WORK_DIR}/package.spdx.json"
grep -Fq '"copyrightText":"NOASSERTION"' "${WORK_DIR}/package.spdx.json"
grep -Fq '"relationshipType":"DEPENDS_ON"' "${WORK_DIR}/package.spdx.json"
grep -Fq "\"referenceLocator\":\"sha256:${dependency_sha}\"" "${WORK_DIR}/package.spdx.json"
jq -e '
  .spdxVersion == "SPDX-2.3" and
  .dataLicense == "CC0-1.0" and
  .SPDXID == "SPDXRef-DOCUMENT" and
  (.creationInfo.created == "2026-08-22T00:00:00Z") and
  (.creationInfo.creators | length == 1) and
  (.packages | length == 2) and
  (all(.packages[]; .filesAnalyzed == false and .copyrightText == "NOASSERTION")) and
  (any(.relationships[]; .relationshipType == "DEPENDS_ON"))
' "${WORK_DIR}/package.spdx.json" >/dev/null
"${SHORT}" "${PACKAGE}/src/main.short" run >"${WORK_DIR}/package-run.out"
grep -Fq 'enterprise package 7' "${WORK_DIR}/package-run.out"

cp "${PACKAGE}/shorthand.package" "${WORK_DIR}/root-manifest"
sed 's/dependency acme.math 2.1.0/dependency acme.math ^2.0/' \
  "${WORK_DIR}/root-manifest" >"${PACKAGE}/shorthand.package"
expect_failure SHD2033 "${SHORT}" "${PACKAGE}/src/main.short" lock
sed 's/ MIT$/ GPL-3.0-only/' \
  "${WORK_DIR}/root-manifest" >"${PACKAGE}/shorthand.package"
expect_failure SHD2032 "${SHORT}" "${PACKAGE}/src/main.short" lock
cp "${WORK_DIR}/root-manifest" "${PACKAGE}/shorthand.package"

cp "${PACKAGE}/vendor/acme.math/shorthand.package" "${WORK_DIR}/dependency-manifest"
printf 'dependency nested.package 1.0.0 vendor/nested sha256:0000000000000000000000000000000000000000000000000000000000000000 MIT\n' \
  >>"${PACKAGE}/vendor/acme.math/shorthand.package"
nested_sha="$(sha256sum "${PACKAGE}/vendor/acme.math/shorthand.package" | awk '{print $1}')"
sed "s/${dependency_sha}/${nested_sha}/" \
  "${WORK_DIR}/root-manifest" >"${PACKAGE}/shorthand.package"
expect_failure SHD2031 "${SHORT}" "${PACKAGE}/src/main.short" lock
cp "${WORK_DIR}/dependency-manifest" "${PACKAGE}/vendor/acme.math/shorthand.package"
cp "${WORK_DIR}/root-manifest" "${PACKAGE}/shorthand.package"

printf '\n# dependency manifest tamper\n' >>"${PACKAGE}/vendor/acme.math/shorthand.package"
expect_failure SHD2031 "${SHORT}" "${PACKAGE}/src/main.short" run
cp "${WORK_DIR}/dependency-manifest" "${PACKAGE}/vendor/acme.math/shorthand.package"

printf '\n# tampered after lock\n' >>"${PACKAGE}/vendor/acme.math/src/ops.short"
expect_failure SHD2028 "${SHORT}" "${PACKAGE}/src/main.short" run

BAD_LICENSE="${WORK_DIR}/bad-license"
mkdir -p "${BAD_LICENSE}/src"
cat >"${BAD_LICENSE}/shorthand.package" <<'EOF'
format shorthand.package.v2
package invalid.license
version 1.0.0
license GPL-3.0-only
module invalid.license.main src/main.short
EOF
cat >"${BAD_LICENSE}/src/main.short" <<'EOF'
package invalid.license;
module invalid.license.main;
int value;
value = 1;
EOF
expect_failure SHD2032 "${SHORT}" "${BAD_LICENSE}/src/main.short" lock

BAD_VERSION="${WORK_DIR}/bad-version"
mkdir -p "${BAD_VERSION}/src"
sed 's/invalid.license/invalid.version/g; s/version 1.0.0/version ^1.0/' \
  "${BAD_LICENSE}/shorthand.package" | sed 's/license GPL-3.0-only/license MIT/' >"${BAD_VERSION}/shorthand.package"
sed 's/invalid.license/invalid.version/g' "${BAD_LICENSE}/src/main.short" >"${BAD_VERSION}/src/main.short"
expect_failure SHD2033 "${SHORT}" "${BAD_VERSION}/src/main.short" lock

MISSING="${WORK_DIR}/missing-dependency"
mkdir -p "${MISSING}/src"
cat >"${MISSING}/shorthand.package" <<'EOF'
format shorthand.package.v2
package missing.dependency
version 1.0.0
license MIT
module missing.dependency.main src/main.short
dependency absent.package 1.0.0 vendor/absent sha256:0000000000000000000000000000000000000000000000000000000000000000 MIT
EOF
cat >"${MISSING}/src/main.short" <<'EOF'
package missing.dependency;
module missing.dependency.main;
int value;
value = 1;
EOF
expect_failure SHD2031 "${SHORT}" "${MISSING}/src/main.short" lock

grep -Fq 'shorthand_core' "${ROOT_DIR}/CMakeLists.txt"
grep -Fq 'shorthand-core.pc' "${ROOT_DIR}/CMakeLists.txt"
grep -Fq 'core_c_consumer.c' "${ROOT_DIR}/tests/packaging/installed_consumer/CMakeLists.txt"
grep -Fq 'core_cpp_consumer.cpp' "${ROOT_DIR}/tests/packaging/installed_consumer/CMakeLists.txt"

while IFS= read -r license; do
  [[ -n "${license}" && "${license}" != \#* ]] || continue
  grep -Fq "\"${license}\"" "${SRC_DIR}/module/ModuleResolver.cpp" || {
    echo "error: package v2 license policy is missing repository allowlist entry ${license}" >&2
    exit 1
  }
done <"${ROOT_DIR}/security/license_allowlist.txt"

printf 'PASS enterprise packages standard library and safe FFI gate\n'
