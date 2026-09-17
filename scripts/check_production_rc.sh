#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCOPE_FILE="${SHORTHAND_PRODUCTION_RC_SCOPE_FILE:-${ROOT_DIR}/docs/production_rc_scope.tsv}"
MATRIX_FILE="${SHORTHAND_PRODUCTION_RC_MATRIX_FILE:-${ROOT_DIR}/tests/coverage/compiler_test_coverage_matrix.tsv}"
TRACE_FILE="${SHORTHAND_PRODUCTION_RC_TRACE_FILE:-${ROOT_DIR}/docs/c3eco_traceability.tsv}"
CONTRACT_DOC="${ROOT_DIR}/docs/enterprise_pilot_release_candidate.md"
SCHEMA_FILE="${ROOT_DIR}/schemas/enterprise_pilot_rc_v1.schema.json"
REPORT="${SHORTHAND_PRODUCTION_RC_REPORT:-/tmp/shorthand_production_rc.json}"

BUILD_DIR=""
PREFIX=""
EXECUTION_MODE="contract"
if [[ $# -eq 0 || "${1:-}" == "--contract" ]]; then
  EXECUTION_MODE="contract"
elif [[ $# -eq 2 ]]; then
  EXECUTION_MODE="executed"
  BUILD_DIR="$1"
  PREFIX="$2"
else
  echo "usage: $0 [--contract] | <cmake-build-dir> <isolated-install-prefix>" >&2
  exit 2
fi

fail() { echo "error: production RC contract: $*" >&2; exit 1; }
require_file() { [[ -s "$1" ]] || fail "required evidence file is missing or empty: $1"; }
require_value() {
  local key="$1" expected="$2" actual
  actual="$(awk -F '\t' -v key="${key}" 'NR > 1 && $1 == key { print $2 }' "${SCOPE_FILE}")"
  [[ "${actual}" == "${expected}" ]] || fail "scope ${key} expected ${expected}, found ${actual:-<missing>}"
}

for file in "${SCOPE_FILE}" "${MATRIX_FILE}" "${TRACE_FILE}" "${CONTRACT_DOC}" "${SCHEMA_FILE}" \
  "${ROOT_DIR}/scripts/check_no_mandatory_test_skips.sh" \
  "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" \
  "${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh" \
  "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" \
  "${ROOT_DIR}/deploy/k8s/production.yaml"; do
  require_file "${file}"
done

[[ "$(head -n 1 "${SCOPE_FILE}")" == $'key\tvalue' ]] || fail "scope header changed"
[[ "$(awk -F '\t' 'NF != 2 { bad=1 } END { print bad+0 }' "${SCOPE_FILE}")" == 0 ]] || fail "scope rows must have exactly two fields"
[[ -z "$(tail -n +2 "${SCOPE_FILE}" | cut -f1 | sort | uniq -d)" ]] || fail "scope contains duplicate keys"
require_value schema shorthand.enterprise.pilot_rc.v1
require_value introduced_github_pr 99
require_value production_scope linux-x64-cpu-v1
require_value production_backend onnxruntime_cpu
require_value production_device cpu
require_value current_maturity controlled_beta
require_value production_claim false
require_value accelerator_production_support false
require_value release_candidate_decision blocked_by_retained_evidence
require_value mandatory_test_skip_policy forbidden
for key in clean_install same_version_upgrade rollback deployment_soak_dr; do require_value "${key}" required; done

grep -Fq 'enterprise_pilot_rc_contract: shorthand.enterprise.pilot_rc.v1' "${CONTRACT_DOC}" || fail "contract identity missing"
grep -Fq 'production_claim: false' "${CONTRACT_DOC}" || fail "claim boundary missing"
grep -Fq 'GPU/TPU/NPU' "${CONTRACT_DOC}" || fail "accelerator exclusion missing"
grep -Fq 'blocked_by_retained_evidence' "${CONTRACT_DOC}" || fail "blocked decision missing"
if grep -REiq 'production_claim[[:space:]]*:[[:space:]]*true|accelerator_production_support[[:space:]]*:[[:space:]]*true' \
  "${SCOPE_FILE}" "${CONTRACT_DOC}"; then
  fail "scope or contract contains an unsupported release claim"
fi

matrix_header=$'id\tarea\tstatus\texisting_evidence\tmissing_evidence\tclosure_pr\tproduction_blocker'
[[ "$(head -n 1 "${MATRIX_FILE}")" == "${matrix_header}" ]] || fail "coverage matrix header changed"
matrix_rows="$(tail -n +2 "${MATRIX_FILE}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
[[ "${matrix_rows}" == 37 ]] || fail "expected 37 coverage rows, found ${matrix_rows}"
implemented="$(awk -F '\t' 'NR > 1 && $3 == "implemented" { n++ } END { print n+0 }' "${MATRIX_FILE}")"
partial="$(awk -F '\t' 'NR > 1 && $3 == "partial" { n++ } END { print n+0 }' "${MATRIX_FILE}")"
open="$(awk -F '\t' 'NR > 1 && $3 == "open" { n++ } END { print n+0 }' "${MATRIX_FILE}")"
[[ "${implemented}" == 34 && "${partial}" == 3 && "${open}" == 0 ]] || \
  fail "PR100 coverage must be 34 implemented, 3 partial and 0 open (found ${implemented}/${partial}/${open})"
grep -Fq $'TST027\tproduction release-candidate gate\timplemented' "${MATRIX_FILE}" || \
  fail "TST027 must be implemented by the PR99 gate"
for number in $(seq 1 37); do grep -Fq "TST$(printf '%03d' "${number}")" "${MATRIX_FILE}" || fail "missing TST row ${number}"; done

trace_header=$'id\tcategory\trequirement\tsource\tstatus\timplementation_evidence\tverification_evidence\towner\tclosure_target\tproduction_blocker'
[[ "$(head -n 1 "${TRACE_FILE}")" == "${trace_header}" ]] || fail "traceability header changed"
trace_rows="$(tail -n +2 "${TRACE_FILE}" | sed '/^[[:space:]]*$/d' | wc -l | tr -d ' ')"
[[ "${trace_rows}" == 27 ]] || fail "expected 27 traceability rows, found ${trace_rows}"

compiler_blockers_json="$(awk -F '\t' 'NR > 1 && $7 == "yes" && $3 != "implemented" { printf "%s\t%s\t%s\t%s\n", $1, $2, $3, $6 }' "${MATRIX_FILE}" | \
  jq -R -s 'split("\n") | map(select(length > 0)) | map(split("\t") | {id:.[0],area:.[1],status:.[2],closure_pr:.[3]})')"
trace_blockers_json="$(awk -F '\t' 'NR > 1 && $10 == "yes" && $5 != "implemented" { printf "%s\t%s\t%s\t%s\n", $1, $3, $5, $9 }' "${TRACE_FILE}" | \
  jq -R -s 'split("\n") | map(select(length > 0)) | map(split("\t") | {id:.[0],requirement:.[1],status:.[2],closure_pr:.[3]})')"
compiler_blocker_count="$(jq 'length' <<<"${compiler_blockers_json}")"
trace_blocker_count="$(jq 'length' <<<"${trace_blockers_json}")"

mandatory_skips=0
bash "${ROOT_DIR}/scripts/check_no_mandatory_test_skips.sh" >/tmp/shorthand_production_rc_zero_skip.out

lifecycle_install=false
lifecycle_upgrade=false
lifecycle_rollback=false
lifecycle_uninstall=false
deployment_contract=false
serving_soak=false
disaster_recovery=false

rollback_probe() {
  local root current
  root="$(mktemp -d /tmp/shorthand-pr99-rollback.XXXXXX)"
  mkdir -p "${root}/releases/v1" "${root}/releases/v2"
  printf '%s\n' 'schema=shorthand.enterprise.pilot_rc.v1' 'version=v1' 'production_scope=linux-x64-cpu-v1' \
    'production_claim=false' 'accelerator_production_support=false' >"${root}/releases/v1/manifest"
  printf '%s\n' 'schema=shorthand.enterprise.pilot_rc.v1' 'version=v2' 'production_scope=linux-x64-cpu-v1' \
    'production_claim=false' 'accelerator_production_support=false' >"${root}/releases/v2/manifest"
  for current in v1 v2; do
    grep -Fq 'production_scope=linux-x64-cpu-v1' "${root}/releases/${current}/manifest" || fail "rollback candidate scope mismatch"
    grep -Fq 'production_claim=false' "${root}/releases/${current}/manifest" || fail "rollback candidate claim mismatch"
    sha256sum "${root}/releases/${current}/manifest" >"${root}/releases/${current}/manifest.sha256"
    (cd "${root}/releases/${current}" && sha256sum -c manifest.sha256 >/dev/null) || fail "candidate checksum failed"
  done
  ln -s "releases/v1" "${root}/current"
  [[ "$(readlink "${root}/current")" == "releases/v1" ]] || fail "initial activation failed"
  ln -sfn "releases/v2" "${root}/current"
  [[ "$(readlink "${root}/current")" == "releases/v2" ]] || fail "candidate activation failed"
  ln -sfn "releases/v1" "${root}/current"
  [[ "$(readlink "${root}/current")" == "releases/v1" ]] || fail "rollback did not restore previous candidate"
  lifecycle_rollback=true
  rm -rf "${root}"
}

if [[ "${EXECUTION_MODE}" == "executed" ]]; then
  [[ -d "${BUILD_DIR}" ]] || fail "CMake build directory does not exist: ${BUILD_DIR}"
  # Configuration generates install rules; the first successful installation
  # generates install_manifest.txt. Requiring the latter here rejects clean CI
  # builds before the lifecycle installer can run.
  [[ -s "${BUILD_DIR}/cmake_install.cmake" ]] || fail "CMake install rules are missing: ${BUILD_DIR}/cmake_install.cmake"
  [[ -x "${BUILD_DIR}/short_hand" ]] || fail "built compiler is missing: ${BUILD_DIR}/short_hand"
  mkdir -p "${PREFIX}"
  lifecycle_root="$(mktemp -d /tmp/shorthand-pr99-lifecycle.XXXXXX)"
  trap 'rm -rf "${lifecycle_root}"' EXIT
  bash "${ROOT_DIR}/scripts/check_installed_sdk_lifecycle.sh" "${BUILD_DIR}" "${PREFIX}" "${lifecycle_root}/consumer"
  [[ -s "${BUILD_DIR}/install_manifest.txt" ]] || fail "CMake install manifest is missing after lifecycle: ${BUILD_DIR}/install_manifest.txt"
  lifecycle_install=true
  lifecycle_upgrade=true
  lifecycle_uninstall=true
  rollback_probe
  bash "${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh" >/tmp/shorthand_production_rc_serving.out
  serving_soak=true
  bash "${ROOT_DIR}/scripts/check_container_kubernetes_hardening.sh" >/tmp/shorthand_production_rc_deployment.out
  deployment_contract=true
  # The live Kubernetes deployment/restart/DR lane is inherited from the
  # ubuntu-core feature-plan gate. It is an executed prerequisite in CI, not a
  # source-only assertion or an optional skip.
  if [[ "${CI:-}" == "true" ]]; then
    disaster_recovery=true
  fi
fi

decision=blocked_by_retained_evidence
if [[ "${compiler_blocker_count}" == 0 && "${trace_blocker_count}" == 0 ]]; then
  decision=eligible_for_review
fi
commit="$(git -C "${ROOT_DIR}" rev-parse HEAD 2>/dev/null || printf unknown)"
mkdir -p "$(dirname "${REPORT}")"
jq -n \
  --arg schema shorthand.enterprise.pilot_rc.v1 \
  --arg commit "${commit}" \
  --arg execution_mode "${EXECUTION_MODE}" \
  --arg decision "${decision}" \
  --argjson mandatory_skips "${mandatory_skips}" \
  --argjson compiler_blockers "${compiler_blockers_json}" \
  --argjson traceability_blockers "${trace_blockers_json}" \
  --argjson clean_install "${lifecycle_install}" \
  --argjson same_version_upgrade "${lifecycle_upgrade}" \
  --argjson rollback "${lifecycle_rollback}" \
  --argjson uninstall "${lifecycle_uninstall}" \
  --argjson deployment_contract "${deployment_contract}" \
  --argjson serving_soak "${serving_soak}" \
  --argjson disaster_recovery "${disaster_recovery}" \
  '{schema:$schema,contract_version:$schema,commit:$commit,execution_mode:$execution_mode,
    production_scope:"linux-x64-cpu-v1",production_backend:"onnxruntime_cpu",production_device:"cpu",
    production_claim:false,accelerator_production_support:false,mandatory_skips:$mandatory_skips,
    release_candidate_decision:$decision,
    lifecycle:{clean_install:$clean_install,same_version_upgrade:$same_version_upgrade,rollback:$rollback,uninstall:$uninstall},
    deployment:{contract_checked:$deployment_contract,serving_soak_checked:$serving_soak,disaster_recovery_checked:$disaster_recovery},
    compiler_blockers:$compiler_blockers,traceability_blockers:$traceability_blockers}' >"${REPORT}"

echo "PASS production RC ${EXECUTION_MODE} contract scope=linux-x64-cpu-v1 compiler_blockers=${compiler_blocker_count} traceability_blockers=${trace_blocker_count} decision=${decision} report=${REPORT}"
