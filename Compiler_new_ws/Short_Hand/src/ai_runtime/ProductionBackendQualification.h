#pragma once

#include "HardwareDiscovery.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace shorthand::ai {

inline constexpr const char *kProductionBackendQualificationSchema =
    "shorthand.backend_hardware_qualification.v1";

inline bool productionBackendDeviceQualified(BackendKind backend, DeviceClass device) {
    // PR80/PR81 production support contract v1 is intentionally narrow: the
    // only backend/device pair allowed to carry a production execution claim is
    // ONNX Runtime CPU on the host CPU. Accelerator backends remain usable only
    // through the explicit experimental override until live device-backed
    // numerical qualification exists for that exact pair.
    return backend == BackendKind::OnnxRuntimeCPU && device == DeviceClass::CPU;
}

inline bool allowUnqualifiedBackendHardwareFromEnvironment() {
    const char *raw = std::getenv("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE");
    if (!raw) return false;
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

inline HardwareRoute enforceProductionBackendQualification(HardwareRoute route) {
    const bool experimental_override = allowUnqualifiedBackendHardwareFromEnvironment();
    const bool qualified = route.selected &&
                           productionBackendDeviceQualified(route.backend, route.device_class);

    const std::string original_device_class =
        route.selected ? deviceClassToString(route.device_class) : "none";
    const std::string original_device_id = route.device_id;
    const std::string original_backend = route.backend_name;

    if (route.selected && !qualified && !experimental_override) {
        route.selected = false;
        route.device_class = DeviceClass::Unknown;
        route.device_id = "none";
        route.backend = BackendKind::Fallback;
        route.backend_name = "none";
        route.reason = "backend_device_not_production_qualified";
    }

    std::ostringstream selection;
    selection << "{\"schema\":\"shorthand.hardware.selection.v1\""
              << ",\"qualification_schema\":\"" << kProductionBackendQualificationSchema << "\""
              << ",\"selected\":" << (route.selected ? "true" : "false")
              << ",\"device_class\":\""
              << (route.selected ? deviceClassToString(route.device_class) : "none") << "\""
              << ",\"device_id\":\"" << hardware_detail::jsonEscape(route.device_id) << "\""
              << ",\"backend\":\"" << hardware_detail::jsonEscape(route.backend_name) << "\""
              << ",\"production_qualified\":" << (qualified ? "true" : "false")
              << ",\"experimental_override\":" << (experimental_override ? "true" : "false")
              << ",\"reason\":\"" << hardware_detail::jsonEscape(route.reason) << "\"";

    if (!qualified && original_backend != "none") {
        selection << ",\"rejected_device_class\":\""
                  << hardware_detail::jsonEscape(original_device_class) << "\""
                  << ",\"rejected_device_id\":\""
                  << hardware_detail::jsonEscape(original_device_id) << "\""
                  << ",\"rejected_backend\":\""
                  << hardware_detail::jsonEscape(original_backend) << "\"";
    }
    selection << "}";
    route.selection_json = selection.str();
    return route;
}

inline std::string productionBackendHardwareSupportMatrixJson() {
    return std::string("{\"schema\":\"") + kProductionBackendQualificationSchema +
           "\",\"production_scope\":\"linux-x64-cpu-v1\",\"rows\":["
           "{\"backend\":\"onnxruntime_cpu\",\"device_class\":\"cpu\",\"status\":\"production_supported_live_qualification_required\"},"
           "{\"backend\":\"onnxruntime_cuda\",\"device_class\":\"gpu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"onnxruntime_tensorrt\",\"device_class\":\"gpu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"tensorrt\",\"device_class\":\"gpu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"openvino\",\"device_class\":\"npu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"libtorch\",\"device_class\":\"gpu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"llamacpp\",\"device_class\":\"gpu\",\"status\":\"not_production_supported_live_fixture_required\"},"
           "{\"backend\":\"none\",\"device_class\":\"tpu\",\"status\":\"not_production_supported_no_backend\"}]}";
}

} // namespace shorthand::ai
