#include "AI_Runtime.h"
#include "../runtime/RuntimePhaseProfile.h"
#include "ProductionBackendQualification.h"
#include "backends/FallbackBackend.h"
#include "backends/LibTorchBackend.h"
#include "backends/LlamaCppBackend.h"
#include "backends/OnnxRuntimeBackend.h"
#include "backends/OpenVINOBackend.h"
#include "backends/TensorRTBackend.h"

#include <sstream>
#include <fstream>
#include <utility>

namespace shorthand::ai {
namespace {

bool sameTensor(const TensorSpec &a,const TensorSpec &b) {
    return a.name==b.name && a.element_type==b.element_type && a.shape==b.shape &&
           a.dynamic==b.dynamic && a.element_count==b.element_count;
}
bool sameModel(const ModelSpec &a,const ModelSpec &b) {
    return a.name==b.name && a.path==b.path && a.format==b.format && a.task==b.task &&
           a.precision==b.precision && sameTensor(a.input,b.input) && sameTensor(a.output,b.output) &&
           a.backend_preference==b.backend_preference && a.compact==b.compact &&
           a.allow_fallback==b.allow_fallback && a.quality_metric==b.quality_metric &&
           a.quality_op==b.quality_op && a.quality_threshold==b.quality_threshold;
}
bool readCacheableSnapshot(const ModelSpec &model,std::vector<unsigned char> &bytes) {
    if (model.format!=ModelFormat::Onnx || model.precision!="float32") return false;
    std::ifstream file(model.path,std::ios::binary|std::ios::ate);
    if (!file) return false;
    const auto size=file.tellg();
    if (size<=0 || size>16*1024*1024) return false;
    const auto count=static_cast<std::size_t>(size);
    // Avoid geometric growth past the 16 MiB retention limit on file churn.
    if (bytes.capacity()<count) std::vector<unsigned char>(count).swap(bytes);
    else bytes.resize(count);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    return bool(file) && file.peek()==std::char_traits<char>::eof();
}

void registerBackends(BackendRegistry &registry) {
    registry.registerBackend(std::make_unique<TensorRTBackend>());
    registry.registerBackend(std::make_unique<OnnxRuntimeBackend>());
    registry.registerBackend(std::make_unique<OpenVINOBackend>());
    registry.registerBackend(std::make_unique<LibTorchBackend>());
    registry.registerBackend(std::make_unique<LlamaCppBackend>());
    registry.registerBackend(std::make_unique<FallbackBackend>());
}

InferenceResult attachHardwareEvidence(InferenceResult result, const HardwareRoute &route) {
    SHORTHAND_PHASE_SCOPE(runtime_telemetry);
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
    if (!result.evidence_json_fragment.empty())
        telemetry << ",\"execution_evidence\":" << result.evidence_json_fragment;
    telemetry << "}";
    result.telemetry_json_fragment = telemetry.str();
    return result;
}

InferenceResult writeRequestedOutput(InferenceResult result,float *output,std::size_t output_elements) {
    if (!output) return result;
    if (result.status!=InferenceStatus::Success) return result;
    if (!output_elements || result.output_f32.size()!=output_elements) {
        result.status=InferenceStatus::RuntimeError;
        result.reason="runtime_output_size_mismatch";
        result.output_f32.clear();
        result.output_elements=0;
        return result;
    }
    std::copy(result.output_f32.begin(),result.output_f32.end(),output);
    result.output_f32.clear();
    result.output_elements=output_elements;
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
    : AIRuntime(std::make_shared<SystemHardwareProbe>(), hardwareRoutingPolicyFromEnvironment()) {
    policy_environment_signature_=hardwareRoutingPolicyEnvironmentSignature();
}

AIRuntime::AIRuntime(std::shared_ptr<HardwareProbe> hardware_probe, HardwareRoutingPolicy hardware_policy)
    : hardware_probe_(hardware_probe ? std::move(hardware_probe) : std::make_shared<SystemHardwareProbe>()),
      hardware_policy_(std::move(hardware_policy)) {
    registerBackends(registry);
}

std::vector<BackendCapabilities> AIRuntime::capabilities() const {
    return registry.capabilities();
}

std::unique_ptr<PreparedInference> AIRuntime::prepare(const ModelSpec &model,
    const InferenceConfiguration &configuration,std::string &error) {
    refreshHardwareInventoryIfNeeded();
    const auto &route=routeForModel(model);
    if (!route.selected) { error=route.reason; return nullptr; }
    auto routed=model; routed.backend_preference={route.backend}; routed.allow_fallback=false;
    auto *backend=registry.select(routed);
    if (!backend) { error="prepared_backend_unavailable"; return nullptr; }
    return backend->prepare(routed,configuration,error);
}

InferenceResult AIRuntime::infer(const ModelSpec &model, const TensorBuffer &input) {
    return inferImpl(model,input,nullptr,nullptr,0);
}

InferenceResult AIRuntime::inferCached(const ModelSpec &model,const TensorBuffer &input,PreparedInferenceCache &cache) {
    return inferImpl(model,input,&cache,nullptr,0);
}

InferenceResult AIRuntime::inferCachedInto(const ModelSpec &model,const TensorBuffer &input,PreparedInferenceCache &cache,
                                           float *output,std::size_t output_elements) {
    if (!output || !output_elements) {
        InferenceResult result;
        result.status=InferenceStatus::InvalidInput;
        result.reason="invalid_preallocated_output";
        return result;
    }
    return inferImpl(model,input,&cache,output,output_elements);
}

void AIRuntime::refreshPolicyFromEnvironment() {
    const auto signature=hardwareRoutingPolicyEnvironmentSignature();
    if (signature==policy_environment_signature_) return;
    hardware_policy_=hardwareRoutingPolicyFromEnvironment();
    policy_environment_signature_=signature;
    route_cache_valid_=false;
}

void AIRuntime::refreshHardwareInventoryIfNeeded() {
    const auto generation=hardware_probe_->generationToken();
    if (hardware_inventory_valid_ && generation==hardware_generation_token_) return;
    hardware_devices_=hardware_probe_->probe();
    hardware_generation_token_=generation;
    hardware_inventory_valid_=true;
    route_cache_valid_=false;
    ++hardware_probe_count_;
}

const HardwareRoute &AIRuntime::routeForModel(const ModelSpec &model) {
    if (route_cache_valid_ && sameModel(route_model_,model)) {
        last_route_cache_hit_=true;
        return route_cache_;
    }
    last_route_cache_hit_=false;
    route_cache_=enforceProductionBackendQualification(
        selectHardwareRoute(hardware_devices_,registry.capabilities(),model,hardware_policy_));
    route_model_=model;
    route_cache_valid_=true;
    return route_cache_;
}

InferenceResult AIRuntime::inferImpl(const ModelSpec &model,const TensorBuffer &input,PreparedInferenceCache *cache,
                                     float *output,std::size_t output_elements) {
    SHORTHAND_PHASE_SCOPE(hardware_probe);
    refreshHardwareInventoryIfNeeded();
    SHORTHAND_PHASE_NEXT(routing);
    const auto &route=routeForModel(model);

    SHORTHAND_PHASE_NEXT(runtime_validation);
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
            if (cache && route.backend==BackendKind::OnnxRuntimeCPU) {
                SHORTHAND_PHASE_NEXT(snapshot);
                if (readCacheableSnapshot(routed_model,cache->candidate_snapshot_)) {
                    const bool hit=cache->session_ && sameModel(cache->model_,routed_model) &&
                                   cache->snapshot_==cache->candidate_snapshot_;
                    if (!hit) {
                        SHORTHAND_PHASE_SCOPE(session_prepare);
                        InferenceConfiguration configuration;
                        configuration.model_bytes.swap(cache->candidate_snapshot_);
                        cache->clear();
                        std::string error;
                        cache->session_=backend->prepare(routed_model,configuration,error);
                        if (cache->session_) {
                            cache->model_=routed_model;
                            cache->snapshot_=std::move(configuration.model_bytes);
                            ++cache->preparations_;
                        }
                    }
                    if (cache->session_) {
                        SHORTHAND_PHASE_NEXT(backend_other);
                        auto result=output
                            ? cache->session_->runInto(input,output,output_elements)
                            : cache->session_->run(input); // Public finite/shape checks remain mandatory.
                        SHORTHAND_PHASE_NEXT(runtime_telemetry);
                        result.evidence_json_fragment=std::string("{\"schema\":\"shorthand.prepared_cache.v1\",\"hit\":")+
                            (hit?"true":"false")+",\"preparations\":"+std::to_string(cache->preparations_)+
                            ",\"route_cache_hit\":"+(last_route_cache_hit_?"true":"false")+
                            ",\"hardware_probes\":"+std::to_string(hardware_probe_count_)+
                            ",\"preallocated_output\":"+(output?"true":"false")+"}";
                        if (result.status!=InferenceStatus::Success) cache->clear();
                        return attachHardwareEvidence(std::move(result),route);
                    }
                    // Memory loading rejects external-data models. They retain
                    // path-based inference, so changes to external weights are
                    // never hidden behind a stale prepared session.
                } else cache->clear();
            } else if (cache) cache->clear();
            SHORTHAND_PHASE_NEXT(backend_other);
            auto result = writeRequestedOutput(backend->infer(routed_model, input),output,output_elements);
            return attachHardwareEvidence(std::move(result), route);
        }
    }

    if (cache) cache->clear();

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
