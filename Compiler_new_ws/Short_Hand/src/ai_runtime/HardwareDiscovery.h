#pragma once

#include "AI_Types.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace shorthand::ai {

enum class DeviceClass { CPU, GPU, TPU, NPU, Unknown };

struct HardwareDeviceCapability {
    DeviceClass device_class = DeviceClass::Unknown;
    std::string device_id;
    std::string provider;
    bool detected = false;
    bool accessible = false;
    std::size_t memory_mb = 0;
    std::string reason;
};

struct HardwareRoutingPolicy {
    std::vector<DeviceClass> preference = {DeviceClass::GPU, DeviceClass::NPU, DeviceClass::TPU, DeviceClass::CPU};
    std::optional<DeviceClass> override_device;
    std::set<DeviceClass> deny_list;
    bool allow_cpu_fallback = true;
    std::size_t minimum_memory_mb = 0;
};

struct HardwareRoute {
    bool selected = false;
    DeviceClass device_class = DeviceClass::Unknown;
    std::string device_id = "none";
    BackendKind backend = BackendKind::Fallback;
    std::string backend_name = "none";
    std::string reason = "no_execution_ready_hardware_backend";
    std::string inventory_json = "{}";
    std::string selection_json = "{}";
};

class HardwareProbe {
public:
    virtual ~HardwareProbe() = default;
    virtual std::vector<HardwareDeviceCapability> probe() const = 0;
};

namespace hardware_detail {

inline std::string normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        if (c == '-') return '_';
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline std::string jsonEscape(const std::string &value) {
    std::string out;
    for (char c : value) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out += c;
        }
    }
    return out;
}

inline bool pathExists(const std::string &path) {
    std::error_code error;
    return std::filesystem::exists(path, error);
}

inline const char *environment(const char *name) {
    const char *value = std::getenv(name);
    return value ? value : "";
}

inline bool environmentTrue(const char *name) {
    const std::string value = normalize(environment(name));
    return !value.empty() && value != "0" && value != "false" && value != "no" && value != "off";
}

inline bool environmentHasDeviceValue(const char *name) {
    const std::string value = normalize(environment(name));
    return !value.empty() && value != "-1" && value != "none" && value != "void" && value != "false";
}

inline std::size_t hostMemoryMb() {
    std::ifstream input("/proc/meminfo");
    std::string key;
    std::size_t value_kb = 0;
    std::string unit;
    while (input >> key >> value_kb >> unit) {
        if (key == "MemTotal:") return value_kb / 1024;
    }
    return 0;
}

inline bool precisionSupported(const BackendCapabilities &capability, const std::string &precision) {
    if (precision.empty() || capability.supported_precisions.empty()) return true;
    const std::string wanted = normalize(precision);
    for (const auto &candidate : capability.supported_precisions) {
        if (normalize(candidate) == wanted) return true;
    }
    return false;
}

inline bool modelAllowsBackend(const ModelSpec &model, BackendKind backend) {
    if (model.backend_preference.empty()) return true;
    return std::find(model.backend_preference.begin(), model.backend_preference.end(), backend) != model.backend_preference.end();
}

inline bool backendSupportsDevice(BackendKind backend, DeviceClass device) {
    switch (backend) {
        case BackendKind::TensorRT:
        case BackendKind::OnnxRuntimeTensorRT:
        case BackendKind::OnnxRuntimeCUDA:
            return device == DeviceClass::GPU;
        case BackendKind::OnnxRuntimeCPU:
            return device == DeviceClass::CPU;
        case BackendKind::OpenVINO:
            return device == DeviceClass::CPU || device == DeviceClass::GPU || device == DeviceClass::NPU;
        case BackendKind::LibTorch:
        case BackendKind::LlamaCpp:
            return device == DeviceClass::CPU || device == DeviceClass::GPU;
        case BackendKind::Fallback:
            return false;
    }
    return false;
}

inline bool policyAllowsDevice(const HardwareRoutingPolicy &policy, const HardwareDeviceCapability &device) {
    if (policy.deny_list.count(device.device_class) != 0) return false;
    if (policy.minimum_memory_mb > 0 && device.memory_mb < policy.minimum_memory_mb) return false;
    return true;
}

inline bool backendCompatible(const HardwareDeviceCapability &device,
                              const std::vector<BackendCapabilities> &capabilities,
                              const ModelSpec &model) {
    for (const auto &capability : capabilities) {
        if (capability.kind == BackendKind::Fallback) continue;
        if (!modelAllowsBackend(model, capability.kind)) continue;
        if (!backendSupportsDevice(capability.kind, device.device_class)) continue;
        if (!backendSupportsFormat(capability, model.format)) continue;
        if (!precisionSupported(capability, model.precision)) continue;
        return true;
    }
    return false;
}

inline const BackendCapabilities *executionReadyBackend(const HardwareDeviceCapability &device,
                                                         const std::vector<BackendCapabilities> &capabilities,
                                                         const ModelSpec &model,
                                                         const HardwareRoutingPolicy &policy) {
    if (!device.detected || !device.accessible || !policyAllowsDevice(policy, device)) return nullptr;
    for (const auto &capability : capabilities) {
        if (!capability.available || capability.kind == BackendKind::Fallback) continue;
        if (!modelAllowsBackend(model, capability.kind)) continue;
        if (!backendSupportsDevice(capability.kind, device.device_class)) continue;
        if (!backendSupportsFormat(capability, model.format)) continue;
        if (!precisionSupported(capability, model.precision)) continue;
        return &capability;
    }
    return nullptr;
}

inline std::vector<DeviceClass> routingOrder(const HardwareRoutingPolicy &policy) {
    std::vector<DeviceClass> order;
    if (policy.override_device.has_value()) {
        order.push_back(*policy.override_device);
        if (policy.allow_cpu_fallback && *policy.override_device != DeviceClass::CPU) order.push_back(DeviceClass::CPU);
    } else {
        order = policy.preference;
        if (policy.allow_cpu_fallback && std::find(order.begin(), order.end(), DeviceClass::CPU) == order.end()) {
            order.push_back(DeviceClass::CPU);
        }
    }
    if (!policy.allow_cpu_fallback) {
        order.erase(std::remove(order.begin(), order.end(), DeviceClass::CPU), order.end());
    }
    std::vector<DeviceClass> unique;
    for (DeviceClass device : order) {
        if (std::find(unique.begin(), unique.end(), device) == unique.end()) unique.push_back(device);
    }
    return unique;
}

} // namespace hardware_detail

inline std::string deviceClassToString(DeviceClass device) {
    switch (device) {
        case DeviceClass::CPU: return "cpu";
        case DeviceClass::GPU: return "gpu";
        case DeviceClass::TPU: return "tpu";
        case DeviceClass::NPU: return "npu";
        case DeviceClass::Unknown: return "unknown";
    }
    return "unknown";
}

inline DeviceClass parseDeviceClass(const std::string &value) {
    const std::string normalized = hardware_detail::normalize(value);
    if (normalized == "cpu") return DeviceClass::CPU;
    if (normalized == "gpu") return DeviceClass::GPU;
    if (normalized == "tpu") return DeviceClass::TPU;
    if (normalized == "npu") return DeviceClass::NPU;
    return DeviceClass::Unknown;
}

class SystemHardwareProbe final : public HardwareProbe {
public:
    std::vector<HardwareDeviceCapability> probe() const override {
        std::vector<HardwareDeviceCapability> devices;
        devices.push_back({DeviceClass::CPU, "cpu:0", "host", true, true, hardware_detail::hostMemoryMb(), "host_cpu_available"});

        const bool nvidia_node = hardware_detail::pathExists("/dev/nvidia0") || hardware_detail::pathExists("/dev/nvidiactl");
        const bool amd_node = hardware_detail::pathExists("/dev/kfd");
        const bool render_node = hardware_detail::pathExists("/dev/dri/renderD128");
        const bool gpu_environment = hardware_detail::environmentHasDeviceValue("NVIDIA_VISIBLE_DEVICES") ||
                                     hardware_detail::environmentHasDeviceValue("CUDA_VISIBLE_DEVICES") ||
                                     hardware_detail::environmentTrue("SHORTHAND_GPU_DETECTED");
        const bool gpu_detected = nvidia_node || amd_node || render_node || gpu_environment;
        const bool gpu_accessible = nvidia_node || amd_node || render_node || hardware_detail::environmentTrue("SHORTHAND_GPU_ACCESSIBLE");
        std::string gpu_provider = nvidia_node || hardware_detail::environmentHasDeviceValue("NVIDIA_VISIBLE_DEVICES") ? "nvidia" :
                                   (amd_node ? "amd" : (render_node ? "drm" : "unknown"));
        std::string gpu_reason = gpu_accessible ? "accelerator_device_accessible_backend_probe_required" :
                                 (gpu_detected ? "accelerator_signal_detected_but_not_accessible" : "no_gpu_signal_detected");
        devices.push_back({DeviceClass::GPU, "gpu:0", gpu_provider, gpu_detected, gpu_accessible, 0, gpu_reason});

        const bool tpu_node = hardware_detail::pathExists("/dev/accel0");
        const bool tpu_environment = hardware_detail::environmentHasDeviceValue("TPU_NAME") ||
                                     hardware_detail::environmentHasDeviceValue("COLAB_TPU_ADDR") ||
                                     hardware_detail::environmentHasDeviceValue("XRT_TPU_CONFIG") ||
                                     hardware_detail::environmentTrue("SHORTHAND_TPU_DETECTED");
        const bool tpu_detected = tpu_node || tpu_environment;
        const bool tpu_accessible = tpu_node || hardware_detail::environmentTrue("SHORTHAND_TPU_ACCESSIBLE");
        devices.push_back({DeviceClass::TPU, "tpu:0", "google", tpu_detected, tpu_accessible, 0,
                           tpu_accessible ? "tpu_accessible_backend_probe_required" :
                           (tpu_detected ? "tpu_signal_detected_but_not_accessible" : "no_tpu_signal_detected")});

        const bool npu_node = hardware_detail::pathExists("/dev/accel/accel0") || hardware_detail::pathExists("/dev/apex_0");
        bool npu_detected = npu_node || hardware_detail::environmentTrue("SHORTHAND_NPU_DETECTED");
        bool npu_accessible = npu_node || hardware_detail::environmentTrue("SHORTHAND_NPU_ACCESSIBLE");
        std::string npu_provider = npu_node ? "system_accelerator" : "unknown";
#if defined(__APPLE__) && defined(__aarch64__)
        npu_detected = true;
        if (!npu_accessible) npu_provider = "apple";
#endif
        devices.push_back({DeviceClass::NPU, "npu:0", npu_provider, npu_detected, npu_accessible, 0,
                           npu_accessible ? "npu_accessible_backend_probe_required" :
                           (npu_detected ? "npu_detected_but_backend_access_not_confirmed" : "no_npu_signal_detected")});
        return devices;
    }
};

class StaticHardwareProbe final : public HardwareProbe {
public:
    explicit StaticHardwareProbe(std::vector<HardwareDeviceCapability> devices) : devices_(std::move(devices)) {}
    std::vector<HardwareDeviceCapability> probe() const override { return devices_; }

private:
    std::vector<HardwareDeviceCapability> devices_;
};

inline HardwareRoutingPolicy hardwareRoutingPolicyFromEnvironment() {
    HardwareRoutingPolicy policy;

    const std::string preference = hardware_detail::environment("SHORTHAND_DEVICE_PREFERENCE");
    if (!preference.empty()) {
        policy.preference.clear();
        std::stringstream stream(preference);
        std::string token;
        while (std::getline(stream, token, ',')) {
            const DeviceClass parsed = parseDeviceClass(token);
            if (parsed != DeviceClass::Unknown) policy.preference.push_back(parsed);
        }
    }

    const DeviceClass override_device = parseDeviceClass(hardware_detail::environment("SHORTHAND_DEVICE_OVERRIDE"));
    if (override_device != DeviceClass::Unknown) policy.override_device = override_device;

    const std::string deny = hardware_detail::environment("SHORTHAND_DEVICE_DENY");
    std::stringstream deny_stream(deny);
    std::string deny_token;
    while (std::getline(deny_stream, deny_token, ',')) {
        const DeviceClass parsed = parseDeviceClass(deny_token);
        if (parsed != DeviceClass::Unknown) policy.deny_list.insert(parsed);
    }

    const std::string fallback = hardware_detail::environment("SHORTHAND_ALLOW_CPU_FALLBACK");
    if (!fallback.empty()) policy.allow_cpu_fallback = hardware_detail::environmentTrue("SHORTHAND_ALLOW_CPU_FALLBACK");

    const std::string minimum_memory = hardware_detail::environment("SHORTHAND_MIN_DEVICE_MEMORY_MB");
    if (!minimum_memory.empty()) {
        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(minimum_memory.c_str(), &end, 10);
        if (end != minimum_memory.c_str() && *end == '\0') policy.minimum_memory_mb = static_cast<std::size_t>(parsed);
    }
    return policy;
}

inline HardwareRoute selectHardwareRoute(const std::vector<HardwareDeviceCapability> &devices,
                                          const std::vector<BackendCapabilities> &capabilities,
                                          const ModelSpec &model,
                                          const HardwareRoutingPolicy &policy) {
    HardwareRoute route;

    for (DeviceClass wanted : hardware_detail::routingOrder(policy)) {
        for (const auto &device : devices) {
            if (device.device_class != wanted) continue;
            const BackendCapabilities *backend = hardware_detail::executionReadyBackend(device, capabilities, model, policy);
            if (!backend) continue;
            route.selected = true;
            route.device_class = device.device_class;
            route.device_id = device.device_id.empty() ? deviceClassToString(device.device_class) + ":0" : device.device_id;
            route.backend = backend->kind;
            route.backend_name = backend->name.empty() ? backendKindToString(backend->kind) : backend->name;
            route.reason = "selected_execution_ready_backend";
            break;
        }
        if (route.selected) break;
    }

    std::ostringstream inventory;
    inventory << "{\"schema\":\"shorthand.hardware.inventory.v1\",\"devices\":[";
    for (std::size_t index = 0; index < devices.size(); ++index) {
        const auto &device = devices[index];
        const bool compatible = hardware_detail::backendCompatible(device, capabilities, model);
        const bool execution_ready = hardware_detail::executionReadyBackend(device, capabilities, model, policy) != nullptr;
        if (index) inventory << ',';
        inventory << "{\"class\":\"" << deviceClassToString(device.device_class)
                  << "\",\"id\":\"" << hardware_detail::jsonEscape(device.device_id)
                  << "\",\"provider\":\"" << hardware_detail::jsonEscape(device.provider)
                  << "\",\"detected\":" << (device.detected ? "true" : "false")
                  << ",\"accessible\":" << (device.accessible ? "true" : "false")
                  << ",\"backend_compatible\":" << (compatible ? "true" : "false")
                  << ",\"execution_ready\":" << (execution_ready ? "true" : "false")
                  << ",\"memory_mb\":" << device.memory_mb
                  << ",\"reason\":\"" << hardware_detail::jsonEscape(device.reason) << "\"}";
    }
    inventory << "]}";
    route.inventory_json = inventory.str();

    std::ostringstream selection;
    selection << "{\"schema\":\"shorthand.hardware.selection.v1\""
              << ",\"selected\":" << (route.selected ? "true" : "false")
              << ",\"device_class\":\"" << (route.selected ? deviceClassToString(route.device_class) : "none") << "\""
              << ",\"device_id\":\"" << hardware_detail::jsonEscape(route.device_id) << "\""
              << ",\"backend\":\"" << hardware_detail::jsonEscape(route.backend_name) << "\""
              << ",\"reason\":\"" << hardware_detail::jsonEscape(route.reason) << "\""
              << ",\"allow_cpu_fallback\":" << (policy.allow_cpu_fallback ? "true" : "false")
              << ",\"minimum_memory_mb\":" << policy.minimum_memory_mb
              << "}";
    route.selection_json = selection.str();
    return route;
}

} // namespace shorthand::ai
