#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace shorthand::energy {
enum class EvidenceClass { Unavailable, Measured, SyntheticTest, Estimate };
struct Instrument {
    std::string id, calibration_id, calibration_date, validation_ref;
    std::string boundary = "cpu_packages";
    std::string isolation = "concurrent workload state unknown";
    std::optional<double> uncertainty_percent;
    // Validated upper bound for detecting ambiguous wraps, never an energy estimate.
    double maximum_power_w = 0;
};
struct DomainReading {
    std::string id, name;
    std::uint64_t counter_uj = 0;
    std::optional<std::uint64_t> range_uj;
    bool contributes = false;
};
struct EnergySample {
    bool available = false;
    std::string reason = "telemetry_unavailable";
    double unix_seconds = 0;
    std::chrono::steady_clock::time_point monotonic;
    std::vector<DomainReading> domains;
};
struct DomainMeasurement { DomainReading start, end; double joules = 0; };
struct EnergyMeasurement {
    bool available = false;
    EvidenceClass evidence_class = EvidenceClass::Unavailable;
    std::string source_kind = "unavailable", reason = "telemetry_unavailable";
    Instrument instrument;
    double start_unix_seconds = 0, end_unix_seconds = 0, elapsed_seconds = 0;
    double joules = 0, average_watts = 0, joules_per_fu = 0;
    std::uint64_t functional_units = 0;
    std::size_t sample_count = 0;
    double maximum_sample_gap_seconds = 0;
    // Actual samples inside the window, excluding interpolated endpoints.
    std::size_t source_sample_count = 0;
    double maximum_source_gap_seconds = 0;
    std::string trace_sha256;
    std::vector<DomainMeasurement> domains;
    // Physical meter samples (Unix seconds, watts), including interpolated window endpoints.
    std::vector<std::pair<double,double>> power_samples;
    bool claimEligible() const;
};
class EnergyCollector {
public:
    virtual ~EnergyCollector() = default;
    virtual EnergySample begin() = 0;
    virtual EnergyMeasurement end(const EnergySample &, std::uint64_t functional_units) = 0;
};
class UnavailableCollector final : public EnergyCollector {
public:
    EnergySample begin() override;
    EnergyMeasurement end(const EnergySample &, std::uint64_t) override;
};
std::uint64_t counterDelta(std::uint64_t, std::uint64_t, std::optional<std::uint64_t>);
void normalize(EnergyMeasurement &, std::uint64_t);
double unixSeconds();
std::string isoTimestamp(double);
std::string evidenceClassName(EvidenceClass);
bool validDate(const std::string &);
} // namespace shorthand::energy
