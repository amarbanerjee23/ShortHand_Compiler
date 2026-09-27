#include "AI_Telemetry.h"

#include <charconv>
#include <sstream>
#include <system_error>
#include <utility>

namespace shorthand::ai {
namespace {

void appendEscaped(std::string &out, const std::string &value) {
    for (char c : value) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out += c;
        }
    }
}

template <class Integer>
void appendInteger(std::string &out, Integer value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{}) {
        throw std::runtime_error("telemetry_integer_serialization_failed");
    }
    out.append(buffer, result.ptr);
}

void appendDefaultDouble(std::string &out, double value) {
    // The inference hot path records exactly zero here. Preserve the historical
    // iostream representation without constructing a stream for that common
    // case. Nonzero values retain the previous defaultfloat/precision behavior.
    if (value == 0.0) {
        out += '0';
        return;
    }
    std::ostringstream stream;
    stream << value;
    out += stream.str();
}

void appendKeyString(std::string &out, const char *key, const std::string &value, bool comma = true) {
    out += '"';
    out += key;
    out += "\":\"";
    appendEscaped(out, value);
    out += '"';
    if (comma) out += ',';
}

template <class Integer>
void appendKeyInteger(std::string &out, const char *key, Integer value, bool comma = true) {
    out += '"';
    out += key;
    out += "\":";
    appendInteger(out, value);
    if (comma) out += ',';
}

void appendKeyBool(std::string &out, const char *key, bool value, bool comma = true) {
    out += '"';
    out += key;
    out += "\":";
    out += value ? "true" : "false";
    if (comma) out += ',';
}

} // namespace

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
    std::string out;
    // Typical prepared-inference telemetry is ~250 bytes. Reserving once avoids
    // repeated growth while keeping the public JSON schema byte-compatible.
    out.reserve(320 + record.reason.size() + record.model.size());
    out += '{';
    appendKeyString(out, "component", record.component);
    appendKeyString(out, "backend", record.backend);
    appendKeyString(out, "model", record.model);
    appendKeyString(out, "status", record.status);
    appendKeyString(out, "reason", record.reason);
    appendKeyInteger(out, "latency_ns", record.latency_ns);
    appendKeyInteger(out, "input_elements", record.input_elements);
    appendKeyInteger(out, "output_elements", record.output_elements);
    appendKeyBool(out, "measured_energy_available", record.measured_energy_available);
    out += "\"measured_energy_kwh\":";
    appendDefaultDouble(out, record.measured_energy_kwh);
    out += '}';
    return out;
}

std::string telemetryToOtlpLikeSpanJson(const TelemetryRecord &record) {
    std::string out;
    out.reserve(384 + record.reason.size() + record.model.size());
    out += "{\"name\":\"shorthand.ai.infer\",\"kind\":\"SPAN_KIND_INTERNAL\",\"attributes\":{";
    appendKeyString(out, "ai.system", std::string("shorthand"));
    appendKeyString(out, "ai.backend", record.backend);
    appendKeyString(out, "ai.model.name", record.model);
    appendKeyString(out, "ai.inference.status", record.status);
    appendKeyString(out, "ai.inference.reason", record.reason);
    appendKeyInteger(out, "ai.input.elements", record.input_elements);
    appendKeyInteger(out, "ai.output.elements", record.output_elements);
    appendKeyInteger(out, "ai.latency.ns", record.latency_ns);
    appendKeyBool(out, "ai.energy.measured", record.measured_energy_available);
    out += "\"ai.energy.kwh\":";
    appendDefaultDouble(out, record.measured_energy_kwh);
    out += "}}";
    return out;
}

} // namespace shorthand::ai
