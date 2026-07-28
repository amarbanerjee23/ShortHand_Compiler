#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cat > "${WORK_DIR}/hardware_capability_routing_test.cpp" <<'CPP'
#include "ai_runtime/AI_Runtime.h"
#include "ai_runtime/HardwareDiscovery.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace shorthand::ai;

namespace {
BackendCapabilities backend(BackendKind kind, const std::string &name, bool available, ModelFormat format) {
    BackendCapabilities capability;
    capability.kind = kind;
    capability.name = name;
    capability.available = available;
    capability.supported_precisions = {"float32"};
    capability.supports_onnx = format == ModelFormat::Onnx;
    capability.supports_openvino_ir = format == ModelFormat::OpenVINOIR;
    capability.supports_torchscript = format == ModelFormat::TorchScript;
    return capability;
}

HardwareDeviceCapability device(DeviceClass kind, const std::string &id, bool detected, bool accessible) {
    HardwareDeviceCapability capability;
    capability.device_class = kind;
    capability.device_id = id;
    capability.provider = "test";
    capability.detected = detected;
    capability.accessible = accessible;
    capability.memory_mb = 16384;
    capability.reason = "injected_test_probe";
    return capability;
}

ModelSpec model(ModelFormat format) {
    ModelSpec spec;
    spec.name = "routing_model";
    spec.path = "models/routing_fixture";
    spec.format = format;
    spec.precision = "float32";
    spec.allow_fallback = true;
    return spec;
}

bool contains(const std::string &value, const std::string &needle) {
    return value.find(needle) != std::string::npos;
}
}

int main() {
    const std::vector<HardwareDeviceCapability> all_devices = {
        device(DeviceClass::CPU, "cpu:0", true, true),
        device(DeviceClass::GPU, "gpu:0", true, true),
        device(DeviceClass::TPU, "tpu:0", true, true),
        device(DeviceClass::NPU, "npu:0", true, true)
    };

    const std::vector<BackendCapabilities> onnx_backends = {
        backend(BackendKind::OnnxRuntimeCUDA, "onnxruntime_cuda", true, ModelFormat::Onnx),
        backend(BackendKind::OnnxRuntimeCPU, "onnxruntime_cpu", true, ModelFormat::Onnx)
    };

    HardwareRoutingPolicy policy;
    policy.preference = {DeviceClass::GPU, DeviceClass::NPU, DeviceClass::TPU, DeviceClass::CPU};
    const auto gpu_route = selectHardwareRoute(all_devices, onnx_backends, model(ModelFormat::Onnx), policy);
    if (!gpu_route.selected || gpu_route.device_class != DeviceClass::GPU || gpu_route.backend != BackendKind::OnnxRuntimeCUDA) return 10;
    if (!contains(gpu_route.inventory_json, "\"class\":\"gpu\"") ||
        !contains(gpu_route.inventory_json, "\"backend_compatible\":true") ||
        !contains(gpu_route.inventory_json, "\"execution_ready\":true")) return 11;

    HardwareRoutingPolicy deny_gpu = policy;
    deny_gpu.deny_list.insert(DeviceClass::GPU);
    const auto cpu_route = selectHardwareRoute(all_devices, onnx_backends, model(ModelFormat::Onnx), deny_gpu);
    if (!cpu_route.selected || cpu_route.device_class != DeviceClass::CPU || cpu_route.backend != BackendKind::OnnxRuntimeCPU) return 12;

    const std::vector<BackendCapabilities> openvino_backends = {
        backend(BackendKind::OpenVINO, "openvino", true, ModelFormat::OpenVINOIR)
    };
    HardwareRoutingPolicy npu_override;
    npu_override.override_device = DeviceClass::NPU;
    npu_override.allow_cpu_fallback = false;
    const auto npu_route = selectHardwareRoute(all_devices, openvino_backends, model(ModelFormat::OpenVINOIR), npu_override);
    if (!npu_route.selected || npu_route.device_class != DeviceClass::NPU || npu_route.backend != BackendKind::OpenVINO) return 13;

    const std::vector<HardwareDeviceCapability> inaccessible_gpu = {
        device(DeviceClass::CPU, "cpu:0", true, true),
        device(DeviceClass::GPU, "gpu:0", true, false)
    };
    const auto inaccessible_route = selectHardwareRoute(inaccessible_gpu, onnx_backends, model(ModelFormat::Onnx), policy);
    if (!inaccessible_route.selected || inaccessible_route.device_class != DeviceClass::CPU) return 14;

    HardwareRoutingPolicy strict_tpu;
    strict_tpu.preference = {DeviceClass::TPU};
    strict_tpu.allow_cpu_fallback = false;
    const auto tpu_route = selectHardwareRoute(all_devices, onnx_backends, model(ModelFormat::Onnx), strict_tpu);
    if (tpu_route.selected || !contains(tpu_route.selection_json, "\"selected\":false")) return 15;

    auto static_probe = std::make_shared<StaticHardwareProbe>(all_devices);
    if (static_probe->probe().size() != 4) return 16;

    ModelSpec unavailable_model = model(ModelFormat::Onnx);
    unavailable_model.backend_preference = {BackendKind::OnnxRuntimeCUDA};
    TensorBuffer input;
    input.spec.name = "input";
    input.spec.element_type = ElementType::Float32;
    input.spec.shape = {1};
    input.spec.element_count = 1;
    input.f32_data = {42.0f};

    AIRuntime runtime(static_probe, policy);
    const auto result = runtime.infer(unavailable_model, input);
    if (result.status != InferenceStatus::NotExecuted || result.backend_name != "fallback") return 17;
    if (!contains(result.hardware_inventory_json, "shorthand.hardware.inventory.v1")) return 18;
    if (!contains(result.hardware_selection_json, "\"selected\":false")) return 19;
    if (!contains(result.telemetry_json_fragment, "shorthand.ai_runtime.telemetry.v2")) return 20;
    if (!contains(result.telemetry_json_fragment, "shorthand.hardware.selection.v1")) return 21;
    if (result.selected_device_class != "none") return 22;

    SystemHardwareProbe system_probe;
    const auto system_devices = system_probe.probe();
    bool cpu_found = false;
    bool gpu_found = false;
    bool tpu_found = false;
    bool npu_found = false;
    for (const auto &entry : system_devices) {
        cpu_found = cpu_found || entry.device_class == DeviceClass::CPU;
        gpu_found = gpu_found || entry.device_class == DeviceClass::GPU;
        tpu_found = tpu_found || entry.device_class == DeviceClass::TPU;
        npu_found = npu_found || entry.device_class == DeviceClass::NPU;
    }
    if (!cpu_found || !gpu_found || !tpu_found || !npu_found) return 23;

    std::cout << "HARDWARE_ROUTE selected=gpu backend=onnxruntime_cuda status=execution_ready\n";
    std::cout << "HARDWARE_ROUTE deny_gpu_selected=cpu cpu_fallback=true\n";
    std::cout << "HARDWARE_ROUTE npu_override=openvino tpu_without_backend=not_selected\n";
    std::cout << "PASS hardware capability discovery and routing gate\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic \
  -DSHORTHAND_HAS_ONNXRUNTIME=0 \
  -DSHORTHAND_HAS_TENSORRT=0 \
  -DSHORTHAND_HAS_OPENVINO=0 \
  -DSHORTHAND_HAS_LIBTORCH=0 \
  -DSHORTHAND_HAS_LLAMACPP=0 \
  -DSHORTHAND_HAS_OPENBLAS=0 \
  -DSHORTHAND_HAS_EIGEN=0 \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/hardware_capability_routing_test.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Types.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Telemetry.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Backend.cpp" \
  "${SRC_DIR}/ai_runtime/AI_Runtime.cpp" \
  "${SRC_DIR}/ai_runtime/backends/FallbackBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/TensorRTBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/OpenVINOBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/LibTorchBackend.cpp" \
  "${SRC_DIR}/ai_runtime/backends/LlamaCppBackend.cpp" \
  -o "${WORK_DIR}/hardware_capability_routing_test"

"${WORK_DIR}/hardware_capability_routing_test" \
  >/tmp/shorthand_hardware_capability_routing.out \
  2>/tmp/shorthand_hardware_capability_routing.err

grep -q 'PASS hardware capability discovery and routing gate' /tmp/shorthand_hardware_capability_routing.out
grep -q 'selected=gpu backend=onnxruntime_cuda status=execution_ready' /tmp/shorthand_hardware_capability_routing.out
grep -q 'deny_gpu_selected=cpu cpu_fallback=true' /tmp/shorthand_hardware_capability_routing.out
grep -q 'npu_override=openvino tpu_without_backend=not_selected' /tmp/shorthand_hardware_capability_routing.out
cat /tmp/shorthand_hardware_capability_routing.out
