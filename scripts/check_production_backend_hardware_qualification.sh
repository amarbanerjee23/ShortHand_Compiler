#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOC="${ROOT_DIR}/docs/production_backend_hardware_qualification.md"
HEADER="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/ai_runtime/ProductionBackendQualification.h"
CONTRACT_TEST="${ROOT_DIR}/tests/integration/test_production_backend_hardware_qualification.sh"
LIVE_TEST="${ROOT_DIR}/tests/integration/test_compiled_hook_onnxruntime_success.sh"
MATRIX_TEST="${ROOT_DIR}/tests/integration/test_backend_live_sdk_matrix.sh"
REPORT="/tmp/shorthand_production_backend_hardware_qualification.json"

require_file() {
  [[ -s "$1" ]] || { echo "error: missing production backend qualification evidence: $1" >&2; exit 1; }
}
require_contains() {
  require_file "$1"
  grep -Fq "$2" "$1" || { echo "error: $1 missing required production backend qualification text: $2" >&2; exit 1; }
}

for file in "${DOC}" "${HEADER}" "${CONTRACT_TEST}" "${LIVE_TEST}" "${MATRIX_TEST}"; do
  require_file "${file}"
done

for anchor in \
  'backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1' \
  'production_scope: linux-x64-cpu-v1' \
  'production_supported_backend: onnxruntime_cpu' \
  'production_supported_device_class: cpu' \
  'accelerator_support_status: not_production_supported_until_live_qualified'; do
  require_contains "${DOC}" "${anchor}"
done
require_contains "${HEADER}" 'kProductionBackendQualificationSchema'
require_contains "${HEADER}" 'backend_device_not_production_qualified'
require_contains "${HEADER}" 'SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE'
require_contains "${LIVE_TEST}" 'Output: 42'
require_contains "${LIVE_TEST}" 'PASS compiled hook ONNX Runtime success gate'

bash "${CONTRACT_TEST}"

if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo "error: production backend qualification v1 must execute on Linux x86_64" >&2
  exit 1
fi

if [[ -z "${ONNXRUNTIME_ROOT:-}" ]]; then
  echo "error: ONNXRUNTIME_ROOT is mandatory for production backend qualification; skip is not accepted" >&2
  exit 1
fi
[[ -s "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" ]] || { echo "error: qualified ONNX Runtime header missing" >&2; exit 1; }
[[ -d "${ONNXRUNTIME_ROOT}/lib" ]] || { echo "error: qualified ONNX Runtime library directory missing" >&2; exit 1; }

ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" bash "${LIVE_TEST}" \
  >/tmp/shorthand_production_backend_onnx_cpu.out \
  2>/tmp/shorthand_production_backend_onnx_cpu.err || {
    cat /tmp/shorthand_production_backend_onnx_cpu.out >&2 || true
    cat /tmp/shorthand_production_backend_onnx_cpu.err >&2 || true
    exit 1
  }
grep -Fq 'Output: 42' /tmp/shorthand_compiled_hook_onnxruntime_success.out
grep -Fq 'PASS compiled hook ONNX Runtime success gate' /tmp/shorthand_production_backend_onnx_cpu.out
if grep -Eqi 'SKIP|fallback|backend_not_available|not_executed' \
    /tmp/shorthand_production_backend_onnx_cpu.out /tmp/shorthand_production_backend_onnx_cpu.err; then
  echo "error: production-supported ONNX Runtime CPU row skipped or used non-execution path" >&2
  cat /tmp/shorthand_production_backend_onnx_cpu.out >&2 || true
  cat /tmp/shorthand_production_backend_onnx_cpu.err >&2 || true
  exit 1
fi

ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" bash "${MATRIX_TEST}" \
  >/tmp/shorthand_production_backend_matrix.out \
  2>/tmp/shorthand_production_backend_matrix.err || {
    cat /tmp/shorthand_production_backend_matrix.out >&2 || true
    cat /tmp/shorthand_production_backend_matrix.err >&2 || true
    exit 1
  }
MATRIX_REPORT="/tmp/shorthand_backend_live_sdk_matrix.jsonl"
[[ -s "${MATRIX_REPORT}" ]] || { echo "error: backend live matrix report missing" >&2; exit 1; }
grep -Fq '"backend":"onnxruntime_cpu","format":"onnx","status":"live_success"' "${MATRIX_REPORT}" || {
  echo "error: production-supported ONNX Runtime CPU row lacks live_success" >&2
  cat "${MATRIX_REPORT}" >&2
  exit 1
}
if grep -E '"backend":"(onnxruntime_cuda|onnxruntime_tensorrt|tensorrt|openvino|libtorch|llamacpp)"[^\n]*"status":"live_success"' "${MATRIX_REPORT}" >/dev/null; then
  echo "error: an unqualified accelerator/backend row reported live_success" >&2
  cat "${MATRIX_REPORT}" >&2
  exit 1
fi

cat >"${REPORT}" <<'JSON'
{"schema":"shorthand.backend_hardware_qualification.v1","production_scope":"linux-x64-cpu-v1","mandatory_skips":0,"rows":[{"backend":"onnxruntime_cpu","device_class":"cpu","status":"qualified_live","numerical_fixture":"identity_float32_v13","expected_output":42.0},{"backend":"onnxruntime_cuda","device_class":"gpu","status":"not_production_supported_live_fixture_required"},{"backend":"onnxruntime_tensorrt","device_class":"gpu","status":"not_production_supported_live_fixture_required"},{"backend":"tensorrt","device_class":"gpu","status":"not_production_supported_live_fixture_required"},{"backend":"openvino","device_class":"npu","status":"not_production_supported_live_fixture_required"},{"backend":"libtorch","device_class":"gpu","status":"not_production_supported_live_fixture_required"},{"backend":"llamacpp","device_class":"gpu","status":"not_production_supported_live_fixture_required"},{"backend":"none","device_class":"tpu","status":"not_production_supported_no_backend"}]}
JSON

grep -Fq '"mandatory_skips":0' "${REPORT}"
grep -Fq '"backend":"onnxruntime_cpu","device_class":"cpu","status":"qualified_live"' "${REPORT}"

cat /tmp/shorthand_compiled_hook_onnxruntime_success.out
cat /tmp/shorthand_production_backend_onnx_cpu.out
printf 'PRODUCTION_BACKEND_QUALIFIED backend=onnxruntime_cpu device=cpu platform=linux-x64 output=42\n'
printf 'PRODUCTION_ACCELERATOR_BOUNDARY gpu=not_production_supported npu=not_production_supported tpu=not_production_supported\n'
printf 'QUALIFICATION_REPORT %s\n' "${REPORT}"
printf 'PASS production backend and hardware qualification gate\n'
