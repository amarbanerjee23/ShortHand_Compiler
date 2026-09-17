#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/Compiler_new_ws/Short_Hand/src/evidence/LifecycleCloudBoundary.cpp"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
TOOL="$TMP/shorthand_c3eco_lifecycle_boundary"
"${CXX:-c++}" ${C3ECO_BOUNDARY_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} "$SRC" -o "$TOOL"
HEADER=$'record_id\tlifecycle_phase\tcomponent\tshared_resource_id\tallocation_basis\tallocation_fraction\toperational_carbon_kgco2e\tenergy_workbook_ref\tdata_bytes\tstorage_gb_hours\tnetwork_gb\tcloud_region\tcarbon_factor_gco2e_per_kwh\tfactor_source\tfactor_date\tembodied_asset_id\tembodied_carbon_kgco2e\tasset_lifetime_hours\tattributed_usage_hours\tevidence_ref'
printf '%s\n' "$HEADER" \
 $'a\tdata_ingest\tingest\tinput-meter\tbyte\t1\t0.01\tworkbooks/run.json\t1000\t0\t0\tin-1\t700\tgrid\t2026-09-01\t\t0\t0\t0\tevidence/ingest.json' \
 $'b\tdata_storage\tstorage\tstorage-pool\tgb_hour\t0.5\t0.02\tworkbooks/run.json\t0\t10\t0\tin-1\t700\tgrid\t2026-09-01\t\t0\t0\t0\tevidence/storage.json' \
 $'c\tdata_egress\tnetwork\tnetwork-pool\tnetwork_gb\t1\t0.03\tworkbooks/run.json\t0\t0\t2\tin-1\t700\tgrid\t2026-09-01\t\t0\t0\t0\tevidence/egress.json' \
 $'d\tmodel_lifecycle\tmodel\tmodel-pool\ttenant_request\t1\t0.04\tworkbooks/run.json\t0\t0\t0\tin-1\t700\tgrid\t2026-09-01\t\t0\t0\t0\tevidence/model.json' \
 $'e\tcloud_compute\tcompute\tcompute-pool\tusage_hour\t0.5\t0.10\tworkbooks/run.json\t0\t0\t0\tin-1\t700\tgrid\t2026-09-01\t\t0\t0\t0\tevidence/compute.json' \
 $'f\thardware_lifecycle\tserver\tserver-42\tusage_hour\t1\t0\tworkbooks/run.json\t0\t0\t0\tin-1\t700\tgrid\t2026-09-01\tserver-42\t1200\t60000\t600\tevidence/asset.json' > "$TMP/positive.tsv"
"$TOOL" "$TMP/positive.tsv" "$TMP/out.json"
grep -Fq '"boundary_status":"candidate_evidence_complete_for_declared_scope"' "$TMP/out.json"
grep -Fq '"allocated_embodied_carbon_kgco2e":12' "$TMP/out.json"
grep -Fq '"official_certification_granted":false' "$TMP/out.json"
expect_fail(){ local name="$1" msg="$2"; shift 2; "$@"; if "$TOOL" "$TMP/$name.tsv" "$TMP/$name.json" >"$TMP/$name.out" 2>"$TMP/$name.err"; then echo "expected failure: $name" >&2; exit 1; fi; grep -Fq "$msg" "$TMP/$name.err"; }
cp "$TMP/positive.tsv" "$TMP/double.tsv"; sed -i 's/^b\tdata_storage\tstorage\tstorage-pool\tgb_hour\t0.5/^b\tdata_storage\tstorage\tcompute-pool\tgb_hour\t1/' "$TMP/double.tsv"; expect_fail double 'double counting detected:' true
cp "$TMP/positive.tsv" "$TMP/missing.tsv"; sed -i '/^c\tdata_egress/d' "$TMP/missing.tsv"; expect_fail missing 'boundary is incomplete: missing required lifecycle_phase data_egress' true
cp "$TMP/positive.tsv" "$TMP/asset.tsv"; sed -i 's/server-42\t1200\t60000\t600/server-42\t1200\t600\t6000/' "$TMP/asset.tsv"; expect_fail asset 'hardware lifecycle requires bounded embodied asset evidence' true
echo 'PASS PR100 lifecycle cloud carbon boundary, shared allocation, data phases and hardware lifetime gate'
