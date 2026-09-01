#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mandatory=(
  tests/integration/test_onnxruntime_sdk_gate.sh
  tests/integration/test_compiled_hook_onnxruntime_success.sh
  tests/integration/test_production_backend_hardware_qualification.sh
  scripts/check_production_backend_hardware_qualification.sh
  scripts/check_kubernetes_ephemeral_cluster.sh
  scripts/check_concurrent_serving_runtime.sh
  scripts/check_c3eco_certification_profile.sh
)

for rel in "${mandatory[@]}"; do
  file="${ROOT_DIR}/${rel}"
  [[ -s "${file}" ]] || { echo "error: mandatory qualification gate missing: ${rel}" >&2; exit 1; }
  if grep -Eq '(^|[[:space:]"=])SKIP([[:space:]:"=]|$)|skip_safe' "${file}"; then
    echo "error: mandatory qualification gate contains skip-success semantics: ${rel}" >&2
    grep -En 'SKIP[[:space:]:]|skip_safe' "${file}" >&2 || true
    exit 1
  fi
done

grep -Fq 'ONNXRUNTIME_ROOT is mandatory' "${ROOT_DIR}/tests/integration/test_backend_live_sdk_matrix.sh"
grep -Fq 'negative_qualified' "${ROOT_DIR}/tests/integration/test_backend_live_sdk_matrix.sh"
grep -Fq 'not_production_qualified' "${ROOT_DIR}/tests/integration/test_backend_live_sdk_matrix.sh"
grep -Fq 'mandatory_skips":0' "${ROOT_DIR}/scripts/check_production_backend_hardware_qualification.sh"
grep -Fq 'PASS ephemeral Kubernetes production gate' "${ROOT_DIR}/scripts/check_kubernetes_ephemeral_cluster.sh"
grep -Fq 'PASS concurrent serving cancellation deadline backpressure quota isolation health load soak restart and graceful shutdown gate' "${ROOT_DIR}/scripts/check_concurrent_serving_runtime.sh"
grep -Fq 'PASS typed C3-ECO profile identity units links boundary materiality lifecycle validity migration and claim-safety gate' "${ROOT_DIR}/scripts/check_c3eco_certification_profile.sh"

echo "PASS mandatory qualification zero-skip policy gate"
