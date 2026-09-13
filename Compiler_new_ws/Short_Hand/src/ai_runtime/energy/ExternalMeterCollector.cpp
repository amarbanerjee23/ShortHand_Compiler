#include "ExternalMeterCollector.h"
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>
namespace shorthand::energy {
EnergyMeasurement integratePowerSamples(const std::vector<PowerSample> &s, std::uint64_t count, const Instrument &i, EvidenceClass c) {
    if (s.size() < 2 || s.size() > 1000000) throw std::runtime_error("invalid_meter_sample_count");
    EnergyMeasurement m; m.instrument = i; m.source_kind = "physical_meter"; m.evidence_class = c;
    m.start_unix_seconds = s.front().first; m.end_unix_seconds = s.back().first;
    m.elapsed_seconds = m.end_unix_seconds - m.start_unix_seconds; m.sample_count = s.size();
    long double sum = 0;
    for (std::size_t n = 0; n < s.size(); ++n) {
        const auto [time, watts] = s[n];
        if (!std::isfinite(time) || !std::isfinite(watts) || watts < 0) throw std::runtime_error("invalid_meter_sample");
        (void)isoTimestamp(time);
        if (!n) continue;
        const double dt = time - s[n-1].first;
        if (dt <= 0 || !std::isfinite(dt)) throw std::runtime_error("nonmonotonic_meter_timestamps");
        m.maximum_sample_gap_seconds = std::max(m.maximum_sample_gap_seconds, dt);
        sum += (static_cast<long double>(watts) + s[n-1].second) * 0.5L * dt;
    }
    m.power_samples = s;
    m.joules = static_cast<double>(sum); normalize(m,count); m.available = true;
    m.reason = "imported_trace_requires_external_provenance_validation"; return m;
}
EnergyMeasurement importPhysicalMeter(const std::string &p, std::uint64_t count, const Instrument &i) {
    namespace fs = std::filesystem;
    if (fs::is_symlink(fs::symlink_status(p)) || !fs::is_regular_file(p) || fs::file_size(p) > 32U*1024U*1024U)
        throw std::runtime_error("unsafe_meter_file");
    std::ifstream in(p); std::string line;
    if (!std::getline(in,line) || line != "unix_time_s,power_w") throw std::runtime_error("invalid_meter_header");
    std::vector<PowerSample> samples;
    while (std::getline(in,line)) {
        if (line.size() > 256 || samples.size() >= 1000000) throw std::runtime_error("meter_input_limit");
        auto pos = line.find(',');
        if (pos == std::string::npos || line.find(',',pos+1) != std::string::npos) throw std::runtime_error("invalid_meter_row");
        auto parse = [](const std::string &s) { std::size_t n=0; double v=std::stod(s,&n);
            if (n != s.size() || !std::isfinite(v)) throw std::runtime_error("invalid_meter_number");
            return v;
        };
        samples.emplace_back(parse(line.substr(0,pos)),parse(line.substr(pos+1)));
    }
    if (in.bad()) throw std::runtime_error("meter_read_failure");
    return integratePowerSamples(samples,count,i,EvidenceClass::Measured);
}
EnergyMeasurement integrateMeterWindow(const std::vector<PowerSample> &samples,double start,double end,std::uint64_t count,const Instrument &i) {
    // Validate the complete trace before clipping, including samples outside the window.
    (void)integratePowerSamples(samples,count,i,EvidenceClass::Measured);
    if (!std::isfinite(start) || !std::isfinite(end) || end<=start || samples.front().first>start || samples.back().first<end)
        throw std::runtime_error("meter_trace_does_not_bracket_execution_window");
    auto interpolate=[&](double t) {
        auto hi=std::lower_bound(samples.begin(),samples.end(),t,[](const PowerSample &p,double v){ return p.first<v; });
        if (hi->first==t) return *hi;
        const auto lo=hi-1;
        const double weight=(t-lo->first)/(hi->first-lo->first);
        return PowerSample{t,lo->second*(1-weight)+hi->second*weight};
    };
    std::vector<PowerSample> clipped{interpolate(start)};
    for (const auto &s:samples) if (s.first>start && s.first<end) clipped.push_back(s);
    clipped.push_back(interpolate(end));
    auto m=integratePowerSamples(clipped,count,i,EvidenceClass::Measured);
    m.reason="physical_meter_execution_window_interpolated_no_idle_subtraction";
    return m;
}
EnergySample PhysicalMeterCollector::begin() {
    EnergySample s; s.available=true; s.unix_seconds=unixSeconds(); s.monotonic=std::chrono::steady_clock::now(); return s;
}
EnergyMeasurement PhysicalMeterCollector::end(const EnergySample &s,std::uint64_t count) {
    // Allow the independently running logger one sample interval to flush. Never
    // extend the measured execution window to include this acquisition overhead.
    const double end=unixSeconds();
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-s.monotonic).count();
    EnergyMeasurement m; m.instrument=instrument_; m.functional_units=count;
    m.start_unix_seconds=s.unix_seconds; m.end_unix_seconds=end; m.elapsed_seconds=elapsed;
    try {
        if (std::abs((end-s.unix_seconds)-elapsed)>0.01) throw std::runtime_error("meter_clock_discontinuity");
        auto trace=importPhysicalMeter(path_,count,instrument_);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
        while (trace.end_unix_seconds<end && std::chrono::steady_clock::now()<deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            trace=importPhysicalMeter(path_,count,instrument_);
        }
        m=integrateMeterWindow(trace.power_samples,s.unix_seconds,end,count,instrument_);
    } catch (const std::exception &e) { m.reason=e.what(); }
    return m;
}
}
