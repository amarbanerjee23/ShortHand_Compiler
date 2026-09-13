#include "EnergyMeasurement.h"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace shorthand::energy {
std::uint64_t counterDelta(std::uint64_t start, std::uint64_t end, std::optional<std::uint64_t> range) {
    if (range && (*range == 0 || start >= *range || end >= *range)) throw std::runtime_error("counter_out_of_range");
    if (end >= start) return end - start;
    if (!range) throw std::runtime_error("counter_wrap_without_range");
    return (*range - start) + end;
}
void normalize(EnergyMeasurement &m, std::uint64_t count) {
    if (count == 0 || count > (std::uint64_t{1} << 53)) throw std::runtime_error("invalid_functional_units");
    if (!std::isfinite(m.elapsed_seconds) || m.elapsed_seconds <= 0 || !std::isfinite(m.joules) || m.joules < 0)
        throw std::runtime_error("invalid_energy_or_elapsed_time");
    m.functional_units = count;
    m.joules_per_fu = m.joules / static_cast<double>(count);
    m.average_watts = m.joules / m.elapsed_seconds;
    if (!std::isfinite(m.average_watts) || !std::isfinite(m.joules_per_fu)) throw std::runtime_error("energy_normalization_overflow");
}
bool validDate(const std::string &s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (std::size_t i = 0; i < s.size(); ++i) if (i != 4 && i != 7 && (s[i] < '0' || s[i] > '9')) return false;
    const int y = std::stoi(s.substr(0,4)), m = std::stoi(s.substr(5,2)), d = std::stoi(s.substr(8,2));
    const int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    return y >= 2000 && y <= 2100 && m >= 1 && m <= 12 && d >= 1 &&
        d <= days[m] + (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}
bool EnergyMeasurement::claimEligible() const {
    if (!available || evidence_class != EvidenceClass::Measured ||
        (source_kind != "rapl" && source_kind != "physical_meter" && source_kind != "accelerator_counter") ||
        !std::isfinite(joules) || joules <= 0 || !std::isfinite(elapsed_seconds) || elapsed_seconds <= 0 ||
        functional_units == 0 || functional_units > (std::uint64_t{1} << 53) || !std::isfinite(joules_per_fu) || joules_per_fu <= 0 ||
        joules_per_fu != joules / static_cast<double>(functional_units) ||
        !std::isfinite(average_watts) || average_watts != joules / elapsed_seconds ||
        !std::isfinite(start_unix_seconds) || !std::isfinite(end_unix_seconds) || end_unix_seconds <= start_unix_seconds ||
        !instrument.uncertainty_percent || !std::isfinite(*instrument.uncertainty_percent) ||
        *instrument.uncertainty_percent < 0 || *instrument.uncertainty_percent > 100 ||
        instrument.id.empty() || instrument.calibration_id.empty() || instrument.validation_ref.empty() ||
        instrument.boundary.empty() || instrument.isolation.empty() || !validDate(instrument.calibration_date)) return false;
    try { return instrument.calibration_date <= isoTimestamp(start_unix_seconds).substr(0,10); }
    catch (...) { return false; }
}
double unixSeconds() { return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); }
std::string isoTimestamp(double v) {
    if (!std::isfinite(v) || v < 946684800 || v > 4102444800) throw std::runtime_error("timestamp_out_of_range");
    const auto time = static_cast<std::time_t>(v);
    std::tm utc{};
#ifdef _WIN32
    if (gmtime_s(&utc, &time) != 0) throw std::runtime_error("invalid_timestamp");
#else
    if (!gmtime_r(&time, &utc)) throw std::runtime_error("invalid_timestamp");
#endif
    std::ostringstream out; out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ"); return out.str();
}
std::string evidenceClassName(EvidenceClass v) {
    switch (v) {
    case EvidenceClass::Measured: return "measured";
    case EvidenceClass::SyntheticTest: return "synthetic_test";
    case EvidenceClass::Estimate: return "estimate";
    default: return "unavailable";
    }
}
EnergySample UnavailableCollector::begin() {
    EnergySample s; s.unix_seconds = unixSeconds(); s.monotonic = std::chrono::steady_clock::now(); return s;
}
EnergyMeasurement UnavailableCollector::end(const EnergySample &s, std::uint64_t count) {
    if (!count) throw std::runtime_error("invalid_functional_units");
    EnergyMeasurement m; m.start_unix_seconds = s.unix_seconds; m.end_unix_seconds = unixSeconds();
    m.elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - s.monotonic).count();
    m.functional_units = count; return m;
}
} // namespace shorthand::energy
