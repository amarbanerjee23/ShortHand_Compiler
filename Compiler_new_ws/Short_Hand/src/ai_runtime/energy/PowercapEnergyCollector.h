#pragma once
#include "EnergyMeasurement.h"
#include <filesystem>
#include <functional>
namespace shorthand::energy {
class PowercapEnergyCollector final : public EnergyCollector {
public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;
    explicit PowercapEnergyCollector(Instrument = {});
    // Arbitrary filesystem input is always synthetic, never claim eligible.
    static PowercapEnergyCollector forTesting(const std::filesystem::path &, Instrument = {}, Clock = {});
    EnergySample begin() override;
    EnergyMeasurement end(const EnergySample &, std::uint64_t) override;
private:
    PowercapEnergyCollector(std::filesystem::path, Instrument, EvidenceClass);
    std::vector<DomainReading> readDomains() const;
    std::filesystem::path root_;
    Instrument instrument_;
    EvidenceClass evidence_class_;
    Clock clock_ = [] { return std::chrono::steady_clock::now(); };
};
} // namespace shorthand::energy
