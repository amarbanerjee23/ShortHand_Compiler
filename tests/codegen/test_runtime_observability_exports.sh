#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ ! -f "${RUNTIME_LIB}" ]]; then
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_runtime_observability_lib_build.out 2>&1
fi

test -s "${RUNTIME_LIB}"

cat > "${WORK_DIR}/runtime_observability_exports_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"
#include <cstring>

int main() {
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 1;

    const char *initial_metrics = short_runtime_prometheus_metrics();
    if (std::strstr(initial_metrics, "# HELP shorthand_runtime_models") == nullptr) return 2;
    if (std::strstr(initial_metrics, "shorthand_runtime_infer_total 0") == nullptr) return 3;

    if (short_ai_register_tensor("input", "float32", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 10;
    if (short_ai_register_tensor("output", "float32", "1,2", "2", "2") != SHORTHAND_RUNTIME_OK) return 11;
    if (short_ai_register_model("classifier", "onnx", "models/classifier.onnx", "classification", "float32", "1,4", "1,2", "fallback") != SHORTHAND_RUNTIME_OK) return 12;
    if (short_greenai_register_contract("workload", "1 inference", "accuracy >= 1", "compute", "MQ1", "DQ1", "1", "evidence_only") != SHORTHAND_RUNTIME_OK) return 13;
    if (short_greenai_record_measurement("workload", "classifier", "1", "1", "1") != SHORTHAND_RUNTIME_OK) return 14;

    if (short_ai_infer("classifier", "input", "output") != SHORTHAND_RUNTIME_NOT_EXECUTED) return 20;

    const char *metrics = short_runtime_prometheus_metrics();
    if (std::strstr(metrics, "shorthand_runtime_models 1") == nullptr) return 21;
    if (std::strstr(metrics, "shorthand_runtime_tensors 2") == nullptr) return 22;
    if (std::strstr(metrics, "shorthand_runtime_contracts 1") == nullptr) return 23;
    if (std::strstr(metrics, "shorthand_runtime_measurements 1") == nullptr) return 24;
    if (std::strstr(metrics, "shorthand_runtime_infer_total 1") == nullptr) return 25;
    if (std::strstr(metrics, "shorthand_runtime_infer_not_executed_total 1") == nullptr) return 26;
    if (std::strstr(metrics, "shorthand_runtime_last_infer_info") == nullptr) return 27;
    if (std::strstr(metrics, "status=\"not_executed\"") == nullptr) return 28;
    if (std::strstr(metrics, "reason=\"ai_runtime_execution_bridge_pending\"") == nullptr) return 29;

    const char *spans = short_runtime_otlp_spans_json();
    if (std::strstr(spans, "shorthand.runtime.otlp_spans.v1") == nullptr) return 30;
    if (std::strstr(spans, "service.name") == nullptr) return 31;
    if (std::strstr(spans, "short_ai_infer") == nullptr) return 32;
    if (std::strstr(spans, "runtime.infer.calls") == nullptr) return 33;
    if (std::strstr(spans, "last_infer_telemetry") == nullptr) return 34;
    if (std::strstr(spans, "infer_bridge_request") == nullptr) return 35;
    if (std::strstr(spans, "shorthand.runtime.compiled_infer_bridge_request.v1") == nullptr) return 36;

    const char *observability = short_runtime_observability_json();
    if (std::strstr(observability, "shorthand.runtime.observability.v1") == nullptr) return 40;
    if (std::strstr(observability, "infer_bridge_request") == nullptr) return 41;

    return 0;
}
CPP

${CXX:-g++} -std=c++17 -I"${SRC_DIR}" "${WORK_DIR}/runtime_observability_exports_probe.cpp" "${RUNTIME_LIB}" -o "${WORK_DIR}/runtime_observability_exports_probe"
"${WORK_DIR}/runtime_observability_exports_probe"

printf 'PASS runtime observability export gate\n'
