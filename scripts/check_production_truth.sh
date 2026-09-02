#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TRUTH="${SHORTHAND_PRODUCTION_TRUTH_FILE:-${ROOT_DIR}/docs/production_truth.tsv}"
TRACE="${SHORTHAND_C3ECO_TRACEABILITY_FILE:-${ROOT_DIR}/docs/c3eco_traceability.tsv}"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
STRATEGY="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
TRUTH_DOC="${ROOT_DIR}/docs/production_truth.md"
LANGUAGE_SPEC="${ROOT_DIR}/docs/language_spec.md"
LANGUAGE_COMPATIBILITY="${ROOT_DIR}/docs/language_compatibility.md"
LIMITATIONS="${ROOT_DIR}/docs/known_limitations.md"
PROFILE_CONTRACT="${ROOT_DIR}/docs/c3eco_certification_profile.md"
PROFILE_GATE="${ROOT_DIR}/scripts/check_c3eco_certification_profile.sh"
MEASUREMENT_CONTRACT="${ROOT_DIR}/docs/c3eco_measurement_workbook.md"
MEASUREMENT_SCHEMA="${ROOT_DIR}/schemas/c3eco_measurement_workbook_v1.schema.json"
MEASUREMENT_TOOL="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/evidence/MeasurementWorkbook.cpp"
MEASUREMENT_GATE="${ROOT_DIR}/scripts/check_c3eco_measurement_workbook.sh"
CONTROL_FLOW_CONTRACT="${ROOT_DIR}/docs/functions_control_error_semantics.md"
CONTROL_FLOW_GATE="${ROOT_DIR}/scripts/check_functions_control_error_semantics.sh"
ENTERPRISE_CONTRACT="${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md"
ENTERPRISE_GATE="${ROOT_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh"
SERVING_CONTRACT="${ROOT_DIR}/docs/concurrent_serving_runtime.md"
SERVING_GATE="${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh"

require_file() { [[ -s "$1" ]] || { echo "error: missing or empty production truth evidence: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing production truth anchor: $2" >&2; exit 1; }; }
truth_value() { awk -F '\t' -v key="$1" 'NR > 1 && $1 == key { print $2 }' "${TRUTH}"; }

for file in \
  "${TRUTH}" "${TRACE}" "${PLAN}" "${STATUS}" "${STRATEGY}" "${MATRIX}" "${TRUTH_DOC}" \
  "${LANGUAGE_SPEC}" "${LANGUAGE_COMPATIBILITY}" "${LIMITATIONS}" \
  "${PROFILE_CONTRACT}" "${PROFILE_GATE}" "${MEASUREMENT_CONTRACT}" "${MEASUREMENT_SCHEMA}" \
  "${MEASUREMENT_TOOL}" "${MEASUREMENT_GATE}" "${CONTROL_FLOW_CONTRACT}" "${CONTROL_FLOW_GATE}" \
  "${ENTERPRISE_CONTRACT}" "${ENTERPRISE_GATE}" "${SERVING_CONTRACT}" "${SERVING_GATE}"; do
  require_file "$file"
done

[[ "$(head -n 1 "${TRUTH}")" == $'key\tvalue' ]] || { echo "error: invalid production truth header" >&2; exit 1; }
awk -F '\t' 'NF != 2 { print "error: production truth row " NR " has " NF " fields" > "/dev/stderr"; bad=1 } END { exit bad }' "${TRUTH}"
duplicate_keys="$(tail -n +2 "${TRUTH}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_keys}" ]] || { echo "error: duplicate production truth keys: ${duplicate_keys}" >&2; exit 1; }

expected_truth=(
  'schema=shorthand.production.truth.v1'
  'as_of_date=2026-09-02'
  'plan_status=active'
  'current_maturity=controlled_beta'
  'production_claim=false'
  'active_language_version=beta-0.7'
  'base_grammar_version=beta-0.2'
  'last_merged_github_pr=88'
  'current_github_pr=89'
  'last_planned_github_pr=96'
  'remaining_implementation_prs_including_current=8'
  'remaining_implementation_prs_after_current=7'
  'coverage_matrix_status=implemented=28,partial=3,open=3,total=34'
  'type_system_contract=shorthand.type_memory.v1'
  'control_flow_contract=shorthand.control_flow.v1'
  'enterprise_language_contract=shorthand.enterprise_language.v1'
  'enterprise_package_contract=shorthand.package.v2+shorthand.lock.v2'
  'core_ffi_abi=1.0.0'
  'serving_runtime_contract=shorthand.serving.runtime.v1'
  'production_backend_scope=linux-x64-cpu-v1'
  'accelerator_production_support=false'
  'c3eco_language_contract=shorthand.c3eco.language.v1'
  'c3eco_profile_contract=shorthand.c3eco.profile.v2'
  'c3eco_measurement_contract=shorthand.c3eco.measurement_workbook.v1'
  'c3eco_measurement_status=instrumented_accounting_candidate'
  'c3eco_normative_candidate=draft-v0.6'
  'c3eco_inclusion_overlay=draft-v0.7-2026-07-18'
  'c3eco_claim_status=candidate_evidence_only'
  'comparative_energy_claim=false'
  'protected_release_exercise=pending'
  'mandatory_test_skip_policy=forbidden'
)
for expected in "${expected_truth[@]}"; do
  key="${expected%%=*}"
  value="${expected#*=}"
  actual="$(truth_value "$key")"
  [[ "$actual" == "$value" ]] || { echo "error: production truth ${key} expected ${value}, found ${actual:-<missing>}" >&2; exit 1; }
done

[[ "$(head -n 1 "${TRACE}")" == $'id\tcategory\trequirement\tsource\tstatus\timplementation_evidence\tverification_evidence\towner\tclosure_target\tproduction_blocker' ]] || { echo "error: invalid C3-ECO traceability header" >&2; exit 1; }
awk -F '\t' 'NF != 10 { print "error: C3-ECO traceability row " NR " has " NF " fields" > "/dev/stderr"; bad=1 } END { exit bad }' "${TRACE}"
duplicate_ids="$(tail -n +2 "${TRACE}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_ids}" ]] || { echo "error: duplicate C3-ECO traceability IDs: ${duplicate_ids}" >&2; exit 1; }

required_ids=()
for ((number=1; number<=14; number++)); do required_ids+=("G${number}"); done
for domain in A B C D E F G H I J K; do required_ids+=("${domain}"); done
required_ids+=(S9 S12)
for id in "${required_ids[@]}"; do
  count="$(awk -F '\t' -v id="$id" 'NR > 1 && $1 == id { count++ } END { print count+0 }' "${TRACE}")"
  [[ "$count" == 1 ]] || { echo "error: C3-ECO traceability requires exactly one ${id} row, found ${count}" >&2; exit 1; }
done
[[ "$(tail -n +2 "${TRACE}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')" == 27 ]] || { echo "error: expected 27 C3-ECO traceability rows" >&2; exit 1; }

required_gate_mappings=(
  'G1=system_identity|v0.6-g1+v0.7-g1'
  'G2=functional_unit|v0.6-g2+v0.7-g2'
  'G3=boundary_declaration|v0.6-g3+v0.7-g3'
  'G4=energy_evidence|v0.6-g4+v0.7-g4'
  'G5=carbon_and_cost_calculation_where_claimed|v0.6-g5+v0.7-g5-g6'
  'G6=security_floor|v0.6-g6+v0.7-g7'
  'G7=safety_privacy_accessibility_floor|v0.6-g7+v0.7-g8'
  'G8=repeatability|v0.6-g8+v0.7-g9'
  'G9=evidence_retention|v0.6-g9+v0.7-g10'
  'G10=claims_integrity|v0.6-g10+v0.7-g11'
  'G11=no_offset_only_claim|v0.6-g11+v0.7-g12'
  'G12=recertification_acceptance|v0.6-g12+v0.7-eligibility-prerequisite'
  'G13=no_quality_degradation|v0.6-g13+v0.7-g13'
  'G14=materiality_control|v0.6-g14+v0.7-g14'
)
for mapping in "${required_gate_mappings[@]}"; do
  id="${mapping%%=*}"
  expected="${mapping#*=}"
  actual="$(awk -F '\t' -v id="$id" 'NR > 1 && $1 == id { print $3 "|" $4 }' "${TRACE}")"
  [[ "$actual" == "$expected" ]] || { echo "error: C3-ECO ${id} mapping expected ${expected}, found ${actual:-<missing>}" >&2; exit 1; }
done

while IFS=$'\t' read -r id category requirement source status implementation verification owner closure blocker; do
  [[ "$id" == id ]] && continue
  [[ "$category" == mandatory_gate || "$category" == scoring_domain || "$category" == software_class ]] || { echo "error: invalid category for ${id}: ${category}" >&2; exit 1; }
  [[ "$status" == implemented || "$status" == partial || "$status" == open ]] || { echo "error: invalid traceability status for ${id}: ${status}" >&2; exit 1; }
  [[ "$closure" =~ ^PR([8-9][0-9])$ ]] || { echo "error: invalid closure target for ${id}: ${closure}" >&2; exit 1; }
  [[ "$blocker" == yes || "$blocker" == no ]] || { echo "error: invalid blocker flag for ${id}: ${blocker}" >&2; exit 1; }
  [[ -n "$requirement" && -n "$source" && -n "$owner" ]] || { echo "error: incomplete traceability metadata for ${id}" >&2; exit 1; }
  if [[ "$implementation" != none ]]; then
    IFS=';' read -r -a paths <<< "$implementation"
    for path in "${paths[@]}"; do require_file "${ROOT_DIR}/${path}"; done
  fi
  if [[ "$verification" != none ]]; then
    IFS=';' read -r -a paths <<< "$verification"
    for path in "${paths[@]}"; do require_file "${ROOT_DIR}/${path}"; done
  fi
  if [[ "$status" == implemented ]]; then
    [[ "$implementation" != none && "$verification" != none ]] || { echo "error: implemented traceability row ${id} lacks execution evidence" >&2; exit 1; }
  fi
done < "${TRACE}"

for id in G4 G5; do
  status="$(awk -F '\t' -v id="$id" 'NR > 1 && $1 == id { print $5 }' "${TRACE}")"
  blocker="$(awk -F '\t' -v id="$id" 'NR > 1 && $1 == id { print $10 }' "${TRACE}")"
  [[ "$status" == implemented && "$blocker" == no ]] || { echo "error: PR89 requires ${id} implemented and non-blocking" >&2; exit 1; }
done

for anchor in \
  'production_readiness_plan_version: 2026-09-02-pr89' \
  'LAST_MERGED_GITHUB_PR: 88' \
  'CURRENT_GITHUB_PR: 89' \
  'LAST_PLANNED_GITHUB_PR: 96' \
  'remaining_planned_implementation_prs_pr89_through_pr96: 8' \
  'remaining_planned_implementation_prs_after_pr89: 7' \
  'PR90 - Eligibility, scoring, claims and eco-regression'; do
  require_contains "${PLAN}" "$anchor"
done
for anchor in \
  'feature_status_version: 2026-09-02-pr89' \
  'current_github_pr: 89' \
  'current_roadmap_scope: measurement_carbon_accounting_cost_workbook' \
  '28 implemented, 3 partial and 3 open' \
  'comparative_energy_claim: false'; do
  require_contains "${STATUS}" "$anchor"
done
for anchor in \
  'compiler_test_strategy_version: 2026-09-02-pr89' \
  '34-area production test matrix' \
  '28 implemented areas' \
  '3 partial areas' \
  '3 open areas' \
  'Measured-accounting changes must reject declared/modelled evidence'; do
  require_contains "${STRATEGY}" "$anchor"
done

[[ "$(tail -n +2 "${MATRIX}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')" == 34 ]] || { echo "error: expected 34 compiler test coverage rows" >&2; exit 1; }
[[ "$(awk -F '\t' 'NR > 1 && $3 == "implemented" { n++ } END { print n+0 }' "${MATRIX}")" == 28 ]] || { echo "error: expected 28 implemented compiler test rows" >&2; exit 1; }
[[ "$(awk -F '\t' 'NR > 1 && $3 == "partial" { n++ } END { print n+0 }' "${MATRIX}")" == 3 ]] || { echo "error: expected 3 partial compiler test rows" >&2; exit 1; }
[[ "$(awk -F '\t' 'NR > 1 && $3 == "open" { n++ } END { print n+0 }' "${MATRIX}")" == 3 ]] || { echo "error: expected 3 open compiler test rows" >&2; exit 1; }
for number in $(seq 1 34); do require_contains "${MATRIX}" "$(printf 'TST%03d' "$number")"; done
require_contains "${MATRIX}" $'TST034\tinstrumented energy carbon and cost accounting\timplemented'
require_contains "${MATRIX}" $'TST026\tmeasured energy comparison with Python\topen'

require_contains "${MEASUREMENT_CONTRACT}" 'shorthand.c3eco.measurement_workbook.v1'
require_contains "${MEASUREMENT_CONTRACT}" '`modelled`, `declared_budget_only`'
require_contains "${MEASUREMENT_CONTRACT}" 'cumulative allocation must not exceed 1.0'
require_contains "${MEASUREMENT_CONTRACT}" 'PR95 owns equivalent-workload ShortHand/Python performance and energy comparison'
require_contains "${MEASUREMENT_SCHEMA}" 'shorthand.c3eco.measurement_workbook.v1'
require_contains "${MEASUREMENT_SCHEMA}" 'official_certification_granted'
require_contains "${MEASUREMENT_TOOL}" 'double counting detected'
require_contains "${MEASUREMENT_TOOL}" 'base_footprint_not_reduced_by_offsets'
require_contains "${MEASUREMENT_GATE}" 'PASS: PR89 C3-ECO measurement, carbon accounting and cost workbook gate'

require_contains "${PROFILE_CONTRACT}" 'c3eco_profile_contract: shorthand.c3eco.profile.v2'
require_contains "${CONTROL_FLOW_CONTRACT}" 'shorthand.control_flow.v1'
require_contains "${ENTERPRISE_CONTRACT}" 'shorthand.enterprise_language.v1'
require_contains "${SERVING_CONTRACT}" 'shorthand.serving.runtime.v1'
require_contains "${LANGUAGE_SPEC}" 'Language version: beta-0.7'
require_contains "${LANGUAGE_SPEC}" 'production_claim: false'
require_contains "${LANGUAGE_COMPATIBILITY}" 'active_language_version: beta-0.7'
require_contains "${LIMITATIONS}" 'production_backend_scope: linux-x64-cpu-v1'
require_contains "${TRUTH_DOC}" 'production_truth_contract: shorthand.production.truth.v1'
require_contains "${TRUTH_DOC}" 'candidate evidence'

bash "${MEASUREMENT_GATE}"

printf 'PASS production truth and C3-ECO traceability gate\n'
