#include "ShorthandRuntime.h"

#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace {
struct ModelRecord {
    std::string name;
    std::string format;
    std::string path;
    std::string task;
    std::string precision;
    std::string input_shape;
    std::string output_shape;
    std::string backend_preference;
};

struct TensorRecord {
    std::string name;
    std::string element_type;
    std::string shape;
    std::string rank;
    std::string total_elements;
};

struct ContractRecord {
    std::string name;
    std::string functional_unit;
    std::string success_criteria;
    std::string boundary;
    std::string measurement_quality;
    std::string data_quality;
    std::string carbon_factor;
    std::string claims_mode;
};

struct MeasurementRecord {
    std::string workload;
    std::string backend;
    std::string inferences;
    std::string watts;
    std::string seconds;
};

std::map<std::string, ModelRecord> models;
std::map<std::string, TensorRecord> tensors;
std::map<std::string, ContractRecord> contracts;
std::vector<MeasurementRecord> measurements;

const char *safe(const char *value) {
    return value ? value : "";
}

std::string s(const char *value) {
    return std::string(safe(value));
}

bool blank(const char *value) {
    return s(value).empty();
}

void log_status(const char *operation, int status, const std::string &message) {
    std::fprintf(stderr, "[shorthand-runtime] %s status=%d %s\n", operation, status, message.c_str());
}
}

extern "C" int short_runtime_reset(void) {
    models.clear();
    tensors.clear();
    contracts.clear();
    measurements.clear();
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_runtime_model_count(void) {
    return static_cast<int>(models.size());
}

extern "C" int short_runtime_tensor_count(void) {
    return static_cast<int>(tensors.size());
}

extern "C" int short_runtime_contract_count(void) {
    return static_cast<int>(contracts.size());
}

extern "C" int short_runtime_measurement_count(void) {
    return static_cast<int>(measurements.size());
}

extern "C" int short_ai_register_model(const char *name,
                                        const char *format,
                                        const char *path,
                                        const char *task,
                                        const char *precision,
                                        const char *input_shape,
                                        const char *output_shape,
                                        const char *backend_preference) {
    if (blank(name)) {
        log_status("model", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_name");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    ModelRecord record{s(name), s(format), s(path), s(task), s(precision), s(input_shape), s(output_shape), s(backend_preference)};
    models[record.name] = record;
    std::fprintf(stderr,
                 "[shorthand-runtime] model name=%s format=%s path=%s task=%s precision=%s input_shape=%s output_shape=%s backend_preference=%s status=registered\n",
                 record.name.c_str(), record.format.c_str(), record.path.c_str(), record.task.c_str(), record.precision.c_str(), record.input_shape.c_str(), record.output_shape.c_str(), record.backend_preference.c_str());
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_ai_register_tensor(const char *name,
                                         const char *element_type,
                                         const char *shape,
                                         const char *rank,
                                         const char *total_elements) {
    if (blank(name)) {
        log_status("tensor", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_name");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    TensorRecord record{s(name), s(element_type), s(shape), s(rank), s(total_elements)};
    tensors[record.name] = record;
    std::fprintf(stderr,
                 "[shorthand-runtime] tensor name=%s element_type=%s shape=%s rank=%s total_elements=%s status=registered\n",
                 record.name.c_str(), record.element_type.c_str(), record.shape.c_str(), record.rank.c_str(), record.total_elements.c_str());
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_greenai_register_contract(const char *name,
                                                const char *functional_unit,
                                                const char *success_criteria,
                                                const char *boundary,
                                                const char *measurement_quality,
                                                const char *data_quality,
                                                const char *carbon_factor,
                                                const char *claims_mode) {
    if (blank(name)) {
        log_status("greenai_contract", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_name");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    ContractRecord record{s(name), s(functional_unit), s(success_criteria), s(boundary), s(measurement_quality), s(data_quality), s(carbon_factor), s(claims_mode)};
    contracts[record.name] = record;
    std::fprintf(stderr,
                 "[shorthand-runtime] greenai_contract name=%s functional_unit=%s success_criteria=%s boundary=%s measurement_quality=%s data_quality=%s carbon_factor=%s claims_mode=%s status=registered\n",
                 record.name.c_str(), record.functional_unit.c_str(), record.success_criteria.c_str(), record.boundary.c_str(), record.measurement_quality.c_str(), record.data_quality.c_str(), record.carbon_factor.c_str(), record.claims_mode.c_str());
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_greenai_record_measurement(const char *workload,
                                                 const char *backend,
                                                 const char *inferences,
                                                 const char *watts,
                                                 const char *seconds) {
    if (blank(workload)) {
        log_status("greenai_measure", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_workload");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    MeasurementRecord record{s(workload), s(backend), s(inferences), s(watts), s(seconds)};
    measurements.push_back(record);
    std::fprintf(stderr,
                 "[shorthand-runtime] greenai_measure workload=%s backend=%s inferences=%s watts=%s seconds=%s status=recorded\n",
                 record.workload.c_str(), record.backend.c_str(), record.inferences.c_str(), record.watts.c_str(), record.seconds.c_str());
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_ai_infer(const char *model_name,
                               const char *input_name,
                               const char *output_name) {
    if (blank(model_name) || blank(input_name) || blank(output_name)) {
        log_status("infer", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_model_input_or_output");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    const std::string model_key = s(model_name);
    const std::string input_key = s(input_name);
    const std::string output_key = s(output_name);

    auto model_it = models.find(model_key);
    if (model_it == models.end()) {
        log_status("infer", SHORTHAND_RUNTIME_MODEL_NOT_FOUND, "model=" + model_key + " reason=model_not_registered");
        return SHORTHAND_RUNTIME_MODEL_NOT_FOUND;
    }

    if (tensors.find(input_key) == tensors.end()) {
        log_status("infer", SHORTHAND_RUNTIME_TENSOR_NOT_FOUND, "model=" + model_key + " input=" + input_key + " reason=input_tensor_not_registered");
        return SHORTHAND_RUNTIME_TENSOR_NOT_FOUND;
    }

    if (tensors.find(output_key) == tensors.end()) {
        log_status("infer", SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND, "model=" + model_key + " output=" + output_key + " reason=output_tensor_not_registered");
        return SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND;
    }

    std::fprintf(stderr,
                 "[shorthand-runtime] infer model=%s input=%s output=%s status=not_executed backend_preference=%s reason=runtime_bridge_pending\n",
                 model_key.c_str(), input_key.c_str(), output_key.c_str(), model_it->second.backend_preference.c_str());
    return SHORTHAND_RUNTIME_NOT_EXECUTED;
}

extern "C" int short_ai_infer_legacy(const char *model_path,
                                      const char *shape_csv,
                                      const char *input_csv) {
    if (blank(model_path) || blank(shape_csv) || blank(input_csv)) {
        log_status("infer_legacy", SHORTHAND_RUNTIME_INVALID_ARGUMENT, "reason=missing_model_shape_or_input");
        return SHORTHAND_RUNTIME_INVALID_ARGUMENT;
    }

    std::fprintf(stderr,
                 "[shorthand-runtime] infer_legacy model_path=%s shape=%s input=%s status=not_executed reason=legacy_runtime_bridge_pending\n",
                 safe(model_path), safe(shape_csv), safe(input_csv));
    return SHORTHAND_RUNTIME_NOT_EXECUTED;
}
