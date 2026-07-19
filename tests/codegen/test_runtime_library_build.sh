#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ ! -f "${RUNTIME_LIB}" ]]; then
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_runtime_lib_build.out 2>&1
fi

test -s "${RUNTIME_LIB}"

cat > "${WORK_DIR}/runtime_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"
#include <cstring>

int main() {
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 10;
    if (short_runtime_model_count() != 0) return 11;
    if (short_ai_infer("missing", "input", "output") != SHORTHAND_RUNTIME_MODEL_NOT_FOUND) return 12;
    if (short_runtime_infer_count() != 1) return 13;
    if (short_runtime_last_infer_status() != SHORTHAND_RUNTIME_MODEL_NOT_FOUND) return 14;
    if (std::strstr(short_runtime_last_infer_telemetry_json(), "shorthand.runtime.infer_telemetry.v1") == nullptr) return 15;
    if (std::strcmp(short_runtime_infer_bridge_request_json(), "{}") != 0) return 16;

    if (short_ai_register_tensor("input", "float", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 20;
    if (short_ai_register_tensor("output", "float", "1,2", "2", "2") != SHORTHAND_RUNTIME_OK) return 21;
    if (short_runtime_tensor_count() != 2) return 22;

    if (short_ai_register_model("classifier", "onnx", "models/classifier.onnx", "classification", "float", "1,4", "1,2", "fallback") != SHORTHAND_RUNTIME_OK) return 30;
    if (short_runtime_model_count() != 1) return 31;

    if (short_ai_infer("classifier", "missing_input", "output") != SHORTHAND_RUNTIME_TENSOR_NOT_FOUND) return 40;
    if (short_ai_infer("classifier", "input", "missing_output") != SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND) return 41;
    if (short_ai_infer("classifier", "input", "output") != SHORTHAND_RUNTIME_NOT_EXECUTED) return 42;
    if (short_runtime_last_infer_status() != SHORTHAND_RUNTIME_NOT_EXECUTED) return 43;
    if (std::strstr(short_runtime_last_infer_reason(), "ai_runtime_execution_bridge_pending") == nullptr) return 44;
    if (short_runtime_infer_not_executed_count() < 1) return 45;
    if (std::strstr(short_runtime_observability_json(), "shorthand.runtime.observability.v1") == nullptr) return 46;
    if (std::strstr(short_runtime_observability_json(), "infer_calls") == nullptr) return 47;
    if (std::strstr(short_runtime_infer_bridge_request_json(), "shorthand.runtime.compiled_infer_bridge_request.v1") == nullptr) return 48;
    if (std::strstr(short_runtime_infer_bridge_request_json(), "input_buffer_required_for_ai_runtime_execution") == nullptr) return 49;
    if (std::strstr(short_runtime_observability_json(), "infer_bridge_request") == nullptr) return 50;

    if (short_greenai_register_contract("workload", "1 inference", "accuracy >= 1", "compute", "MQ1", "DQ1", "1", "evidence_only") != SHORTHAND_RUNTIME_OK) return 60;
    if (short_runtime_contract_count() != 1) return 61;
    if (short_greenai_record_measurement("workload", "classifier", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 70;
    if (short_runtime_measurement_count() != 1) return 71;

    if (short_ai_infer_legacy("model.onnx", "1,1", "0.0") != SHORTHAND_RUNTIME_NOT_EXECUTED) return 80;
    if (short_runtime_last_infer_status() != SHORTHAND_RUNTIME_NOT_EXECUTED) return 81;
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -I"${SRC_DIR}" "${WORK_DIR}/runtime_probe.cpp" "${RUNTIME_LIB}" -o "${WORK_DIR}/runtime_probe"
"${WORK_DIR}/runtime_probe" >/tmp/shorthand_runtime_probe.out 2>/tmp/shorthand_runtime_probe.err

grep -q '\[shorthand-runtime\] tensor name=input' /tmp/shorthand_runtime_probe.err
grep -q '\[shorthand-runtime\] model name=classifier' /tmp/shorthand_runtime_probe.err
grep -q '\[shorthand-runtime\] infer model=classifier input=input output=output status=not_executed' /tmp/shorthand_runtime_probe.err
grep -q 'bridge_request=created' /tmp/shorthand_runtime_probe.err
grep -q '\[shorthand-runtime\] greenai_measure workload=workload' /tmp/shorthand_runtime_probe.err

echo "PASS shorthand runtime library registry, infer observability, compiled-infer bridge request, and exported hooks"
