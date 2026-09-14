#pragma once
#include "AI_Runtime.h"
#include "ExecutionPlan.h"
#include "training/TrainingQualification.h"
#include "../evidence/C3EcoEvidenceIO.h"
namespace shorthand::ai {
struct QualificationConfiguration {
    QualificationProtocol protocol;
    TrainingConfiguration training;
    ModelSpec model;
    std::vector<unsigned> threads;
    energy::Instrument instrument;
    std::string mode="inference",energy_source="rapl",meter_csv,configuration_sha256,model_sha256;
    unsigned seed=42;
};
QualificationConfiguration readQualificationConfiguration(const std::string &);
std::unique_ptr<energy::EnergyCollector> qualificationCollector(const QualificationConfiguration &);
shorthand::c3eco::Json qualifyWorkload(const QualificationConfiguration &);
shorthand::c3eco::Json executeQualifiedProfile(const QualificationConfiguration &,const std::string &profile,const std::string &trusted_sha256);
std::string qualificationJson(const shorthand::c3eco::Json &);
shorthand::c3eco::Json measurementJson(const energy::EnergyMeasurement &);
void exportQualificationWorkbook(const std::string &report,const std::string &trusted_sha256,const std::string &accounting,const std::string &output);
}
