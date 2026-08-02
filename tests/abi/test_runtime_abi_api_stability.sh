#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
LIB="${BUILD_DIR}/libshorthand_runtime.a"
MANIFEST="${ROOT_DIR}/abi/runtime_public_symbols_v1.txt"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_runtime_abi_make.out 2>&1 || {
  cat /tmp/shorthand_runtime_abi_make.out >&2 || true
  exit 1
}

if [[ ! -f "${LIB}" ]]; then
  echo "error: runtime library was not produced: ${LIB}" >&2
  exit 1
fi

NM_TOOL="${NM:-nm}"
if ! command -v "${NM_TOOL}" >/dev/null 2>&1; then
  echo "error: nm is required for runtime ABI symbol validation" >&2
  exit 1
fi

if "${NM_TOOL}" -g --defined-only "${LIB}" >"${WORK_DIR}/nm.out" 2>/dev/null; then
  :
else
  "${NM_TOOL}" -g "${LIB}" >"${WORK_DIR}/nm.out"
fi

awk '{print $NF}' "${WORK_DIR}/nm.out" \
  | sed 's/^_//' \
  | grep '^short_' \
  | LC_ALL=C sort -u >"${WORK_DIR}/actual_symbols.txt"
LC_ALL=C sort -u "${MANIFEST}" >"${WORK_DIR}/expected_symbols.txt"

diff -u "${WORK_DIR}/expected_symbols.txt" "${WORK_DIR}/actual_symbols.txt"

symbol_count="$(wc -l <"${WORK_DIR}/actual_symbols.txt" | tr -d ' ')"
if [[ "${symbol_count}" != "25" ]]; then
  echo "error: runtime ABI v1 must expose exactly 25 frozen external symbols; observed ${symbol_count}" >&2
  cat "${WORK_DIR}/actual_symbols.txt" >&2
  exit 1
fi

cat >"${WORK_DIR}/frozen_v1_consumer.c" <<'C'
#include "abi/shorthand_runtime_abi_v1.h"

#include <stddef.h>

int main(void) {
    if (SHORTHAND_RUNTIME_OK != 0 ||
        SHORTHAND_RUNTIME_INVALID_ARGUMENT != 1 ||
        SHORTHAND_RUNTIME_MODEL_NOT_FOUND != 2 ||
        SHORTHAND_RUNTIME_TENSOR_NOT_FOUND != 3 ||
        SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND != 4 ||
        SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE != 5 ||
        SHORTHAND_RUNTIME_NOT_EXECUTED != 6 ||
        SHORTHAND_RUNTIME_INVALID_INPUT != 7 ||
        SHORTHAND_RUNTIME_RUNTIME_ERROR != 8) return 10;

    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 11;
    if (short_runtime_model_count() != 0 || short_runtime_tensor_count() != 0 ||
        short_runtime_contract_count() != 0 || short_runtime_measurement_count() != 0) return 12;

    if (short_ai_register_tensor("input", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 13;
    if (short_ai_register_tensor("output", "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 14;
    if (short_ai_register_model("model", "onnx", "models/not_present.onnx", "inference", "float32", "1", "1", "onnxruntime_cpu") != SHORTHAND_RUNTIME_OK) return 15;
    if (short_greenai_register_contract("contract", "inference", "quality>=0.9", "runtime", "measured", "declared", "location", "candidate") != SHORTHAND_RUNTIME_OK) return 16;
    if (short_greenai_record_measurement("workload", "onnxruntime_cpu", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 17;

    if (short_runtime_model_count() != 1 || short_runtime_tensor_count() != 2 ||
        short_runtime_contract_count() != 1 || short_runtime_measurement_count() != 1) return 18;

    if (short_ai_infer("model", "input", "output") == SHORTHAND_RUNTIME_OK) return 19;

    {
        float input[1] = {1.0f};
        float output[1] = {-9.0f};
        int output_count = -1;
        if (short_ai_infer_f32("model", "input", input, 1, "output", output, 1, &output_count) == SHORTHAND_RUNTIME_OK) return 20;
        if (output_count != 0 || output[0] != -9.0f) return 21;
    }

    if (short_ai_infer_legacy("models/not_present.onnx", "1", "1.0") == SHORTHAND_RUNTIME_OK) return 22;
    if (short_runtime_infer_count() < 3) return 23;
    if (short_runtime_infer_success_count() != 0) return 24;
    (void)short_runtime_infer_not_executed_count();
    (void)short_runtime_infer_backend_unavailable_count();
    (void)short_runtime_infer_invalid_input_count();
    (void)short_runtime_last_infer_status();
    if (short_runtime_last_infer_backend() == NULL || short_runtime_last_infer_reason() == NULL) return 25;
    if (short_runtime_last_infer_telemetry_json() == NULL || short_runtime_infer_bridge_request_json() == NULL) return 26;
    if (short_runtime_observability_json() == NULL || short_runtime_prometheus_metrics() == NULL ||
        short_runtime_otlp_spans_json() == NULL) return 27;
    return 0;
}
C

${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"${ROOT_DIR}" \
  -c "${WORK_DIR}/frozen_v1_consumer.c" -o "${WORK_DIR}/frozen_v1_consumer.o"
${CXX:-g++} "${WORK_DIR}/frozen_v1_consumer.o" "${LIB}" -o "${WORK_DIR}/frozen_v1_consumer"
"${WORK_DIR}/frozen_v1_consumer"

cat >"${WORK_DIR}/current_api_consumer.c" <<'C'
#include "Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h"

#include <string.h>

int main(void) {
    if (strcmp(short_runtime_abi_version(), "1.0.0") != 0) return 30;
    if (strcmp(short_runtime_api_version(), "1.0.0") != 0) return 31;
    if (!short_runtime_is_abi_compatible(1, 0)) return 32;
    if (!short_runtime_is_api_compatible(1, 0)) return 33;
    if (short_runtime_is_abi_compatible(1, 1)) return 34;
    if (short_runtime_is_api_compatible(1, 1)) return 35;
    if (short_runtime_is_abi_compatible(2, 0)) return 36;
    if (short_runtime_is_api_compatible(2, 0)) return 37;
    if (short_runtime_is_abi_compatible(-1, 0)) return 38;
    if (short_runtime_is_api_compatible(1, -1)) return 39;
    return short_runtime_reset() == SHORTHAND_RUNTIME_OK ? 0 : 40;
}
C

${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"${ROOT_DIR}" \
  -c "${WORK_DIR}/current_api_consumer.c" -o "${WORK_DIR}/current_api_consumer.o"
${CXX:-g++} "${WORK_DIR}/current_api_consumer.o" "${LIB}" -o "${WORK_DIR}/current_api_consumer"
"${WORK_DIR}/current_api_consumer"

printf 'ABI_SYMBOL_COUNT %s\n' "${symbol_count}"
printf 'ABI_VERSION %s API_VERSION %s\n' "1.0.0" "1.0.0"
printf 'PASS frozen runtime ABI v1 consumer\n'
printf 'PASS runtime ABI and API stability test\n'
