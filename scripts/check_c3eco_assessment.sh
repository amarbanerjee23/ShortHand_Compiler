#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/Compiler_new_ws/Short_Hand/src"
SHORT="${SHORTHAND_BIN:-$ROOT/Compiler_new_ws/Short_Hand/build/short_hand}"
MEASURE="${SHORTHAND_C3ECO_MEASURE_BIN:-}"
ASSESS="${SHORTHAND_C3ECO_ASSESS_BIN:-}"
CXX_BIN="${CXX:-c++}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP" "$ROOT/c3eco_profile_v2.bc"' EXIT

command -v jq >/dev/null 2>&1 || { echo "error: jq is required for PR90 assessment validation" >&2; exit 1; }

if [[ -z "$ASSESS" ]]; then
  ASSESS="$TMP/shorthand_c3eco_assess"
  "$CXX_BIN" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
    -I"$SRC/evidence" \
    "$SRC/evidence/C3EcoAssessmentIO.cpp" \
    "$SRC/evidence/C3EcoAssessmentScoring.cpp" \
    "$SRC/evidence/AssessmentEngine.cpp" \
    -o "$ASSESS"
fi
[[ -x "$ASSESS" ]] || { echo "assessment tool is not executable: $ASSESS" >&2; exit 1; }

# Strict scoring unit is independent of the JSON/TSV adapter so level and cap rules cannot be masked by fixtures.
"$CXX_BIN" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
  -I"$SRC/evidence" \
  "$SRC/evidence/C3EcoAssessmentScoring.cpp" \
  "$ROOT/tests/c3eco/assessment/test_c3eco_assessment_scoring.cpp" \
  -o "$TMP/scoring-unit"
"$TMP/scoring-unit"

if [[ ! -x "$SHORT" ]]; then
  make -C "$SRC" short_hand >/tmp/shorthand_pr90_profile_build.out 2>&1 || {
    cat /tmp/shorthand_pr90_profile_build.out >&2 || true
    exit 1
  }
fi
"$SHORT" "$ROOT/tests/c3eco/profile/c3eco_profile_v2.short" c3eco-report --output "$TMP/profile.json"
jq -e '.schema == "shorthand.c3eco.candidate_report.v1" and
       .c3eco_profile_contract == "shorthand.c3eco.profile.v2" and
       .c3eco_profile_status == "conformant" and
       .official_certification_granted == false' "$TMP/profile.json" >/dev/null

if [[ -z "$MEASURE" ]]; then
  MEASURE="$TMP/shorthand_c3eco_measure"
  "$CXX_BIN" ${C3ECO_ASSESS_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
    "$SRC/evidence/MeasurementWorkbook.cpp" -o "$MEASURE"
fi
[[ -x "$MEASURE" ]] || { echo "measurement tool is not executable: $MEASURE" >&2; exit 1; }

MHEADER=$'record_id\tcomponent\tsource_kind\tinstrument_id\tcalibration_id\tcalibration_date\tmeasured_at\traw_energy_j\tallocation_fraction\tpue\tcarbon_factor_gco2e_per_kwh\tfactor_source\tfactor_date\ttariff_per_kwh\ttariff_currency\ttariff_source\tuncertainty_percent\tmeasurement_quality\tdata_quality\tevidence_ref'
printf '%s\n' "$MHEADER" \
  $'m1\tcompute\trapl\tcpu-package-0\tcal-rapl-2026\t2026-08-01\t2026-09-01T10:00:00Z\t3600000\t1\t1\t400\tgrid-operator\t2026-09-01\t8.5\tINR\tutility-tariff\t5\thigh\thigh\tevidence/rapl-run.json' \
  > "$TMP/measurement.tsv"
"$MEASURE" "$TMP/measurement.tsv" "$TMP/measurement.csv" "$TMP/measurement.json" >/dev/null

AHEADER=$'kind\tid\tvalue\tapplicable\tevidence_status\tevidence_ref'
make_assessment() {
  local path="$1" requested="$2" baseline="$3" omission="$4"
  printf '%s\n' "$AHEADER" > "$path"
  local i domain
  for ((i=1; i<=14; ++i)); do
    printf 'gate\tG%s\tpass\ttrue\tverified\tevidence/g%s.json\n' "$i" "$i" >> "$path"
  done
  for domain in A B C D; do
    for ((i=1; i<=8; ++i)); do printf 'criterion\t%s%s\t5\ttrue\tverified\tevidence/%s%s.json\n' "$domain" "$i" "$domain" "$i" >> "$path"; done
  done
  for ((i=1; i<=6; ++i)); do printf 'criterion\tE%s\t5\ttrue\tverified\tevidence/E%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=7; ++i)); do printf 'criterion\tF%s\t5\ttrue\tverified\tevidence/F%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=10; ++i)); do printf 'criterion\tG%s\t5\ttrue\tverified\tevidence/AI-G%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=5; ++i)); do printf 'criterion\tH%s\t5\ttrue\tverified\tevidence/H%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=5; ++i)); do printf 'criterion\tI%s\t5\ttrue\tverified\tevidence/I%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=4; ++i)); do printf 'criterion\tJ%s\t5\ttrue\tverified\tevidence/J%s.json\n' "$i" "$i" >> "$path"; done
  for ((i=1; i<=7; ++i)); do printf 'criterion\tK%s\t5\ttrue\tverified\tevidence/K%s.json\n' "$i" "$i" >> "$path"; done
  printf 'control\tassessment_id\tpr90-assessment-001\ttrue\tverified\tevidence/assessment-id.json\n' >> "$path"
  printf 'control\trequested_claims\t%s\ttrue\tverified\tevidence/claims.json\n' "$requested" >> "$path"
  printf 'control\tapproved_na_domains\tnone\ttrue\tindependent\tevidence/na-review.json\n' >> "$path"
  printf 'control\tcumulative_omitted_percent\t%s\ttrue\tverified\tevidence/materiality.json\n' "$omission" >> "$path"
  printf 'control\tlargest_unaddressed_omission_percent\t0\ttrue\tverified\tevidence/materiality.json\n' >> "$path"
  printf 'control\teco_regression_control_defined\ttrue\ttrue\tverified\tevidence/eco-ci.json\n' >> "$path"
  printf 'control\tbaseline_energy_per_unit_j\t%s\ttrue\tverified\tevidence/baseline.json\n' "$baseline" >> "$path"
  printf 'control\teco_regression_justification_approved\tfalse\ttrue\tverified\tevidence/regression-review.json\n' >> "$path"
  printf 'control\thigh_severity_claim_risk\tfalse\ttrue\tverified\tevidence/claims-risk.json\n' >> "$path"
  printf 'control\tindependent_review\ttrue\ttrue\tindependent\tevidence/independent-review.json\n' >> "$path"
  printf 'control\tpublic_report\ttrue\ttrue\tverified\tevidence/public-report.json\n' >> "$path"
  printf 'control\tcontinuous_telemetry\ttrue\ttrue\tverified\tevidence/telemetry.json\n' >> "$path"
  printf 'control\tpublic_evidence_summary\ttrue\ttrue\tverified\tevidence/public-summary.json\n' >> "$path"
  printf 'control\tregistry_ready\ttrue\ttrue\tverified\tevidence/registry.json\n' >> "$path"
  printf 'control\tmeasurement_mq_override\t4\ttrue\tindependent\tevidence/mq4.json\n' >> "$path"
  printf 'control\tmeasurement_dq_override\t4\ttrue\tindependent\tevidence/dq4.json\n' >> "$path"
  printf 'control\tpenalty_points\t0\ttrue\tverified\tevidence/penalties.json\n' >> "$path"
  printf 'control\tfull_boundary_legal_claim_support\tfalse\ttrue\tverified\tevidence/legal-claims.json\n' >> "$path"
  printf 'control\toffsets_reported_separately\ttrue\ttrue\tverified\tevidence/offsets.json\n' >> "$path"
  printf 'control\tgreen_ai_supporting_evidence\ttrue\ttrue\tindependent\tevidence/green-ai-boundary.json\n' >> "$path"
}

make_assessment "$TMP/assessment.tsv" certified_diamond none 0
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/assessment.tsv" "$TMP/positive.json" "$TMP/positive.md"
jq -e '.schema == "shorthand.c3eco.assessment.v1" and
       .official_certification_granted == false and
       .production_claim == false and
       .eligibility.status == "eligible" and
       .score.diagnostic_total == 100 and
       .score.score_band == "Diamond" and
       .score.candidate_level_after_caps == "Diamond" and
       .measurement.effective_mq == 4 and .measurement.effective_dq == 4 and
       ([.criteria[]] | length == 76) and ([.gates[]] | length == 14) and
       ([.claims[] | select(.requested == "certified_diamond" and .status == "candidate_for_external_review")] | length == 1)' \
  "$TMP/positive.json" >/dev/null
grep -Fq 'does **not** grant C3-ECO certification' "$TMP/positive.md"

# Deterministic output must not depend on TSV row order when the source path is unchanged.
cp "$TMP/assessment.tsv" "$TMP/original.tsv"
{ head -n 1 "$TMP/original.tsv"; tail -n +2 "$TMP/original.tsv" | sort -r; } > "$TMP/assessment.tsv"
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/assessment.tsv" "$TMP/reordered.json" "$TMP/reordered.md" >/dev/null
cmp "$TMP/positive.json" "$TMP/reordered.json"
cmp "$TMP/positive.md" "$TMP/reordered.md"
cp "$TMP/original.tsv" "$TMP/assessment.tsv"

# Critical mandatory gate failure invalidates the level regardless of the numerical score.
awk -F '\t' 'BEGIN{OFS="\t"} $1=="gate" && $2=="G6" {$3="fail"} {print}' "$TMP/original.tsv" > "$TMP/gate-fail.tsv"
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/gate-fail.tsv" "$TMP/gate-fail.json" "$TMP/gate-fail.md" >/dev/null
jq -e '.eligibility.status == "not_eligible" and .eligibility.score_valid == false and
       .score.candidate_level_after_caps == "None" and (.eligibility.failed_gates | index("G6") != null)' \
  "$TMP/gate-fail.json" >/dev/null

# Insufficient evidence caps a scored criterion at 1 even if the applicant supplied 5.
awk -F '\t' 'BEGIN{OFS="\t"} $1=="criterion" && $2=="A1" {$5="insufficient"} {print}' "$TMP/original.tsv" > "$TMP/insufficient.tsv"
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/insufficient.tsv" "$TMP/insufficient.json" "$TMP/insufficient.md" >/dev/null
jq -e '[.criteria[] | select(.id == "A1" and .raw_score == 5 and .effective_score == 1)] | length == 1' "$TMP/insufficient.json" >/dev/null

# Unaddressed cumulative omissions above 5% fail G14 and therefore eligibility.
make_assessment "$TMP/materiality.tsv" certified_diamond none 5.1
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/materiality.tsv" "$TMP/materiality.json" "$TMP/materiality.md" >/dev/null
jq -e '.eligibility.status == "not_eligible" and (.eligibility.failed_gates | index("G14") != null)' "$TMP/materiality.json" >/dev/null

# The v0.6 >10% unexplained eco-regression trigger caps an otherwise valid assessment at Bronze.
make_assessment "$TMP/regression.tsv" certified_bronze 3000 0
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/regression.tsv" "$TMP/regression.json" "$TMP/regression.md" >/dev/null
jq -e '.eligibility.status == "eligible" and .eco_regression.triggered == true and
       .eco_regression.delta_percent > 10 and .score.candidate_level_after_caps == "Bronze" and
       .corrective_action_required == true' "$TMP/regression.json" >/dev/null

# Comparative superiority remains explicitly deferred to PR95 and cannot pass claims integrity in PR90.
make_assessment "$TMP/comparative.tsv" comparative none 0
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/comparative.tsv" "$TMP/comparative.json" "$TMP/comparative.md" >/dev/null
jq -e '.eligibility.status == "not_eligible" and (.eligibility.failed_gates | index("G10") != null) and
       ([.claims[] | select(.requested == "comparative" and .status == "deferred_pr95")] | length == 1)' "$TMP/comparative.json" >/dev/null

# Removed/restricted claims must never be promoted into an official C3-ECO result.
make_assessment "$TMP/net-positive.tsv" net_positive none 0
"$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/net-positive.tsv" "$TMP/net-positive.json" "$TMP/net-positive.md" >/dev/null
jq -e '.official_certification_granted == false and .eligibility.status == "not_eligible" and
       ([.claims[] | select(.requested == "net_positive" and .status == "denied_not_a_c3eco_level")] | length == 1)' \
  "$TMP/net-positive.json" >/dev/null

# Forged upstream artifacts are rejected rather than converted into score evidence.
jq '.official_certification_granted = true' "$TMP/profile.json" > "$TMP/forged-profile.json"
if "$ASSESS" "$TMP/forged-profile.json" "$TMP/measurement.json" "$TMP/original.tsv" "$TMP/forged.json" "$TMP/forged.md" >"$TMP/forged.out" 2>"$TMP/forged.err"; then
  echo 'expected forged profile rejection' >&2; exit 1
fi
grep -Fq 'profile evidence cannot already grant certification' "$TMP/forged.err"

jq '.records[0].source_kind = "modelled"' "$TMP/measurement.json" > "$TMP/forged-measurement.json"
if "$ASSESS" "$TMP/profile.json" "$TMP/forged-measurement.json" "$TMP/original.tsv" "$TMP/forged-m.json" "$TMP/forged-m.md" >"$TMP/forged-m.out" 2>"$TMP/forged-m.err"; then
  echo 'expected forged measurement rejection' >&2; exit 1
fi
grep -Fq 'non-instrumented source' "$TMP/forged-m.err"

# Duplicate and incomplete scorecards are fail-closed.
cp "$TMP/original.tsv" "$TMP/duplicate.tsv"
grep $'^criterion\tA1\t' "$TMP/original.tsv" >> "$TMP/duplicate.tsv"
if "$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/duplicate.tsv" "$TMP/duplicate.json" "$TMP/duplicate.md" >"$TMP/duplicate.out" 2>"$TMP/duplicate.err"; then
  echo 'expected duplicate criterion rejection' >&2; exit 1
fi
grep -Fq 'duplicate criterion id: A1' "$TMP/duplicate.err"

grep -v $'^criterion\tA1\t' "$TMP/original.tsv" > "$TMP/incomplete.tsv"
if "$ASSESS" "$TMP/profile.json" "$TMP/measurement.json" "$TMP/incomplete.tsv" "$TMP/incomplete.json" "$TMP/incomplete.md" >"$TMP/incomplete.out" 2>"$TMP/incomplete.err"; then
  echo 'expected incomplete catalog rejection' >&2; exit 1
fi
grep -Fq 'complete 76-criterion' "$TMP/incomplete.err"

echo 'PASS: PR90 C3-ECO eligibility, A-K scoring, evidence caps, claims and eco-regression gate'
