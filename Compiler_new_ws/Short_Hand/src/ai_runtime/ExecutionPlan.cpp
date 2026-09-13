#include "ExecutionPlan.h"
#include "../module/Sha256.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <tuple>
#ifdef __linux__
#include <sched.h>
#include <sys/utsname.h>
#elif defined(__APPLE__)
#include <sys/utsname.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace shorthand::ai {
void validateProtocol(const QualificationProtocol &p) {
    if (p.workload.empty() || p.workload.size()>256 || p.functional_unit.empty() || p.functional_unit.size()>256 ||
        (p.device_policy!="cpu_first" && p.device_policy!="cpu_only") || p.precision!="float32" ||
        p.math_policy!="preserve_fp32_no_quantization") throw std::runtime_error("invalid_workload_or_precision_contract");
    if (!p.batch_size || p.batch_size>1024 || !p.warmups || p.warmups>1000 || !p.repetitions || p.repetitions>10000 ||
        p.trials<2 || p.trials>100 || !p.units_per_trial || p.units_per_trial>10000000 ||
        !p.maximum_memory_bytes || p.maximum_memory_bytes>8ULL*1024ULL*1024ULL*1024ULL)
        throw std::runtime_error("qualification_protocol_limit");
    if (!std::isfinite(p.absolute_tolerance) || p.absolute_tolerance<0 || p.absolute_tolerance>1e-3 ||
        !std::isfinite(p.relative_tolerance) || p.relative_tolerance<0 || p.relative_tolerance>1e-2 ||
        !std::isfinite(p.quality_threshold) || p.quality_threshold<0 || p.quality_threshold>1 ||
        !std::isfinite(p.maximum_latency_ms) || p.maximum_latency_ms<0 ||
        !std::isfinite(p.maximum_uncertainty_percent) || p.maximum_uncertainty_percent<=0 || p.maximum_uncertainty_percent>100)
        throw std::runtime_error("invalid_quality_or_sla_contract");
    if (p.quality_metric!="output_agreement" && p.quality_metric!="top1_agreement" && p.quality_metric!="cosine_similarity" &&
        p.quality_metric!="detector_tensor_agreement" && p.quality_metric!="validation_accuracy")
        throw std::runtime_error("unsupported_quality_metric");
}
NumericalValidation validateNumerical(const std::vector<float> &r,const std::vector<float> &a,
    const std::vector<std::int64_t> &rs,const std::vector<std::int64_t> &as,const std::string &dtype,const QualificationProtocol &p) {
    NumericalValidation n; n.shape_equal=rs==as && r.size()==a.size() && !r.empty(); n.dtype_equal=dtype==p.precision;
    n.reason="shape_or_dtype_mismatch";
    if (!n.shape_equal || !n.dtype_equal) return n;
    bool close=true; double dot=0,rn=0,an=0;
    for (std::size_t i=0;i<r.size();++i) {
        if (!std::isfinite(r[i]) || !std::isfinite(a[i])) { ++n.nonfinite_values; close=false; continue; }
        double e=std::abs(double(a[i])-r[i]);
        n.maximum_absolute_error=std::max(n.maximum_absolute_error,e);
        n.maximum_relative_error=std::max(n.maximum_relative_error,e/std::max(std::abs(double(r[i])),1e-30));
        if (e>p.absolute_tolerance+p.relative_tolerance*std::abs(double(r[i]))) close=false;
        dot+=double(r[i])*a[i]; rn+=double(r[i])*r[i]; an+=double(a[i])*a[i];
    }
    n.quality_score=close?1:0;
    if (p.quality_metric=="top1_agreement") {
        if (r.size()%p.batch_size) return n;
        const auto w=r.size()/p.batch_size; unsigned matches=0;
        for (unsigned b=0;b<p.batch_size;++b) {
            auto ri=std::max_element(r.begin()+b*w,r.begin()+(b+1)*w)-(r.begin()+b*w);
            auto ai=std::max_element(a.begin()+b*w,a.begin()+(b+1)*w)-(a.begin()+b*w);
            if (ri==ai) ++matches;
        }
        n.quality_score=double(matches)/p.batch_size;
    } else if (p.quality_metric=="cosine_similarity")
        n.quality_score=rn>0 && an>0?std::clamp(dot/std::sqrt(rn*an),0.0,1.0):(rn==an?1:0);
    n.valid=close && !n.nonfinite_values && n.quality_score>=p.quality_threshold;
    n.reason=n.valid?"numerical_and_quality_contract_satisfied":"numerical_or_quality_contract_failed"; return n;
}
Distribution summarize(const std::vector<double> &values) {
    if (values.empty()) throw std::runtime_error("empty_distribution");
    auto s=values; long double sum=0;
    for (double x:s) { if (!std::isfinite(x) || x<0) throw std::runtime_error("invalid_distribution_value"); sum+=x; }
    std::sort(s.begin(),s.end()); Distribution d; d.minimum=s.front(); d.maximum=s.back(); d.mean=double(sum/s.size());
    d.median=s.size()%2?s[s.size()/2]:s[s.size()/2-1]/2+s[s.size()/2]/2;
    long double variance=0; for (double x:s) variance+=(static_cast<long double>(x)-d.mean)*(x-d.mean);
    d.standard_deviation=s.size()>1?std::sqrt(double(variance/(s.size()-1))):0;
    if (!std::isfinite(d.mean) || !std::isfinite(d.standard_deviation)) throw std::runtime_error("distribution_overflow");
    return d;
}
namespace {
bool measured(const energy::EnergyMeasurement &m,bool simulation) {
    if (!simulation) return m.claimEligible();
    return m.available && m.evidence_class==energy::EvidenceClass::SyntheticTest &&
        std::isfinite(m.joules_per_fu) && m.joules_per_fu>0 && m.instrument.uncertainty_percent &&
        std::isfinite(*m.instrument.uncertainty_percent) && *m.instrument.uncertainty_percent>=0;
}
void qualify(CandidateQualification &c,const QualificationProtocol &p,bool simulation) {
    c.execution_valid=c.energy_valid=false; c.reason="candidate_execution_or_quality_failed";
    if (!c.candidate.execution_ready || c.candidate.precision!=p.precision || c.candidate.math_policy!=p.math_policy ||
        c.candidate.batch_size!=p.batch_size || c.trials.size()!=p.trials || !c.candidate.threads) return;
    std::vector<double> l,e; double uncertainty=0; std::string boundary,source,instrument;
    for (const auto &t:c.trials) {
        if (!t.success || !t.numerical.valid || !t.numerical.shape_equal || !t.numerical.dtype_equal || t.numerical.nonfinite_values ||
            !std::isfinite(t.numerical.quality_score) || t.numerical.quality_score<p.quality_threshold ||
            !std::isfinite(t.latency_ms_per_fu) || t.latency_ms_per_fu<=0 ||
            (p.maximum_latency_ms>0 && t.latency_ms_per_fu>p.maximum_latency_ms)) return;
        l.push_back(t.latency_ms_per_fu); const auto &m=t.energy;
        if (!measured(m,simulation) || m.functional_units!=p.units_per_trial) continue;
        if (!e.empty() && (boundary!=m.instrument.boundary || source!=m.source_kind || instrument!=m.instrument.id)) continue;
        boundary=m.instrument.boundary; source=m.source_kind; instrument=m.instrument.id;
        e.push_back(m.joules_per_fu); uncertainty=std::max(uncertainty,*m.instrument.uncertainty_percent);
    }
    c.execution_valid=true; c.latency_ms=summarize(l); c.reason="direct_energy_unavailable_or_unqualified";
    if (e.size()!=c.trials.size()) return;
    c.joules_per_fu=summarize(e);
    c.expanded_uncertainty_percent=uncertainty+200*c.joules_per_fu.standard_deviation/c.joules_per_fu.mean;
    if (!std::isfinite(c.expanded_uncertainty_percent) || c.expanded_uncertainty_percent>p.maximum_uncertainty_percent) {
        c.reason="measurement_uncertainty_exceeds_contract"; return;
    }
    if (!simulation && p.guardrail_evidence_ref.empty()) { c.reason="guardrail_evidence_missing"; return; }
    c.energy_valid=true; c.reason=simulation?"synthetic_test_candidate":"valid_measured_candidate";
}
EnergyAwareExecutionPlan select(std::vector<CandidateQualification> candidates,const QualificationProtocol &p,bool simulation) {
    validateProtocol(p); EnergyAwareExecutionPlan plan; plan.candidates=std::move(candidates); plan.synthetic_test=simulation;
    if (plan.candidates.empty() || plan.candidates.size()>16) throw std::runtime_error("invalid_candidate_count");
    std::set<std::string> ids; std::optional<std::size_t> baseline;
    for (std::size_t i=0;i<plan.candidates.size();++i) {
        auto &c=plan.candidates[i];
        if (c.candidate.id.empty() || !ids.insert(c.candidate.id).second) throw std::runtime_error("duplicate_candidate_id");
        qualify(c,p,simulation);
        if (c.candidate.device=="cpu" && c.candidate.threads==1 && c.execution_valid) baseline=i;
    }
    if (!baseline) { plan.selection_reason="cpu_baseline_failed"; return plan; }
    const auto &b=plan.candidates[*baseline];
    for (std::size_t i=0;i<plan.candidates.size();++i) {
        auto &c=plan.candidates[i];
        if (!c.energy_valid || (c.candidate.device!="cpu" && p.device_policy=="cpu_only")) continue;
        const auto &m=c.trials.front().energy, &bm=b.trials.front().energy;
        if (c.candidate.model_sha256!=b.candidate.model_sha256 || c.candidate.model_format!=b.candidate.model_format ||
            !b.energy_valid || m.instrument.boundary!=bm.instrument.boundary || m.source_kind!=bm.source_kind || m.instrument.id!=bm.instrument.id) {
            c.energy_valid=false; c.reason="model_or_measurement_boundary_not_comparable"; continue;
        }
        if (c.candidate.device!="cpu" && c.joules_per_fu.mean>=b.joules_per_fu.mean) continue;
        auto key=[](const CandidateQualification &x) { return std::make_tuple(x.joules_per_fu.mean,x.expanded_uncertainty_percent,
            x.latency_ms.mean,x.candidate.device!="cpu",x.candidate.id); };
        if (!plan.selected_candidate || key(c)<key(plan.candidates[*plan.selected_candidate])) plan.selected_candidate=i;
    }
    if (plan.selected_candidate) {
        const auto &s=plan.candidates[*plan.selected_candidate];
        plan.selection_reason=simulation?"synthetic_test_minimum_energy":"minimum_measured_joules_per_functional_unit";
        plan.baseline_joules_per_fu=b.joules_per_fu.mean; plan.selected_joules_per_fu=s.joules_per_fu.mean;
        if (plan.baseline_joules_per_fu<=0) throw std::runtime_error("invalid_energy_baseline");
        plan.reduction_percent=(plan.baseline_joules_per_fu-plan.selected_joules_per_fu)/plan.baseline_joules_per_fu*100;
        plan.comparative_energy_claim=!simulation && *plan.selected_candidate!=*baseline &&
            s.joules_per_fu.mean*(1+s.expanded_uncertainty_percent/100)<b.joules_per_fu.mean*(1-b.expanded_uncertainty_percent/100);
    } else if (!p.require_measured_energy) {
        plan.selected_candidate=baseline; plan.selection_reason="cpu_baseline_without_energy_optimization";
    } else plan.selection_reason="required_measured_energy_unavailable";
    return plan;
}
}
EnergyAwareExecutionPlan selectEnergyPlan(std::vector<CandidateQualification> c,const QualificationProtocol &p) { return select(std::move(c),p,false); }
EnergyAwareExecutionPlan simulateEnergyPlan(std::vector<CandidateQualification> c,const QualificationProtocol &p) { return select(std::move(c),p,true); }
unsigned availableCpuThreads() {
    unsigned n=std::max(1U,std::thread::hardware_concurrency());
#ifdef __linux__
    cpu_set_t set; CPU_ZERO(&set);
    if (sched_getaffinity(0,sizeof(set),&set)==0 && CPU_COUNT(&set)>0) n=std::min(n,static_cast<unsigned>(CPU_COUNT(&set)));
#endif
    return std::min(n,256U);
}
std::vector<unsigned> cpuThreadCandidates(const std::vector<unsigned> &requested) {
    std::set<unsigned> v{1}; unsigned cpus=availableCpuThreads();
    if (requested.empty()) { for (unsigned n:{2U,4U,8U}) if (n<=cpus) v.insert(n); }
    else for (unsigned n:requested) { if (!n || n>cpus) throw std::runtime_error("thread_candidate_exceeds_available_cpus"); v.insert(n); }
    if (v.size()>16) throw std::runtime_error("too_many_thread_candidates");
    return {v.begin(),v.end()};
}
std::string hardwareFingerprint() {
    std::ostringstream out; out<<"cpu_threads="<<availableCpuThreads()<<'\n';
#ifdef __linux__
    struct utsname host{}; if (uname(&host)==0) out<<host.sysname<<':'<<host.release<<':'<<host.machine<<'\n';
    std::ifstream cpu("/proc/cpuinfo"); std::string line;
    while (std::getline(cpu,line)) {
        if (line.rfind("model name",0)==0 || line.rfind("vendor_id",0)==0 || line.rfind("flags",0)==0 || line.rfind("Features",0)==0) out<<line<<'\n';
        if (out.tellp()>1024*1024) break;
    }
    std::ifstream governor("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"); if (std::getline(governor,line)) out<<line;
#elif defined(_WIN32)
    SYSTEM_INFO info{}; GetNativeSystemInfo(&info);
    char host[MAX_COMPUTERNAME_LENGTH+1]{}; DWORD length=sizeof(host);
    if (!GetComputerNameA(host,&length)) throw std::runtime_error("hardware_identity_unavailable");
    out<<"windows:"<<host<<':'<<info.wProcessorArchitecture<<':'<<info.wProcessorLevel<<':'<<info.wProcessorRevision<<':'<<info.dwNumberOfProcessors;
#elif defined(__APPLE__)
    struct utsname host{}; if (uname(&host)!=0) throw std::runtime_error("hardware_identity_unavailable");
    out<<host.sysname<<':'<<host.release<<':'<<host.machine<<':'<<host.nodename;
    char model[256]{}; std::size_t size=sizeof(model);
    if (sysctlbyname("hw.model",model,&size,nullptr,0)!=0 || !size || size>sizeof(model)) throw std::runtime_error("hardware_identity_unavailable");
    out<<':'<<std::string(model,size);
#else
    out<<"unsupported_platform";
#endif
    return shorthand::crypto::sha256(out.str());
}
}
