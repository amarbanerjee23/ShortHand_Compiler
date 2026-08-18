#include "AI_Runtime.h"
#include "ProductionBackendQualification.h"
#include "backends/FallbackBackend.h"
#include "backends/LibTorchBackend.h"
#include "backends/LlamaCppBackend.h"
#include "backends/OnnxRuntimeBackend.h"
#include "backends/OpenVINOBackend.h"
#include "backends/TensorRTBackend.h"

#include <sstream>
#include <utility>

namespace shorthand::ai {
namespace {

void registerBackends(BackendRegistry &registry) {
    registry.registerBackend(std::make_unique<TensorRTBackend>());
    registry.registerBackend(std::make_unique<OnnxRuntimeBackend>());
    registry.registerBackend(std::make_unique<OpenVINOBackend>());
    registry.registerBackend(std::make_unique<LibTorchBackend>());
    registry.registerBackend(std::make_unique<LlamaCppBackend>());
    registry.registerBackend(std::make_unique<FallbackBackend>());
}

InferenceResult attachHardwareEvidence(InferenceResult result, const HardwareRoute &route) {
    result.hardware_inventory_json = route.inventory_json;
    result.hardware_selection_json = route.selection_json;
    result.selected_device_class = route.selected ? deviceClassToString(route.device_class) : "none";
    result.selected_device_id = route.selected ? route.device_id : "none";

    std::ostringstream telemetry;
    telemetry << "{\"schema\":\"shorthand.ai_runtime.telemetry.v2\""
              << ",\"hardware_inventory\":" << route.inventory_json
              << ",\"hardware_selection\":" << route.selection_json;
    if (!result.telemetry_json_fragment.empty() && result.telemetry_json_fragment != "{}") {
        telemetry << ",\"backend_telemetry\":" << result.telemetry_json_fragment;
    }
    telemetry << "}";
    result.telemetry_json_fragment = telemetry.str();
    return result;
}

InferenceResult unavailableResult(const std::string &reason) {
    InferenceResult result;
    result.status = InferenceStatus::BackendUnavailable;
    result.backend = BackendKind::Fallback;
    result.backend_name = "none";
    result.provider_name = "none";
    result.reason = reason;
    result.output_f32.clear();
    return result;
}

} // namespace

AIRuntime::AIRuntime()
    : AIRuntime(std::make_shared<SystemHardwareProbe>(), hardwareRoutingPolicyFromEnvironment()) {}

AIRuntime::AIRuntime(std::shared_ptr<HardwareProbe> hardware_probe, HardwareRoutingPolicy hardware_policy)
    : hardware_probe_(hardware_probe ? std::move(hardware_probe) : std::make_shared<SystemHardwareProbe>()),
      hardware_policy_(std::move(hardware_policy)) {
    registerBackends(registry);
}

std::vector<BackendCapabilities> AIRuntime::capabilities() const {
    return registry.capabilities();
}

InferenceResult AIRuntime::infer(const ModelSpec &model, const TensorBuffer &input) {
    const auto devices = hardware_probe_->probe();
    const auto route = enforceProductionBackendQualification(
        selectHardwareRoute(devices, registry.capabilities(), model, hardware_policy_));

    if (!validateInputMatchesShape(input)) {
        InferenceResult result;
        result.status = InferenceStatus::InvalidInput;
        result.backend = BackendKind::Fallback;
        result.backend_name = "none";
        result.provider_name = "none";
        result.reason = "input_shape_mismatch";
        return attachHardwareEvidence(std::move(result), route);
    }

    if (route.selected) {
        ModelSpec routed_model = model;
        routed_model.backend_preference = {route.backend};
        routed_model.allow_fallback = false;
        auto *backend = registry.select(routed_model);
        if (backend) {
            auto result = backend->infer(routed_model, input);
            return attachHardwareEvidence(std::move(result), route);
        }
    }

    if (model.allow_fallback) {
        ModelSpec fallback_model = model;
        fallback_model.backend_preference = {BackendKind::Fallback};
        fallback_model.allow_fallback = true;
        auto *backend = registry.select(fallback_model);
        if (backend) {
            auto result = backend->infer(fallback_model, input);
            result.status = InferenceStatus::NotExecuted;
            result.backend = BackendKind::Fallback;
            result.backend_name = "fallback";
            result.provider_name = "none";
            result.reason = route.reason == "backend_device_not_production_qualified"
                                ? route.reason
                                : "backend_not_available";
            result.output_f32.clear();
            return attachHardwareEvidence(std::move(result), route);
        }
    }

    return attachHardwareEvidence(
        unavailableResult(route.reason == "backend_device_not_production_qualified"
                              ? route.reason
                              : "no_execution_ready_hardware_backend"),
        route);
}

} // namespace shorthand::ai

bool AI_Runtime::loadModel(const std::string &model_path){
    model_path_=model_path;
    last_error_.clear();
    if(model_path_.empty()){
        last_error_="missing_model_path";
        return false;
    }
    return true;
}

bool AI_Runtime::run(const TensorData &input_tensor, std::vector<float> &output){
    output.clear();

    shorthand::ai::TensorBuffer input;
    input.spec.name="input";
    input.spec.element_type=shorthand::ai::ElementType::Float32;
    input.spec.shape=input_tensor.shape;
    input.spec.element_count=shorthand::ai::productOfShape(input.spec.shape);
    input.f32_data=input_tensor.data;

    shorthand::ai::ModelSpec model;
    model.name="legacy_onnx_model";
    model.path=model_path_;
    model.format=shorthand::ai::ModelFormat::Onnx;
    model.task="inference";
    model.precision="float32";
    model.input=input.spec;
    model.output.name="output";
    model.output.element_type=shorthand::ai::ElementType::Float32;
    model.backend_preference={shorthand::ai::BackendKind::OnnxRuntimeCPU};
    model.allow_fallback=false;

    shorthand::ai::AIRuntime runtime;
    auto result=runtime.infer(model,input);
    if(result.status==shorthand::ai::InferenceStatus::Success){
        output=result.output_f32;
        last_error_.clear();
        return true;
    }

    last_error_=result.backend_name+":"+shorthand::ai::inferenceStatusToString(result.status)+":"+result.reason;
    return false;
}

std::string AI_Runtime::getLastError() const { return last_error_; }

// shorthand_runtime_hook_integration_ready:
// AI_Runtime owns SDK-backed C++ inference. The public compiled-hook C ABI is
// owned by runtime/ShorthandRuntime.* so future linked builds do not get duplicate
// C symbols such as short_ai_infer or short_ai_infer_f32.
