#pragma once
#include "EnergyMeasurement.h"
#include <filesystem>
namespace shorthand::energy {
class PowercapEnergyCollector final : public EnergyCollector {
public:
    explicit PowercapEnergyCollector(Instrument = {});
    // Arbitrary filesystem input is always synthetic, never claim eligible.
    static PowercapEnergyCollector forTesting(const std::filesystem::path &, Instrument = {});
    EnergySample begin() override;
    EnergyMeasurement end(const EnergySample &, std::uint64_t) override;
private:
    PowercapEnergyCollector(std::filesystem::path, Instrument, EvidenceClass);
    std::vector<DomainReading> readDomains() const;
    std::filesystem::path root_;
    Instrument instrument_;
    EvidenceClass evidence_class_;
};
} // namespace shorthand::energy
