#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required hardware-routing text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/hardware_capability_routing.md
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.h
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp
require_file Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h
require_file tests/integration/test_hardware_capability_routing.sh

require_contains docs/hardware_capability_routing.md 'hardware_capability_routing_status: inventory_and_execution_ready_selection'
require_contains docs/hardware_capability_routing.md 'inventory_schema: shorthand.hardware.inventory.v1'
require_contains docs/hardware_capability_routing.md 'selection_schema: shorthand.hardware.selection.v1'
require_contains docs/hardware_capability_routing.md 'production_claim_boundary: detection_is_not_execution_readiness'
require_contains docs/hardware_capability_routing.md '`detected`'
require_contains docs/hardware_capability_routing.md '`accessible`'
require_contains docs/hardware_capability_routing.md '`backend_compatible`'
require_contains docs/hardware_capability_routing.md '`execution_ready`'
require_contains docs/hardware_capability_routing.md 'SHORTHAND_DEVICE_OVERRIDE'
require_contains docs/hardware_capability_routing.md 'SHORTHAND_DEVICE_DENY'
require_contains docs/hardware_capability_routing.md 'SHORTHAND_ALLOW_CPU_FALLBACK'
require_contains docs/hardware_capability_routing.md 'StaticHardwareProbe'

require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'enum class DeviceClass { CPU, GPU, TPU, NPU, Unknown }'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'class SystemHardwareProbe'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'class StaticHardwareProbe'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.inventory.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'shorthand.hardware.selection.v1'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'selected_execution_ready_backend'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'SHORTHAND_DEVICE_PREFERENCE'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'SHORTHAND_DEVICE_OVERRIDE'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'SHORTHAND_DEVICE_DENY'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/HardwareDiscovery.h 'SHORTHAND_MIN_DEVICE_MEMORY_MB'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'selectHardwareRoute'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'shorthand.ai_runtime.telemetry.v2'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp 'no_execution_ready_hardware_backend'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h 'hardware_inventory_json'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h 'hardware_selection_json'
require_contains Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Types.h 'selected_device_class'

require_contains tests/integration/test_hardware_capability_routing.sh 'selected=gpu backend=onnxruntime_cuda status=execution_ready'
require_contains tests/integration/test_hardware_capability_routing.sh 'deny_gpu_selected=cpu cpu_fallback=true'
require_contains tests/integration/test_hardware_capability_routing.sh 'npu_override=openvino tpu_without_backend=not_selected'
require_contains tests/integration/test_hardware_capability_routing.sh 'PASS hardware capability discovery and routing gate'

bash tests/integration/test_hardware_capability_routing.sh

echo "PASS hardware capability discovery and routing gate"
