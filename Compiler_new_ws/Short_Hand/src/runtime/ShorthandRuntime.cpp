#include "ShorthandRuntime.h"

#include <cstdio>

namespace {
const char *safe(const char *value) {
    return value ? value : "";
}
}

extern "C" int short_ai_register_model(const char *name,
                                        const char *format,
                                        const char *path,
                                        const char *task,
                                        const char *precision,
                                        const char *input_shape,
                                        const char *output_shape,
                                        const char *backend_preference) {
    std::fprintf(stderr,
                 "[shorthand-runtime] model name=%s format=%s path=%s task=%s precision=%s input_shape=%s output_shape=%s backend_preference=%s\n",
                 safe(name), safe(format), safe(path), safe(task), safe(precision), safe(input_shape), safe(output_shape), safe(backend_preference));
    return 0;
}

extern "C" int short_ai_register_tensor(const char *name,
                                         const char *element_type,
                                         const char *shape,
                                         const char *rank,
                                         const char *total_elements) {
    std::fprintf(stderr,
                 "[shorthand-runtime] tensor name=%s element_type=%s shape=%s rank=%s total_elements=%s\n",
                 safe(name), safe(element_type), safe(shape), safe(rank), safe(total_elements));
    return 0;
}

extern "C" int short_greenai_register_contract(const char *name,
                                                const char *functional_unit,
                                                const char *success_criteria,
                                                const char *boundary,
                                                const char *measurement_quality,
                                                const char *data_quality,
                                                const char *carbon_factor,
                                                const char *claims_mode) {
    std::fprintf(stderr,
                 "[shorthand-runtime] greenai_contract name=%s functional_unit=%s success_criteria=%s boundary=%s measurement_quality=%s data_quality=%s carbon_factor=%s claims_mode=%s\n",
                 safe(name), safe(functional_unit), safe(success_criteria), safe(boundary), safe(measurement_quality), safe(data_quality), safe(carbon_factor), safe(claims_mode));
    return 0;
}

extern "C" int short_greenai_record_measurement(const char *workload,
                                                 const char *backend,
                                                 const char *inferences,
                                                 const char *watts,
                                                 const char *seconds) {
    std::fprintf(stderr,
                 "[shorthand-runtime] greenai_measure workload=%s backend=%s inferences=%s watts=%s seconds=%s\n",
                 safe(workload), safe(backend), safe(inferences), safe(watts), safe(seconds));
    return 0;
}

extern "C" int short_ai_infer(const char *model_name,
                               const char *input_name,
                               const char *output_name) {
    std::fprintf(stderr,
                 "[shorthand-runtime] infer model=%s input=%s output=%s\n",
                 safe(model_name), safe(input_name), safe(output_name));
    return 0;
}

extern "C" int short_ai_infer_legacy(const char *model_path,
                                      const char *shape_csv,
                                      const char *input_csv) {
    std::fprintf(stderr,
                 "[shorthand-runtime] infer_legacy model_path=%s shape=%s input=%s\n",
                 safe(model_path), safe(shape_csv), safe(input_csv));
    return 0;
}
