#pragma once
#include "EnergyMeasurement.h"
#include <utility>
namespace shorthand::energy {
using PowerSample = std::pair<double,double>;
EnergyMeasurement integratePowerSamples(const std::vector<PowerSample> &, std::uint64_t, const Instrument &, EvidenceClass);
EnergyMeasurement importPhysicalMeter(const std::string &, std::uint64_t, const Instrument &);
EnergyMeasurement integrateMeterWindow(const std::vector<PowerSample> &,double start,double end,std::uint64_t,const Instrument &);
// The meter logger runs independently. Each completed window must be bracketed
// by real timestamped samples already flushed to disk; otherwise fail closed.
class PhysicalMeterCollector final : public EnergyCollector {
public:
    PhysicalMeterCollector(std::string path,Instrument instrument):path_(std::move(path)),instrument_(std::move(instrument)) {}
    EnergySample begin() override;
    EnergyMeasurement end(const EnergySample &,std::uint64_t) override;
private:
    std::string path_;
    Instrument instrument_;
};
}
