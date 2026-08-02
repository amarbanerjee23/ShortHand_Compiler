#pragma once

#define SHORTHAND_RUNTIME_ABI_VERSION_MAJOR 1
#define SHORTHAND_RUNTIME_ABI_VERSION_MINOR 0
#define SHORTHAND_RUNTIME_ABI_VERSION_PATCH 0
#define SHORTHAND_RUNTIME_ABI_VERSION_STRING "1.0.0"

#define SHORTHAND_RUNTIME_API_VERSION_MAJOR 1
#define SHORTHAND_RUNTIME_API_VERSION_MINOR 0
#define SHORTHAND_RUNTIME_API_VERSION_PATCH 0
#define SHORTHAND_RUNTIME_API_VERSION_STRING "1.0.0"

#if defined(_WIN32) && defined(SHORTHAND_RUNTIME_SHARED)
#  if defined(SHORTHAND_RUNTIME_BUILDING_LIBRARY)
#    define SHORTHAND_RUNTIME_API __declspec(dllexport)
#  else
#    define SHORTHAND_RUNTIME_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#  define SHORTHAND_RUNTIME_API __attribute__((visibility("default")))
#else
#  define SHORTHAND_RUNTIME_API
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define SHORTHAND_RUNTIME_DEPRECATED(message) __attribute__((deprecated(message)))
#elif defined(_MSC_VER)
#  define SHORTHAND_RUNTIME_DEPRECATED(message) __declspec(deprecated(message))
#else
#  define SHORTHAND_RUNTIME_DEPRECATED(message)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum ShortHandRuntimeStatus {
    SHORTHAND_RUNTIME_OK = 0,
    SHORTHAND_RUNTIME_INVALID_ARGUMENT = 1,
    SHORTHAND_RUNTIME_MODEL_NOT_FOUND = 2,
    SHORTHAND_RUNTIME_TENSOR_NOT_FOUND = 3,
    SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND = 4,
    SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE = 5,
    SHORTHAND_RUNTIME_NOT_EXECUTED = 6,
    SHORTHAND_RUNTIME_INVALID_INPUT = 7,
    SHORTHAND_RUNTIME_RUNTIME_ERROR = 8
};

/* Header-level negotiation helpers do not add external ABI symbols. */
static inline const char *short_runtime_abi_version(void) {
    return SHORTHAND_RUNTIME_ABI_VERSION_STRING;
}

static inline const char *short_runtime_api_version(void) {
    return SHORTHAND_RUNTIME_API_VERSION_STRING;
}

static inline int short_runtime_is_abi_compatible(int requested_major, int requested_minor) {
    return requested_major == SHORTHAND_RUNTIME_ABI_VERSION_MAJOR &&
           requested_minor >= 0 &&
           requested_minor <= SHORTHAND_RUNTIME_ABI_VERSION_MINOR;
}

static inline int short_runtime_is_api_compatible(int requested_major, int requested_minor) {
    return requested_major == SHORTHAND_RUNTIME_API_VERSION_MAJOR &&
           requested_minor >= 0 &&
           requested_minor <= SHORTHAND_RUNTIME_API_VERSION_MINOR;
}

SHORTHAND_RUNTIME_API int short_runtime_reset(void);
SHORTHAND_RUNTIME_API int short_runtime_model_count(void);
SHORTHAND_RUNTIME_API int short_runtime_tensor_count(void);
SHORTHAND_RUNTIME_API int short_runtime_contract_count(void);
SHORTHAND_RUNTIME_API int short_runtime_measurement_count(void);

SHORTHAND_RUNTIME_API int short_runtime_infer_count(void);
SHORTHAND_RUNTIME_API int short_runtime_infer_success_count(void);
SHORTHAND_RUNTIME_API int short_runtime_infer_not_executed_count(void);
SHORTHAND_RUNTIME_API int short_runtime_infer_backend_unavailable_count(void);
SHORTHAND_RUNTIME_API int short_runtime_infer_invalid_input_count(void);

SHORTHAND_RUNTIME_API int short_runtime_last_infer_status(void);
SHORTHAND_RUNTIME_API const char *short_runtime_last_infer_backend(void);
SHORTHAND_RUNTIME_API const char *short_runtime_last_infer_reason(void);
SHORTHAND_RUNTIME_API const char *short_runtime_last_infer_telemetry_json(void);
SHORTHAND_RUNTIME_API const char *short_runtime_infer_bridge_request_json(void);
SHORTHAND_RUNTIME_API const char *short_runtime_observability_json(void);
SHORTHAND_RUNTIME_API const char *short_runtime_prometheus_metrics(void);
SHORTHAND_RUNTIME_API const char *short_runtime_otlp_spans_json(void);

SHORTHAND_RUNTIME_API int short_ai_register_model(const char *name,
                                                   const char *format,
                                                   const char *path,
                                                   const char *task,
                                                   const char *precision,
                                                   const char *input_shape,
                                                   const char *output_shape,
                                                   const char *backend_preference);

SHORTHAND_RUNTIME_API int short_ai_register_tensor(const char *name,
                                                    const char *element_type,
                                                    const char *shape,
                                                    const char *rank,
                                                    const char *total_elements);

SHORTHAND_RUNTIME_API int short_greenai_register_contract(const char *name,
                                                           const char *functional_unit,
                                                           const char *success_criteria,
                                                           const char *boundary,
                                                           const char *measurement_quality,
                                                           const char *data_quality,
                                                           const char *carbon_factor,
                                                           const char *claims_mode);

SHORTHAND_RUNTIME_API int short_greenai_record_measurement(const char *workload,
                                                            const char *backend,
                                                            const char *inferences,
                                                            const char *watts,
                                                            const char *seconds);

SHORTHAND_RUNTIME_API int short_ai_infer(const char *model_name,
                                         const char *input_name,
                                         const char *output_name);

SHORTHAND_RUNTIME_API int short_ai_infer_f32(const char *model_name,
                                              const char *input_name,
                                              const float *input_values,
                                              int input_count,
                                              const char *output_name,
                                              float *output_values,
                                              int output_capacity,
                                              int *output_count);

SHORTHAND_RUNTIME_API int short_ai_infer_legacy(const char *model_path,
                                                 const char *shape_csv,
                                                 const char *input_csv);

#ifdef __cplusplus
}
#endif
