#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TRUTH="${SHORTHAND_PRODUCTION_TRUTH_FILE:-${ROOT_DIR}/docs/production_truth.tsv}"
TRACE="${SHORTHAND_C3ECO_TRACEABILITY_FILE:-${ROOT_DIR}/docs/c3eco_traceability.tsv}"
TRUTH_DOC="${ROOT_DIR}/docs/production_truth.md"
PLAN="${ROOT_DIR}/docs/production_readiness_pr_plan.md"
STATUS="${ROOT_DIR}/docs/feature_implementation_status.md"
STRATEGY="${ROOT_DIR}/docs/compiler_test_strategy.md"
MATRIX="${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv"
LANGUAGE_SPEC="${ROOT_DIR}/docs/language_spec.md"
LANGUAGE_COMPATIBILITY="${ROOT_DIR}/docs/language_compatibility.md"
LIMITATIONS="${ROOT_DIR}/docs/known_limitations.md"
RELEASE_STATUS="${ROOT_DIR}/docs/release_level_status.md"
PUBLIC_READINESS="${ROOT_DIR}/docs/public_release_readiness.md"
ENTERPRISE_SCORECARD="${ROOT_DIR}/docs/enterprise_release_scorecard.md"
SBOM_STATUS="${ROOT_DIR}/docs/sbom_plan.md"
OBSERVABILITY_STATUS="${ROOT_DIR}/docs/observability_plan.md"
PIPELINE="${ROOT_DIR}/docs/ci_pipeline_architecture.md"
C3ECO_CONTRACT="${ROOT_DIR}/docs/c3eco_language_contract.md"
CONTROL_FLOW_CONTRACT="${ROOT_DIR}/docs/functions_control_error_semantics.md"
CONTROL_FLOW_MATRIX="${ROOT_DIR}/tests/conformance/functions_control_matrix_beta_0_5.tsv"
CONTROL_FLOW_GATE="${ROOT_DIR}/scripts/check_functions_control_error_semantics.sh"
ENTERPRISE_CONTRACT="${ROOT_DIR}/docs/enterprise_packages_stdlib_ffi.md"
ENTERPRISE_MATRIX="${ROOT_DIR}/tests/conformance/enterprise_matrix_beta_0_6.tsv"
ENTERPRISE_GATE="${ROOT_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh"
README="${ROOT_DIR}/README.md"
OBJECTIVES="${ROOT_DIR}/docs/language_objectives.md"
ENTERPRISE_STRATEGY="${ROOT_DIR}/docs/enterprise_release_strategy.md"
HISTORICAL_RELEASE_PLAN="${ROOT_DIR}/docs/release_plan_v3_beta.md"
HISTORICAL_BETA_REQUIREMENTS="${ROOT_DIR}/docs/beta_enterprise_requirements.md"
HISTORICAL_DIAGNOSTICS_PLAN="${ROOT_DIR}/docs/diagnostics_runtime_mlir_release_plan.md"

require_file() { [[ -s "$1" ]] || { echo "error: missing or empty production truth evidence: $1" >&2; exit 1; }; }
require_contains() { require_file "$1"; grep -Fq "$2" "$1" || { echo "error: $1 missing production truth anchor: $2" >&2; exit 1; }; }
truth_value() { awk -F '\t' -v key="$1" 'NR > 1 && $1 == key { print $2 }' "${TRUTH}"; }

for file in "${TRUTH}" "${TRACE}" "${TRUTH_DOC}" "${PLAN}" "${STATUS}" "${STRATEGY}" "${MATRIX}" \
  "${LANGUAGE_SPEC}" "${LANGUAGE_COMPATIBILITY}" "${LIMITATIONS}" "${RELEASE_STATUS}" \
  "${PUBLIC_READINESS}" "${ENTERPRISE_SCORECARD}" "${SBOM_STATUS}" "${OBSERVABILITY_STATUS}" "${PIPELINE}" "${C3ECO_CONTRACT}" \
  "${CONTROL_FLOW_CONTRACT}" "${CONTROL_FLOW_MATRIX}" "${CONTROL_FLOW_GATE}" \
  "${ENTERPRISE_CONTRACT}" "${ENTERPRISE_MATRIX}" "${ENTERPRISE_GATE}" "${README}"; do
  require_file "${file}"
done
for file in "${OBJECTIVES}" "${ENTERPRISE_STRATEGY}" "${HISTORICAL_RELEASE_PLAN}" \
  "${HISTORICAL_BETA_REQUIREMENTS}" "${HISTORICAL_DIAGNOSTICS_PLAN}"; do
  require_file "${file}"
done

[[ "$(head -n 1 "${TRUTH}")" == $'key\tvalue' ]] || { echo "error: invalid production truth header" >&2; exit 1; }
awk -F '\t' 'NF != 2 { print "error: production truth row " NR " has " NF " fields" > "/dev/stderr"; bad=1 } END { exit bad }' "${TRUTH}"
duplicate_keys="$(tail -n +2 "${TRUTH}" | cut -f1 | sort | uniq -d)"
[[ -z "${duplicate_keys}" ]] || { echo "error: duplicate production truth keys: ${duplicate_keys}" >&2; exit 1; }

expected_truth=(
  'schema=shorthand.production.truth.v1'
  'as_of_date=2026-08-22'
  'plan_status=active'
  'current_maturity=controlled_beta'
  'production_claim=false'
  'active_language_version=beta-0.6'
  'base_grammar_version=beta-0.2'
  'last_merged_github_pr=85'
  'current_github_pr=86'
  'last_planned_github_pr=96'
  'remaining_implementation_prs_including_current=11'
  'remaining_implementation_prs_after_current=10'
  'coverage_matrix_status=implemented=25,partial=3,open=3,total=31'
  'type_system_contract=shorthand.type_memory.v1'
  'control_flow_contract=shorthand.control_flow.v1'
  'enterprise_language_contract=shorthand.enterprise_language.v1'
  'enterprise_package_contract=shorthand.package.v2+shorthand.lock.v2'
  'core_ffi_abi=1.0.0'
  'production_backend_scope=linux-x64-cpu-v1'
  'accelerator_production_support=false'
  'c3eco_language_contract=shorthand.c3eco.language.v1'
  'c3eco_normative_candidate=draft-v0.6'
  'c3eco_inclusion_overlay=draft-v0.7-2026-07-18'
  'c3eco_claim_status=candidate_evidence_only'
  'protected_release_exercise=pending'
  'mandatory_test_skip_policy=forbidden'
)
for expected in "${expected_truth[@]}"; do
  key="${expected%%=*}"
  value="${expected#*=}"
  actual="$(truth_value "${key}")"
  [[ "${actual}" == "${value}" ]] || { echo "error: production truth ${key} expected ${value}, found ${actual:-<missing>}" >&2; exit 1; }
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
  count="$(awk -F '\t' -v id="${id}" 'NR > 1 && $1 == id { count++ } END { print count+0 }' "${TRACE}")"
  [[ "${count}" == 1 ]] || { echo "error: C3-ECO traceability requires exactly one ${id} row, found ${count}" >&2; exit 1; }
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
  actual="$(awk -F '\t' -v id="${id}" 'NR > 1 && $1 == id { print $3 "|" $4 }' "${TRACE}")"
  [[ "${actual}" == "${expected}" ]] || { echo "error: C3-ECO ${id} mapping expected ${expected}, found ${actual:-<missing>}" >&2; exit 1; }
done

while IFS=$'\t' read -r id category requirement source status implementation verification owner closure blocker; do
  [[ "${id}" == "id" ]] && continue
  [[ "${category}" == "mandatory_gate" || "${category}" == "scoring_domain" || "${category}" == "software_class" ]] || { echo "error: invalid category for ${id}: ${category}" >&2; exit 1; }
  [[ "${status}" == "implemented" || "${status}" == "partial" || "${status}" == "open" ]] || { echo "error: invalid traceability status for ${id}: ${status}" >&2; exit 1; }
  [[ "${closure}" =~ ^PR([8-9][0-9])$ ]] || { echo "error: invalid closure target for ${id}: ${closure}" >&2; exit 1; }
  [[ "${blocker}" == "yes" || "${blocker}" == "no" ]] || { echo "error: invalid blocker flag for ${id}: ${blocker}" >&2; exit 1; }
  [[ -n "${requirement}" && -n "${source}" && -n "${owner}" ]] || { echo "error: incomplete traceability metadata for ${id}" >&2; exit 1; }
  if [[ "${implementation}" != "none" ]]; then
    IFS=';' read -r -a paths <<< "${implementation}"
    for path in "${paths[@]}"; do require_file "${ROOT_DIR}/${path}"; done
  fi
  if [[ "${verification}" != "none" ]]; then
    IFS=';' read -r -a paths <<< "${verification}"
    for path in "${paths[@]}"; do require_file "${ROOT_DIR}/${path}"; done
  fi
  if [[ "${status}" == "implemented" ]]; then
    [[ "${implementation}" != "none" && "${verification}" != "none" ]] || { echo "error: implemented traceability row ${id} lacks execution evidence" >&2; exit 1; }
  fi
done < "${TRACE}"

for anchor in \
  'production_truth_contract: shorthand.production.truth.v1' \
  'current_maturity: controlled_beta' \
  'production_claim: false' \
  'beta-0.6' \
  'shorthand.type_memory.v1' \
  'shorthand.control_flow.v1' \
  'shorthand.enterprise_language.v1' \
  'draft v0.6' \
  'v0.7' \
  'candidate evidence' \
  'v0.7 inserts cost calculation where claimed at G6' \
  'C3-ECO_Green_Software_Certification_Standard_v0.6_updated.docx' \
  'C3-ECO_draft_annotated_review.pdf' \
  'C3-ECO_UK_Market_Savings_Figures.docx' \
  'linux-x64-cpu-v1'; do
  require_contains "${TRUTH_DOC}" "${anchor}"
done

for anchor in \
  'production_readiness_plan_version: 2026-08-22-pr86' \
  'LAST_MERGED_GITHUB_PR: 85' \
  'CURRENT_GITHUB_PR: 86' \
  'LAST_PLANNED_GITHUB_PR: 96' \
  'remaining_planned_implementation_prs_pr86_through_pr96: 11' \
  'remaining_planned_implementation_prs_after_pr86: 10'; do
  require_contains "${PLAN}" "${anchor}"
done

for anchor in \
  'feature_status_version: 2026-08-22-pr86' \
  'current_github_pr: 86' \
  'current_roadmap_scope: enterprise_packages_standard_library_and_safe_ffi' \
  '25 implemented, 3 partial and 3 open'; do
  require_contains "${STATUS}" "${anchor}"
done

require_contains "${STRATEGY}" 'compiler_test_strategy_version: 2026-08-22-pr86'
require_contains "${STRATEGY}" '31-area production test matrix'
require_contains "${MATRIX}" $'TST028\tproduction truth and C3-ECO traceability\timplemented'
require_contains "${MATRIX}" $'TST029\tproduction type system and memory model\timplemented'
require_contains "${MATRIX}" $'TST030\tfunctions structured control flow and deterministic errors\timplemented'
require_contains "${MATRIX}" $'TST031\tenterprise language packages core library and FFI\timplemented'

for anchor in \
  'Language version: beta-0.6' \
  'Base grammar version: beta-0.2' \
  'production_claim: false'; do
  require_contains "${LANGUAGE_SPEC}" "${anchor}"
done
for anchor in \
  'language_compatibility_contract: shorthand.language.compatibility.v1' \
  'active_language_version: beta-0.6' \
  'production_claim: false'; do
  require_contains "${LANGUAGE_COMPATIBILITY}" "${anchor}"
done
for anchor in \
  'known_limitations_version: 2026-08-22-pr86' \
  'current_maturity: controlled_beta' \
  'production_backend_scope: linux-x64-cpu-v1'; do
  require_contains "${LIMITATIONS}" "${anchor}"
done
for anchor in \
  'release_level_status_version: 2026-08-22-pr86' \
  'current_maturity: controlled_beta' \
  'final_planned_github_pr: 96'; do
  require_contains "${RELEASE_STATUS}" "${anchor}"
done
for anchor in \
  'public_release_readiness_version: 2026-08-22-pr86' \
  'current_maturity: controlled_beta' \
  'release_candidate_target: PR96'; do
  require_contains "${PUBLIC_READINESS}" "${anchor}"
done
for anchor in \
  'enterprise_release_scorecard_version: 2026-08-22-pr86' \
  'current_state: ER3-controlled-beta' \
  'target_state: ER4-enterprise-release-candidate'; do
  require_contains "${ENTERPRISE_SCORECARD}" "${anchor}"
done
require_contains "${SBOM_STATUS}" 'current_status: implemented_candidate_and_artifact_baseline'
require_contains "${OBSERVABILITY_STATUS}" 'current_status: partial_dependency_free_exports'
require_contains "${PIPELINE}" 'ci_pipeline_architecture_version: 2026-08-22-pr86'
require_contains "${CONTROL_FLOW_CONTRACT}" 'control_flow_contract: shorthand.control_flow.v1'
require_contains "${CONTROL_FLOW_MATRIX}" $'CTL025\tcompatibility'
require_contains "${CONTROL_FLOW_GATE}" 'PASS beta-0.5 functions scopes control flow deterministic errors and cleanup gate'
require_contains "${ENTERPRISE_CONTRACT}" 'enterprise_contract: shorthand.enterprise_language.v1'
require_contains "${ENTERPRISE_MATRIX}" $'ENT024\tinstalled-consumer'
require_contains "${ENTERPRISE_GATE}" 'PASS enterprise packages standard library and safe FFI gate'
require_contains "${C3ECO_CONTRACT}" 'normative_candidate: C3-ECO draft v0.6'
require_contains "${C3ECO_CONTRACT}" 'inclusion_overlay: C3-ECO draft v0.7 dated 2026-07-18'
require_contains "${C3ECO_CONTRACT}" 'A programming language, framework, cloud, backend or model is not inherently green.'
require_contains "${README}" 'Current maturity: `controlled_beta`. Production claim: `false`.'
require_contains "${README}" 'Active language: beta-0.6'
require_contains "${README}" 'shorthand.control_flow.v1'
require_contains "${README}" 'The only qualified backend scope is `linux-x64-cpu-v1`'
require_contains "${OBJECTIVES}" 'Production truth and certification traceability: PR83.'
require_contains "${OBJECTIVES}" 'Production type and memory model: PR84.'
require_contains "${OBJECTIVES}" 'Functions, lexical scopes, structured control flow and deterministic errors: PR85.'
require_contains "${OBJECTIVES}" 'Enterprise composite/ownership schemas, cryptographic offline packages, core library and safe FFI: PR86.'
require_contains "${OBJECTIVES}" 'Measured performance/energy and the enterprise release-candidate aggregate: PR95 through PR96.'
require_contains "${ENTERPRISE_STRATEGY}" 'Current maturity: controlled enterprise beta (ER3), not an enterprise release candidate.'
require_contains "${HISTORICAL_RELEASE_PLAN}" 'document_status: historical_superseded'
require_contains "${HISTORICAL_BETA_REQUIREMENTS}" 'document_status: historical_baseline'
require_contains "${HISTORICAL_DIAGNOSTICS_PLAN}" 'document_status: historical_superseded'

implemented="$(awk -F '\t' 'NR > 1 && $5 == "implemented" { count++ } END { print count+0 }' "${TRACE}")"
partial="$(awk -F '\t' 'NR > 1 && $5 == "partial" { count++ } END { print count+0 }' "${TRACE}")"
open="$(awk -F '\t' 'NR > 1 && $5 == "open" { count++ } END { print count+0 }' "${TRACE}")"
printf 'PRODUCTION_TRUTH current_pr=86 remaining=11 maturity=controlled_beta production_claim=false\n'
printf 'C3ECO_TRACEABILITY implemented=%s partial=%s open=%s total=27\n' "${implemented}" "${partial}" "${open}"
printf 'PASS production truth and C3-ECO traceability gate\n'
