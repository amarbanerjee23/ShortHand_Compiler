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
TOOL="${SHORTHAND_AI_QUALIFY_BIN:-${WORK_DIR}/shorthand_ai_qualify}"
MEASURE="${SHORTHAND_C3ECO_MEASURE_BIN:-${WORK_DIR}/shorthand_c3eco_measure}"
if [[ -z "${SHORTHAND_AI_QUALIFY_BIN:-}" ]]; then
  SDK_FLAGS=(-DSHORTHAND_HAS_ONNXRUNTIME=0)
  SDK_LIBS=()
  if [[ "${SHORTHAND_AI_ENERGY_REQUIRE_ONNX:-0}" == 1 ]]; then
    [[ -s "${ONNXRUNTIME_ROOT:?mandatory real SDK required}/include/onnxruntime_cxx_api.h" ]]
    SDK_FLAGS=(-DSHORTHAND_HAS_ONNXRUNTIME=1 -I"${ONNXRUNTIME_ROOT}/include")
    SDK_LIBS=(-L"${ONNXRUNTIME_ROOT}/lib" -Wl,-rpath,"${ONNXRUNTIME_ROOT}/lib" -lonnxruntime)
  fi
  "${CXX_BIN}" "${FLAGS[@]}" -pthread -DSHORTHAND_C3ECO_ASSESS_LIBRARY \
    "-DSHORTHAND_QUALIFICATION_REVISION=\"$(git -C "${ROOT_DIR}" rev-parse HEAD)\"" "${SDK_FLAGS[@]}" \
    "${SRC_DIR}/ai_runtime/QualificationMain.cpp" "${SRC_DIR}/ai_runtime/Qualification.cpp" "${SRC_DIR}/ai_runtime/OnnxArtifact.cpp" \
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
echo 'PASS native CPU energy qualification, training, evidence and profile gate'
# The SDK must opt out before creating the vendor telemetry uploader.
grep -Fq '"ORT_DISABLE_TELEMETRY", "1"' "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp"
grep -Fq 'value.DisableTelemetryEvents();' "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp"
