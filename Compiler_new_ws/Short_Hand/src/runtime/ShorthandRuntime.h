#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
