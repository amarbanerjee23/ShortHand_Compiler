#include "PowercapEnergyCollector.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>
#ifdef __linux__
#include <sys/vfs.h>
#endif
namespace shorthand::energy {
namespace {
namespace fs = std::filesystem;
bool below(const fs::path &p, const fs::path &root) {
    auto a = p.begin();
    for (auto b = root.begin(); b != root.end(); ++b, ++a) if (a == p.end() || *a != *b) return false;
    return true;
}
std::string readSmall(const fs::path &p) {
    if (fs::is_symlink(fs::symlink_status(p))) throw std::runtime_error("powercap_attribute_symlink");
    std::ifstream in(p, std::ios::binary); if (!in) throw std::runtime_error("powercap_read_failed");
    char bytes[513]; in.read(bytes, sizeof(bytes)); const auto n = in.gcount();
    if (n > 512 || in.bad()) throw std::runtime_error("malformed_powercap_data");
    std::string s(bytes, static_cast<std::size_t>(n));
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.empty()) throw std::runtime_error("malformed_powercap_data");
    for (unsigned char c : s) if (c < 32 || c == 127) throw std::runtime_error("malformed_powercap_data");
    return s;
}
std::uint64_t readCounter(const fs::path &p) {
    const auto s = readSmall(p); std::uint64_t v = 0;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec != std::errc{} || r.ptr != s.data() + s.size()) throw std::runtime_error("malformed_powercap_counter");
    return v;
}
}
PowercapEnergyCollector::PowercapEnergyCollector(Instrument i)
    : PowercapEnergyCollector("/sys/class/powercap", std::move(i), EvidenceClass::Measured) {}
PowercapEnergyCollector::PowercapEnergyCollector(fs::path p, Instrument i, EvidenceClass e)
    : root_(std::move(p)), instrument_(std::move(i)), evidence_class_(e) {}
PowercapEnergyCollector PowercapEnergyCollector::forTesting(const fs::path &p, Instrument i) {
    return PowercapEnergyCollector(p, std::move(i), EvidenceClass::SyntheticTest);
}
std::vector<DomainReading> PowercapEnergyCollector::readDomains() const {
    std::error_code ec;
    if (!fs::is_directory(root_, ec)) throw std::runtime_error("powercap_unavailable");
    if (evidence_class_ == EvidenceClass::Measured) {
#ifdef __linux__
        struct statfs info{};
        if (statfs(root_.c_str(), &info) != 0 || info.f_type != 0x62656572) throw std::runtime_error("powercap_not_sysfs");
#else
        throw std::runtime_error("powercap_unsupported_platform");
#endif
    }
    const auto root = fs::canonical(root_);
    const auto allowed = evidence_class_ == EvidenceClass::Measured ? fs::path("/sys/devices") : root;
    std::vector<fs::path> pending{root_}; std::set<fs::path> visited; std::vector<DomainReading> out;
    while (!pending.empty()) {
        const auto p = fs::canonical(pending.back()); pending.pop_back();
        if (p != root && !below(p, allowed)) throw std::runtime_error("powercap_path_escape");
        if (!visited.insert(p).second) continue;
        if (visited.size() > 512) throw std::runtime_error("too_many_powercap_domains");
        if (fs::exists(p / "energy_uj")) {
            DomainReading d; d.id = p.string(); d.name = fs::exists(p / "name") ? readSmall(p / "name") : p.filename().string();
            d.counter_uj = readCounter(p / "energy_uj");
            if (fs::exists(p / "max_energy_range_uj")) d.range_uj = readCounter(p / "max_energy_range_uj");
            (void)counterDelta(d.counter_uj, d.counter_uj, d.range_uj);
            d.contributes = d.name.rfind("package-", 0) == 0; out.push_back(d);
        }
        for (const auto &e : fs::directory_iterator(p)) {
            const auto name = e.path().filename().string();
            if (name.find("rapl") != std::string::npos && e.is_directory()) pending.push_back(e.path());
        }
    }
    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.id < b.id; });
    std::set<std::string> packages;
    for (const auto &d : out) if (d.contributes && !packages.insert(d.name).second) throw std::runtime_error("overlapping_package_domains");
    if (packages.empty()) throw std::runtime_error("no_cpu_package_energy_domains");
    return out;
}
EnergySample PowercapEnergyCollector::begin() {
    EnergySample s; s.unix_seconds = unixSeconds(); s.monotonic = std::chrono::steady_clock::now();
    try { s.domains = readDomains(); s.available = true; s.reason = "counter_sampled"; }
    catch (const std::exception &e) { s.reason = e.what(); }
    return s;
}
EnergyMeasurement PowercapEnergyCollector::end(const EnergySample &a, std::uint64_t count) {
    if (!count) throw std::runtime_error("invalid_functional_units");
    const auto b = begin(); EnergyMeasurement m; m.source_kind = "rapl"; m.instrument = instrument_;
    m.start_unix_seconds = a.unix_seconds; m.end_unix_seconds = b.unix_seconds;
    m.elapsed_seconds = std::chrono::duration<double>(b.monotonic - a.monotonic).count();
    m.functional_units = count; m.sample_count = 2; m.maximum_sample_gap_seconds = m.elapsed_seconds;
    m.reason = !a.available ? a.reason : b.reason;
    if (!a.available || !b.available) return m;
    try {
        if (a.domains.size() != b.domains.size()) throw std::runtime_error("powercap_domains_changed");
        for (std::size_t i = 0; i < a.domains.size(); ++i) {
            const auto &x = a.domains[i], &y = b.domains[i];
            if (x.id != y.id || x.name != y.name || x.range_uj != y.range_uj || x.contributes != y.contributes)
                throw std::runtime_error("powercap_domains_changed");
            const double j = static_cast<double>(counterDelta(x.counter_uj, y.counter_uj, x.range_uj)) / 1000000.0;
            if (x.contributes) {
                if (evidence_class_ == EvidenceClass::Measured && (!x.range_uj || !std::isfinite(instrument_.maximum_power_w) ||
                    instrument_.maximum_power_w <= 0 || m.elapsed_seconds * instrument_.maximum_power_w >= double(*x.range_uj) / 1000000.0))
                    throw std::runtime_error("counter_wrap_interval_unqualified");
                if (instrument_.maximum_power_w > 0 && j > m.elapsed_seconds * instrument_.maximum_power_w)
                    throw std::runtime_error("counter_delta_exceeds_validated_power_bound");
                m.joules += j;
            }
            m.domains.push_back({x,y,j});
        }
        normalize(m, count); m.available = true; m.evidence_class = evidence_class_;
        m.reason = "package_energy_not_process_attributed";
    } catch (const std::exception &e) {
        m.reason = e.what(); m.joules = m.average_watts = m.joules_per_fu = 0; m.domains.clear();
    }
    return m;
}
} // namespace shorthand::energy
