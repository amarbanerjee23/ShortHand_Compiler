#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
VALID="${ROOT_DIR}/tests/c3eco/profile/c3eco_profile_v2.short"
NEGATIVE="${ROOT_DIR}/tests/c3eco/profile/c3eco_profile_v2_negative_matrix.short"
LEGACY="${ROOT_DIR}/tests/c3eco/c3eco_all_blocks.short"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}" "${ROOT_DIR}/c3eco_profile_v2.bc"' EXIT

if [[ "${SHORT}" != /* ]]; then
  SHORT="$(pwd)/${SHORT}"
fi
if ! command -v jq >/dev/null 2>&1; then
  echo "error: jq is required for typed C3-ECO profile JSON validation" >&2
  exit 1
fi
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -I"${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" \
  "${ROOT_DIR}/tests/c3eco/profile/test_c3eco_profile_unit.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ast/AST.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/SemanticAnalyzer.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/visitors/Diagnostics.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/evidence/EvidenceEmitter.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/type_system/ProductionTypeSystem.cpp" \
  "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.cpp" \
  -o "${WORK_DIR}/c3eco-profile-unit"
"${WORK_DIR}/c3eco-profile-unit"

if [[ ! -x "${SHORT}" ]]; then
  make -C "${ROOT_DIR}/Compiler_new_ws/Short_Hand/src" short_hand >/tmp/shorthand_c3eco_profile_build.out 2>&1 || {
    cat /tmp/shorthand_c3eco_profile_build.out >&2 || true
    exit 1
  }
fi

"${SHORT}" "${VALID}" parse
"${SHORT}" "${VALID}" print >"${WORK_DIR}/ast.out"
grep -Eq '^certification_profile product_profile fields=9$' "${WORK_DIR}/ast.out"

"${SHORT}" "${VALID}" c3eco-report --output "${WORK_DIR}/report.json"
jq -e '
  .official_certification_granted == false and
  .c3eco_language_contract == "shorthand.c3eco.language.v1" and
  .c3eco_profile_contract == "shorthand.c3eco.profile.v2" and
  .c3eco_profile_status == "conformant" and
  .c3eco_profile_migration_required == false and
  ([.c3eco_declarations[].kind] | index("certification_profile") != null) and
  ([.c3eco_declarations[] | select(.kind == "functional_unit") |
     .typed_fields.denominator[0] | select(.type == "integer" and .value == 1000)] | length == 1) and
  ([.c3eco_declarations[] | select(.kind == "functional_unit") |
     .typed_fields.quality_threshold[0] | select(.type == "decimal" and .value == 0.9)] | length == 1) and
  ([.c3eco_declarations[] | select(.kind == "ai_lifecycle") |
     .typed_fields.training_included[0] | select(.type == "boolean" and .value == false)] | length == 1)
' "${WORK_DIR}/report.json" >/dev/null

"${SHORT}" "${VALID}" c3eco-check --output "${WORK_DIR}/check.json"
jq -e '.official_certification_granted == false and .c3eco_profile_status == "conformant"' \
  "${WORK_DIR}/check.json" >/dev/null

"${SHORT}" "${VALID}" c3eco-migrate --output "${WORK_DIR}/current-migration.json"
jq -e '
  .schema == "shorthand.c3eco.profile_migration.v1" and
  .target_contract == "shorthand.c3eco.profile.v2" and
  .status == "already_current" and
  .migration_required == false and
  .official_certification_granted == false
' "${WORK_DIR}/current-migration.json" >/dev/null

"${SHORT}" "${LEGACY}" c3eco-report --output "${WORK_DIR}/legacy-report.json"
jq -e '
  .c3eco_profile_status == "legacy_migration_review_required" and
  .c3eco_profile_migration_required == true and
  .official_certification_granted == false
' "${WORK_DIR}/legacy-report.json" >/dev/null
"${SHORT}" "${LEGACY}" c3eco-check --output "${WORK_DIR}/legacy-check.json"
jq -e '
  .c3eco_profile_status == "legacy_migration_review_required" and
  .c3eco_profile_migration_required == true and
  .official_certification_granted == false
' "${WORK_DIR}/legacy-check.json" >/dev/null
"${SHORT}" "${LEGACY}" c3eco-migrate --output "${WORK_DIR}/legacy-migration.json"
jq -e '
  .schema == "shorthand.c3eco.profile_migration.v1" and
  .status == "review_required" and
  .migration_required == true and
  .suggested_profile_references.certification == "c3eco_v1" and
  .suggested_profile_references.functional_unit == "inference_request" and
  (.review_reasons | index("legacy_string_fields_require_typed_review") != null) and
  .official_certification_granted == false
' "${WORK_DIR}/legacy-migration.json" >/dev/null

if "${SHORT}" "${NEGATIVE}" c3eco-report --output "${WORK_DIR}/negative.json" \
    >"${WORK_DIR}/negative.out" 2>"${WORK_DIR}/negative.err"; then
  echo "error: invalid typed C3-ECO profile unexpectedly passed" >&2
  exit 1
fi
for code in SHD5201 SHD5202 SHD5203 SHD5204 SHD5205 SHD5206 SHD5207 SHD5208; do
  grep -Fq "[${code}]" "${WORK_DIR}/negative.err" || {
    cat "${WORK_DIR}/negative.err" >&2 || true
    echo "error: typed C3-ECO negative matrix missing ${code}" >&2
    exit 1
  }
done

cat >"${WORK_DIR}/contextual.short" <<'EOF'
int certification_profile;
certification_profile = 2;
EOF
"${SHORT}" "${WORK_DIR}/contextual.short" parse >/dev/null

(
  cd "${ROOT_DIR}"
  "${SHORT}" "${VALID}" compile-bc >/dev/null
  test -s c3eco_profile_v2.bc
  if command -v llvm-dis >/dev/null 2>&1; then
    llvm-dis c3eco_profile_v2.bc -o - | grep -Fq 'kind=certification_profile'
  fi
)

jq -e '.properties.profile_contract.const == "shorthand.c3eco.profile.v2"' \
  "${ROOT_DIR}/schemas/c3eco/profile_v2.schema.json" >/dev/null
jq -e '.properties.schema.const == "shorthand.c3eco.profile_migration.v1"' \
  "${ROOT_DIR}/schemas/c3eco/profile_migration.schema.json" >/dev/null

for artifact in "${WORK_DIR}/report.json" "${WORK_DIR}/check.json" \
                "${WORK_DIR}/legacy-report.json" "${WORK_DIR}/legacy-check.json" \
                "${WORK_DIR}/legacy-migration.json"; do
  ! grep -Fq '"official_certification_granted": true' "${artifact}"
  ! grep -Fq 'C3-ECO Certified' "${artifact}"
done

echo "PASS typed C3-ECO profile identity units links boundary materiality lifecycle validity migration and claim-safety gate"
