#!/usr/bin/env bash
# Shared real-producer fixture for assessment and auditor lifecycle qualification.
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

