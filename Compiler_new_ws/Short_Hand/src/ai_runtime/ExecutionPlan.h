#pragma once
#include "energy/EnergyMeasurement.h"
#include <string>
#include <vector>
namespace shorthand::ai {
struct NumericalValidation {
    bool valid=false, shape_equal=false, dtype_equal=false;
    double maximum_absolute_error=0, maximum_relative_error=0, quality_score=0;
    std::size_t nonfinite_values=0;
    std::string reason;
};
struct QualificationProtocol {
    std::string workload, functional_unit="successful_inference", quality_metric="output_agreement";
    std::string device_policy="cpu_first", precision="float32", math_policy="preserve_fp32_no_quantization";
    std::string guardrail_evidence_ref, profile_sha256;
    unsigned batch_size=1, warmups=2, repetitions=5, trials=3;
    std::uint64_t units_per_trial=5, maximum_memory_bytes=1024ULL*1024ULL*1024ULL;
    double absolute_tolerance=1e-5, relative_tolerance=1e-4, quality_threshold=1;
    double maximum_latency_ms=0, maximum_uncertainty_percent=20;
    bool require_measured_energy=false;
};
struct ExecutionCandidate {
    std::string id, backend="onnxruntime_cpu", device="cpu", device_id="cpu:0";
    std::string precision="float32", math_policy="preserve_fp32_no_quantization";
    std::string model_sha256, model_format="onnx", backend_version;
    unsigned threads=1, batch_size=1;
    bool execution_ready=false;
};
struct ExecutionTrial {
    bool success=false;
    std::string reason;
    double latency_ms_per_fu=0;
    NumericalValidation numerical;
    energy::EnergyMeasurement energy;
};
struct Distribution { double mean=0, median=0, standard_deviation=0, minimum=0, maximum=0; };
struct CandidateQualification {
    ExecutionCandidate candidate;
    std::vector<ExecutionTrial> trials;
    std::vector<std::pair<std::string,energy::EnergyMeasurement>> phases;
    bool execution_valid=false, energy_valid=false;
    std::string reason;
    Distribution latency_ms, joules_per_fu;
    double expanded_uncertainty_percent=0;
};
struct EnergyAwareExecutionPlan {
    std::vector<CandidateQualification> candidates;
    std::optional<std::size_t> selected_candidate;
    std::string selection_reason="unavailable";
    bool comparative_energy_claim=false, synthetic_test=false;
    double reduction_percent=0, baseline_joules_per_fu=0, selected_joules_per_fu=0;
};
void validateProtocol(const QualificationProtocol &);
NumericalValidation validateNumerical(const std::vector<float> &,const std::vector<float> &,
    const std::vector<std::int64_t> &,const std::vector<std::int64_t> &,const std::string &,const QualificationProtocol &);
Distribution summarize(const std::vector<double> &);
EnergyAwareExecutionPlan selectEnergyPlan(std::vector<CandidateQualification>,const QualificationProtocol &);
// Unit-test-only simulation produces synthetic_test decisions and never claims.
EnergyAwareExecutionPlan simulateEnergyPlan(std::vector<CandidateQualification>,const QualificationProtocol &);
unsigned availableCpuThreads();
std::vector<unsigned> cpuThreadCandidates(const std::vector<unsigned> &requested={});
std::string hardwareFingerprint();
}
