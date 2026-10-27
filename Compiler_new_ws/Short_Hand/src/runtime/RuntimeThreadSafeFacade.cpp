#include "ShorthandRuntime.h"
#include "RuntimePhaseProfile.h"
#include "ImmutableModelInternal.h"

#if SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE && SHORTHAND_HAS_ONNXRUNTIME
#define SHORTHAND_IMMUTABLE_ENABLED 1
#include "ImmutableOnnxEnvelope.h"
#include "../ai_runtime/AI_Runtime.h"
#include "../module/Sha256.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#else
#define SHORTHAND_IMMUTABLE_ENABLED 0
#endif

#include <mutex>
#include <string>

extern "C" {
int shimpl_runtime_reset(void);
int shimpl_runtime_model_count(void);
int shimpl_runtime_tensor_count(void);
int shimpl_runtime_contract_count(void);
int shimpl_runtime_measurement_count(void);
int shimpl_runtime_infer_count(void);
int shimpl_runtime_infer_success_count(void);
int shimpl_runtime_infer_not_executed_count(void);
int shimpl_runtime_infer_backend_unavailable_count(void);
int shimpl_runtime_infer_invalid_input_count(void);
int shimpl_runtime_last_infer_status(void);
const char *shimpl_runtime_last_infer_backend(void);
const char *shimpl_runtime_last_infer_reason(void);
const char *shimpl_runtime_last_infer_telemetry_json(void);
const char *shimpl_runtime_infer_bridge_request_json(void);
const char *shimpl_runtime_observability_json(void);
const char *shimpl_runtime_prometheus_metrics(void);
const char *shimpl_runtime_otlp_spans_json(void);
int shimpl_ai_register_model(const char *name,
                             const char *format,
                             const char *path,
                             const char *task,
                             const char *precision,
                             const char *input_shape,
                             const char *output_shape,
                             const char *backend_preference);
int shimpl_ai_register_tensor(const char *name,
                              const char *element_type,
                              const char *shape,
                              const char *rank,
                              const char *total_elements);
int shimpl_greenai_register_contract(const char *name,
                                     const char *functional_unit,
                                     const char *success_criteria,
                                     const char *boundary,
                                     const char *measurement_quality,
                                     const char *data_quality,
                                     const char *carbon_factor,
                                     const char *claims_mode);
int shimpl_greenai_record_measurement(const char *workload,
                                      const char *backend,
                                      const char *inferences,
                                      const char *watts,
                                      const char *seconds);
int shimpl_ai_infer(const char *model_name,
                    const char *input_name,
                    const char *output_name);
int shimpl_ai_infer_f32(const char *model_name,
                        const char *input_name,
                        const float *input_values,
                        int input_count,
                        const char *output_name,
                        float *output_values,
                        int output_capacity,
                        int *output_count);
int shimpl_ai_infer_legacy(const char *model_path,
                           const char *shape_csv,
                           const char *input_csv);
}

namespace {
std::recursive_mutex &runtimeMutex() {
    static std::recursive_mutex mutex;
    return mutex;
}

#if SHORTHAND_IMMUTABLE_ENABLED
struct ImmutableSlot {
    // Declaration order keeps the snapshot alive beyond session destruction.
    shorthand::ai::InferenceConfiguration configuration;
    shorthand::ai::ModelSpec model;
    shorthand::ai::AIRuntime runtime;
    std::unique_ptr<shorthand::ai::PreparedInference> session;
    std::vector<float> scratch;
    std::string digest;
    std::uint64_t generation=0;
};
std::unique_ptr<ImmutableSlot> immutable_slot;
std::uint64_t immutable_generation=0; // Never reset; exhaustion fails closed.

bool immutableShape(const shorthand::ai::TensorSpec &spec) {
    if(spec.element_type!=shorthand::ai::ElementType::Float32 || spec.dynamic ||
       spec.shape.empty() || spec.shape.size()>8) return false;
    std::size_t count=1;
    for(auto dim:spec.shape) {
        if(dim<=0 || static_cast<std::uint64_t>(dim)>65536/count) return false;
        count*=static_cast<std::size_t>(dim);
    }
    return count==spec.element_count;
}
bool immutableRange(const float *pointer,std::size_t count) {
    const auto address=reinterpret_cast<std::uintptr_t>(pointer);
    return pointer && address%alignof(float)==0 && count && count<=65536 &&
        count*sizeof(float)<=std::numeric_limits<std::uintptr_t>::max()-address;
}
#endif

shorthand::ai::InferenceResult immutableFailure(shorthand::ai::InferenceStatus status,const std::string &reason) {
    shorthand::ai::InferenceResult result;
    result.status=status; result.reason=reason;
    return result;
}

template <typename Fn>
int lockedInt(Fn &&fn) {
    SHORTHAND_PHASE_SCOPE(facade_lock);
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
    SHORTHAND_PHASE_NEXT(entry_residual);
    return fn();
}

template <typename Fn>
const char *lockedSnapshot(Fn &&fn) {
    thread_local std::string snapshot;
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
    const char *value = fn();
    snapshot = value ? value : "";
    return snapshot.c_str();
}
} // namespace

extern "C" int short_runtime_reset(void) {
    return lockedInt([] {
#if SHORTHAND_IMMUTABLE_ENABLED
        immutable_slot.reset();
#endif
        return shimpl_runtime_reset();
    });
}
extern "C" int short_runtime_model_count(void) {
    return lockedInt([] { return shimpl_runtime_model_count(); });
}
extern "C" int short_runtime_tensor_count(void) {
    return lockedInt([] { return shimpl_runtime_tensor_count(); });
}
extern "C" int short_runtime_contract_count(void) {
    return lockedInt([] { return shimpl_runtime_contract_count(); });
}
extern "C" int short_runtime_measurement_count(void) {
    return lockedInt([] { return shimpl_runtime_measurement_count(); });
}
extern "C" int short_runtime_infer_count(void) {
    return lockedInt([] { return shimpl_runtime_infer_count(); });
}
extern "C" int short_runtime_infer_success_count(void) {
    return lockedInt([] { return shimpl_runtime_infer_success_count(); });
}
extern "C" int short_runtime_infer_not_executed_count(void) {
    return lockedInt([] { return shimpl_runtime_infer_not_executed_count(); });
}
extern "C" int short_runtime_infer_backend_unavailable_count(void) {
    return lockedInt([] { return shimpl_runtime_infer_backend_unavailable_count(); });
}
extern "C" int short_runtime_infer_invalid_input_count(void) {
    return lockedInt([] { return shimpl_runtime_infer_invalid_input_count(); });
}
extern "C" int short_runtime_last_infer_status(void) {
    return lockedInt([] { return shimpl_runtime_last_infer_status(); });
}
extern "C" const char *short_runtime_last_infer_backend(void) {
    return lockedSnapshot([] { return shimpl_runtime_last_infer_backend(); });
}
extern "C" const char *short_runtime_last_infer_reason(void) {
    return lockedSnapshot([] { return shimpl_runtime_last_infer_reason(); });
}
extern "C" const char *short_runtime_last_infer_telemetry_json(void) {
    return lockedSnapshot([] { return shimpl_runtime_last_infer_telemetry_json(); });
}
extern "C" const char *short_runtime_infer_bridge_request_json(void) {
    return lockedSnapshot([] { return shimpl_runtime_infer_bridge_request_json(); });
}
extern "C" const char *short_runtime_observability_json(void) {
    return lockedSnapshot([] { return shimpl_runtime_observability_json(); });
}
extern "C" const char *short_runtime_prometheus_metrics(void) {
    return lockedSnapshot([] { return shimpl_runtime_prometheus_metrics(); });
}
extern "C" const char *short_runtime_otlp_spans_json(void) {
    return lockedSnapshot([] { return shimpl_runtime_otlp_spans_json(); });
}

extern "C" int short_ai_register_model(const char *name,
                                        const char *format,
                                        const char *path,
                                        const char *task,
                                        const char *precision,
                                        const char *input_shape,
                                        const char *output_shape,
                                        const char *backend_preference) {
    return lockedInt([&] {
        return shimpl_ai_register_model(name, format, path, task, precision,
                                        input_shape, output_shape, backend_preference);
    });
}

extern "C" int short_ai_register_tensor(const char *name,
                                         const char *element_type,
                                         const char *shape,
                                         const char *rank,
                                         const char *total_elements) {
    return lockedInt([&] {
        return shimpl_ai_register_tensor(name, element_type, shape, rank, total_elements);
    });
}

extern "C" int short_greenai_register_contract(const char *name,
                                                const char *functional_unit,
                                                const char *success_criteria,
                                                const char *boundary,
                                                const char *measurement_quality,
                                                const char *data_quality,
                                                const char *carbon_factor,
                                                const char *claims_mode) {
    return lockedInt([&] {
        return shimpl_greenai_register_contract(name, functional_unit, success_criteria,
                                                boundary, measurement_quality, data_quality,
                                                carbon_factor, claims_mode);
    });
}

extern "C" int short_greenai_record_measurement(const char *workload,
                                                 const char *backend,
                                                 const char *inferences,
                                                 const char *watts,
                                                 const char *seconds) {
    return lockedInt([&] {
        return shimpl_greenai_record_measurement(workload, backend, inferences, watts, seconds);
    });
}

extern "C" int short_ai_infer(const char *model_name,
                               const char *input_name,
                               const char *output_name) {
    return lockedInt([&] { return shimpl_ai_infer(model_name, input_name, output_name); });
}

extern "C" int short_ai_infer_f32(const char *model_name,
                                   const char *input_name,
                                   const float *input_values,
                                   int input_count,
                                   const char *output_name,
                                   float *output_values,
                                   int output_capacity,
                                   int *output_count) {
    return lockedInt([&] {
        return shimpl_ai_infer_f32(model_name, input_name, input_values, input_count,
                                   output_name, output_values, output_capacity, output_count);
    });
}

extern "C" int short_ai_infer_legacy(const char *model_path,
                                      const char *shape_csv,
                                      const char *input_csv) {
    return lockedInt([&] { return shimpl_ai_infer_legacy(model_path, shape_csv, input_csv); });
}

namespace shorthand::runtime::immutable_v1 {
Binding bind(const ai::ModelSpec &model,const void *bytes,std::size_t size,const std::string &sha256) {
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
#if SHORTHAND_IMMUTABLE_ENABLED
    try {
        if(!bytes || !size || size>16U*1024U*1024U || sha256.size()!=64 ||
           sha256.find_first_not_of("0123456789abcdef")!=std::string::npos)
            return {{},"invalid_immutable_snapshot_or_digest"};
        if(model.format!=ai::ModelFormat::Onnx || model.precision!="float32" ||
           model.allow_fallback || model.backend_preference!=std::vector<ai::BackendKind>{ai::BackendKind::OnnxRuntimeCPU} ||
           !immutableShape(model.input) || !immutableShape(model.output))
            return {{},"immutable_requires_concrete_fp32_cpu_contract"};
        if(immutable_generation==std::numeric_limits<std::uint64_t>::max())
            return {{},"immutable_generation_exhausted"};
        auto candidate=std::make_unique<ImmutableSlot>();
        auto &snapshot=candidate->configuration.model_bytes;
        const auto *begin=static_cast<const unsigned char *>(bytes);
        snapshot.assign(begin,begin+size);
        candidate->digest=crypto::sha256(std::string(reinterpret_cast<const char *>(snapshot.data()),snapshot.size()));
        if(candidate->digest!=sha256) return {{},"immutable_sha256_mismatch"};
        immutable_detail::Envelope(snapshot).validate();
        candidate->model=model;
        // Never resolve a caller-supplied path, even during construction.
        candidate->model.path="immutable-sha256:"+candidate->digest;
        candidate->configuration.maximum_tensor_elements=65536;
        std::string error;
        candidate->session=candidate->runtime.prepare(candidate->model,candidate->configuration,error);
        if(!candidate->session) return {{},error};
        candidate->scratch.resize(model.output.element_count);
        candidate->generation=++immutable_generation;
        const Handle handle{candidate->generation};
        immutable_slot=std::move(candidate);
        return {handle,{}};
    } catch(const std::exception &error) { return {{},error.what()}; }
#else
    (void)model; (void)bytes; (void)size; (void)sha256;
    return {{},"immutable_onnxruntime_unavailable"};
#endif
}

ai::InferenceResult infer(Handle handle,const float *input,std::size_t input_count,
                         float *output,std::size_t output_count) {
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
#if SHORTHAND_IMMUTABLE_ENABLED
    try {
        if(!handle.generation || !immutable_slot || handle.generation!=immutable_slot->generation)
            return immutableFailure(ai::InferenceStatus::InvalidInput,"stale_immutable_handle");
        auto &slot=*immutable_slot;
        if(input_count!=slot.model.input.element_count || output_count!=slot.model.output.element_count ||
           !immutableRange(input,input_count) || !immutableRange(output,output_count))
            return immutableFailure(ai::InferenceStatus::InvalidInput,"invalid_immutable_io");
        ai::TensorBuffer buffer;
        buffer.spec=slot.model.input;
        buffer.bindExternalF32(input,input_count); // Local to this synchronous call.
        slot.runtime.refreshPolicyFromEnvironment();
        auto result=slot.runtime.inferPreparedCPUInto(slot.model,buffer,*slot.session,slot.scratch.data(),output_count);
        if(result.status!=ai::InferenceStatus::Success) return result;
        if(result.output_elements!=output_count ||
           !std::all_of(slot.scratch.begin(),slot.scratch.end(),[](float value){return std::isfinite(value);}))
            return immutableFailure(ai::InferenceStatus::RuntimeError,"invalid_immutable_output");
        result.evidence_json_fragment="{\"schema\":\"shorthand.immutable_model.v1\",\"generation\":"+
            std::to_string(slot.generation)+",\"sha256\":\""+slot.digest+"\"}";
        // All potentially throwing work precedes this transactional commit.
        std::memcpy(output,slot.scratch.data(),output_count*sizeof(float));
        return result;
    } catch(const std::exception &error) {
        return immutableFailure(ai::InferenceStatus::RuntimeError,error.what());
    }
#else
    (void)handle; (void)input; (void)input_count; (void)output; (void)output_count;
    return immutableFailure(ai::InferenceStatus::BackendUnavailable,"immutable_onnxruntime_unavailable");
#endif
}

bool release(Handle handle) {
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
#if SHORTHAND_IMMUTABLE_ENABLED
    if(!handle.generation || !immutable_slot || handle.generation!=immutable_slot->generation) return false;
    immutable_slot.reset();
    return true;
#else
    (void)handle;
    return false;
#endif
}
}
