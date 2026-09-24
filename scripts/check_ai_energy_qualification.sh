#!/usr/bin/env bash
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
CXX_BIN="${CXX:-c++}"
export ORT_DISABLE_TELEMETRY=1
# Intentional vector splitting follows the existing native evidence gates.
read -r -a FLAGS <<< "${AI_ENERGY_CXXFLAGS:--std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror}"
ENERGY_SOURCES=("${SRC_DIR}/ai_runtime/ExecutionPlan.cpp" "${SRC_DIR}/ai_runtime/energy/"*.cpp "${SRC_DIR}/ai_runtime/training/TrainingQualification.cpp" "${SRC_DIR}/module/Sha256.cpp")
"${CXX_BIN}" "${FLAGS[@]}" -pthread -I"${ROOT_DIR}" "${ROOT_DIR}/tests/ai_energy/test_energy.cpp" "${ENERGY_SOURCES[@]}" -o "${WORK_DIR}/test_energy"
"${WORK_DIR}/test_energy" "${WORK_DIR}/counters"

# Host-only regression uses a deterministic prepared-session test double.
# It is additional coverage, never a replacement for mandatory real ONNX tests.
HOST_LINK_FLAGS=(-Wl,--gc-sections)
if [[ "$(uname -s)" == Darwin ]]; then HOST_LINK_FLAGS=(-Wl,-dead_strip); fi
"${CXX_BIN}" "${FLAGS[@]}" -ffunction-sections -fdata-sections -pthread -I"${ROOT_DIR}" \
  "${ROOT_DIR}/tests/ai_application/test_classification_host.cpp" \
  "${SRC_DIR}/ai_runtime/ApplicationQualification.cpp" "${SRC_DIR}/ai_runtime/AI_Types.cpp" \
  "${HOST_LINK_FLAGS[@]}" -o "${WORK_DIR}/test_classification_host"
"${WORK_DIR}/test_classification_host"

SDK_FLAGS=(-DSHORTHAND_HAS_ONNXRUNTIME=0)
SDK_LIBS=()
if [[ "${SHORTHAND_AI_ENERGY_REQUIRE_ONNX:-0}" == 1 ]]; then
  [[ -s "${ONNXRUNTIME_ROOT:?mandatory real SDK required}/include/onnxruntime_cxx_api.h" ]]
  SDK_FLAGS=(-DSHORTHAND_HAS_ONNXRUNTIME=1 -I"${ONNXRUNTIME_ROOT}/include")
  SDK_LIBS=(-L"${ONNXRUNTIME_ROOT}/lib" -Wl,-rpath,"${ONNXRUNTIME_ROOT}/lib" -lonnxruntime)
fi

TOOL="${SHORTHAND_AI_QUALIFY_BIN:-${WORK_DIR}/shorthand_ai_qualify}"
MEASURE="${SHORTHAND_C3ECO_MEASURE_BIN:-${WORK_DIR}/shorthand_c3eco_measure}"
if [[ -z "${SHORTHAND_AI_QUALIFY_BIN:-}" ]]; then
  "${CXX_BIN}" "${FLAGS[@]}" -pthread -DSHORTHAND_C3ECO_ASSESS_LIBRARY \
    "-DSHORTHAND_QUALIFICATION_REVISION=\"$(git -C "${ROOT_DIR}" rev-parse HEAD)\"" "${SDK_FLAGS[@]}" \
    "${SRC_DIR}/ai_runtime/QualificationMain.cpp" "${SRC_DIR}/ai_runtime/Qualification.cpp" "${SRC_DIR}/ai_runtime/OnnxArtifact.cpp" \
    "${SRC_DIR}/ai_runtime/ApplicationQualification.cpp" "${SRC_DIR}/serving/ServingRuntime.cpp" \
    "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" "${SRC_DIR}/ai_runtime/AI_Backend.cpp" \
    "${SRC_DIR}/ai_runtime/AI_Types.cpp" "${SRC_DIR}/ai_runtime/AI_Telemetry.cpp" \
    "${SRC_DIR}/ai_runtime/backends/"*.cpp "${ENERGY_SOURCES[@]}" \
    "${SRC_DIR}/evidence/CertificationAssessment.cpp" "${SRC_DIR}/evidence/C3EcoAssessmentScoring.cpp" \
    "${SDK_LIBS[@]}" -o "${TOOL}"
fi
if [[ -z "${SHORTHAND_C3ECO_MEASURE_BIN:-}" ]]; then
  "${CXX_BIN}" "${FLAGS[@]}" "${SRC_DIR}/evidence/MeasurementWorkbook.cpp" -o "${MEASURE}"
fi

python3 "${ROOT_DIR}/tests/ai_energy/test_qualification.py" "${ROOT_DIR}" "${TOOL}" "${WORK_DIR}" "${MEASURE}" "${SHORTHAND_AI_ENERGY_REQUIRE_ONNX:-0}"
python3 "${ROOT_DIR}/tests/ai_application/test_application.py" "${ROOT_DIR}" "${TOOL}" "${WORK_DIR}" "${SHORTHAND_AI_ENERGY_REQUIRE_ONNX:-0}"
python3 "${ROOT_DIR}/tests/ai_application/test_comparison_evidence.py" "${ROOT_DIR}" "${TOOL}" "${WORK_DIR}"

# PR98 family execution: deterministic fixtures prove CPU runtime mechanics only.
# They do not substitute for held-out task quality or calibrated energy evidence.
FAMILY_DIR="${WORK_DIR}/family-fixtures"
python3 "${ROOT_DIR}/tests/ai_benchmark/create_family_fixtures.py" "${FAMILY_DIR}"
FAMILY_TEST="${WORK_DIR}/test_benchmark_runtime"
"${CXX_BIN}" "${FLAGS[@]}" -pthread -I"${ROOT_DIR}" "${SDK_FLAGS[@]}" \
  "${ROOT_DIR}/tests/ai_benchmark/test_benchmark_runtime.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" "${SRC_DIR}/ai_runtime/AI_Backend.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Types.cpp" "${SRC_DIR}/ai_runtime/AI_Telemetry.cpp" \
  "${SRC_DIR}/ai_runtime/backends/"*.cpp "${SDK_LIBS[@]}" -o "${FAMILY_TEST}"
"${FAMILY_TEST}" "${SHORTHAND_AI_ENERGY_REQUIRE_ONNX:-0}" \
  "${FAMILY_DIR}/retrieval.onnx" "${FAMILY_DIR}/detection.onnx" "${FAMILY_DIR}/quantized.onnx"
python3 "${ROOT_DIR}/scripts/validate_ai_benchmark_suite.py" "${ROOT_DIR}/tests/ai_benchmark/benchmark_suite_v1.json" "${ROOT_DIR}"
python3 "${ROOT_DIR}/tests/ai_benchmark/test_benchmark_contract.py" "${ROOT_DIR}"

echo 'PASS native CPU energy qualification, training, application comparison and PR98 AI family gate'
# The SDK must opt out before creating the vendor telemetry uploader.
grep -Fq '"ORT_DISABLE_TELEMETRY", "1"' "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp"
grep -Fq 'value.DisableTelemetryEvents();' "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp"
