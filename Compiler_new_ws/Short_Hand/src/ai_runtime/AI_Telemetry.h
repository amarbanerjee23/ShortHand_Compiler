#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace shorthand::ai {

struct TelemetryRecord {
    std::string component;
    std::string backend;
    std::string model;
    std::string status;
    std::string reason;
    long long latency_ns = 0;
    size_t input_elements = 0;
    size_t output_elements = 0;
    double measured_energy_kwh = 0.0;
    bool measured_energy_available = false;
};

class TelemetryTimer {
public:
    TelemetryTimer(std::string backend, std::string model);
    TelemetryRecord finish(std::string status, std::string reason, size_t input_elements, size_t output_elements) const;

private:
    std::string backend_;
    std::string model_;
    std::chrono::steady_clock::time_point start_;
};

std::string telemetryToJson(const TelemetryRecord &record);
std::string telemetryToOtlpLikeSpanJson(const TelemetryRecord &record);

} // namespace shorthand::ai
