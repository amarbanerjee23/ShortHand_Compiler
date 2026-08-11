#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
STRESS_SRC="${ROOT_DIR}/tests/runtime/tsan_runtime_stress.cpp"
WORK_DIR="$(mktemp -d)"
CXX="${SHORTHAND_TSAN_CXX:-clang++}"
trap 'rm -rf "${WORK_DIR}"' EXIT

command -v "${CXX}" >/dev/null 2>&1 || {
  echo "error: ThreadSanitizer compiler unavailable: ${CXX}" >&2
  exit 1
}
[[ -f "${STRESS_SRC}" ]] || { echo "error: missing TSan stress source" >&2; exit 1; }

COMMON_FLAGS=(
  -std=c++17
  -O1
  -g
  -fno-omit-frame-pointer
  -fsanitize=thread
  -fno-sanitize-recover=all
  -fPIE
  -pthread
  -Wall
  -Wextra
  -Wpedantic
  -I"${SRC_DIR}"
)

IMPL_RENAMES=(
  -Dshort_runtime_reset=shimpl_runtime_reset
  -Dshort_runtime_model_count=shimpl_runtime_model_count
  -Dshort_runtime_tensor_count=shimpl_runtime_tensor_count
  -Dshort_runtime_contract_count=shimpl_runtime_contract_count
  -Dshort_runtime_measurement_count=shimpl_runtime_measurement_count
  -Dshort_runtime_infer_count=shimpl_runtime_infer_count
  -Dshort_runtime_infer_success_count=shimpl_runtime_infer_success_count
  -Dshort_runtime_infer_not_executed_count=shimpl_runtime_infer_not_executed_count
  -Dshort_runtime_infer_backend_unavailable_count=shimpl_runtime_infer_backend_unavailable_count
  -Dshort_runtime_infer_invalid_input_count=shimpl_runtime_infer_invalid_input_count
  -Dshort_runtime_last_infer_status=shimpl_runtime_last_infer_status
  -Dshort_runtime_last_infer_backend=shimpl_runtime_last_infer_backend
  -Dshort_runtime_last_infer_reason=shimpl_runtime_last_infer_reason
  -Dshort_runtime_last_infer_telemetry_json=shimpl_runtime_last_infer_telemetry_json
  -Dshort_runtime_infer_bridge_request_json=shimpl_runtime_infer_bridge_request_json
  -Dshort_runtime_observability_json=shimpl_runtime_observability_json
  -Dshort_runtime_prometheus_metrics=shimpl_runtime_prometheus_metrics
  -Dshort_runtime_otlp_spans_json=shimpl_runtime_otlp_spans_json
  -Dshort_ai_register_model=shimpl_ai_register_model
  -Dshort_ai_register_tensor=shimpl_ai_register_tensor
  -Dshort_greenai_register_contract=shimpl_greenai_register_contract
  -Dshort_greenai_record_measurement=shimpl_greenai_record_measurement
  -Dshort_ai_infer=shimpl_ai_infer
  -Dshort_ai_infer_f32=shimpl_ai_infer_f32
  -Dshort_ai_infer_legacy=shimpl_ai_infer_legacy
)

"${CXX}" "${COMMON_FLAGS[@]}" "${IMPL_RENAMES[@]}" \
  -c "${SRC_DIR}/runtime/ShorthandRuntime.cpp" -o "${WORK_DIR}/runtime_impl.o"
"${CXX}" "${COMMON_FLAGS[@]}" \
  -c "${SRC_DIR}/runtime/RuntimeThreadSafeFacade.cpp" -o "${WORK_DIR}/runtime_facade.o"
"${CXX}" "${COMMON_FLAGS[@]}" \
  -c "${STRESS_SRC}" -o "${WORK_DIR}/tsan_stress.o"
"${CXX}" -fsanitize=thread -pie -pthread \
  "${WORK_DIR}/tsan_stress.o" "${WORK_DIR}/runtime_impl.o" "${WORK_DIR}/runtime_facade.o" \
  -o "${WORK_DIR}/tsan_runtime_stress"

set +e
TSAN_OPTIONS="halt_on_error=1:history_size=7:second_deadlock_stack=1" \
  timeout --signal=TERM --kill-after=2 45 \
  "${WORK_DIR}/tsan_runtime_stress" \
  >/tmp/shorthand_tsan_runtime.out \
  2>/tmp/shorthand_tsan_runtime.err
status=$?
set -e

cat /tmp/shorthand_tsan_runtime.out
cat /tmp/shorthand_tsan_runtime.err >&2 || true

if (( status != 0 )); then
  echo "error: ThreadSanitizer race stress failed with status ${status}" >&2
  exit "${status}"
fi
if grep -Eq 'WARNING: ThreadSanitizer|SUMMARY: ThreadSanitizer|data race|lock-order-inversion' \
    /tmp/shorthand_tsan_runtime.out /tmp/shorthand_tsan_runtime.err; then
  echo "error: ThreadSanitizer reported a concurrency defect" >&2
  exit 1
fi
grep -Fq 'PASS runtime concurrency stress' /tmp/shorthand_tsan_runtime.out

printf 'TSAN contract=shorthand.runtime.tsan.v1 compiler=%s\n' "${CXX}"
printf 'PASS mandatory ThreadSanitizer race gate\n'
