#include "AI_Telemetry.h"

#include <sstream>
#include <utility>

namespace shorthand::ai {
namespace {
std::string esc(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}
}

TelemetryTimer::TelemetryTimer(std::string backend, std::string model)
    : backend_(std::move(backend)), model_(std::move(model)), start_(std::chrono::steady_clock::now()) {}

TelemetryRecord TelemetryTimer::finish(std::string status, std::string reason, size_t input_elements, size_t output_elements) const {
    auto end = std::chrono::steady_clock::now();
    TelemetryRecord record;
    record.component = "shorthand.ai_runtime";
    record.backend = backend_;
    record.model = model_;
    record.status = std::move(status);
    record.reason = std::move(reason);
    record.latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
    record.input_elements = input_elements;
    record.output_elements = output_elements;
    record.measured_energy_kwh = 0.0;
    record.measured_energy_available = false;
    return record;
}

std::string telemetryToJson(const TelemetryRecord &record) {
    std::ostringstream out;
    out << "{"
        << "\"component\":\"" << esc(record.component) << "\","
        << "\"backend\":\"" << esc(record.backend) << "\","
        << "\"model\":\"" << esc(record.model) << "\","
        << "\"status\":\"" << esc(record.status) << "\","
        << "\"reason\":\"" << esc(record.reason) << "\","
        << "\"latency_ns\":" << record.latency_ns << ","
        << "\"input_elements\":" << record.input_elements << ","
        << "\"output_elements\":" << record.output_elements << ","
        << "\"measured_energy_available\":" << (record.measured_energy_available ? "true" : "false") << ","
        << "\"measured_energy_kwh\":" << record.measured_energy_kwh
        << "}";
    return out.str();
}

std::string telemetryToOtlpLikeSpanJson(const TelemetryRecord &record) {
    std::ostringstream out;
    out << "{"
        << "\"name\":\"shorthand.ai.infer\","
        << "\"kind\":\"SPAN_KIND_INTERNAL\","
        << "\"attributes\":{"
        << "\"ai.system\":\"shorthand\","
        << "\"ai.backend\":\"" << esc(record.backend) << "\","
        << "\"ai.model.name\":\"" << esc(record.model) << "\","
        << "\"ai.inference.status\":\"" << esc(record.status) << "\","
        << "\"ai.inference.reason\":\"" << esc(record.reason) << "\","
        << "\"ai.input.elements\":" << record.input_elements << ","
        << "\"ai.output.elements\":" << record.output_elements << ","
        << "\"ai.latency.ns\":" << record.latency_ns << ","
        << "\"ai.energy.measured\":" << (record.measured_energy_available ? "true" : "false") << ","
        << "\"ai.energy.kwh\":" << record.measured_energy_kwh
        << "}}";
    return out.str();
}

} // namespace shorthand::ai
