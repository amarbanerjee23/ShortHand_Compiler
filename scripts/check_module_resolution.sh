#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
if [[ "${SHORT}" != /* ]]; then
  short_dir="$(cd "$(dirname "${SHORT}")" 2>/dev/null && pwd)" || {
    echo "error: unable to resolve SHORTHAND_BIN relative to the caller working directory: ${SHORT}" >&2
    exit 1
  }
  SHORT="${short_dir}/$(basename "${SHORT}")"
fi
FIXTURE="${ROOT_DIR}/tests/modules/resolver/valid_project"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

[[ -d "${FIXTURE}" ]] || { echo "error: missing resolver fixture" >&2; exit 1; }
if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >/tmp/shorthand_module_resolver_build.out 2>&1 || {
    cat /tmp/shorthand_module_resolver_build.out >&2 || true
    exit 1
  }
fi

expect_failure() {
  local expected="$1"
  shift
  local out="${WORK_DIR}/failure-$RANDOM.out"
  if "$@" >"${out}" 2>&1; then
    echo "error: command unexpectedly succeeded: $*" >&2
    cat "${out}" >&2 || true
    exit 1
  fi
  grep -Fq "[${expected}]" "${out}" || {
    echo "error: expected ${expected}: $*" >&2
    cat "${out}" >&2 || true
    exit 1
  }
  grep -Fq '[range ' "${out}" || {
    echo "error: ${expected} did not carry stable source provenance" >&2
    cat "${out}" >&2 || true
    exit 1
  }
  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' "${out}"; then
    echo "error: sanitizer failure while expecting ${expected}" >&2
    cat "${out}" >&2 || true
    exit 1
  fi
}

cp -R "${FIXTURE}" "${WORK_DIR}/valid"
APP="${WORK_DIR}/valid/src/app.short"
NATIVE_APP="${WORK_DIR}/valid/src/native_app.short"

"${SHORT}" "${APP}" lock >"${WORK_DIR}/lock-path-1.out" 2>"${WORK_DIR}/lock-1.err"
cp "${WORK_DIR}/valid/shorthand.lock" "${WORK_DIR}/lock-first"
"${SHORT}" "${APP}" lock >"${WORK_DIR}/lock-path-2.out" 2>"${WORK_DIR}/lock-2.err"
cmp -s "${WORK_DIR}/lock-first" "${WORK_DIR}/valid/shorthand.lock" || {
  echo "error: repeated lock generation is not deterministic" >&2
  exit 1
}
grep -Fq 'format shorthand.lock.v1' "${WORK_DIR}/valid/shorthand.lock"
grep -Fq 'entry acme.demo.app' "${WORK_DIR}/valid/shorthand.lock"
grep -Fq 'module acme.demo.lib src/lib.short fnv1a64:' "${WORK_DIR}/valid/shorthand.lock"

"${SHORT}" "${APP}" module-graph >"${WORK_DIR}/graph.json" 2>"${WORK_DIR}/graph.err"
grep -Fq '"schema":"shorthand.module.graph.v1"' "${WORK_DIR}/graph.json"
grep -Fq '"package":"acme.demo"' "${WORK_DIR}/graph.json"
grep -Fq '"lock_status":"verified"' "${WORK_DIR}/graph.json"
grep -Fq '"order":["acme.demo.lib","acme.demo.app"]' "${WORK_DIR}/graph.json"

"${SHORT}" "${APP}" run >"${WORK_DIR}/run.out" 2>"${WORK_DIR}/run.err"
grep -Fq 'root value 7' "${WORK_DIR}/run.out"

(
  cd "${WORK_DIR}/valid"
  "${SHORT}" "${APP}" compile-bc >"${WORK_DIR}/compile-bc.out" 2>"${WORK_DIR}/compile-bc.err"
  [[ -s app.bc ]]
)

"${SHORT}" "${NATIVE_APP}" lock >"${WORK_DIR}/native-lock.out" 2>"${WORK_DIR}/native-lock.err"
expect_failure SHD2030 "${SHORT}" "${NATIVE_APP}" run
(
  cd "${WORK_DIR}/valid"
  "${SHORT}" "${NATIVE_APP}" compile-native >"${WORK_DIR}/native-compile.out" 2>"${WORK_DIR}/native-compile.err"
  ./native_app >"${WORK_DIR}/native-run.out" 2>"${WORK_DIR}/native-run.err"
)
grep -Fq 'imported helper' "${WORK_DIR}/native-run.out"
grep -Fq 'native root' "${WORK_DIR}/native-run.out"

# A changed source invalidates the exact graph fingerprint.
cp -R "${FIXTURE}" "${WORK_DIR}/stale"
"${SHORT}" "${WORK_DIR}/stale/src/app.short" lock >/dev/null 2>&1
printf '\n# changed after lock\n' >>"${WORK_DIR}/stale/src/lib.short"
expect_failure SHD2028 "${SHORT}" "${WORK_DIR}/stale/src/app.short" run

# Missing manifest.
mkdir -p "${WORK_DIR}/missing-manifest"
cp "${FIXTURE}/src/app.short" "${WORK_DIR}/missing-manifest/app.short"
expect_failure SHD2020 "${SHORT}" "${WORK_DIR}/missing-manifest/app.short" lock

# Invalid manifest schema/identity.
mkdir -p "${WORK_DIR}/invalid-manifest/src"
cat >"${WORK_DIR}/invalid-manifest/shorthand.package" <<'EOF'
format shorthand.package.v999
package invalid.pkg
module invalid.pkg.app src/app.short
EOF
cat >"${WORK_DIR}/invalid-manifest/src/app.short" <<'EOF'
package invalid.pkg;
module invalid.pkg.app;
int value;
value = 1;
EOF
expect_failure SHD2021 "${SHORT}" "${WORK_DIR}/invalid-manifest/src/app.short" lock

# Missing import mapping.
cp -R "${FIXTURE}" "${WORK_DIR}/missing-import"
sed -i 's/acme.demo.lib/acme.demo.missing/' "${WORK_DIR}/missing-import/src/app.short"
expect_failure SHD2022 "${SHORT}" "${WORK_DIR}/missing-import/src/app.short" lock

# Unsafe manifest path escape. Any explicit parent traversal is rejected, even if lexical normalization could re-enter the root.
mkdir -p "${WORK_DIR}/escape/src"
cat >"${WORK_DIR}/escape/shorthand.package" <<'EOF'
format shorthand.package.v1
package escape.pkg
module escape.pkg.app src/../src/app.short
EOF
cat >"${WORK_DIR}/escape/src/app.short" <<'EOF'
package escape.pkg;
module escape.pkg.app;
int value;
value = 1;
EOF
expect_failure SHD2023 "${SHORT}" "${WORK_DIR}/escape/src/app.short" lock

# Manifest/source identity mismatch.
mkdir -p "${WORK_DIR}/identity/src"
cat >"${WORK_DIR}/identity/shorthand.package" <<'EOF'
format shorthand.package.v1
package identity.pkg
module identity.pkg.app src/app.short
module identity.pkg.lib src/lib.short
EOF
cat >"${WORK_DIR}/identity/src/app.short" <<'EOF'
package identity.pkg;
module identity.pkg.app;
import identity.pkg.lib;
int value;
value = 1;
EOF
cat >"${WORK_DIR}/identity/src/lib.short" <<'EOF'
package identity.pkg;
module identity.pkg.other;
int other;
other = 1;
EOF
expect_failure SHD2024 "${SHORT}" "${WORK_DIR}/identity/src/app.short" lock

# Package mismatch.
cp -R "${FIXTURE}" "${WORK_DIR}/package-mismatch"
sed -i 's/package acme.demo;/package wrong.demo;/' "${WORK_DIR}/package-mismatch/src/lib.short"
expect_failure SHD2027 "${SHORT}" "${WORK_DIR}/package-mismatch/src/app.short" lock

# Duplicate/ambiguous module mapping.
mkdir -p "${WORK_DIR}/ambiguous/src"
cat >"${WORK_DIR}/ambiguous/shorthand.package" <<'EOF'
format shorthand.package.v1
package amb.pkg
module amb.pkg.app src/app.short
module amb.pkg.other src/app.short
EOF
cat >"${WORK_DIR}/ambiguous/src/app.short" <<'EOF'
package amb.pkg;
module amb.pkg.app;
int value;
value = 1;
EOF
expect_failure SHD2026 "${SHORT}" "${WORK_DIR}/ambiguous/src/app.short" lock

# Import cycle.
mkdir -p "${WORK_DIR}/cycle/src"
cat >"${WORK_DIR}/cycle/shorthand.package" <<'EOF'
format shorthand.package.v1
package cycle.pkg
module cycle.pkg.a src/a.short
module cycle.pkg.b src/b.short
EOF
cat >"${WORK_DIR}/cycle/src/a.short" <<'EOF'
package cycle.pkg;
module cycle.pkg.a;
import cycle.pkg.b;
int a_value;
a_value = 1;
EOF
cat >"${WORK_DIR}/cycle/src/b.short" <<'EOF'
package cycle.pkg;
module cycle.pkg.b;
import cycle.pkg.a;
int b_value;
b_value = 1;
EOF
expect_failure SHD2025 "${SHORT}" "${WORK_DIR}/cycle/src/a.short" lock

# Graph-wide LLVM symbol collision is rejected before lowering.
mkdir -p "${WORK_DIR}/collision/src"
cat >"${WORK_DIR}/collision/shorthand.package" <<'EOF'
format shorthand.package.v1
package collision.pkg
module collision.pkg.app src/app.short
module collision.pkg.a src/a.short
module collision.pkg.b src/b.short
EOF
cat >"${WORK_DIR}/collision/src/app.short" <<'EOF'
package collision.pkg;
module collision.pkg.app;
import collision.pkg.a;
import collision.pkg.b;
int root_value;
root_value = 1;
EOF
for name in a b; do
cat >"${WORK_DIR}/collision/src/${name}.short" <<EOF
package collision.pkg;
module collision.pkg.${name};
int ${name}_value;
def int shared(int x;) { return x; };
${name}_value = 1;
EOF
done
expect_failure SHD2029 "${SHORT}" "${WORK_DIR}/collision/src/app.short" lock

# Bounded deterministic graph stress: 128 modules in a dependency chain.
STRESS="${WORK_DIR}/stress"
mkdir -p "${STRESS}/src"
{
  echo 'format shorthand.package.v1'
  echo 'package stress.pkg'
  for index in $(seq -w 0 127); do
    echo "module stress.pkg.m${index} src/m${index}.short"
  done
} >"${STRESS}/shorthand.package"
for number in $(seq 0 127); do
  index=$(printf '%03d' "${number}")
  next=$((number + 1))
  {
    echo 'package stress.pkg;'
    echo "module stress.pkg.m${index};"
    if [[ "${number}" -lt 127 ]]; then
      printf 'import stress.pkg.m%03d;\n' "${next}"
    fi
    echo "int v${index};"
    echo "v${index} = ${number};"
  } >"${STRESS}/src/m${index}.short"
done
timeout --signal=TERM --kill-after=2 20 "${SHORT}" "${STRESS}/src/m000.short" lock >"${WORK_DIR}/stress-lock.out" 2>"${WORK_DIR}/stress-lock.err"
timeout --signal=TERM --kill-after=2 20 "${SHORT}" "${STRESS}/src/m000.short" module-graph >"${WORK_DIR}/stress-graph.json" 2>"${WORK_DIR}/stress-graph.err"
grep -Fq '"entry":"stress.pkg.m000"' "${WORK_DIR}/stress-graph.json"
grep -Fq '"stress.pkg.m127"' "${WORK_DIR}/stress-graph.json"

for out in "${WORK_DIR}"/*.out "${WORK_DIR}"/*.err; do
  [[ -f "${out}" ]] || continue
  if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|Segmentation fault' "${out}"; then
    echo "error: sanitizer failure in module resolver gate: ${out}" >&2
    cat "${out}" >&2 || true
    exit 1
  fi
done

printf 'PASS deterministic module resolver, package lock and multi-file codegen gate\n'
