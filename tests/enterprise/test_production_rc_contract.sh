#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK_DIR="$(mktemp -d /tmp/shorthand-pr99-contract.XXXXXX)"
trap 'rm -rf "${WORK_DIR}"' EXIT

REPORT="${WORK_DIR}/report.json"
SHORTHAND_PRODUCTION_RC_REPORT="${REPORT}" bash "${ROOT_DIR}/scripts/check_production_rc.sh" --contract
jq -e '
  .schema == "shorthand.enterprise.pilot_rc.v1" and
  .execution_mode == "contract" and
  .production_scope == "linux-x64-cpu-v1" and
  .production_claim == false and
  .accelerator_production_support == false and
  .mandatory_skips == 0 and
  .release_candidate_decision == "blocked_by_retained_evidence" and
  (.compiler_blockers | map(.id) | sort) == ["TST017","TST025","TST026"] and
  (.lifecycle.clean_install == false) and
  (.deployment.contract_checked == false)
' "${REPORT}" >/dev/null

cp "${ROOT_DIR}/docs/production_rc_scope.tsv" "${WORK_DIR}/scope.tsv"
sed -i 's/^production_scope\tlinux-x64-cpu-v1$/production_scope\tlinux-x64-gpu-v1/' "${WORK_DIR}/scope.tsv"
if SHORTHAND_PRODUCTION_RC_SCOPE_FILE="${WORK_DIR}/scope.tsv" \
  SHORTHAND_PRODUCTION_RC_REPORT="${WORK_DIR}/bad-scope.json" \
  bash "${ROOT_DIR}/scripts/check_production_rc.sh" --contract >/dev/null 2>&1; then
  echo "error: invalid accelerator scope unexpectedly passed" >&2
  exit 1
fi

cp "${ROOT_DIR}/docs/production_rc_scope.tsv" "${WORK_DIR}/claim.tsv"
sed -i 's/^production_claim\tfalse$/production_claim\ttrue/' "${WORK_DIR}/claim.tsv"
if SHORTHAND_PRODUCTION_RC_SCOPE_FILE="${WORK_DIR}/claim.tsv" \
  SHORTHAND_PRODUCTION_RC_REPORT="${WORK_DIR}/bad-claim.json" \
  bash "${ROOT_DIR}/scripts/check_production_rc.sh" --contract >/dev/null 2>&1; then
  echo "error: unsupported production claim unexpectedly passed" >&2
  exit 1
fi

cp "${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv" "${WORK_DIR}/matrix.tsv"
sed -i 's/^TST027\tproduction release-candidate gate\timplemented/TST027\tproduction release-candidate gate\topen/' "${WORK_DIR}/matrix.tsv"
if SHORTHAND_PRODUCTION_RC_MATRIX_FILE="${WORK_DIR}/matrix.tsv" \
  SHORTHAND_PRODUCTION_RC_REPORT="${WORK_DIR}/bad-matrix.json" \
  bash "${ROOT_DIR}/scripts/check_production_rc.sh" --contract >/dev/null 2>&1; then
  echo "error: open TST027 unexpectedly passed" >&2
  exit 1
fi

echo "PASS PR99 production RC contract and negative lifecycle/scope cases"
