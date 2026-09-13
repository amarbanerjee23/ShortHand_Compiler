#ifndef SHORT_TORCH_TRAINER_H
#define SHORT_TORCH_TRAINER_H

#include <string>
#include "training/TrainingQualification.h"

class TorchTrainer {
public:
    TorchTrainer() {}
    ~TorchTrainer() {}

    bool trainLinearModel(int epochs, double learning_rate);
    // Additive CPU qualification fixture; optional LibTorch behavior is unchanged.
    shorthand::ai::TrainingOutcome trainQualification(const shorthand::ai::TrainingConfiguration &c,
        shorthand::energy::EnergyCollector &collector) {
        auto result=shorthand::ai::trainCpuQualification(c,collector);
        last_error_=result.success?"":result.reason; return result;
    }
    bool save(const std::string &path);
    std::string getLastError() const;

private:
    std::string last_error_;
};

#endif
