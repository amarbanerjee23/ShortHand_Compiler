#pragma once

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

int short_runtime_reset(void);
int short_runtime_model_count(void);
int short_runtime_tensor_count(void);
int short_runtime_contract_count(void);
int short_runtime_measurement_count(void);

int short_runtime_infer_count(void);
int short_runtime_infer_success_count(void);
int short_runtime_infer_not_executed_count(void);
int short_runtime_infer_backend_unavailable_count(void);
int short_runtime_infer_invalid_input_count(void);

int short_runtime_last_infer_status(void);
const char *short_runtime_last_infer_backend(void);
const char *short_runtime_last_infer_reason(void);
const char *short_runtime_last_infer_telemetry_json(void);
const char *short_runtime_infer_bridge_request_json(void);
const char *short_runtime_observability_json(void);

int short_ai_register_model(const char *name,
                            const char *format,
                            const char *path,
                            const char *task,
                            const char *precision,
                            const char *input_shape,
                            const char *output_shape,
                            const char *backend_preference);

int short_ai_register_tensor(const char *name,
                             const char *element_type,
                             const char *shape,
                             const char *rank,
                             const char *total_elements);

int short_greenai_register_contract(const char *name,
                                    const char *functional_unit,
                                    const char *success_criteria,
                                    const char *boundary,
                                    const char *measurement_quality,
                                    const char *data_quality,
                                    const char *carbon_factor,
                                    const char *claims_mode);

int short_greenai_record_measurement(const char *workload,
                                     const char *backend,
                                     const char *inferences,
                                     const char *watts,
                                     const char *seconds);

int short_ai_infer(const char *model_name,
                   const char *input_name,
                   const char *output_name);

int short_ai_infer_legacy(const char *model_path,
                          const char *shape_csv,
                          const char *input_csv);

#ifdef __cplusplus
}
#endif
