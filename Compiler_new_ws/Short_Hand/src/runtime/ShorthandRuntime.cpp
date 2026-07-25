#include "ShorthandRuntime.h"

#ifndef SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE
#define SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE 0
#endif

#if SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE
#include "AIRuntimeBridgeAdapter.h"
#include "ai_runtime/AI_Runtime.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <map>
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
std::string infer_bridge_request_json_cache = "{}";
std::string observability_json_cache = "{}";

const char *safe(const char *value) { return value ? value : ""; }
std::string s(const char *value) { return std::string(safe(value)); }
bool blank(const char *value) { return s(value).empty(); }

std::string json_escape(const std::string &value) {
    std::string out;
    for (char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::vector<long long> parse_shape(const std::string &csv) {
    std::vector<long long> out;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        char *end = nullptr;
        long long value = std::strtoll(token.c_str(), &end, 10);
        if (end == token.c_str() || value <= 0) return {};
        out.push_back(value);
    }
    return out;
}

bool shape_is_valid(const std::string &csv) {
    return !parse_shape(csv).empty();
}

long long shape_product(const std::vector<long long> &shape) {
    if (shape.empty()) return 0;
    long long product = 1;
    for (long long dim : shape) {
        if (dim <= 0) return 0;
        product *= dim;
    }
    return product;
}

long long declared_element_count(const TensorRecord &tensor) {
    if (!tensor.total_elements.empty()) {
        char *end = nullptr;
        long long value = std::strtoll(tensor.total_elements.c_str(), &end, 10);
        if (end != tensor.total_elements.c_str() && value > 0) return value;
    }
    return shape_product(parse_shape(tensor.shape));
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

void update_stats(int status) {
    stats.infer_calls++;
    if (status == SHORTHAND_RUNTIME_OK) stats.infer_success++;
    else if (status == SHORTHAND_RUNTIME_NOT_EXECUTED) stats.infer_not_executed++;
    else if (status == SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE) stats.infer_backend_unavailable++;
    else if (status == SHORTHAND_RUNTIME_INVALID_INPUT || status == SHORTHAND_RUNTIME_INVALID_ARGUMENT) stats.infer_invalid_input++;
}

void set_last_infer(int status, const std::string &backend, const std::string &reason) {
    last_infer_status = status;
    last_infer_backend = backend.empty() ? "none" : backend;
    last_infer_reason = reason.empty() ? "unspecified" : reason;
    infer_bridge_request_json_cache = "{}";
    last_infer_telemetry_json = std::string("{\"schema\":\"shorthand.runtime.infer_telemetry.v1\",\"status\":\"") +
        json_escape(status_name(status)) + "\",\"backend\":\"" + json_escape(last_infer_backend) +
        "\",\"reason\":\"" + json_escape(last_infer_reason) + "\"}";
    update_stats(status);
}

std::string build_compiled_infer_bridge_request(const ModelRecord &model,
                                                const TensorRecord &input,
                                                const TensorRecord &output,
                                                const std::string &backend,
                                                int status,
                                                const std::string &reason) {
    std::ostringstream out;
    out << "{\"schema\":\"shorthand.runtime.compiled_infer_bridge_request.v1\""
        << ",\"status\":\"" << json_escape(status_name(status)) << "\""
        << ",\"bridge_status\":\"candidate_request_only\""
        << ",\"execution_ready\":false"
        << ",\"reason\":\"" << json_escape(reason) << "\""
        << ",\"runtime_target\":\"AI_Runtime\""
        << ",\"input_buffer_required\":true"
        << ",\"model\":{\"name\":\"" << json_escape(model.name)
        << "\",\"format\":\"" << json_escape(model.format)
        << "\",\"path\":\"" << json_escape(model.path)
        << "\",\"task\":\"" << json_escape(model.task)
        << "\",\"precision\":\"" << json_escape(model.precision)
        << "\",\"input_shape\":\"" << json_escape(model.input_shape)
        << "\",\"output_shape\":\"" << json_escape(model.output_shape)
        << "\",\"backend_preference\":\"" << json_escape(backend) << "\"}"
        << ",\"input\":{\"name\":\"" << json_escape(input.name)
        << "\",\"element_type\":\"" << json_escape(input.element_type)
        << "\",\"shape\":\"" << json_escape(input.shape)
        << "\",\"rank\":\"" << json_escape(input.rank)
        << "\",\"total_elements\":\"" << json_escape(input.total_elements) << "\"}"
        << ",\"output\":{\"name\":\"" << json_escape(output.name)
        << "\",\"element_type\":\"" << json_escape(output.element_type)
        << "\",\"shape\":\"" << json_escape(output.shape)
        << "\",\"rank\":\"" << json_escape(output.rank)
        << "\",\"total_elements\":\"" << json_escape(output.total_elements) << "\"}}";
    return out.str();
}

std::string build_typed_buffer_bridge_request(const ModelRecord &model,
                                              const TensorRecord &input,
                                              const TensorRecord &output,
                                              const std::string &backend,
                                              int status,
                                              const std::string &reason,
                                              int input_count,
                                              int output_capacity,
                                              long long expected_input_elements,
                                              long long expected_output_elements,
                                              int output_count) {
    const bool pending = reason == "ai_runtime_typed_buffer_bridge_pending";
    const bool rejected = reason == "ai_runtime_adapter_request_not_execution_ready";
    const bool execution_ready = !pending && !rejected;
    const char *bridge_status = pending
        ? "typed_buffer_received_execution_pending"
        : (rejected ? "typed_buffer_received_execution_rejected" : (status == SHORTHAND_RUNTIME_OK ? "ai_runtime_execution_succeeded" : "ai_runtime_execution_attempted"));

    std::ostringstream out;
    out << "{\"schema\":\"shorthand.runtime.typed_infer_buffer_bridge_request.v1\""
        << ",\"parent_schema\":\"shorthand.runtime.compiled_infer_bridge_request.v1\""
        << ",\"status\":\"" << json_escape(status_name(status)) << "\""
        << ",\"bridge_status\":\"" << bridge_status << "\""
        << ",\"execution_ready\":" << (execution_ready ? "true" : "false")
        << ",\"runtime_target\":\"AI_Runtime\""
        << ",\"input_buffer_available\":true"
        << ",\"input_element_count\":" << input_count
        << ",\"expected_input_elements\":" << expected_input_elements
        << ",\"output_capacity\":" << output_capacity
        << ",\"expected_output_elements\":" << expected_output_elements
        << ",\"output_count\":" << output_count
        << ",\"reason\":\"" << json_escape(reason) << "\""
        << ",\"model\":{\"name\":\"" << json_escape(model.name)
        << "\",\"format\":\"" << json_escape(model.format)
        << "\",\"path\":\"" << json_escape(model.path)
        << "\",\"task\":\"" << json_escape(model.task)
        << "\",\"precision\":\"" << json_escape(model.precision)
        << "\",\"backend_preference\":\"" << json_escape(backend) << "\"}"
        << ",\"input\":{\"name\":\"" << json_escape(input.name)
        << "\",\"element_type\":\"" << json_escape(input.element_type)
        << "\",\"shape\":\"" << json_escape(input.shape) << "\"}"
        << ",\"output\":{\"name\":\"" << json_escape(output.name)
        << "\",\"element_type\":\"" << json_escape(output.element_type)
        << "\",\"shape\":\"" << json_escape(output.shape) << "\"}}";
    return out.str();
}

#if SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE
shorthand::runtime_bridge::RuntimeBridgeModelInput bridge_model_input(const ModelRecord &model) {
    return {model.name.c_str(), model.format.c_str(), model.path.c_str(), model.task.c_str(), model.precision.c_str(), model.input_shape.c_str(), model.output_shape.c_str(), model.backend_preference.c_str(), true};
}

shorthand::runtime_bridge::RuntimeBridgeTensorInput bridge_tensor_input(const TensorRecord &tensor) {
    return {tensor.name.c_str(), tensor.element_type.c_str(), tensor.shape.c_str(), tensor.rank.c_str(), tensor.total_elements.c_str()};
}

void attach_ai_runtime_telemetry_if_available(int status,
                                              const std::string &backend,
                                              const std::string &reason,
                                              const shorthand::ai::InferenceResult &result) {
    if (result.telemetry_json_fragment.empty()) return;
    last_infer_telemetry_json = std::string("{\"schema\":\"shorthand.runtime.infer_telemetry.v1\",\"status\":\"") +
        json_escape(status_name(status)) + "\",\"backend\":\"" + json_escape(backend) +
        "\",\"reason\":\"" + json_escape(reason) + "\",\"ai_runtime_telemetry\":" + result.telemetry_json_fragment + "}";
}

int execute_typed_buffer_through_ai_runtime(const ModelRecord &model,
                                            const TensorRecord &input,
                                            const TensorRecord &output,
                                            const float *input_values,
                                            int input_count,
                                            float *output_values,
                                            int output_capacity,
                                            int *output_count,
                                            long long expected_input,
                                            long long expected_output) {
    const auto bridge_model = bridge_model_input(model);
    const auto bridge_input = bridge_tensor_input(input);
    const auto bridge_output = bridge_tensor_input(output);

    auto model_spec = shorthand::runtime_bridge::buildModelSpec(bridge_model, bridge_input, bridge_output);
    auto input_buffer = shorthand::runtime_bridge::buildInputTensorBuffer(bridge_input, input_values, input_count);
    const std::string requested_backend = model.backend_preference.empty() ? "fallback" : model.backend_preference;

    if (!shorthand::runtime_bridge::bridgeRequestIsExecutionReady(model_spec, input_buffer, output_capacity)) {
        const int status = SHORTHAND_RUNTIME_INVALID_INPUT;
        const std::string reason = "ai_runtime_adapter_request_not_execution_ready";
        set_last_infer(status, requested_backend, reason);
        *output_count = 0;
        infer_bridge_request_json_cache = build_typed_buffer_bridge_request(model, input, output, requested_backend, status, reason, input_count, output_capacity, expected_input, expected_output, *output_count);
        std::fprintf(stderr,
                     "[shorthand-runtime] infer_f32 model=%s input=%s output=%s status=invalid_input backend_preference=%s reason=%s ai_runtime_bridge=rejected\n",
                     model.name.c_str(), input.name.c_str(), output.name.c_str(), requested_backend.c_str(), reason.c_str());
        return status;
    }

    shorthand::ai::AIRuntime runtime;
    auto result = runtime.infer(model_spec, input_buffer);
    int status = shorthand::runtime_bridge::runtimeStatusFromInferenceStatus(result.status);
    std::string backend = result.backend_name.empty() ? shorthand::ai::backendKindToString(result.backend) : result.backend_name;
    std::string reason = result.reason.empty() ? shorthand::ai::inferenceStatusToString(result.status) : result.reason;

    *output_count = 0;
    if (status == SHORTHAND_RUNTIME_OK) {
        if (result.output_f32.size() > static_cast<std::size_t>(output_capacity)) {
            status = SHORTHAND_RUNTIME_RUNTIME_ERROR;
            reason = "ai_runtime_output_exceeds_registered_capacity";
        } else {
            for (std::size_t i = 0; i < result.output_f32.size(); ++i) {
                output_values[i] = result.output_f32[i];
            }
            *output_count = static_cast<int>(result.output_f32.size());
        }
    }

    set_last_infer(status, backend, reason);
    attach_ai_runtime_telemetry_if_available(status, backend, reason, result);
    infer_bridge_request_json_cache = build_typed_buffer_bridge_request(model, input, output, backend, status, reason, input_count, output_capacity, expected_input, expected_output, *output_count);
    std::fprintf(stderr,
                 "[shorthand-runtime] infer_f32 model=%s input=%s output=%s status=%s backend=%s reason=%s ai_runtime_bridge=attempted output_count=%d\n",
                 model.name.c_str(), input.name.c_str(), output.name.c_str(), status_name(status).c_str(), backend.c_str(), reason.c_str(), *output_count);
    return status;
}
#endif

void log_status(const char *operation, int status, const std::string &message) {
    std::fprintf(stderr, "[shorthand-runtime] %s status=%d %s\n", operation, status, message.c_str());
}
} // namespace

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
    infer_bridge_request_json_cache = "{}";
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
extern "C" const char *short_runtime_infer_bridge_request_json(void) { return infer_bridge_request_json_cache.c_str(); }

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
        << ",\"last_reason\":\"" << json_escape(last_infer_reason) << "\""
        << ",\"infer_bridge_request\":" << infer_bridge_request_json_cache << "}";
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
        set_last_infer(status, "none", "missing_model_input_or_output");
        log_status("infer", status, "reason=missing_model_input_or_output");
        return status;
    }

    const std::string model_key = s(model_name);
    const std::string input_key = s(input_name);
    const std::string output_key = s(output_name);

    auto model_it = models.find(model_key);
    if (model_it == models.end()) {
        const int status = SHORTHAND_RUNTIME_MODEL_NOT_FOUND;
        set_last_infer(status, "none", "model_not_registered");
        log_status("infer", status, "model=" + model_key + " reason=model_not_registered");
        return status;
    }

    auto input_it = tensors.find(input_key);
    if (input_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "input_tensor_not_registered");
        log_status("infer", status, "model=" + model_key + " input=" + input_key + " reason=input_tensor_not_registered");
        return status;
    }

    auto output_it = tensors.find(output_key);
    if (output_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "output_tensor_not_registered");
        log_status("infer", status, "model=" + model_key + " output=" + output_key + " reason=output_tensor_not_registered");
        return status;
    }

    if (!shape_is_valid(input_it->second.shape) || !shape_is_valid(output_it->second.shape)) {
        const int status = SHORTHAND_RUNTIME_INVALID_INPUT;
        set_last_infer(status, "none", "registered_tensor_shape_invalid");
        std::fprintf(stderr,
                     "[shorthand-runtime] infer model=%s input=%s output=%s status=invalid_input reason=registered_tensor_shape_invalid\n",
                     model_key.c_str(), input_key.c_str(), output_key.c_str());
        return status;
    }

    const int status = SHORTHAND_RUNTIME_NOT_EXECUTED;
    const std::string backend = model_it->second.backend_preference.empty() ? "fallback" : model_it->second.backend_preference;
    const std::string reason = "ai_runtime_execution_bridge_pending";
    set_last_infer(status, backend, reason);
    infer_bridge_request_json_cache = build_compiled_infer_bridge_request(model_it->second,
                                                                          input_it->second,
                                                                          output_it->second,
                                                                          backend,
                                                                          status,
                                                                          "input_buffer_required_for_ai_runtime_execution");
    std::fprintf(stderr,
                 "[shorthand-runtime] infer model=%s input=%s output=%s status=not_executed backend_preference=%s reason=%s bridge_request=created\n",
                 model_key.c_str(), input_key.c_str(), output_key.c_str(), backend.c_str(), reason.c_str());
    return status;
}

extern "C" int short_ai_infer_f32(const char *model_name,
                                  const char *input_name,
                                  const float *input_values,
                                  int input_count,
                                  const char *output_name,
                                  float *output_values,
                                  int output_capacity,
                                  int *output_count) {
    if (output_count) *output_count = 0;

    if (blank(model_name) || blank(input_name) || blank(output_name) ||
        input_values == nullptr || output_values == nullptr || output_count == nullptr ||
        input_count <= 0 || output_capacity <= 0) {
        const int status = SHORTHAND_RUNTIME_INVALID_ARGUMENT;
        set_last_infer(status, "none", "missing_or_invalid_typed_buffer_arguments");
        log_status("infer_f32", status, "reason=missing_or_invalid_typed_buffer_arguments");
        return status;
    }

    const std::string model_key = s(model_name);
    const std::string input_key = s(input_name);
    const std::string output_key = s(output_name);

    auto model_it = models.find(model_key);
    if (model_it == models.end()) {
        const int status = SHORTHAND_RUNTIME_MODEL_NOT_FOUND;
        set_last_infer(status, "none", "model_not_registered");
        log_status("infer_f32", status, "model=" + model_key + " reason=model_not_registered");
        return status;
    }

    auto input_it = tensors.find(input_key);
    if (input_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "input_tensor_not_registered");
        log_status("infer_f32", status, "model=" + model_key + " input=" + input_key + " reason=input_tensor_not_registered");
        return status;
    }

    auto output_it = tensors.find(output_key);
    if (output_it == tensors.end()) {
        const int status = SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND;
        set_last_infer(status, "none", "output_tensor_not_registered");
        log_status("infer_f32", status, "model=" + model_key + " output=" + output_key + " reason=output_tensor_not_registered");
        return status;
    }

    const long long expected_input = declared_element_count(input_it->second);
    const long long expected_output = declared_element_count(output_it->second);
    if (expected_input <= 0 || expected_output <= 0 ||
        input_count != expected_input || output_capacity < expected_output) {
        const int status = SHORTHAND_RUNTIME_INVALID_INPUT;
        set_last_infer(status, "none", "typed_buffer_shape_or_capacity_mismatch");
        std::fprintf(stderr,
                     "[shorthand-runtime] infer_f32 model=%s input=%s output=%s status=invalid_input reason=typed_buffer_shape_or_capacity_mismatch input_count=%d expected_input=%lld output_capacity=%d expected_output=%lld\n",
                     model_key.c_str(), input_key.c_str(), output_key.c_str(), input_count, expected_input, output_capacity, expected_output);
        return status;
    }

#if SHORTHAND_RUNTIME_ENABLE_AI_RUNTIME_BRIDGE
    return execute_typed_buffer_through_ai_runtime(model_it->second,
                                                  input_it->second,
                                                  output_it->second,
                                                  input_values,
                                                  input_count,
                                                  output_values,
                                                  output_capacity,
                                                  output_count,
                                                  expected_input,
                                                  expected_output);
#else
    const int status = SHORTHAND_RUNTIME_NOT_EXECUTED;
    const std::string backend = model_it->second.backend_preference.empty() ? "fallback" : model_it->second.backend_preference;
    const std::string reason = "ai_runtime_typed_buffer_bridge_pending";
    set_last_infer(status, backend, reason);
    *output_count = 0;
    infer_bridge_request_json_cache = build_typed_buffer_bridge_request(model_it->second,
                                                                        input_it->second,
                                                                        output_it->second,
                                                                        backend,
                                                                        status,
                                                                        reason,
                                                                        input_count,
                                                                        output_capacity,
                                                                        expected_input,
                                                                        expected_output,
                                                                        *output_count);
    std::fprintf(stderr,
                 "[shorthand-runtime] infer_f32 model=%s input=%s output=%s status=not_executed backend_preference=%s reason=%s typed_buffer_bridge=created\n",
                 model_key.c_str(), input_key.c_str(), output_key.c_str(), backend.c_str(), reason.c_str());
    return status;
#endif
}

extern "C" int short_ai_infer_legacy(const char *model_path,
                                      const char *shape_csv,
                                      const char *input_csv) {
    if (blank(model_path) || blank(shape_csv) || blank(input_csv)) {
        const int status = SHORTHAND_RUNTIME_INVALID_ARGUMENT;
        set_last_infer(status, "none", "missing_model_shape_or_input");
        log_status("infer_legacy", status, "reason=missing_model_shape_or_input");
        return status;
    }

    const int status = SHORTHAND_RUNTIME_NOT_EXECUTED;
    set_last_infer(status, "legacy", "legacy_runtime_bridge_pending");
    std::fprintf(stderr,
                 "[shorthand-runtime] infer_legacy model_path=%s shape=%s input=%s status=not_executed reason=legacy_runtime_bridge_pending\n",
                 safe(model_path), safe(shape_csv), safe(input_csv));
    return status;
}
