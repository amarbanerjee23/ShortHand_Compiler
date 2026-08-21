#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat >"${WORK_DIR}/production_backend_hardware_qualification_test.cpp" <<'CPP'
#include "ai_runtime/ProductionBackendQualification.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace shorthand::ai;

namespace {
bool contains(const std::string &value, const std::string &needle) {
    return value.find(needle) != std::string::npos;
}

HardwareRoute route(DeviceClass device, BackendKind backend, const std::string &backend_name) {
    HardwareRoute value;
    value.selected = true;
    value.device_class = device;
    value.device_id = deviceClassToString(device) + ":0";
    value.backend = backend;
    value.backend_name = backend_name;
    value.reason = "selected_execution_ready_backend";
    value.inventory_json = "{\"schema\":\"shorthand.hardware.inventory.v1\"}";
    return value;
}
}

int main() {
    unsetenv("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE");

    const auto cpu = enforceProductionBackendQualification(
        route(DeviceClass::CPU, BackendKind::OnnxRuntimeCPU, "onnxruntime_cpu"));
    if (!cpu.selected || cpu.backend != BackendKind::OnnxRuntimeCPU) return 10;
    if (!contains(cpu.selection_json, "\"production_qualified\":true")) return 11;
    if (!contains(cpu.selection_json, kProductionBackendQualificationSchema)) return 12;

    const auto gpu = enforceProductionBackendQualification(
        route(DeviceClass::GPU, BackendKind::OnnxRuntimeCUDA, "onnxruntime_cuda"));
    if (gpu.selected || gpu.reason != "backend_device_not_production_qualified") return 20;
    if (!contains(gpu.selection_json, "\"production_qualified\":false")) return 21;
    if (!contains(gpu.selection_json, "\"rejected_backend\":\"onnxruntime_cuda\"")) return 22;
    if (!contains(gpu.selection_json, "\"rejected_device_class\":\"gpu\"")) return 23;

    const auto npu = enforceProductionBackendQualification(
        route(DeviceClass::NPU, BackendKind::OpenVINO, "openvino"));
    if (npu.selected || npu.reason != "backend_device_not_production_qualified") return 30;

    const auto tpu = enforceProductionBackendQualification(
        route(DeviceClass::TPU, BackendKind::Fallback, "none"));
    if (tpu.selected || tpu.reason != "backend_device_not_production_qualified") return 40;

    setenv("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE", "1", 1);
    const auto experimental_gpu = enforceProductionBackendQualification(
        route(DeviceClass::GPU, BackendKind::OnnxRuntimeCUDA, "onnxruntime_cuda"));
    if (!experimental_gpu.selected || experimental_gpu.backend != BackendKind::OnnxRuntimeCUDA) return 50;
    if (!contains(experimental_gpu.selection_json, "\"production_qualified\":false")) return 51;
    if (!contains(experimental_gpu.selection_json, "\"experimental_override\":true")) return 52;
    unsetenv("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE");

    const std::string matrix = productionBackendHardwareSupportMatrixJson();
    if (!contains(matrix, "\"production_scope\":\"linux-x64-cpu-v1\"")) return 60;
    if (!contains(matrix, "\"backend\":\"onnxruntime_cpu\"")) return 61;
    if (!contains(matrix, "\"device_class\":\"tpu\"")) return 62;
    if (!contains(matrix, "not_production_supported_live_fixture_required")) return 63;

    std::cout << "QUALIFICATION cpu=onnxruntime_cpu production_qualified=true\n";
    std::cout << "QUALIFICATION gpu=onnxruntime_cuda production_qualified=false default_rejected=true\n";
    std::cout << "QUALIFICATION npu=openvino production_qualified=false tpu_backend=none\n";
    std::cout << "PASS production backend hardware qualification contract\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/production_backend_hardware_qualification_test.cpp" \
  -o "${WORK_DIR}/production_backend_hardware_qualification_test"

"${WORK_DIR}/production_backend_hardware_qualification_test" \
  >/tmp/shorthand_production_backend_hardware_qualification_contract.out \
  2>/tmp/shorthand_production_backend_hardware_qualification_contract.err

grep -q 'PASS production backend hardware qualification contract' /tmp/shorthand_production_backend_hardware_qualification_contract.out
grep -q 'cpu=onnxruntime_cpu production_qualified=true' /tmp/shorthand_production_backend_hardware_qualification_contract.out
grep -q 'gpu=onnxruntime_cuda production_qualified=false default_rejected=true' /tmp/shorthand_production_backend_hardware_qualification_contract.out
grep -q 'npu=openvino production_qualified=false tpu_backend=none' /tmp/shorthand_production_backend_hardware_qualification_contract.out
cat /tmp/shorthand_production_backend_hardware_qualification_contract.out
