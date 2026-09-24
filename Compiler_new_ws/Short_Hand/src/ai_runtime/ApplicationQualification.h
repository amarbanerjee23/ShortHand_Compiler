#pragma once
#include "Qualification.h"
#include "../serving/ServingRuntime.h"
#include <iosfwd>

namespace shorthand::ai {
// Additive native host API. Neither configuration nor model paths are accepted
// from serving requests. Hosts supply verified, immutable deployment artifacts.
struct ApplicationConfiguration {
    QualificationConfiguration qualification;
    std::string configuration_sha256, dataset_path, dataset_sha256, dataset_id;
    std::string dataset_source, dataset_license, dataset_split;
    unsigned features=0, classes=0, top_k=1, threads=1, workers=1;
    unsigned request_timeout_ms=1000;
    double input_min=0, input_max=1, offset=0, scale=1, minimum_accuracy=0;
};
struct LabeledDataset {
    std::vector<float> values;
    std::vector<unsigned> labels;
};
struct ClassificationBatch {
    std::vector<float> scores;
    std::vector<unsigned> predictions, top_k;
};
ApplicationConfiguration readApplicationConfiguration(const std::string &);
LabeledDataset readLabeledDataset(const ApplicationConfiguration &);
// Diagnostic clock partition, never energy or uninstrumented latency evidence.
struct ClassificationProfile {
    std::uint64_t preprocessing_ns=0, prepared_call_ns=0, output_validation_ns=0;
    std::uint64_t postprocessing_ns=0, total_ns=0, completed=0;
    bool success=false;
};
class ClassificationApplication {
public:
    explicit ClassificationApplication(ApplicationConfiguration);
    // Concurrent calls use the same prepared session and local request buffers.
    ClassificationBatch classify(const std::vector<float> &) const;
    ClassificationBatch classifyProfiled(const std::vector<float> &,ClassificationProfile &) const;
    serving::HandlerResult handle(const serving::Request &,const serving::CancellationToken &) const;
    std::string runtimeVersion() const;
private:
    ApplicationConfiguration configuration_;
    std::unique_ptr<PreparedInference> session_;
};
shorthand::c3eco::Json profileApplication(const ApplicationConfiguration &);
shorthand::c3eco::Json evaluateApplication(const ApplicationConfiguration &,bool serving);
shorthand::c3eco::Json describeApplication(const ApplicationConfiguration &);
void serveApplicationStream(const ApplicationConfiguration &,std::istream &,std::ostream &);
// Reuses the native physical-meter parser/integrator for independent runners.
shorthand::c3eco::Json applicationMeterWindow(const ApplicationConfiguration &,double,double,std::uint64_t);
}
