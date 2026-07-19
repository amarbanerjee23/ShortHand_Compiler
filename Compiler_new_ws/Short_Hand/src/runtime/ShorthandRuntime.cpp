#include "ShorthandRuntime.h"

#include "../ai_runtime/AI_Backend.h"
#include "../ai_runtime/AI_Types.h"
#include "../ai_runtime/backends/FallbackBackend.h"
#include "../ai_runtime/backends/LibTorchBackend.h"
#include "../ai_runtime/backends/LlamaCppBackend.h"
#include "../ai_runtime/backends/OnnxRuntimeBackend.h"
#include "../ai_runtime/backends/OpenVINOBackend.h"
#include "../ai_runtime/backends/TensorRTBackend.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>
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

struct RuntimeStats {
    int infer_calls = 0;
    int infer_success = 0;
    int infer_not_executed = 0;
    int infer_backend_unavailable = 0;
    int infer_invalid_input = 0;
};

std::map<std::string, ModelRecord> models;
std::map<std::string, TensorRecord> tensors;
std::map<std::string, ContractRecord> contracts;
std::vector<MeasurementRecord> measurements;
RuntimeStats stats;

int last_infer_status = SHORTHAND_RUNTIME_NOT_EXECUTED;
std::string last_infer_backend = "none";
std::string last_infer_reason = "not_run";
std::string last_infer_telemetry_json = "{}";
std::string observability_json_cache = "{}";

const char *safe(const char *value) {
    return value ? value : "";
}

std::string s(const char *value) {
    return std::string(safe(value));
}

bool blank(const char *value) {
    return s(value).empty();
}

std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string json_escape(const std::string &value) {
    std::string out;
    for (char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::vector<std::string> split_csv(const std::string &csv) {
    std::vector<std::string> out;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) out.push_back(token);
    }
    return out;
}

std::vector<shorthand::ai::BackendKind> parse_backend_preference(const std::string &csv) {
    std::vector<shorthand::ai::BackendKind> out;
    for (const auto &token : split_csv(csv)) {
        out.push_back(shorthand::ai::parseBackendKind(token));
    }
    if (out.empty()) out.push_back(shorthand::ai::BackendKind::Fallback);
    return out;
}

shorthand::ai::BackendRegistry make_registry() {
    shorthand::ai::BackendRegistry registry;
    registry.registerBackend(std::make_unique<shorthand::ai::TensorRTBackend>());
    registry.registerBackend(std::make_unique<shorthand::ai::OnnxRuntimeBackend>());
    registry.registerBackend(std::make_unique<shorthand::ai::OpenVINOBackend>());
    registry.registerBackend(std::make_unique<shorthand::ai::LibTorchBackend>());
    registry.registerBackend(std::make_unique<shorthand::ai::LlamaCppBackend>());
    registry.registerBackend(std::make_unique<shorthand::ai::FallbackBackend>());
    return registry;
}

shorthand::ai::TensorSpec tensor_spec_from_record(const TensorRecord &record) {
    shorthand::ai::TensorSpec spec;
    spec.name = record.name;
    spec.element_type = shorthand::ai::parseElementType(record.element_type);
    spec.shape = shorthand::ai::parseShapeCsv(record.shape);
    spec.element_count = shorthand::ai::productOfShape(spec.shape);
    return spec;
}

shorthand::ai::TensorBuffer tensor_buffer_from_record(const TensorRecord &record) {
    shorthand::ai::TensorBuffer buffer;
    buffer.spec = tensor_spec_from_record(record);
    if (buffer.spec.element_count > 0 && buffer.spec.element_count <= 1000000) {
        if (buffer.spec.element_type == shorthand::ai::ElementType::Int8 ||
            buffer.spec.element_type == shorthand::ai::ElementType::Int4) {
            buffer.i8_data.assign(buffer.spec.element_count, 0);
        } else {
            buffer.f32_data.assign(buffer.spec.element_count, 0.0f);
        }
    }
    return buffer;
}

shorthand::ai::ModelSpec model_spec_from_record(const ModelRecord &model_record,
                                                const TensorRecord &input_record,
                                                const TensorRecord &output_record) {
    shorthand::ai::ModelSpec model;
    model.name = model_record.name;
    model.path = model_record.path;
    model.format = shorthand::ai::parseModelFormat(model_record.format);
    model.task = model_record.task;
    model.precision = model_record.precision;
    model.input = tensor_spec_from_record(input_record);
    model.output = tensor_spec_from_record(output_record);
    model.backend_preference = parse_backend_preference(model_record.backend_preference);
    model.allow_fallback = true;
    return model;
}

int map_inference_status(shorthand::ai::InferenceStatus status) {
    switch (status) {
        case shorthand::ai::InferenceStatus::Success:
            return SHORTHAND_RUNTIME_OK;
        case shorthand::ai::InferenceStatus::NotExecuted:
            return SHORTHAND_RUNTIME_NOT_EXECUTED;
        case shorthand::ai::InferenceStatus::BackendUnavailable:
            return SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE;
        case shorthand::ai::InferenceStatus::InvalidInput:
            return SHORTHAND_RUNTIME_INVALID_INPUT;
        case shorthand::ai::InferenceStatus::RuntimeError:
            return SHORTHAND_RUNTIME_RUNTIME_ERROR;
    }
    return SHORTHAND_RUNTIME_RUNTIME_ERROR;
}

void update_stats(int status) {
    stats.infer_calls++;
    if (status == SHORTHAND_RUNTIME_OK) stats.infer_success++;
    else if (status == SHORTHAND_RUNTIME_NOT_EXECUTED) stats.infer_not_executed++;
    else if (status == SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE) stats.infer_backend_unavailable++;
    else if (status == SHORTHAND_RUNTIME_INVALID_INPUT || status == SHORTHAND_RUNTIME_INVALID_ARGUMENT) stats.infer_invalid_input++;
}

void set_last_infer(int status, const std::string &backend, const std::string &reason, const std::string &telemetry) {
    last_infer_status = status;
    last_infer_backend = backend.empty() ? "none" : backend;
    last_infer_reason = reason.empty() ? "unspecified" : reason;
    last_infer_telemetry_json = telemetry.empty()
        ? std::string("{\"schema\":\"shorthand.runtime.infer_telemetry.v1\",\"backend\":\"") + json_escape(last_infer_backend) + "\",\"reason\":\"" + json_escape(last_infer_reason) + "\"}"
        : telemetry;
    update_stats(status);
}

std::string status_name(int status) {
    switch (status) {
        case SHORTHAND_RUNTIME_OK: return "success";
        case SHORTHAND_RUNTIME_INVALID_ARGUMENT: return "invalid_argument";
        case SHORTHAND_RUNTIME_MODEL_NOT_FOUND: return "model_not_found";
        case SHORTHAND_RUNTIME_TENSOR_NOT_FOUND: return "tensor_not_found";
        case SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND: return "output_tensor_not_found";
        case SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE: return "backend_unavailable";
        case SHORTHAND_RUNTIME_NOT_EXECUTED: return "not_executed";
        case SHORTHAND_RUNTIME_INVALID_INPUT: return "invalid_input";
        case SHORTHAND_RUNTIME_RUNTIME_ERROR: return "runtime_error";
    }
    return "runtime_error";
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
    stats = RuntimeStats{};
    last_infer_status = SHORTHAND_RUNTIME_NOT_EXECUTED;
    last_infer_backend = "none";
    last_infer_reason = "not_run";
    last_infer_telemetry_json = "{}";
    observability_json_cache = "{}";
    return SHORTHAND_RUNTIME_OK;
}

extern "C" int short_runtime_model_count(void) { return static_cast<int>(models.size()); }
extern "C" int short_runtime_tensor_count(void) { return static_cast<int>(tensors.size()); }
extern "C" int short_runtime_contract_count(void) { return static_cast<int>(contracts.size()); }
extern "C" int short_runtime_measurement_count(void) { return static_cast<int>(measurements.size()); }
extern "C" int short_runtime_infer_count(void) { return stats.infer_calls; }
extern "C" int short_runtime_infer_success_count(void) { return stats.infer_success; }
extern "C" int short_runtime_infer_not_executed_count(void) { return stats.infer_not_executed; }
extern "C" int short_runtime_infer_backend_unavailable_count(void) { return stats.infer_backend_unavailable; }
extern "C" int short_runtime_infer_invalid_input_count(void) { return stats.infer_invalid_input; }
extern "C" int short_runtime_last_infer_status(void) { return last_infer_status; }
extern "C" const char *short_runtime_last_infer_backend(void) { return last_infer_backend.c_str(); }
extern "C" const char *short_runtime_last_infer_reason(void) { return last_infer_reason.c_str(); }
extern "C" const char *short_runtime_last_infer_telemetry_json(void) { return last_infer_telemetry_json.c_str(); }

extern "C" const char *short_runtime_observability_json(void) {
    std::ostringstream out;
    out << "{\"schema\":\"shorthand.runtime.observability.v1\""
        << ",\"models\":" << models.size()
        << ",\"tensors\":" << tensors.size()
        << ",\"contracts\":" << contracts.size()
        << ",\"measurements\":" << measurements.size()
        << ",\"infer_calls\":" << stats.infer_calls
        << ",\"infer_success\":" << stats.infer_success
        << ",\"infer_not_executed\":" << stats.infer_not_executed
        << ",\"infer_backend_unavailable\":" << stats.infer_backend_unavailable
        << ",\"infer_invalid_input\":" << stats.infer_invalid_input
        << ",\"last_status\":\"" << json_escape(status_name(last_infer_status)) << "\""
        << ",\"last_backend\":\"" << json_escape(last_infer_backend) << "\""
        << ",\"last_reason\":\"" << json_escape(last_infer_reason) << "\"}"
        ;
    observability_json_cache = out.str();
    return observability_json_cache.c_str();
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
        const int status = SHORTHAND_RUNTIME_INVALID_ARGUMENT;
        set_last_infer(status, "none", "missing_model_input_or_output", "");
        log_status("infer", status, "reason=missing_model_input_or_output");
        return status;
    }

    const std::string model_key = s(model_name);
    const std::string input_key = s(input_name);
    const std::string output_key = s(output_name);

    auto model_it = models.find(model_key);
    if (model_it == models.end()) {
        const int status = SHORTHAND_RUNTIME_MODEL_NOT_FOUND;
        set_last_infer(status, "none", "model_not_registered", "");
        log_status("infer", status, "model=" + model_key + " reason=model_not_registered");
        return status;
    }

    auto input_it = tensors.find(input_key);
    if (input_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "input_tensor_not_registered", "");
        log_status("infer", status, "model=" + model_key + " input=" + input_key + " reason=input_tensor_not_registered");
        return status;
    }

    auto output_it = tensors.find(output_key);
    if (output_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "output_tensor_not_registered", "");
        log_status("infer", status, "model=" + model_key + " output=" + output_key + " reason=output_tensor_not_registered");
        return status;
    }

    auto model = model_spec_from_record(model_it->second, input_it->second, output_it->second);
    auto input = tensor_buffer_from_record(input_it->second);
    auto registry = make_registry();
    auto *backend = registry.select(model);
    if (!backend) {
        const int status = SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE;
        set_last_infer(status, "none", "no_compatible_backend", "");
        std::fprintf(stderr,
                     "[shorthand-runtime] infer model=%s input=%s output=%s status=backend_unavailable backend_preference=%s reason=no_compatible_backend\n",
                     model_key.c_str(), input_key.c_str(), output_key.c_str(), model_it->second.backend_preference.c_str());
        return status;
    }

    auto result = backend->infer(model, input);
    if (backend->kind() == shorthand::ai::BackendKind::Fallback) {
        result.status = shorthand::ai::InferenceStatus::NotExecuted;
        result.backend = shorthand::ai::BackendKind::Fallback;
        result.backend_name = "fallback";
        result.provider_name = "none";
        result.reason = "backend_not_available";
        result.output_f32.clear();
    }

    const int status = map_inference_status(result.status);
    const std::string backend_name = result.backend_name.empty() ? backend->name() : result.backend_name;
    set_last_infer(status, backend_name, result.reason, result.telemetry_json_fragment);

    std::fprintf(stderr,
                 "[shorthand-runtime] infer model=%s input=%s output=%s status=%s backend=%s backend_preference=%s reason=%s\n",
                 model_key.c_str(), input_key.c_str(), output_key.c_str(), status_name(status).c_str(), backend_name.c_str(), model_it->second.backend_preference.c_str(), last_infer_reason.c_str());
    return status;
}

extern "C" int short_ai_infer_legacy(const char *model_path,
                                      const char *shape_csv,
                                      const char *input_csv) {
    if (blank(model_path) || blank(shape_csv) || blank(input_csv)) {
        const int status = SHORTHAND_RUNTIME_INVALID_ARGUMENT;
        set_last_infer(status, "none", "missing_model_shape_or_input", "");
        log_status("infer_legacy", status, "reason=missing_model_shape_or_input");
        return status;
    }

    const int status = SHORTHAND_RUNTIME_NOT_EXECUTED;
    set_last_infer(status, "legacy", "legacy_bridge_records_intent_only", "");
    std::fprintf(stderr,
                 "[shorthand-runtime] infer_legacy model_path=%s shape=%s input=%s status=not_executed reason=legacy_bridge_records_intent_only\n",
                 safe(model_path), safe(shape_csv), safe(input_csv));
    return status;
}
