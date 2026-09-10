#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
CXX_BIN="${CXX:-c++}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
for tool in openssl python3 jq; do
  command -v "${tool}" >/dev/null || { echo "error: ${tool} is required for auditor qualification" >&2; exit 1; }
done
TOOL="${SHORTHAND_C3ECO_AUDIT_BIN:-${WORK_DIR}/shorthand_c3eco_audit}"
if [[ -z "${SHORTHAND_C3ECO_AUDIT_BIN:-}" ]]; then
  # Intentional flag-vector splitting, matching the native assessment gate.
  # shellcheck disable=SC2086
  "${CXX_BIN}" ${C3ECO_AUDIT_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror} \
    -DSHORTHAND_C3ECO_ASSESS_LIBRARY \
    "${SRC_DIR}/evidence/AuditorBundle.cpp" \
    "${SRC_DIR}/evidence/CertificationAssessment.cpp" \
    "${SRC_DIR}/evidence/C3EcoAssessmentScoring.cpp" \
    "${SRC_DIR}/module/Sha256.cpp" -lcrypto -o "${TOOL}"
fi
[[ -x "${TOOL}" ]] || { echo "error: native auditor tool is not executable" >&2; exit 1; }
TOOL="$(cd "$(dirname "${TOOL}")" && pwd)/$(basename "${TOOL}")"
C3ECO_ASSESS_CXXFLAGS="${C3ECO_AUDIT_CXXFLAGS:--std=c++17 -Wall -Wextra -Wpedantic -Werror}"
source "${ROOT_DIR}/tests/c3eco/assessment/fixture.sh"
make_candidate "${WORK_DIR}/candidate" 95
python3 "${ROOT_DIR}/tests/c3eco/audit/test_auditor_bundle.py" "${TOOL}" "${WORK_DIR}" "${ROOT_DIR}"
echo 'PASS PR91 signed auditor lineage replay retention surveillance redaction and readiness gate'
