#pragma once
#include "../energy/EnergyMeasurement.h"
#include <array>
namespace shorthand::ai {
struct TrainingConfiguration {
    unsigned epochs=16,samples=64,validation_samples=32,batch_size=8,threads=1;
    std::uint32_t seed=42;
    float learning_rate=0.1f;
    double target_accuracy=0.95;
};
struct TrainingEpoch {
    unsigned index=0;
    std::uint64_t samples_processed=0;
    double loss=0,validation_accuracy=0;
    energy::EnergyMeasurement energy;
};
struct TrainingOutcome {
    bool success=false;
    std::string reason;
    std::string backend="native_cpu_reference_cnn",workload_class="synthetic_qualification_workload";
    std::string precision="float32",optimizer="sgd",boundary="data_generation_initialization_training_validation";
    std::uint64_t samples_processed=0,optimizer_steps=0;
    double initial_loss=0,final_loss=0,validation_accuracy=0,wall_seconds=0;
    std::vector<float> parameters;
    std::vector<TrainingEpoch> epochs;
    std::vector<energy::EnergyMeasurement> steps;
    energy::EnergyMeasurement total_energy;
};
// Fixed two-convolution qualification workload, not a general autodiff engine.
class ReferenceCNN {
public:
    static constexpr std::size_t parameter_count=354;
    explicit ReferenceCNN(std::uint32_t seed);
    float lossAndGradient(const std::array<float,64> &,unsigned label,std::vector<float> *gradient=nullptr,unsigned *prediction=nullptr) const;
    std::vector<float> parameters;
};
TrainingOutcome trainCpuQualification(const TrainingConfiguration &,energy::EnergyCollector &);
}
