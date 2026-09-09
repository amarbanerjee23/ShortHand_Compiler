#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/evidence/CertificationAssessment.cpp"
SCORING_SOURCE="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/evidence/C3EcoAssessmentScoring.cpp"
CXX_BIN="${CXX:-c++}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if ! command -v jq >/dev/null 2>&1; then
  echo "error: jq is required for C3-ECO assessment validation" >&2
  exit 1
fi

if [[ -n "${SHORTHAND_C3ECO_ASSESS_BIN:-}" ]]; then
  TOOL="${SHORTHAND_C3ECO_ASSESS_BIN}"
  [[ -x "${TOOL}" ]] || { echo "assessment tool is not executable: ${TOOL}" >&2; exit 1; }
else
  TOOL="${WORK_DIR}/shorthand_c3eco_assess"
  # Deliberate word splitting permits the caller to supply a compiler flag vector.
  # shellcheck disable=SC2086
  "${CXX_BIN}" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
    "${SOURCE}" "${SCORING_SOURCE}" -o "${TOOL}"
fi

# Exercise scoring without the input adapter, including every mandatory gate and DQ ceilings.
# shellcheck disable=SC2086
"${CXX_BIN}" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
  -I"${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/evidence" "${SCORING_SOURCE}" \
  "${ROOT_DIR}/tests/c3eco/assessment/test_c3eco_assessment_scoring.cpp" \
  -o "${WORK_DIR}/scoring-unit"
"${WORK_DIR}/scoring-unit"

# Use real PR88 and PR89 producers so fixture-only reports cannot hide integration defects.
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORT="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
if [[ ! -x "${SHORT}" ]]; then
  make -C "${SRC_DIR}" short_hand >"${WORK_DIR}/compiler-build.log" 2>&1 || {
    cat "${WORK_DIR}/compiler-build.log" >&2; exit 1;
  }
fi
SHORT="$(cd "$(dirname "${SHORT}")" && pwd)/$(basename "${SHORT}")"
MEASURE="${SHORTHAND_C3ECO_MEASURE_BIN:-${WORK_DIR}/shorthand_c3eco_measure}"
if [[ -z "${SHORTHAND_C3ECO_MEASURE_BIN:-}" ]]; then
  # shellcheck disable=SC2086
  "${CXX_BIN}" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
    "${SRC_DIR}/evidence/MeasurementWorkbook.cpp" -o "${MEASURE}"
fi
[[ -x "${MEASURE}" ]] || { echo "measurement tool is not executable: ${MEASURE}" >&2; exit 1; }
GENERATED="${WORK_DIR}/generated"
mkdir -p "${GENERATED}"
(cd "${GENERATED}" && "${SHORT}" "${ROOT_DIR}/tests/c3eco/profile/c3eco_profile_v2.short" \
  c3eco-report --output "${GENERATED}/profile.json")
sed 's/S6_AI_GENAI/S9_DEVELOPER_TOOLS_CI_CD/' \
  "${ROOT_DIR}/tests/c3eco/profile/c3eco_profile_v2.short" >"${GENERATED}/non-ai.short"
(cd "${GENERATED}" && "${SHORT}" "${GENERATED}/non-ai.short" \
  c3eco-report --output "${GENERATED}/non-ai-profile.json")
cat >"${GENERATED}/measurement.tsv" <<'EOF'
record_id	component	source_kind	instrument_id	calibration_id	calibration_date	measured_at	raw_energy_j	allocation_fraction	pue	carbon_factor_gco2e_per_kwh	factor_source	factor_date	tariff_per_kwh	tariff_currency	tariff_source	uncertainty_percent	measurement_quality	data_quality	evidence_ref
r1	compute	rapl	cpu-package-0	cal-rapl-2026	2026-08-01	2026-09-01T10:00:00Z	3600000	1	1.2	400	grid-operator-published	2026-09-01	8.5	INR	utility-tariff-2026	5	high	high	evidence/rapl.json
r2	memory	physical_meter	pdu-7	cal-pdu-2026	2026-07-01	2026-09-01T10:00:00Z	1800000	1	1.2	400	grid-operator-published	2026-09-01	8.5	INR	utility-tariff-2026	5	high	high	evidence/pdu.csv
EOF
"${MEASURE}" "${GENERATED}/measurement.tsv" "${GENERATED}/measurement.csv" \
  "${GENERATED}/measurement.json" >/dev/null
jq -e '.c3eco_profile_status == "conformant" and .official_certification_granted == false' \
  "${GENERATED}/profile.json" >/dev/null
jq -e '.measurement_status == "measured_instrumented" and .record_count == 2' \
  "${GENERATED}/measurement.json" >/dev/null

write_artifacts() {
  local directory="$1"
  mkdir -p "${directory}"
  cp "${GENERATED}/profile.json" "${directory}/profile.json"
  cp "${GENERATED}/measurement.json" "${directory}/measurement.json"
}

write_metadata() {
  local directory="$1"
  local mq="${2:-MQ4}"
  local uncertainty="${3:-5}"
  local ai="${4:-true}"
  local role="${5:-application_deployer}"
  local training="${6:-true}"
  local inference="${7:-true}"
  local metric="${8:-true}"
  local exclusions="${9:-true}"
  local frontier="${10:-2}"
  local client="${11:-true}"
  local eco_control="${12:-true}"
  local independent="${13:-true}"
  local public_report="${14:-true}"
  local telemetry="${15:-true}"
  local public_evidence="${16:-true}"
  local claim_risk="${17:-false}"
  local high_scale="${18:-true}"
  local dq="${19:-DQ4}"
  local penalty="${20:-0}"
  local software_class=S6_AI_GENAI
  if [[ "${ai}" == false ]]; then
    software_class=S9_DEVELOPER_TOOLS_CI_CD
    cp "${GENERATED}/non-ai-profile.json" "${directory}/profile.json"
  else
    cp "${GENERATED}/profile.json" "${directory}/profile.json"
  fi
  cat >"${directory}/metadata.tsv" <<EOF
key	value	evidence_status	evidence_ref
product_name	product_identity	verified	evidence/profile.json#product
product_version	1.2.0	verified	evidence/profile.json#version
software_class	${software_class}	verified	evidence/profile.json#software-class
functional_unit	successful_inference_unit	verified	evidence/profile.json#functional-unit
boundary	product_boundary	verified	evidence/profile.json#boundary
workload	representative_workload	verified	evidence/profile.json#workload
measurement_quality	${mq}	independent	evidence/measurement.json#quality
data_quality	${dq}	independent	evidence/measurement.json#data-quality
penalty_points	${penalty}	verified	evidence/penalties.json
uncertainty_percent	${uncertainty}	verified	evidence/measurement.json#uncertainty
ai_in_scope	${ai}	verified	evidence/profile.json#ai-scope
ai_role	${role}	verified	evidence/profile.json#ai-role
training_boundary_declared	${training}	verified	evidence/profile.json#training-boundary
inference_boundary_declared	${inference}	verified	evidence/profile.json#inference-boundary
token_or_inference_metric_declared	${metric}	verified	evidence/profile.json#functional-unit
ai_exclusions_declared	${exclusions}	verified	evidence/profile.json#exclusions
quality_energy_frontier_options	${frontier}	verified	evidence/frontier.json
client_device_impact	${client}	verified	evidence/profile.json#device-impact
eco_regression_control_defined	${eco_control}	verified	evidence/regression-policy.md
independent_review	${independent}	independent	evidence/review.json
public_report	${public_report}	verified	evidence/public-report.json
continuous_telemetry	${telemetry}	verified	evidence/telemetry.json
public_evidence	${public_evidence}	verified	evidence/public-index.json
high_severity_claim_risk	${claim_risk}	verified	evidence/claim-review.json
high_scale_saas	${high_scale}	verified	evidence/deployment.json
EOF
}

write_gates() {
  local directory="$1"
  local failed="${2:-none}"
  printf 'gate_id\tstatus\tevidence_ref\n' >"${directory}/gates.tsv"
  local number status
  for number in $(seq 1 14); do
    status=pass
    [[ "G${number}" == "${failed}" ]] && status=fail
    printf 'G%s\t%s\tevidence/g%s.json\n' "${number}" "${status}" "${number}" >>"${directory}/gates.tsv"
  done
}

criterion_count() {
  case "$1" in
    A|B|C|D) printf '8' ;;
    E) printf '6' ;;
    F|K) printf '7' ;;
    G) printf '10' ;;
    H|I) printf '5' ;;
    J) printf '4' ;;
    *) return 1 ;;
  esac
}

write_uniform_criteria() {
  local directory="$1"
  local percent="$2"
  local insufficient_domain="${3:-none}"
  printf 'criterion_id\tdomain\traw_score\tcriterion_weight\tevidence_status\tapplicable\tauditor_approval_ref\tevidence_ref\n' \
    >"${directory}/criteria.tsv"
  local domain evidence_status count number high_weight low_weight first_weight
  for domain in A B C D E F G H I J K; do
    count="$(criterion_count "${domain}")"
    evidence_status=sufficient
    [[ "${domain}" == "${insufficient_domain}" ]] && evidence_status=insufficient
    if [[ "${percent}" == "100" ]]; then
      for ((number=1; number<=count; ++number)); do
        first_weight=1
        [[ "${number}" == 1 && "${domain}" == "${insufficient_domain}" ]] && first_weight=1000
        evidence_status=sufficient
        [[ "${number}" == 1 && "${domain}" == "${insufficient_domain}" ]] && evidence_status=insufficient
        printf '%s%s\t%s\t5\t%s\t%s\ttrue\t-\tevidence/%s%s.json\n' \
          "${domain}" "${number}" "${domain}" "${first_weight}" "${evidence_status}" \
          "${domain}" "${number}" >>"${directory}/criteria.tsv"
      done
    elif [[ "${percent}" == "0" ]]; then
      for ((number=1; number<=count; ++number)); do
        printf '%s%s\t%s\t0\t1\tsufficient\ttrue\t-\tevidence/%s%s.json\n' \
          "${domain}" "${number}" "${domain}" "${domain}" "${number}" >>"${directory}/criteria.tsv"
      done
    else
      high_weight="$((percent * (count - 1)))"
      low_weight="$((100 - percent))"
      printf '%s1\t%s\t5\t%s\t%s\ttrue\t-\tevidence/%s1.json\n' \
        "${domain}" "${domain}" "${high_weight}" "${evidence_status}" "${domain}" >>"${directory}/criteria.tsv"
      for ((number=2; number<=count; ++number)); do
        printf '%s%s\t%s\t0\t%s\tsufficient\ttrue\t-\tevidence/%s%s.json\n' \
          "${domain}" "${number}" "${domain}" "${low_weight}" "${domain}" "${number}" >>"${directory}/criteria.tsv"
      done
    fi
  done
}

mark_domains_na() {
  local path="$1"
  shift
  local domains=" $* "
  awk -F '\t' -v domains="${domains}" 'BEGIN { OFS="\t" }
    NR == 1 { print; next }
    index(domains, " " $2 " ") {
      print $1, $2, 0, 1, "not_applicable", "false", "auditor/approval.json", "evidence/na.json";
      next
    }
    { print }
  ' "${path}" >"${path}.new"
  mv "${path}.new" "${path}"
}

write_materiality() {
  local directory="$1"
  local disposition="${2:-included}"
  local share="${3:-100}"
  printf 'component\tshare_percent\tdisposition\tevidence_ref\n' >"${directory}/materiality.tsv"
  printf 'runtime_and_service\t%s\t%s\tevidence/boundary.json\n' "${share}" "${disposition}" \
    >>"${directory}/materiality.tsv"
  if [[ "${share}" != "100" ]]; then
    awk -v share="${share}" 'BEGIN { printf "other_accounted_components\t%.12g\tincluded\tevidence/other-boundary.json\n", 100-share }' \
      >>"${directory}/materiality.tsv"
  fi
}

write_restricted_claims() {
  local directory="$1"
  cat >"${directory}/claims.tsv" <<'EOF'
claim_id	claim_type	claim_text	scope_matches	functional_unit_equivalent	boundary_equivalent	quality_equivalent	method_equivalent	legal_technical_evidence	evidence_ref
c1	candidate_assessment	Candidate evidence was assessed for the declared scope.	true	false	false	false	false	false	evidence/assessment.json
c2	certified	Requested certified-level wording.	true	false	false	false	false	true	evidence/internal-score.json
c3	comparative	Uses less energy than the equivalent baseline.	true	true	true	true	true	true	evidence/comparison.json
c4	zero	Requested zero-carbon wording.	true	false	false	false	false	false	evidence/marketing-request.json
c5	green_ai	Green AI evidence for hosted inference.	true	false	false	false	false	false	evidence/ai-boundary.json
c6	offsets_only	Base footprint is reduced by offsets.	true	false	false	false	false	true	evidence/offsets.json
EOF
}

write_claims() {
  local directory="$1"
  cat >"${directory}/claims.tsv" <<'EOF'
claim_id	claim_type	claim_text	scope_matches	functional_unit_equivalent	boundary_equivalent	quality_equivalent	method_equivalent	legal_technical_evidence	evidence_ref
c1	candidate_assessment	Candidate evidence was assessed for the declared scope.	true	false	false	false	false	false	evidence/assessment.json
EOF
}

write_no_regressions() {
  printf 'metric_id\tkind\tbaseline_value\tcurrent_value\tlower_is_better\texplanation\tcorrective_action_ref\tevidence_ref\n' \
    >"$1/regressions.tsv"
}

make_candidate() {
  local directory="$1"
  local score="${2:-95}"
  write_artifacts "${directory}"
  write_metadata "${directory}"
  write_gates "${directory}"
  write_uniform_criteria "${directory}" "${score}"
  write_materiality "${directory}"
  write_claims "${directory}"
  write_no_regressions "${directory}"
}

expect_fail() {
  local name="$1"
  local directory="$2"
  local expected="$3"
  if "${TOOL}" "${directory}" "${WORK_DIR}/${name}.json" \
      >"${WORK_DIR}/${name}.out" 2>"${WORK_DIR}/${name}.err"; then
    echo "expected failure for ${name}" >&2
    exit 1
  fi
  grep -Fq "${expected}" "${WORK_DIR}/${name}.err" || {
    cat "${WORK_DIR}/${name}.err" >&2
    echo "missing expected error for ${name}: ${expected}" >&2
    exit 1
  }
}

BASE="${WORK_DIR}/base"
make_candidate "${BASE}" 95
"${TOOL}" "${BASE}" "${WORK_DIR}/base.json" "${WORK_DIR}/base.md"
jq -e '
  .schema == "shorthand.c3eco.assessment.v1" and
  .decision_kind == "candidate_recommendation_only" and
  .official_certification_granted == false and
  .production_claim == false and
  .comparative_energy_claim == false and
  .assessment_controls.measurement_quality == "MQ4" and
  .assessment_controls.data_quality == "DQ4" and
  .assessment_controls.uncertainty_percent == 5 and
  .assessment_controls.eco_regression_control_defined == true and
  .assessment_controls.high_severity_claim_risk == false and
  .eligibility.status == "eligible_for_external_review" and
  .scoring.total_score == 95 and
  .scoring.score_ceiling_level == "diamond" and
  .scoring.recommended_level_for_external_review == "diamond" and
  .scoring.level_claim_permitted == false and
  .scoring.normative_weights_sum == 100 and
  .scoring.reallocated_weights_sum == 100 and
  ([.scoring.criteria[]] | length) == 76 and
  ([.scoring.domains[].normative_weight] | add) == 100 and
  ([.scoring.domains[].adjusted_weight] | add) == 100 and
  ([.claims[] | select(.id == "c1" and .decision == "permitted")] | length) == 1 and
  .surveillance.cadence_months == 6 and
  (.surveillance.major_change_triggers | index("model") != null)
' "${WORK_DIR}/base.json" >/dev/null

# Every normative score boundary selects the exact level with prerequisites held constant.
for case in '39 none' '40 candidate' '49 candidate' '50 bronze' '64 bronze' '65 silver' \
            '79 silver' '80 gold' '89 gold' '90 platinum' '94 platinum' '95 diamond' '100 diamond'; do
  read -r score expected <<<"${case}"
  directory="${WORK_DIR}/tier-${score}"
  make_candidate "${directory}" "${score}"
  "${TOOL}" "${directory}" "${WORK_DIR}/tier-${score}.json"
  jq -e --arg expected "${expected}" --argjson score "${score}" \
    '.scoring.total_score == $score and .scoring.recommended_level_for_external_review == $expected' \
    "${WORK_DIR}/tier-${score}.json" >/dev/null
done

# A mandatory gate failure always wins over a Diamond-range score.
GATE_FAILURE="${WORK_DIR}/gate-failure"
make_candidate "${GATE_FAILURE}" 100
write_gates "${GATE_FAILURE}" G6
"${TOOL}" "${GATE_FAILURE}" "${WORK_DIR}/gate-failure.json"
jq -e '
  .eligibility.status == "not_eligible" and
  .eligibility.failed_gates == ["G6"] and
  .scoring.total_score == 100 and
  .scoring.recommended_level_for_external_review == "none" and
  (.scoring.decision_reasons | index("mandatory_gate_failure_precedes_scoring") != null)
' "${WORK_DIR}/gate-failure.json" >/dev/null

# Insufficient evidence caps a criterion at one point even when five was entered.
EVIDENCE_CAP="${WORK_DIR}/evidence-cap"
make_candidate "${EVIDENCE_CAP}" 100
write_uniform_criteria "${EVIDENCE_CAP}" 100 A
"${TOOL}" "${EVIDENCE_CAP}" "${WORK_DIR}/evidence-cap.json"
jq -e '
  (.scoring.evidence_caps_applied | index("A1") != null) and
  ([.scoring.criteria[] | select(.id == "A1" and .raw_score == 5 and .effective_score == 1)] | length) == 1 and
  ([.scoring.domains[] | select(.domain == "A" and .domain_percent > 20 and .domain_percent < 21)] | length) == 1 and
  .scoring.recommended_level_for_external_review == "candidate"
' "${WORK_DIR}/evidence-cap.json" >/dev/null

# Approved N/A domains reallocate proportionally and cannot bypass scope rules.
NA_OK="${WORK_DIR}/na-ok"
make_candidate "${NA_OK}" 80
write_metadata "${NA_OK}" MQ3 8 false not_applicable false false false false 0 false true true true false false false false
mark_domains_na "${NA_OK}/criteria.tsv" E G H J
"${TOOL}" "${NA_OK}" "${WORK_DIR}/na-ok.json"
jq -e '
  ([.scoring.domains[] | select((.domain == "E" or .domain == "G" or .domain == "H" or .domain == "J") and
      .applicable == false and .adjusted_weight == 0)] | length) == 4 and
  (([.scoring.domains[].adjusted_weight] | add) - 100 | fabs) < 0.0000001 and
  .scoring.total_score == 80 and
  .surveillance.cadence_months == 12
' "${WORK_DIR}/na-ok.json" >/dev/null

NA_NO_APPROVAL="${WORK_DIR}/na-no-approval"
cp -R "${NA_OK}" "${NA_NO_APPROVAL}"
sed 's/auditor\/approval.json/-/' "${NA_NO_APPROVAL}/criteria.tsv" >"${NA_NO_APPROVAL}/criteria.new"
mv "${NA_NO_APPROVAL}/criteria.new" "${NA_NO_APPROVAL}/criteria.tsv"
expect_fail na-no-approval "${NA_NO_APPROVAL}" 'N/A criterion requires auditor_approval_ref'

NA_AI="${WORK_DIR}/na-ai"
cp -R "${NA_OK}" "${NA_AI}"
write_metadata "${NA_AI}" MQ4 5 true application_deployer true true true true 2 false true true true true true false true
expect_fail na-ai "${NA_AI}" 'AI/ML domain G cannot be N/A when AI is in scope'

NA_DEVICE="${WORK_DIR}/na-device"
cp -R "${NA_OK}" "${NA_DEVICE}"
write_metadata "${NA_DEVICE}" MQ4 5 false not_applicable false false false false 0 true true true true true true false false
expect_fail na-device "${NA_DEVICE}" 'hardware longevity domain H cannot be N/A for material client-device impact'

NA_GOLD_PREREQUISITE="${WORK_DIR}/na-gold-prerequisite"
make_candidate "${NA_GOLD_PREREQUISITE}" 80
mark_domains_na "${NA_GOLD_PREREQUISITE}/criteria.tsv" C
"${TOOL}" "${NA_GOLD_PREREQUISITE}" "${WORK_DIR}/na-gold-prerequisite.json"
jq -e '
  ([.scoring.domains[] | select(.domain == "C" and .applicable == false)] | length) == 1 and
  .scoring.total_score == 80 and
  .scoring.recommended_level_for_external_review == "silver"
' "${WORK_DIR}/na-gold-prerequisite.json" >/dev/null

MIXED_NA="${WORK_DIR}/mixed-na"
make_candidate "${MIXED_NA}" 95
awk -F '\t' 'BEGIN { OFS="\t" }
  $1 == "C8" {
    print $1, $2, 0, 1, "not_applicable", "false", "auditor/approval.json", "evidence/na.json";
    next
  }
  { print }
' "${MIXED_NA}/criteria.tsv" >"${MIXED_NA}/criteria.new"
mv "${MIXED_NA}/criteria.new" "${MIXED_NA}/criteria.tsv"
expect_fail mixed-na "${MIXED_NA}" 'criterion applicability must be all-or-nothing for domain C'

NA_CORE="${WORK_DIR}/na-core"
make_candidate "${NA_CORE}" 95
mark_domains_na "${NA_CORE}/criteria.tsv" A
expect_fail na-core "${NA_CORE}" 'core domains A, B and K cannot be marked N/A'

# Material omissions at or above 1 percent, or cumulatively above 5 percent, fail G14.
MATERIAL="${WORK_DIR}/material"
make_candidate "${MATERIAL}" 95
write_materiality "${MATERIAL}" omitted 1
"${TOOL}" "${MATERIAL}" "${WORK_DIR}/material.json"
jq -e '
  .materiality.individual_material_omission == true and
  (.eligibility.failed_gates | index("G14") != null) and
  .scoring.recommended_level_for_external_review == "none"
' "${WORK_DIR}/material.json" >/dev/null

CUMULATIVE="${WORK_DIR}/cumulative"
make_candidate "${CUMULATIVE}" 95
{
  printf 'component\tshare_percent\tdisposition\tevidence_ref\n'
  for number in 1 2 3 4 5 6; do
    printf 'component%s\t0.9\tomitted\tevidence/component%s.json\n' "${number}" "${number}"
  done
  printf 'accounted_remainder\t94.6\tincluded\tevidence/remainder.json\n'
} >"${CUMULATIVE}/materiality.tsv"
"${TOOL}" "${CUMULATIVE}" "${WORK_DIR}/cumulative.json"
jq -e '
  .materiality.individual_material_omission == false and
  .materiality.cumulative_omission_exceeded == true and
  (.eligibility.failed_gates | index("G14") != null)
' "${WORK_DIR}/cumulative.json" >/dev/null

ESTIMATE="${WORK_DIR}/estimate"
make_candidate "${ESTIMATE}" 95
write_materiality "${ESTIMATE}" conservative_estimate 15
"${TOOL}" "${ESTIMATE}" "${WORK_DIR}/estimate.json"
jq -e '.eligibility.all_mandatory_gates_passed == true and .materiality.omitted_share_percent == 0' \
  "${WORK_DIR}/estimate.json" >/dev/null

MATERIALITY_EXACT="${WORK_DIR}/materiality-exact"
make_candidate "${MATERIALITY_EXACT}" 95
{
  printf 'component\tshare_percent\tdisposition\tevidence_ref\n'
  for number in 1 2 3 4 5; do
    printf 'component%s\t0.9\tomitted\tevidence/component%s.json\n' "${number}" "${number}"
  done
  printf 'component6\t0.5\tomitted\tevidence/component6.json\n'
  printf 'accounted_remainder\t95\tincluded\tevidence/remainder.json\n'
} >"${MATERIALITY_EXACT}/materiality.tsv"
"${TOOL}" "${MATERIALITY_EXACT}" "${WORK_DIR}/materiality-exact.json"
jq -e '
  .materiality.individual_material_omission == false and
  .materiality.omitted_share_percent == 5 and
  .materiality.cumulative_omission_exceeded == false and
  .eligibility.all_mandatory_gates_passed == true
' "${WORK_DIR}/materiality-exact.json" >/dev/null

MATERIALITY_GAP="${WORK_DIR}/materiality-gap"
make_candidate "${MATERIALITY_GAP}" 95
sed 's/runtime_and_service\t100/runtime_and_service\t99/' "${MATERIALITY_GAP}/materiality.tsv" \
  >"${MATERIALITY_GAP}/materiality.new"
mv "${MATERIALITY_GAP}/materiality.new" "${MATERIALITY_GAP}/materiality.tsv"
expect_fail materiality-gap "${MATERIALITY_GAP}" 'materiality component shares must account for exactly 100 percent'

# More than 10 percent unresolved eco-regression requires action and caps at Bronze.
REGRESSION="${WORK_DIR}/regression"
make_candidate "${REGRESSION}" 95
cat >"${REGRESSION}/regressions.tsv" <<'EOF'
metric_id	kind	baseline_value	current_value	lower_is_better	explanation	corrective_action_ref	evidence_ref
energy_per_unit	eco	100	111	true	-	-	evidence/regression.json
EOF
"${TOOL}" "${REGRESSION}" "${WORK_DIR}/regression.json"
jq -e '
  .eco_regression.triggered == true and .eco_regression.unresolved == true and
  .eco_regression.corrective_action_required == true and
  .scoring.recommended_level_for_external_review == "bronze" and
  .surveillance.recertification_review_required == true
' "${WORK_DIR}/regression.json" >/dev/null

REMEDIATED="${WORK_DIR}/remediated"
cp -R "${REGRESSION}" "${REMEDIATED}"
cat >"${REMEDIATED}/regressions.tsv" <<'EOF'
metric_id	kind	baseline_value	current_value	lower_is_better	explanation	corrective_action_ref	evidence_ref
energy_per_unit	eco	100	111	true	traffic mix changed	CAPA-2026-09	evidence/regression.json
EOF
"${TOOL}" "${REMEDIATED}" "${WORK_DIR}/remediated.json"
jq -e '
  .eco_regression.triggered == true and .eco_regression.unresolved == false and
  .scoring.recommended_level_for_external_review == "diamond"
' "${WORK_DIR}/remediated.json" >/dev/null

REGRESSION_EXACT="${WORK_DIR}/regression-exact"
make_candidate "${REGRESSION_EXACT}" 95
cat >"${REGRESSION_EXACT}/regressions.tsv" <<'EOF'
metric_id	kind	baseline_value	current_value	lower_is_better	explanation	corrective_action_ref	evidence_ref
energy_per_unit	eco	100	110	true	-	-	evidence/regression.json
EOF
"${TOOL}" "${REGRESSION_EXACT}" "${WORK_DIR}/regression-exact.json"
jq -e '
  .eco_regression.triggered == false and
  .eco_regression.unresolved == false and
  .scoring.recommended_level_for_external_review == "diamond"
' "${WORK_DIR}/regression-exact.json" >/dev/null

# Any measured quality degradation fails G13.
QUALITY="${WORK_DIR}/quality"
make_candidate "${QUALITY}" 95
cat >"${QUALITY}/regressions.tsv" <<'EOF'
metric_id	kind	baseline_value	current_value	lower_is_better	explanation	corrective_action_ref	evidence_ref
accuracy	quality	0.95	0.94	false	-	-	evidence/quality.json
EOF
"${TOOL}" "${QUALITY}" "${WORK_DIR}/quality.json"
jq -e '
  (.eligibility.failed_gates | index("G13") != null) and
  .scoring.recommended_level_for_external_review == "none" and
  .surveillance.recertification_review_required == true
' "${WORK_DIR}/quality.json" >/dev/null

# Uncertainty, MQ, eco-control, claim risk and AI frontier controls downgrade levels.
for case in '12.1 silver' '8.1 gold' '5.1 platinum' '5 diamond'; do
  read -r uncertainty expected <<<"${case}"
  directory="${WORK_DIR}/uncertainty-${uncertainty}"
  make_candidate "${directory}" 95
  write_metadata "${directory}" MQ4 "${uncertainty}"
  "${TOOL}" "${directory}" "${WORK_DIR}/uncertainty-${uncertainty}.json"
  jq -e --arg expected "${expected}" '.scoring.recommended_level_for_external_review == $expected' \
    "${WORK_DIR}/uncertainty-${uncertainty}.json" >/dev/null
done

MQ_DOWN="${WORK_DIR}/mq-down"
make_candidate "${MQ_DOWN}" 95
write_metadata "${MQ_DOWN}" MQ2 5
"${TOOL}" "${MQ_DOWN}" "${WORK_DIR}/mq-down.json"
jq -e '.scoring.recommended_level_for_external_review == "silver"' "${WORK_DIR}/mq-down.json" >/dev/null

CONTROL_DOWN="${WORK_DIR}/control-down"
make_candidate "${CONTROL_DOWN}" 95
write_metadata "${CONTROL_DOWN}" MQ4 5 true application_deployer true true true true 2 true false true true true true false true
"${TOOL}" "${CONTROL_DOWN}" "${WORK_DIR}/control-down.json"
jq -e '.scoring.recommended_level_for_external_review == "bronze"' "${WORK_DIR}/control-down.json" >/dev/null

RISK_DOWN="${WORK_DIR}/risk-down"
make_candidate "${RISK_DOWN}" 95
write_metadata "${RISK_DOWN}" MQ4 5 true application_deployer true true true true 2 true true true true true true true true
"${TOOL}" "${RISK_DOWN}" "${WORK_DIR}/risk-down.json"
jq -e '
  .scoring.recommended_level_for_external_review == "silver"
' "${WORK_DIR}/risk-down.json" >/dev/null

FRONTIER_DOWN="${WORK_DIR}/frontier-down"
make_candidate "${FRONTIER_DOWN}" 95
write_metadata "${FRONTIER_DOWN}" MQ4 5 true application_deployer true true true true 1 true true true true true true false true
"${TOOL}" "${FRONTIER_DOWN}" "${WORK_DIR}/frontier-down.json"
jq -e '.scoring.recommended_level_for_external_review == "silver"' "${WORK_DIR}/frontier-down.json" >/dev/null

AI_BOUNDARY="${WORK_DIR}/ai-boundary"
make_candidate "${AI_BOUNDARY}" 95
write_metadata "${AI_BOUNDARY}" MQ4 5 true application_deployer false true true true 2 true true true true true true false true
expect_fail ai-boundary "${AI_BOUNDARY}" 'AI scope requires a training boundary declaration'

MISCLASSIFIED_CLAIM="${WORK_DIR}/misclassified-claim"
make_candidate "${MISCLASSIFIED_CLAIM}" 95
awk -F '\t' 'BEGIN { OFS="\t" }
  $1 == "c1" { $3="Candidate evidence is officially certified Diamond and best in class." }
  { print }
' "${MISCLASSIFIED_CLAIM}/claims.tsv" >"${MISCLASSIFIED_CLAIM}/claims.new"
mv "${MISCLASSIFIED_CLAIM}/claims.new" "${MISCLASSIFIED_CLAIM}/claims.tsv"
"${TOOL}" "${MISCLASSIFIED_CLAIM}" "${WORK_DIR}/misclassified-claim.json"
jq -e '
  ([.claims[] | select(.id == "c1" and .decision == "blocked" and
      (.reasons | index("candidate_claim_text_is_not_claim_safe") != null))] | length) == 1
' "${WORK_DIR}/misclassified-claim.json" >/dev/null

# Equivalent input row order produces a byte-identical assessment.
DETERMINISTIC="${WORK_DIR}/deterministic"
cp -R "${BASE}" "${DETERMINISTIC}"
reverse_rows() {
  local path="$1"
  awk 'NR == 1 { header=$0; next } { rows[NR]=$0 } END { print header; for (i=NR; i>=2; --i) print rows[i] }' \
    "${path}" >"${path}.new"
  mv "${path}.new" "${path}"
}
for input in metadata.tsv gates.tsv criteria.tsv materiality.tsv claims.tsv regressions.tsv; do
  reverse_rows "${DETERMINISTIC}/${input}"
done
jq '.records |= reverse' "${DETERMINISTIC}/measurement.json" >"${DETERMINISTIC}/measurement.new"
mv "${DETERMINISTIC}/measurement.new" "${DETERMINISTIC}/measurement.json"
"${TOOL}" "${DETERMINISTIC}" "${WORK_DIR}/deterministic.json" "${WORK_DIR}/deterministic.md"
cmp "${WORK_DIR}/base.json" "${WORK_DIR}/deterministic.json"
cmp "${WORK_DIR}/base.md" "${WORK_DIR}/deterministic.md"

# Retain the earlier PR90 quality, penalty, report and claim-integrity protections.
grep -Fq 'does **not** grant C3-ECO certification' "${WORK_DIR}/base.md"
grep -Fq 'Effective evidence quality: MQ4/DQ4' "${WORK_DIR}/base.md"
set_metadata() {
  local directory="$1" key="$2" value="$3" status="${4:-independent}"
  awk -F '\t' -v key="${key}" -v value="${value}" -v status="${status}" \
    'BEGIN { OFS="\t" } $1 == key {$2=value; $3=status} {print}' \
    "${directory}/metadata.tsv" >"${directory}/metadata.new"
  mv "${directory}/metadata.new" "${directory}/metadata.tsv"
}
DQ_DOWN="${WORK_DIR}/dq-down"
make_candidate "${DQ_DOWN}" 100
set_metadata "${DQ_DOWN}" data_quality DQ2
"${TOOL}" "${DQ_DOWN}" "${WORK_DIR}/dq-down.json"
jq -e '.assessment_controls.data_quality == "DQ2" and
       .scoring.recommended_level_for_external_review == "silver"' "${WORK_DIR}/dq-down.json" >/dev/null
for quality in measurement_quality data_quality; do
  directory="${WORK_DIR}/${quality}-self-upgrade"
  make_candidate "${directory}" 100
  if [[ "${quality}" == measurement_quality ]]; then value=MQ4; else value=DQ4; fi
  set_metadata "${directory}" "${quality}" "${value}" verified
  expect_fail "${quality}-self-upgrade" "${directory}" "${value} requires independent evidence"
done
WORKBOOK_UNCERTAINTY="${WORK_DIR}/workbook-uncertainty"
make_candidate "${WORKBOOK_UNCERTAINTY}" 100
jq '.records[0].uncertainty_percent = 40 |
    .records[0].uncertainty_kwh = (.records[0].facility_energy_kwh * 0.4) |
    .records[0].uncertainty_carbon_kgco2e = (.records[0].carbon_kgco2e * 0.4) |
    .totals.uncertainty_kwh = ([.records[].uncertainty_kwh] | add) |
    .totals.uncertainty_carbon_kgco2e = ([.records[].uncertainty_carbon_kgco2e] | add)' \
  "${WORKBOOK_UNCERTAINTY}/measurement.json" >"${WORKBOOK_UNCERTAINTY}/measurement.new"
mv "${WORKBOOK_UNCERTAINTY}/measurement.new" "${WORKBOOK_UNCERTAINTY}/measurement.json"
"${TOOL}" "${WORKBOOK_UNCERTAINTY}" "${WORK_DIR}/workbook-uncertainty.json"
jq -e '.assessment_controls.uncertainty_percent == 40 and
       .scoring.recommended_level_for_external_review == "candidate"' \
  "${WORK_DIR}/workbook-uncertainty.json" >/dev/null
PENALTY="${WORK_DIR}/penalty"
make_candidate "${PENALTY}" 100
set_metadata "${PENALTY}" penalty_points 35 verified
"${TOOL}" "${PENALTY}" "${WORK_DIR}/penalty.json"
jq -e '.assessment_controls.penalty_points == 35 and .scoring.total_score == 65 and
       .scoring.recommended_level_for_external_review == "silver"' "${WORK_DIR}/penalty.json" >/dev/null
RESTRICTED="${WORK_DIR}/restricted"
make_candidate "${RESTRICTED}" 100
write_restricted_claims "${RESTRICTED}"
"${TOOL}" "${RESTRICTED}" "${WORK_DIR}/restricted.json"
jq -e '.eligibility.status == "not_eligible" and
       (.eligibility.failed_gates | index("G10") != null) and
       .scoring.recommended_level_for_external_review == "none" and
       ([.claims[] | select(.id == "c2" and .decision == "blocked" and
         (.reasons | index("external_certification_authority_required") != null))] | length) == 1 and
       ([.claims[] | select(.id == "c3" and .decision == "blocked" and
         (.reasons | index("comparative_qualification_deferred_pr95") != null))] | length) == 1 and
       ([.claims[] | select(.id == "c4" and .decision == "blocked" and
         (.reasons | index("legal_and_technical_evidence_required") != null))] | length) == 1 and
       ([.claims[] | select(.id == "c6" and .decision == "blocked" and
         (.reasons | index("offsets_cannot_replace_base_footprint_reduction") != null))] | length) == 1' \
  "${WORK_DIR}/restricted.json" >/dev/null
GREEN_AI="${WORK_DIR}/green-ai"
make_candidate "${GREEN_AI}" 100
awk -F '\t' 'NR == 1 || $1 == "c5"' "${RESTRICTED}/claims.tsv" >"${GREEN_AI}/claims.tsv"
"${TOOL}" "${GREEN_AI}" "${WORK_DIR}/green-ai.json"
jq -e '.eligibility.all_mandatory_gates_passed == true and .claims[0].decision == "permitted"' \
  "${WORK_DIR}/green-ai.json" >/dev/null
write_uniform_criteria "${GREEN_AI}" 65
"${TOOL}" "${GREEN_AI}" "${WORK_DIR}/green-ai-low.json"
jq -e '(.eligibility.failed_gates | index("G10") != null) and .claims[0].decision == "blocked"' \
  "${WORK_DIR}/green-ai-low.json" >/dev/null
PROFILE_LINK="${WORK_DIR}/profile-link"
make_candidate "${PROFILE_LINK}" 100
jq '.c3eco_declarations |= map(select(.kind != "boundary"))' \
  "${PROFILE_LINK}/profile.json" >"${PROFILE_LINK}/profile.new"
mv "${PROFILE_LINK}/profile.new" "${PROFILE_LINK}/profile.json"
expect_fail profile-link "${PROFILE_LINK}" 'profile.json certification_profile link is unresolved: boundary'
IDENTITY="${WORK_DIR}/identity"
make_candidate "${IDENTITY}" 100
set_metadata "${IDENTITY}" product_version 9.9.9 verified
expect_fail identity "${IDENTITY}" 'metadata identity and scope must match the linked typed profile'
if "${TOOL}" "${BASE}" "${WORK_DIR}/output-fail.json" "${BASE}" \
    >"${WORK_DIR}/output-fail.out" 2>"${WORK_DIR}/output-fail.err"; then
  echo 'expected report write failure' >&2; exit 1
fi
grep -Fq 'cannot open assessment Markdown output' "${WORK_DIR}/output-fail.err"

# Contract spoofing, reconciliation tampering, duplicates and invalid bounds fail closed.
BAD_PROFILE="${WORK_DIR}/bad-profile"
cp -R "${BASE}" "${BAD_PROFILE}"
sed 's/shorthand.c3eco.profile.v2/shorthand.c3eco.profile.v1/' \
  "${BAD_PROFILE}/profile.json" >"${BAD_PROFILE}/profile.new"
mv "${BAD_PROFILE}/profile.new" "${BAD_PROFILE}/profile.json"
expect_fail bad-profile "${BAD_PROFILE}" 'must use shorthand.c3eco.profile.v2'

UNSAFE_PROFILE="${WORK_DIR}/unsafe-profile"
cp -R "${BASE}" "${UNSAFE_PROFILE}"
jq '.production_claim = true' "${UNSAFE_PROFILE}/profile.json" >"${UNSAFE_PROFILE}/profile.new"
mv "${UNSAFE_PROFILE}/profile.new" "${UNSAFE_PROFILE}/profile.json"
expect_fail unsafe-profile "${UNSAFE_PROFILE}" 'profile.json must not enable claim flag: production_claim'

MISSING_PROFILE_DECLARATION="${WORK_DIR}/missing-profile-declaration"
cp -R "${BASE}" "${MISSING_PROFILE_DECLARATION}"
jq '.c3eco_declarations = []' "${MISSING_PROFILE_DECLARATION}/profile.json" \
  >"${MISSING_PROFILE_DECLARATION}/profile.new"
mv "${MISSING_PROFILE_DECLARATION}/profile.new" "${MISSING_PROFILE_DECLARATION}/profile.json"
expect_fail missing-profile-declaration "${MISSING_PROFILE_DECLARATION}" \
  'profile.json must contain exactly one typed certification_profile declaration'

INCOMPLETE_PROFILE="${WORK_DIR}/incomplete-profile"
cp -R "${BASE}" "${INCOMPLETE_PROFILE}"
jq 'del(.c3eco_declarations[0].typed_fields.guardrails)' "${INCOMPLETE_PROFILE}/profile.json" \
  >"${INCOMPLETE_PROFILE}/profile.new"
mv "${INCOMPLETE_PROFILE}/profile.new" "${INCOMPLETE_PROFILE}/profile.json"
expect_fail incomplete-profile "${INCOMPLETE_PROFILE}" \
  'profile.json certification_profile typed_fields must contain exactly the contract keys'

BAD_PROFILE_MQ="${WORK_DIR}/bad-profile-mq"
cp -R "${BASE}" "${BAD_PROFILE_MQ}"
jq '.measurement_quality = "MQ0"' "${BAD_PROFILE_MQ}/profile.json" >"${BAD_PROFILE_MQ}/profile.new"
mv "${BAD_PROFILE_MQ}/profile.new" "${BAD_PROFILE_MQ}/profile.json"
expect_fail bad-profile-mq "${BAD_PROFILE_MQ}" 'profile.json measurement_quality must be MQ1 through MQ4'

BAD_MEASUREMENT="${WORK_DIR}/bad-measurement"
cp -R "${BASE}" "${BAD_MEASUREMENT}"
jq '.official_certification_granted = true' "${BAD_MEASUREMENT}/measurement.json" \
  >"${BAD_MEASUREMENT}/measurement.new"
mv "${BAD_MEASUREMENT}/measurement.new" "${BAD_MEASUREMENT}/measurement.json"
expect_fail bad-measurement "${BAD_MEASUREMENT}" 'must not claim official certification'

BAD_CURRENCY="${WORK_DIR}/bad-currency"
cp -R "${BASE}" "${BAD_CURRENCY}"
jq '.records[0].tariff_currency = "inr"' "${BAD_CURRENCY}/measurement.json" \
  >"${BAD_CURRENCY}/measurement.new"
mv "${BAD_CURRENCY}/measurement.new" "${BAD_CURRENCY}/measurement.json"
expect_fail bad-currency "${BAD_CURRENCY}" 'tariff currency must be three uppercase ASCII letters'

OVERFLOW_DERIVATION="${WORK_DIR}/overflow-derivation"
cp -R "${BASE}" "${OVERFLOW_DERIVATION}"
jq '.records[0].raw_energy_j = 1e308 | .records[0].allocation_fraction = 1 |
    .records[0].allocated_it_energy_j = 1e308 | .records[0].pue = 3 |
    .records[0].facility_energy_kwh = 1e308' "${OVERFLOW_DERIVATION}/measurement.json" \
  >"${OVERFLOW_DERIVATION}/measurement.new"
mv "${OVERFLOW_DERIVATION}/measurement.new" "${OVERFLOW_DERIVATION}/measurement.json"
expect_fail overflow-derivation "${OVERFLOW_DERIVATION}" \
  'measurement.json facility energy does not reconcile within record r1'

UNRECONCILED_MEASUREMENT="${WORK_DIR}/unreconciled-measurement"
cp -R "${BASE}" "${UNRECONCILED_MEASUREMENT}"
jq '.totals.allocated_it_energy_j = 5400001' "${UNRECONCILED_MEASUREMENT}/measurement.json" \
  >"${UNRECONCILED_MEASUREMENT}/measurement.new"
mv "${UNRECONCILED_MEASUREMENT}/measurement.new" "${UNRECONCILED_MEASUREMENT}/measurement.json"
expect_fail unreconciled-measurement "${UNRECONCILED_MEASUREMENT}" \
  'measurement.json allocated IT energy does not reconcile with records'

BAD_DERIVATION="${WORK_DIR}/bad-derivation"
cp -R "${BASE}" "${BAD_DERIVATION}"
jq '.records[0].cost = 10.3 | .totals.cost_by_currency.INR = 15.4' "${BAD_DERIVATION}/measurement.json" \
  >"${BAD_DERIVATION}/measurement.new"
mv "${BAD_DERIVATION}/measurement.new" "${BAD_DERIVATION}/measurement.json"
expect_fail bad-derivation "${BAD_DERIVATION}" 'measurement.json cost does not reconcile within record r1'

MISSING_WORKBOOK_FIELD="${WORK_DIR}/missing-workbook-field"
cp -R "${BASE}" "${MISSING_WORKBOOK_FIELD}"
jq 'del(.allocation_policy)' "${MISSING_WORKBOOK_FIELD}/measurement.json" \
  >"${MISSING_WORKBOOK_FIELD}/measurement.new"
mv "${MISSING_WORKBOOK_FIELD}/measurement.new" "${MISSING_WORKBOOK_FIELD}/measurement.json"
expect_fail missing-workbook-field "${MISSING_WORKBOOK_FIELD}" \
  'measurement.json must contain exactly the contract keys'

DUPLICATE_JSON="${WORK_DIR}/duplicate-json"
cp -R "${BASE}" "${DUPLICATE_JSON}"
sed '/"schema"/a\  "schema": "shorthand.c3eco.candidate_report.v1",' \
  "${DUPLICATE_JSON}/profile.json" >"${DUPLICATE_JSON}/profile.new"
mv "${DUPLICATE_JSON}/profile.new" "${DUPLICATE_JSON}/profile.json"
expect_fail duplicate-json "${DUPLICATE_JSON}" 'duplicate object key: schema'

DUPLICATE_GATE="${WORK_DIR}/duplicate-gate"
cp -R "${BASE}" "${DUPLICATE_GATE}"
printf 'G1\tpass\tevidence/duplicate.json\n' >>"${DUPLICATE_GATE}/gates.tsv"
expect_fail duplicate-gate "${DUPLICATE_GATE}" 'duplicate mandatory gate: G1'

ROW_LIMIT="${WORK_DIR}/row-limit"
make_candidate "${ROW_LIMIT}" 95
printf 'claim_id\tclaim_type\tclaim_text\tscope_matches\tfunctional_unit_equivalent\tboundary_equivalent\tquality_equivalent\tmethod_equivalent\tlegal_technical_evidence\tevidence_ref\n' >"${ROW_LIMIT}/claims.tsv"
for number in $(seq 1 10001); do
  printf 'c%s\tcandidate_assessment\tCandidate evidence was assessed for the declared scope.\ttrue\tfalse\tfalse\tfalse\tfalse\tfalse\tevidence/assessment.json\n' "${number}" >>"${ROW_LIMIT}/claims.tsv"
done
expect_fail row-limit "${ROW_LIMIT}" 'exceeds 10000 data-row limit'

BAD_SCORE="${WORK_DIR}/bad-score"
cp -R "${BASE}" "${BAD_SCORE}"
sed '0,/\t5\t/{s/\t5\t/\t6\t/}' "${BAD_SCORE}/criteria.tsv" >"${BAD_SCORE}/criteria.new"
mv "${BAD_SCORE}/criteria.new" "${BAD_SCORE}/criteria.tsv"
expect_fail bad-score "${BAD_SCORE}" 'raw_score must be in [0,5]'

BAD_SOFTWARE_CLASS="${WORK_DIR}/bad-software-class"
make_candidate "${BAD_SOFTWARE_CLASS}" 95
sed 's/software_class\tS6_AI_GENAI/software_class\tS06_AI_GENAI/' "${BAD_SOFTWARE_CLASS}/metadata.tsv" \
  >"${BAD_SOFTWARE_CLASS}/metadata.new"
mv "${BAD_SOFTWARE_CLASS}/metadata.new" "${BAD_SOFTWARE_CLASS}/metadata.tsv"
expect_fail bad-software-class "${BAD_SOFTWARE_CLASS}" 'software_class must use canonical form S1 through S12'

PADDED_NUMBER="${WORK_DIR}/padded-number"
make_candidate "${PADDED_NUMBER}" 95
sed 's/uncertainty_percent\t5\t/uncertainty_percent\t 5\t/' "${PADDED_NUMBER}/metadata.tsv" \
  >"${PADDED_NUMBER}/metadata.new"
mv "${PADDED_NUMBER}/metadata.new" "${PADDED_NUMBER}/metadata.tsv"
expect_fail padded-number "${PADDED_NUMBER}" 'invalid numeric uncertainty_percent'

MISSING_CRITERION="${WORK_DIR}/missing-criterion"
cp -R "${BASE}" "${MISSING_CRITERION}"
awk -F '\t' '$1 != "K7"' "${MISSING_CRITERION}/criteria.tsv" >"${MISSING_CRITERION}/criteria.new"
mv "${MISSING_CRITERION}/criteria.new" "${MISSING_CRITERION}/criteria.tsv"
expect_fail missing-criterion "${MISSING_CRITERION}" 'criteria.tsv missing normative criterion: K7'

jq -e '[.. | objects | select(has("official_certification_granted")) |
  .official_certification_granted] | all(. == false)' "${WORK_DIR}/base.json" >/dev/null

echo 'PASS: PR90 C3-ECO eligibility scoring claims and eco-regression gate'
