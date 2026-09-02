#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/Compiler_new_ws/Short_Hand/src/evidence/MeasurementWorkbook.cpp"
CXX_BIN="${CXX:-c++}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [[ -n "${SHORTHAND_C3ECO_MEASURE_BIN:-}" ]]; then
  TOOL="$SHORTHAND_C3ECO_MEASURE_BIN"
  [[ -x "$TOOL" ]] || { echo "measurement tool is not executable: $TOOL" >&2; exit 1; }
else
  TOOL="$TMP/shorthand_c3eco_measure"
  "$CXX_BIN" ${C3ECO_MEASURE_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} "$SRC" -o "$TOOL"
fi

HEADER=$'record_id\tcomponent\tsource_kind\tinstrument_id\tcalibration_id\tcalibration_date\tmeasured_at\traw_energy_j\tallocation_fraction\tpue\tcarbon_factor_gco2e_per_kwh\tfactor_source\tfactor_date\ttariff_per_kwh\ttariff_currency\ttariff_source\tuncertainty_percent\tmeasurement_quality\tdata_quality\tevidence_ref'
printf '%s\n' "$HEADER" \
  $'r2\tmemory\tphysical_meter\tpdu-7\tcal-pdu-2026\t2026-07-01\t2026-09-01T10:00:00Z\t1800000\t1\t1.2\t400\tgrid-operator-published\t2026-09-01\t8.5\tINR\tutility-tariff-2026\t10\tmedium\tmedium\tevidence/pdu-7-run.csv' \
  $'r1\tcompute\trapl\tcpu-package-0\tcal-rapl-2026\t2026-08-01\t2026-09-01T10:00:00Z\t3600000\t0.5\t1.2\t400\tgrid-operator-published\t2026-09-01\t8.5\tINR\tutility-tariff-2026\t5\thigh\thigh\tevidence/rapl-run.json' \
  > "$TMP/positive.tsv"

"$TOOL" "$TMP/positive.tsv" "$TMP/workbook.csv" "$TMP/workbook.json"
grep -q 'shorthand.c3eco.measurement_workbook.v1' "$TMP/workbook.json"
grep -q '"measurement_status":"measured_instrumented"' "$TMP/workbook.json"
grep -q '"official_certification_granted":false' "$TMP/workbook.json"
grep -q '"base_footprint_not_reduced_by_offsets":true' "$TMP/workbook.json"
grep -q '"facility_energy_kwh":1.2' "$TMP/workbook.json"
grep -q '"carbon_kgco2e":0.48' "$TMP/workbook.json"
grep -q '"INR":10.2' "$TMP/workbook.json"

# Deterministic output is required independent of input row order.
printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu-package-0\tcal-rapl-2026\t2026-08-01\t2026-09-01T10:00:00Z\t3600000\t0.5\t1.2\t400\tgrid-operator-published\t2026-09-01\t8.5\tINR\tutility-tariff-2026\t5\thigh\thigh\tevidence/rapl-run.json' \
  $'r2\tmemory\tphysical_meter\tpdu-7\tcal-pdu-2026\t2026-07-01\t2026-09-01T10:00:00Z\t1800000\t1\t1.2\t400\tgrid-operator-published\t2026-09-01\t8.5\tINR\tutility-tariff-2026\t10\tmedium\tmedium\tevidence/pdu-7-run.csv' \
  > "$TMP/reordered.tsv"
"$TOOL" "$TMP/reordered.tsv" "$TMP/workbook2.csv" "$TMP/workbook2.json"
cmp "$TMP/workbook.csv" "$TMP/workbook2.csv"
cmp "$TMP/workbook.json" "$TMP/workbook2.json"

expect_fail() {
  local name="$1"; shift
  if "$TOOL" "$TMP/$name.tsv" "$TMP/$name.csv" "$TMP/$name.json" >"$TMP/$name.out" 2>"$TMP/$name.err"; then
    echo "expected failure for $name" >&2
    exit 1
  fi
  grep -Fq "$1" "$TMP/$name.err"
}

# Modelled or declared values can remain candidate evidence, but cannot be promoted to measured evidence.
printf '%s\n' "$HEADER" \
  $'r1\tcompute\tmodelled\tmodel-1\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t1\t1.1\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/model.json' \
  > "$TMP/modelled.tsv"
expect_fail modelled 'real instrumentation'

# Shared meter allocation must not double-count the same raw reading.
printf '%s\n' "$HEADER" \
  $'r1\tcompute\tphysical_meter\tpdu-1\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t0.7\t1.1\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/shared.csv' \
  $'r2\tmemory\tphysical_meter\tpdu-1\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t0.5\t1.1\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/shared.csv' \
  > "$TMP/double_count.tsv"
expect_fail double_count 'double counting detected'

# Calibration and emission factors cannot post-date the measurement.
printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu0\tcal-future\t2026-10-01\t2026-09-01T10:00:00Z\t1000\t1\t1.1\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/run.json' \
  > "$TMP/future_calibration.tsv"
expect_fail future_calibration 'calibration is after measurement'

printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu0\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t1\t1.1\t400\tgrid\t2026-10-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/run.json' \
  > "$TMP/future_factor.tsv"
expect_fail future_factor 'carbon factor is after measurement'

# Bounds and provenance are fail-closed.
printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu0\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t1.1\t1.1\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/run.json' \
  > "$TMP/allocation.tsv"
expect_fail allocation 'allocation_fraction must be in (0,1]'

printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu0\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t1\t0.9\t400\tgrid\t2026-09-01\t8\tINR\ttariff\t5\thigh\thigh\tevidence/run.json' \
  > "$TMP/pue.tsv"
expect_fail pue 'pue must be in [1,3]'

printf '%s\n' "$HEADER" \
  $'r1\tcompute\trapl\tcpu0\tcal-1\t2026-08-01\t2026-09-01T10:00:00Z\t1000\t1\t1.1\t400\tgrid\t2026-09-01\t8\tINR\t\t5\thigh\thigh\tevidence/run.json' \
  > "$TMP/provenance.tsv"
expect_fail provenance 'provenance fields are required'

echo 'PASS: PR89 C3-ECO measurement, carbon accounting and cost workbook gate'
