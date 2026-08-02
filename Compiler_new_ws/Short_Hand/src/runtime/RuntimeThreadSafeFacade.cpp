#include "ShorthandRuntime.h"

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

template <typename Fn>
int lockedInt(Fn &&fn) {
    std::lock_guard<std::recursive_mutex> guard(runtimeMutex());
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
    return lockedInt([] { return shimpl_runtime_reset(); });
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
