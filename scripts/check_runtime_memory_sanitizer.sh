#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
STRESS_SRC="${ROOT_DIR}/tests/runtime/tsan_runtime_stress.cpp"
SERVING_STRESS_SRC="${ROOT_DIR}/tests/runtime/serving_runtime_stress.cpp"
SERVING_SRC="${SRC_DIR}/serving/ServingRuntime.cpp"
WORK_DIR="$(mktemp -d)"
CXX="${SHORTHAND_MEMORY_SANITIZER_CXX:-clang++}"
trap 'rm -rf "${WORK_DIR}"' EXIT

command -v "${CXX}" >/dev/null 2>&1 || {
  echo "error: ASan/UBSan compiler unavailable: ${CXX}" >&2
  exit 1
}
[[ -f "${STRESS_SRC}" ]] || { echo "error: missing runtime stress source" >&2; exit 1; }
[[ -f "${SERVING_STRESS_SRC}" && -f "${SERVING_SRC}" ]] || {
  echo "error: missing concurrent serving sanitizer stress source" >&2
  exit 1
}

COMMON_FLAGS=(
  -std=c++17
  -O1
  -g
  -fno-omit-frame-pointer
  -fsanitize=address,undefined
  -fno-sanitize-recover=all
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
  -c "${STRESS_SRC}" -o "${WORK_DIR}/memory_stress.o"
"${CXX}" -fsanitize=address,undefined -fno-sanitize-recover=all -pthread \
  "${WORK_DIR}/memory_stress.o" "${WORK_DIR}/runtime_impl.o" "${WORK_DIR}/runtime_facade.o" \
  -o "${WORK_DIR}/runtime_memory_stress"

set +e
ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" \
UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
  timeout --signal=TERM --kill-after=2 45 \
  "${WORK_DIR}/runtime_memory_stress" \
  >/tmp/shorthand_runtime_memory_sanitizer.out \
  2>/tmp/shorthand_runtime_memory_sanitizer.err
status=$?
set -e

cat /tmp/shorthand_runtime_memory_sanitizer.out
cat /tmp/shorthand_runtime_memory_sanitizer.err >&2 || true

if (( status != 0 )); then
  echo "error: runtime ASan/LSan/UBSan stress failed with status ${status}" >&2
  exit "${status}"
fi
if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|SUMMARY:.*Sanitizer' \
    /tmp/shorthand_runtime_memory_sanitizer.out /tmp/shorthand_runtime_memory_sanitizer.err; then
  echo "error: runtime memory/undefined-behavior sanitizer reported a defect" >&2
  exit 1
fi
grep -Fq 'PASS runtime concurrency stress' /tmp/shorthand_runtime_memory_sanitizer.out

"${CXX}" "${COMMON_FLAGS[@]}" \
  "${SERVING_SRC}" "${SERVING_STRESS_SRC}" \
  -o "${WORK_DIR}/serving_memory_stress"
set +e
ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" \
UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
  timeout --signal=TERM --kill-after=2 60 \
  "${WORK_DIR}/serving_memory_stress" \
  >/tmp/shorthand_serving_memory_sanitizer.out \
  2>/tmp/shorthand_serving_memory_sanitizer.err
serving_status=$?
set -e
cat /tmp/shorthand_serving_memory_sanitizer.out
cat /tmp/shorthand_serving_memory_sanitizer.err >&2 || true
if (( serving_status != 0 )); then
  echo "error: serving ASan/LSan/UBSan stress failed with status ${serving_status}" >&2
  exit "${serving_status}"
fi
if grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|SUMMARY:.*Sanitizer' \
    /tmp/shorthand_serving_memory_sanitizer.out /tmp/shorthand_serving_memory_sanitizer.err; then
  echo "error: serving memory/undefined-behavior sanitizer reported a defect" >&2
  exit 1
fi
grep -Fq 'PASS serving runtime concurrent load and soak stress' /tmp/shorthand_serving_memory_sanitizer.out

printf 'SERVING_MEMORY_SANITIZER contract=shorthand.serving.runtime.v1 compiler=%s\n' "${CXX}"
printf 'RUNTIME_MEMORY_SANITIZER contract=shorthand.runtime.asan_lsan_ubsan.v1 compiler=%s\n' "${CXX}"
printf 'PASS runtime ASan LSan UBSan stress gate\n'
