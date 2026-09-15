#include "Compiler_new_ws/Short_Hand/src/ai_runtime/ExecutionPlan.h"
#include "Compiler_new_ws/Short_Hand/src/ai_runtime/TorchTrainer.h"
#include "Compiler_new_ws/Short_Hand/src/ai_runtime/energy/ExternalMeterCollector.h"
#include "Compiler_new_ws/Short_Hand/src/ai_runtime/energy/PowercapEnergyCollector.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace shorthand::ai;
using namespace shorthand::energy;
namespace fs=std::filesystem;
unsigned assertions=0;
void check(bool b,const char *reason) { ++assertions; if (!b) throw std::runtime_error(reason); }
template<class F> void rejects(F f,const char *reason) { bool failed=false; try { f(); } catch (const std::exception &) { failed=true; } check(failed,reason); }
void write(const fs::path &p,const std::string &s) { std::ofstream f(p); f<<s; if (!f) throw std::runtime_error("fixture_write_failed"); }
Instrument instrument() {
    Instrument i; i.id="test-meter"; i.calibration_id="test-cal"; i.calibration_date="2026-01-01";
    i.validation_ref="synthetic_test"; i.isolation="unit fixture"; i.uncertainty_percent=1; return i;
}
CandidateQualification candidate(unsigned threads,double joules,double latency) {
    CandidateQualification c; c.candidate.id="cpu-"+std::to_string(threads); c.candidate.threads=threads;
    c.candidate.execution_ready=true; c.candidate.model_sha256="same_model";
    for (int j=0;j<3;++j) {
        ExecutionTrial t; t.success=true; t.latency_ms_per_fu=latency;
        t.numerical.valid=t.numerical.shape_equal=t.numerical.dtype_equal=true; t.numerical.quality_score=1;
        t.energy.available=true; t.energy.evidence_class=EvidenceClass::SyntheticTest; t.energy.source_kind="physical_meter";
        t.energy.instrument=instrument(); t.energy.start_unix_seconds=1780000000+j*2; t.energy.end_unix_seconds=t.energy.start_unix_seconds+1;
        t.energy.elapsed_seconds=1; t.energy.joules=joules; normalize(t.energy,5); c.trials.push_back(t);
    }
    return c;
}
int main(int argc,char **argv) {
    try {
        if (argc!=2) throw std::runtime_error("temporary fixture directory required");
        check(counterDelta(10,40,100)==30,"counter_delta"); check(counterDelta(90,5,100)==15,"counter_wrap");
        rejects([]{counterDelta(90,5,{});},"missing_wrap_range"); rejects([]{counterDelta(100,5,100);},"impossible_counter");
        rejects([]{counterDelta(0,0,0);},"zero_range");
        EnergyMeasurement m; m.joules=200; m.elapsed_seconds=10; normalize(m,5);
        check(m.average_watts==20 && m.joules_per_fu==40,"watts_joules_fu");
        rejects([&]{normalize(m,0);},"zero_fu"); m.elapsed_seconds=-1; rejects([&]{normalize(m,5);},"negative_time");
        m.elapsed_seconds=1; m.joules=std::numeric_limits<double>::infinity(); rejects([&]{normalize(m,1);},"nonfinite_energy");
        auto meter=integratePowerSamples({{1780000000,20},{1780000001,40},{1780000003,60}},2,instrument(),EvidenceClass::SyntheticTest);
        check(meter.joules==130 && meter.joules_per_fu==65,"trapezoid"); check(!meter.claimEligible(),"synthetic_not_claim_eligible");
        auto window=integrateMeterWindow({{1780000000,20},{1780000001,40},{1780000003,60}},1780000000.5,1780000002,1,instrument());
        check(window.joules==62.5 && window.power_samples.size()==3,"meter_window_interpolation");
        check(window.source_sample_count==1 && window.maximum_source_gap_seconds==2,"real_samples_exclude_interpolated_endpoints");
        auto sparse=integrateMeterWindow({{1780000000,20},{1780000010,20}},1780000001,1780000002,1,instrument());
        check(sparse.source_sample_count==0 && sparse.sample_count==2 && sparse.maximum_source_gap_seconds==10,
            "sparse_source_gap_survives_window_clipping");
        auto exact=integrateMeterWindow({{1780000000,20},{1780000001,20},{1780000010,20}},1780000000,1780000001,1,instrument());
        check(exact.source_sample_count==2 && exact.maximum_source_gap_seconds==1,"exact_source_endpoints_count_once");
        rejects([&]{integrateMeterWindow({{1780000000,20},{1780000001,40}},1780000000.5,1780000002,1,instrument());},"meter_window_not_bracketed");
        auto malformed=window; malformed.end_unix_seconds=NAN; check(!malformed.claimEligible(),"nan_end_rejected");
        malformed=window; malformed.joules_per_fu=1; check(!malformed.claimEligible(),"inconsistent_fu_rejected");
        rejects([&]{integratePowerSamples({{1780000000,1}},1,instrument(),EvidenceClass::SyntheticTest);},"single_sample");
        rejects([&]{integratePowerSamples({{1780000000,1},{1780000000,2}},1,instrument(),EvidenceClass::SyntheticTest);},"nonmonotonic");
        rejects([&]{integratePowerSamples({{1780000000,-1},{1780000001,2}},1,instrument(),EvidenceClass::SyntheticTest);},"negative_watts");
        check(validDate("2024-02-29") && !validDate("2026-02-29") && !validDate("2026-13-01"),"calendar");
        fs::path root=argv[1]; fs::create_directories(root/"intel-rapl_0"/"intel-rapl_0_0");
        auto package=root/"intel-rapl_0", dram=package/"intel-rapl_0_0";
        write(package/"name","package-0\n"); write(package/"energy_uj","9000000\n"); write(package/"max_energy_range_uj","10000000\n");
        write(dram/"name","dram\n"); write(dram/"energy_uj","100\n"); write(dram/"max_energy_range_uj","10000000\n");
        auto collector=PowercapEnergyCollector::forTesting(root,instrument()); auto start=collector.begin();
        check(start.available && start.domains.size()==2,"domain_discovery");
        write(package/"energy_uj","1000000\n"); write(dram/"energy_uj","1000100\n");
        auto measurement=collector.end(start,2);
        check(measurement.available && measurement.joules==2 && measurement.domains.size()==2,"no_nested_double_count");
        check(!measurement.claimEligible() && measurement.evidence_class==EvidenceClass::SyntheticTest,"fake_rapl_claim_block");
        write(package/"energy_uj","nan\n"); check(!collector.begin().available,"malformed_telemetry");
        write(package/"energy_uj","0\n"); write(package/"name","package-0\tspoof\n"); check(!collector.begin().available,"control_character");
        write(package/"name","package-0\n");
#ifndef _WIN32
        fs::create_directory_symlink(root.parent_path(),root/"rapl_escape"); check(!collector.begin().available,"symlink_escape"); fs::remove(root/"rapl_escape");
#endif
        QualificationProtocol p; p.workload="unit_workload";
        check(validateNumerical({1,2},{1,2},{1,2},{1,2},"float32",p).valid,"numeric_equal");
        check(!validateNumerical({1},{1},{1},{1},"float16",p).valid,"precision_change");
        check(!validateNumerical({1},{1},{1},{1,1},"float32",p).valid,"shape_change");
        check(!validateNumerical({1},{2},{1},{1},"float32",p).valid,"wrong_result");
        check(!validateNumerical({NAN},{NAN},{1},{1},"float32",p).valid,"nan_behavior");
        check(!validateNumerical({INFINITY},{INFINITY},{1},{1},"float32",p).valid,"inf_behavior");
        auto d=summarize({1,2,3}); check(d.mean==2 && d.median==2 && d.standard_deviation==1,"distribution");
        auto a=candidate(1,10,10), b=candidate(2,8,12), c=candidate(4,9,8);
        auto plan=simulateEnergyPlan({a,b,c},p);
        check(plan.selected_candidate==1 && !plan.comparative_energy_claim && plan.synthetic_test,"energy_not_latency");
        b.trials[0].numerical.quality_score=0.5; check(simulateEnergyPlan({a,b,c},p).selected_candidate==2,"quality_guardrail");
        b=candidate(2,8,12); b.candidate.precision="float16"; check(simulateEnergyPlan({a,b,c},p).selected_candidate==2,"precision_guardrail");
        b=candidate(2,8,12); p.maximum_latency_ms=11; check(simulateEnergyPlan({a,b,c},p).selected_candidate==2,"sla_guardrail"); p.maximum_latency_ms=0;
        b=candidate(2,8,12); b.trials[0].energy.instrument.boundary="gpu_only";
        check(simulateEnergyPlan({a,b,c},p).selected_candidate==2,"boundary_guardrail");
        b=candidate(2,8,12); b.candidate.model_sha256="different_model";
        check(simulateEnergyPlan({a,b,c},p).selected_candidate==2,"model_guardrail");
        b=candidate(2,8,12); b.candidate.device="gpu";
        check(simulateEnergyPlan({a,b},p).selected_candidate==1,"qualified_gpu_simulation");
        b.candidate.execution_ready=false; check(simulateEnergyPlan({a,b},p).selected_candidate==0,"gpu_presence_not_execution");
        auto honest=selectEnergyPlan({a,c},p); check(honest.selected_candidate==0 && !honest.comparative_energy_claim &&
            honest.selection_reason=="cpu_baseline_without_energy_optimization","no_fake_winner");
        p.require_measured_energy=true; check(!selectEnergyPlan({a,c},p).selected_candidate,"required_meter_fails_closed");
        rejects([]{cpuThreadCandidates({0});},"invalid_threads");
        ReferenceCNN cnn(42); std::array<float,64> image{}; for (unsigned i=0;i<64;++i) image[i]=(i%2)?0.8f:-0.8f;
        std::vector<float> grad; cnn.lossAndGradient(image,0,&grad);
        for (unsigned i:{0U,35U,40U,150U,336U,353U}) {
            float saved=cnn.parameters[i], eps=0.002f;
            cnn.parameters[i]=saved+eps; float plus=cnn.lossAndGradient(image,0);
            cnn.parameters[i]=saved-eps; float minus=cnn.lossAndGradient(image,0); cnn.parameters[i]=saved;
            check(std::abs((plus-minus)/(2*eps)-grad[i])<0.002,"finite_difference_gradient");
        }
        UnavailableCollector unavailable; TrainingConfiguration tc; TorchTrainer trainer;
        auto train=trainer.trainQualification(tc,unavailable);
        check(train.success && train.validation_accuracy>=0.95 && train.final_loss<train.initial_loss,"nontrivial_cpu_training");
        check(train.samples_processed==1024 && train.optimizer_steps==128 && train.epochs.size()==16 && train.steps.size()==128,"training_accounting");
        check(!train.total_energy.available && !train.total_energy.claimEligible(),"training_without_telemetry");
        auto again=trainer.trainQualification(tc,unavailable); check(train.parameters==again.parameters,"deterministic_seed");
        if (availableCpuThreads()>=2) {
            tc.threads=2; auto threaded=trainer.trainQualification(tc,unavailable);
            check(train.parameters==threaded.parameters && threaded.success,"deterministic_threaded_training");
        }
        tc.epochs=1; tc.learning_rate=1e-7f; tc.target_accuracy=1;
        check(!trainer.trainQualification(tc,unavailable).success,"training_quality_failure");
        tc.learning_rate=NAN; rejects([&]{trainer.trainQualification(tc,unavailable);},"training_invalid_hyperparameter");
        std::cout<<"PASS "<<assertions<<" native energy, planner and CPU CNN assertions; accuracy="<<train.validation_accuracy<<" loss="<<train.final_loss<<'\n';
    } catch (const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
