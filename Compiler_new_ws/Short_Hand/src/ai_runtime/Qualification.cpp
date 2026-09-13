#include "Qualification.h"
#include "TorchTrainer.h"
#include "OnnxArtifact.h"
#include "energy/PowercapEnergyCollector.h"
#include "energy/ExternalMeterCollector.h"
#include "../module/Sha256.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#ifdef __linux__
#include <sys/resource.h>
#endif
#ifndef SHORTHAND_QUALIFICATION_REVISION
#define SHORTHAND_QUALIFICATION_REVISION "unversioned"
#endif
namespace shorthand::ai {
namespace {
using namespace shorthand::c3eco;
using J=Json;
J str(const std::string &s) { J j; j.kind=J::Kind::String; j.string=s; return j; }
J num(double n) { require(std::isfinite(n),"nonfinite_report_value"); J j; j.kind=J::Kind::Number; j.number=n; return j; }
J flag(bool v) { J j; j.kind=J::Kind::Boolean; j.boolean=v; return j; }
J arr() { J j; j.kind=J::Kind::Array; return j; }
J obj(std::initializer_list<std::pair<const std::string,J>> v={}) { J j; j.kind=J::Kind::Object; j.object=v; return j; }
const J *optional(const J &j,const std::string &key) { auto it=j.object.find(key); return it==j.object.end()?nullptr:&it->second; }
void keys(const J &j,const std::set<std::string> &allowed) {
    require(j.kind==J::Kind::Object,"configuration_requires_object");
    for (const auto &kv:j.object) require(allowed.count(kv.first)!=0,"unknown_configuration_key:"+kv.first);
}
double number(const J &j,const std::string &key,double fallback) { return optional(j,key)?jsonNumber(j,key):fallback; }
std::string string(const J &j,const std::string &key,const std::string &fallback="") { return optional(j,key)?jsonString(j,key):fallback; }
bool boolean(const J &j,const std::string &key,bool fallback) { return optional(j,key)?jsonBoolean(j,key):fallback; }
std::uint64_t integer(const J &j,const std::string &key,std::uint64_t fallback,std::uint64_t maximum) {
    double n=number(j,key,static_cast<double>(fallback));
    require(n>=0 && n<=static_cast<double>(maximum) && std::floor(n)==n,"invalid_integer:"+key);
    return static_cast<std::uint64_t>(n);
}
bool digest(const std::string &s) { return s.size()==64 && s.find_first_not_of("0123456789abcdef")==std::string::npos; }
void clean(const std::string &s) { require(s.size()<=4096,"text_limit"); for (unsigned char c:s) require(c>=32 && c!=127,"unsafe_text"); }
std::string fileHash(const std::string &p,std::uint64_t maximum) {
    namespace fs=std::filesystem;
    require(!fs::is_symlink(fs::symlink_status(p)) && fs::is_regular_file(p) && fs::file_size(p)<=maximum,"unsafe_or_oversize_file:"+p);
    std::string hash; require(shorthand::crypto::sha256File(p,hash),"file_hash_failed"); return hash;
}
std::vector<std::int64_t> shape(const J &j,const std::string &key) {
    std::vector<std::int64_t> out; const auto *s=optional(j,key); if (!s) return out;
    require(s->kind==J::Kind::Array && s->array.size()<=8,"invalid_shape_rank");
    std::uint64_t count=1;
    for (const auto &v:s->array) {
        require(v.kind==J::Kind::Number && v.number>0 && v.number<=16*1024*1024 && std::floor(v.number)==v.number,"invalid_shape_dimension");
        auto d=static_cast<std::uint64_t>(v.number); require(count<=16*1024*1024/d,"tensor_size_limit"); count*=d; out.push_back(static_cast<std::int64_t>(d));
    }
    return out;
}
energy::Instrument instrument(const J &j) {
    keys(j,{"id","calibration_id","calibration_date","validation_ref","boundary","isolation","uncertainty_percent","maximum_power_w"});
    energy::Instrument i;
    i.id=string(j,"id"); i.calibration_id=string(j,"calibration_id"); i.calibration_date=string(j,"calibration_date");
    i.validation_ref=string(j,"validation_ref"); i.boundary=string(j,"boundary","cpu_packages");
    i.isolation=string(j,"isolation","concurrent workload state unknown");
    if (optional(j,"uncertainty_percent")) { i.uncertainty_percent=jsonNumber(j,"uncertainty_percent"); require(*i.uncertainty_percent>=0 && *i.uncertainty_percent<=100,"invalid_instrument_uncertainty"); }
    i.maximum_power_w=number(j,"maximum_power_w",0); require(i.maximum_power_w>=0 && i.maximum_power_w<=1000000,"invalid_power_bound");
    for (const auto &s:{i.id,i.calibration_id,i.calibration_date,i.validation_ref,i.boundary,i.isolation}) clean(s);
    require(i.calibration_date.empty() || energy::validDate(i.calibration_date),"invalid_calibration_date");
    return i;
}
std::unique_ptr<energy::EnergyCollector> collector(const QualificationConfiguration &c) {
    if (c.energy_source=="rapl") return std::make_unique<energy::PowercapEnergyCollector>(c.instrument);
    if (c.energy_source=="physical_meter") return std::make_unique<energy::PhysicalMeterCollector>(c.meter_csv,c.instrument);
    return std::make_unique<energy::UnavailableCollector>();
}
HardwareRoutingPolicy cpuPolicy() { HardwareRoutingPolicy p; p.preference={DeviceClass::CPU}; p.override_device=DeviceClass::CPU; return p; }
J distribution(const Distribution &d) { return obj({{"mean",num(d.mean)},{"median",num(d.median)},{"sample_standard_deviation",num(d.standard_deviation)},{"minimum",num(d.minimum)},{"maximum",num(d.maximum)}}); }
J numeric(const NumericalValidation &v) {
    return obj({{"valid",flag(v.valid)},{"shape_equal",flag(v.shape_equal)},{"dtype_equal",flag(v.dtype_equal)},
        {"maximum_absolute_error",num(v.maximum_absolute_error)},{"maximum_relative_error",num(v.maximum_relative_error)},
        {"quality_score",num(v.quality_score)},{"nonfinite_values",num(v.nonfinite_values)},{"reason",str(v.reason)}});
}
J floats(const std::vector<float> &v) { J a=arr(); for (float n:v) a.array.push_back(num(n)); return a; }
std::string parameterHash(const std::vector<float> &v) { return shorthand::crypto::sha256(qualificationJson(floats(v))); }
std::uint64_t peakMemoryBytes() {
#ifdef __linux__
    rusage r{}; if (getrusage(RUSAGE_SELF,&r)==0 && r.ru_maxrss>=0) return static_cast<std::uint64_t>(r.ru_maxrss)*1024;
#endif
    return 0;
}
void checkMemory(const QualificationConfiguration &c) { auto m=peakMemoryBytes(); require(!m || m<=c.protocol.maximum_memory_bytes,"process_peak_memory_limit_exceeded"); }
TensorBuffer makeInput(const TensorSpec &spec,unsigned seed) {
    TensorBuffer b; b.spec=spec; b.f32_data.reserve(spec.element_count); std::uint32_t state=seed?seed:1;
    for (std::size_t n=0;n<spec.element_count;++n) { state^=state<<13; state^=state>>17; state^=state<<5; b.f32_data.push_back(float(state%2001)/1000-1); }
    return b;
}
InferenceConfiguration preparedConfiguration(const QualificationConfiguration &c,unsigned threads) {
    InferenceConfiguration p; p.threads=threads;
    p.maximum_tensor_elements=static_cast<std::size_t>(std::min<std::uint64_t>(16*1024*1024,c.protocol.maximum_memory_bytes/16)); return p;
}
J trainingJson(const TrainingOutcome &o) {
    J epochs=arr(),steps=arr();
    for (const auto &e:o.epochs) epochs.array.push_back(obj({{"index",num(e.index)},{"samples_processed",num(e.samples_processed)},
        {"loss",num(e.loss)},{"validation_accuracy",num(e.validation_accuracy)},{"energy",measurementJson(e.energy)}}));
    for (const auto &s:o.steps) steps.array.push_back(measurementJson(s));
    return obj({{"success",flag(o.success)},{"reason",str(o.reason)},{"workload_class",str(o.workload_class)},
        {"optimizer",str(o.optimizer)},{"precision",str(o.precision)},{"boundary",str(o.boundary)},
        {"samples_processed",num(o.samples_processed)},{"optimizer_steps",num(o.optimizer_steps)},
        {"initial_loss",num(o.initial_loss)},{"final_loss",num(o.final_loss)},{"validation_accuracy",num(o.validation_accuracy)},
        {"parameter_count",num(o.parameters.size())},{"parameters_sha256",str(parameterHash(o.parameters))},
        {"epochs",epochs},{"steps",steps},{"total_energy",measurementJson(o.total_energy)},
        {"accounting",str("total overlaps step and epoch readings; use total once; checkpoint I/O excluded")}});
}
J candidateJson(const CandidateQualification &c,const J &training) {
    J trials=arr(),phases=arr();
    for (const auto &t:c.trials) trials.array.push_back(obj({{"success",flag(t.success)},{"reason",str(t.reason)},
        {"latency_ms_per_fu",num(t.latency_ms_per_fu)},{"numerical",numeric(t.numerical)},{"energy",measurementJson(t.energy)}}));
    for (const auto &p:c.phases) phases.array.push_back(obj({{"name",str(p.first)},{"energy",measurementJson(p.second)}}));
    return obj({{"id",str(c.candidate.id)},{"backend",str(c.candidate.backend)},{"backend_version",str(c.candidate.backend_version)},
        {"device",str(c.candidate.device)},{"threads",num(c.candidate.threads)},{"batch_size",num(c.candidate.batch_size)},
        {"precision",str(c.candidate.precision)},{"model_sha256",str(c.candidate.model_sha256)},
        {"execution_ready",flag(c.candidate.execution_ready)},{"execution_valid",flag(c.execution_valid)},
        {"energy_valid",flag(c.energy_valid)},{"reason",str(c.reason)},{"latency_ms",distribution(c.latency_ms)},
        {"joules_per_fu",distribution(c.joules_per_fu)},{"expanded_uncertainty_percent",num(c.expanded_uncertainty_percent)},
        {"trials",trials},{"phases",phases},{"training",training}});
}
void validateProfile(const QualificationConfiguration &c,const J &r) {
    require(jsonString(r,"schema")=="shorthand.ai.cpu_qualification.v1","invalid_profile_schema");
    require(jsonString(r,"configuration_sha256")==c.configuration_sha256 && jsonString(r,"model_sha256")==c.model_sha256 &&
        jsonString(r,"hardware_sha256")==hardwareFingerprint() && jsonString(r,"compiler_revision")==SHORTHAND_QUALIFICATION_REVISION,
        "stale_or_incompatible_profile");
    require(jsonString(r,"mode")==c.mode && !jsonBoolean(r,"synthetic_energy_test"),"incompatible_profile_mode");
}
J readTrustedReport(const std::string &path,const std::string &hash) {
    require(digest(hash),"trusted_report_sha256_required"); const auto text=readFile(path);
    require(shorthand::crypto::sha256(text)==hash,"report_digest_mismatch"); return parseJson(text);
}
}
std::string qualificationJson(const shorthand::c3eco::Json &j) {
    switch (j.kind) {
    case J::Kind::Null:return "null";
    case J::Kind::Boolean:return j.boolean?"true":"false";
    case J::Kind::Number:{ require(std::isfinite(j.number),"nonfinite_json"); std::ostringstream s; s.imbue(std::locale::classic()); s<<std::setprecision(17)<<j.number; return s.str(); }
    case J::Kind::String:return "\""+jsonEscape(j.string)+"\"";
    case J::Kind::Array:{ std::string s="["; for (const auto &v:j.array) { if (s.size()>1) s+=','; s+=qualificationJson(v); } return s+"]"; }
    case J::Kind::Object:{ std::string s="{"; for (const auto &kv:j.object) { if (s.size()>1) s+=','; s+=qualificationJson(str(kv.first))+":"+qualificationJson(kv.second); } return s+"}"; }
    }
    throw std::runtime_error("invalid_json_kind");
}
J measurementJson(const energy::EnergyMeasurement &m) {
    const auto &i=m.instrument; J domains=arr(),power=arr();
    for (const auto &d:m.domains) domains.array.push_back(obj({{"id",str(d.start.id)},{"name",str(d.start.name)},
        // Counters are decimal strings: retain all uint64 bits in JSON consumers.
        {"start_uj",str(std::to_string(d.start.counter_uj))},{"end_uj",str(std::to_string(d.end.counter_uj))},
        {"range_uj",d.start.range_uj?str(std::to_string(*d.start.range_uj)):J{}},{"contributes",flag(d.start.contributes)},{"joules",num(d.joules)}}));
    for (const auto &s:m.power_samples) power.array.push_back(obj({{"unix_time_s",num(s.first)},{"power_w",num(s.second)}}));
    return obj({{"available",flag(m.available)},{"evidence_class",str(energy::evidenceClassName(m.evidence_class))},
        {"source_kind",str(m.source_kind)},{"reason",str(m.reason)},{"claim_eligible",flag(m.claimEligible())},
        {"start_unix_seconds",num(m.start_unix_seconds)},{"end_unix_seconds",num(m.end_unix_seconds)},
        {"elapsed_seconds",num(m.elapsed_seconds)},{"joules",num(m.joules)},{"average_watts",num(m.average_watts)},
        {"joules_per_fu",num(m.joules_per_fu)},{"functional_units",num(m.functional_units)},
        {"sample_count",num(m.sample_count)},{"maximum_sample_gap_seconds",num(m.maximum_sample_gap_seconds)},
        {"domains",domains},{"power_samples",power},{"instrument",obj({{"id",str(i.id)},{"calibration_id",str(i.calibration_id)},
        {"calibration_date",str(i.calibration_date)},{"validation_ref",str(i.validation_ref)},{"boundary",str(i.boundary)},
        {"isolation",str(i.isolation)},{"uncertainty_percent",i.uncertainty_percent?num(*i.uncertainty_percent):J{}},
        {"maximum_power_w",num(i.maximum_power_w)}})}});
}
QualificationConfiguration readQualificationConfiguration(const std::string &path) {
    const auto text=readFile(path); const auto j=parseJson(text); QualificationConfiguration c; auto &p=c.protocol;
    keys(j,{"schema","mode","workload","functional_unit","model_path","model_sha256","input_shape","output_shape","threads","seed",
        "batch_size","warmups","repetitions","trials","maximum_memory_bytes","absolute_tolerance","relative_tolerance","quality_metric",
        "quality_threshold","maximum_latency_ms","maximum_uncertainty_percent","require_measured_energy","guardrail_evidence_ref",
        "precision","device_policy","energy_source","meter_csv","instrument","training","external_files"});
    require(jsonString(j,"schema")=="shorthand.ai.cpu_qualification.config.v1","invalid_configuration_schema");
    c.configuration_sha256=shorthand::crypto::sha256(text); c.mode=string(j,"mode","inference"); require(c.mode=="inference" || c.mode=="training","invalid_mode");
    p.workload=jsonString(j,"workload"); p.functional_unit=string(j,"functional_unit",c.mode=="training"?"training_sample":"successful_inference");
    p.precision=string(j,"precision","float32"); p.device_policy=string(j,"device_policy","cpu_first");
    p.batch_size=static_cast<unsigned>(integer(j,"batch_size",c.mode=="training"?8:1,1024));
    p.warmups=static_cast<unsigned>(integer(j,"warmups",2,1000)); p.repetitions=static_cast<unsigned>(integer(j,"repetitions",5,10000));
    p.trials=static_cast<unsigned>(integer(j,"trials",3,100)); p.maximum_memory_bytes=integer(j,"maximum_memory_bytes",p.maximum_memory_bytes,8ULL*1024*1024*1024);
    p.absolute_tolerance=number(j,"absolute_tolerance",p.absolute_tolerance); p.relative_tolerance=number(j,"relative_tolerance",p.relative_tolerance);
    p.quality_metric=string(j,"quality_metric",c.mode=="training"?"validation_accuracy":"output_agreement");
    p.quality_threshold=number(j,"quality_threshold",c.mode=="training"?0.95:1);
    p.maximum_latency_ms=number(j,"maximum_latency_ms",0); p.maximum_uncertainty_percent=number(j,"maximum_uncertainty_percent",20);
    p.require_measured_energy=boolean(j,"require_measured_energy",false); p.guardrail_evidence_ref=string(j,"guardrail_evidence_ref");
    clean(p.workload); clean(p.functional_unit); clean(p.guardrail_evidence_ref);
    c.seed=static_cast<unsigned>(integer(j,"seed",42,std::numeric_limits<std::uint32_t>::max()));
    c.energy_source=string(j,"energy_source","rapl"); require(c.energy_source=="rapl" || c.energy_source=="unavailable" || c.energy_source=="physical_meter","unsupported_energy_source");
    c.meter_csv=string(j,"meter_csv"); if (c.energy_source=="physical_meter") require(!c.meter_csv.empty(),"meter_csv_required");
    if (const auto *i=optional(j,"instrument")) c.instrument=instrument(*i);
    require(c.energy_source!="rapl" || c.instrument.boundary=="cpu_packages","rapl_package_boundary_required");
    std::vector<unsigned> requested;
    if (const auto *a=optional(j,"threads")) {
        require(a->kind==J::Kind::Array && a->array.size()<=16,"invalid_thread_candidates");
        for (const auto &v:a->array) { require(v.kind==J::Kind::Number && v.number>0 && v.number<=256 && std::floor(v.number)==v.number,"invalid_thread_count"); requested.push_back(static_cast<unsigned>(v.number)); }
    }
    c.threads=cpuThreadCandidates(requested); p.units_per_trial=std::uint64_t(p.repetitions)*p.batch_size;
    validateProtocol(p);
    if (c.mode=="inference") {
        require(!optional(j,"training") && p.quality_metric!="validation_accuracy","invalid_inference_quality_or_training_config");
        c.model.path=jsonString(j,"model_path"); c.model_sha256=jsonString(j,"model_sha256"); require(digest(c.model_sha256),"invalid_model_sha256");
        require(fileHash(c.model.path,p.maximum_memory_bytes)==c.model_sha256,"model_sha256_mismatch");
        const auto referenced=inspectOnnxExternalFiles(c.model.path,p.maximum_memory_bytes);
        std::set<std::string> declared; std::uint64_t total=std::filesystem::file_size(c.model.path);
        if (const auto *files=optional(j,"external_files")) {
            require(files->kind==J::Kind::Array && files->array.size()<=32,"invalid_external_manifest");
            for (const auto &f:files->array) {
                requireExactKeys(f,{"name","sha256","size_bytes"},"external weight artifact");
                const auto name=jsonString(f,"name"), hash=jsonString(f,"sha256");
                require(referenced.count(name) && declared.insert(name).second && digest(hash),"external_manifest_name_or_digest_mismatch");
                const auto size=integer(f,"size_bytes",0,p.maximum_memory_bytes);
                require(size && size<=p.maximum_memory_bytes-total,"external_weights_memory_limit"); total+=size;
                const auto path=(std::filesystem::path(c.model.path).parent_path()/name).string();
                require(std::filesystem::file_size(path)==size && fileHash(path,size)==hash,"external_weights_sha256_mismatch");
            }
        }
        require(declared==referenced,"external_weights_manifest_required");
        c.model.name=p.workload; c.model.format=ModelFormat::Onnx; c.model.precision=p.precision; c.model.allow_fallback=false;
        c.model.backend_preference={BackendKind::OnnxRuntimeCPU}; c.model.input.shape=shape(j,"input_shape"); c.model.output.shape=shape(j,"output_shape");
        if (!c.model.input.shape.empty()) require(c.model.input.shape.front()==p.batch_size,"batch_size_shape_mismatch");
    } else {
        require(!optional(j,"model_path") && !optional(j,"model_sha256") && !optional(j,"input_shape") && !optional(j,"output_shape"),"reference_training_has_fixed_model");
        require(p.quality_metric=="validation_accuracy","training_requires_validation_accuracy");
        c.training.seed=c.seed; c.training.batch_size=p.batch_size; c.training.target_accuracy=p.quality_threshold;
        if (const auto *t=optional(j,"training")) {
            keys(*t,{"epochs","samples","validation_samples","learning_rate"});
            c.training.epochs=static_cast<unsigned>(integer(*t,"epochs",16,100)); c.training.samples=static_cast<unsigned>(integer(*t,"samples",64,4096));
            c.training.validation_samples=static_cast<unsigned>(integer(*t,"validation_samples",32,4096));
            c.training.learning_rate=static_cast<float>(number(*t,"learning_rate",0.1));
        }
        p.units_per_trial=std::uint64_t(c.training.epochs)*c.training.samples;
        c.model_sha256=shorthand::crypto::sha256("reference_cnn_v1:354_fp32_parameters:"+c.configuration_sha256);
        for (unsigned n:c.threads) require(n<=c.training.batch_size,"training_threads_exceed_batch_size");
    }
    validateProtocol(p); return c;
}
J qualifyWorkload(const QualificationConfiguration &c) {
    auto meter=collector(c); const auto &p=c.protocol; const auto all_start=meter->begin();
    AIRuntime runtime(std::make_shared<SystemHardwareProbe>(),cpuPolicy());
    std::vector<CandidateQualification> candidates; std::vector<J> training_records;
    std::vector<float> reference; std::vector<std::int64_t> reference_shape;
    J inventory=parseJson(selectHardwareRoute(SystemHardwareProbe{}.probe(),runtime.capabilities(),c.model,cpuPolicy()).inventory_json);
    for (unsigned threads:c.threads) {
        CandidateQualification q; auto &candidate=q.candidate;
        candidate.id="cpu_threads_"+std::to_string(threads); candidate.threads=threads; candidate.batch_size=p.batch_size;
        candidate.model_sha256=c.model_sha256; candidate.precision=p.precision;
        J training=arr(); std::unique_ptr<PreparedInference> session; TensorBuffer input;
        const auto preparation_start=meter->begin();
        try {
            if (c.mode=="inference") {
                std::string error; session=runtime.prepare(c.model,preparedConfiguration(c,threads),error); require(bool(session),"prepare_failed:"+error);
                candidate.backend_version=session->runtimeVersion();
                require(session->inputSpec().shape.front()==p.batch_size,"batch_size_model_mismatch");
                input=makeInput(session->inputSpec(),c.seed); checkMemory(c);
            } else { candidate.backend="native_cpu_reference_cnn"; candidate.backend_version="reference_cnn_v1"; candidate.model_format="native_reference_cnn"; }
            candidate.execution_ready=true;
        } catch (const std::exception &e) { q.reason=e.what(); }
        auto prep=meter->end(preparation_start,1); prep.reason+="; preparation is a separate phase, not deployment J/FU";
        q.phases.emplace_back(candidate.execution_ready?"preparation":"failed_preparation",prep);
        if (candidate.execution_ready) {
            const auto warm_start=meter->begin();
            try {
                for (unsigned w=0;w<p.warmups;++w) {
                    if (session) {
                        auto r=session->run(input); require(r.status==InferenceStatus::Success,r.reason);
                        if (threads==1 && w==0) { reference=r.output_f32; reference_shape=session->outputSpec().shape; }
                        const auto v=validateNumerical(reference,r.output_f32,reference_shape,session->outputSpec().shape,"float32",p); require(v.valid,v.reason);
                    } else {
                        auto config=c.training; config.threads=threads; energy::UnavailableCollector unmeasured;
                        TorchTrainer trainer; auto result=trainer.trainQualification(config,unmeasured); require(result.success,result.reason);
                        if (threads==1 && w==0) { reference=result.parameters; reference_shape={static_cast<std::int64_t>(reference.size())}; }
                        require(result.parameters==reference,"training_warmup_parameter_mismatch");
                    }
                }
            } catch (const std::exception &e) { candidate.execution_ready=false; q.reason=e.what(); }
            q.phases.emplace_back(candidate.execution_ready?"warmup":"failed_warmup",meter->end(warm_start,p.warmups));
        }
        if (candidate.execution_ready) for (unsigned trial=0;trial<p.trials;++trial) {
            ExecutionTrial t;
            if (session) {
                const auto start=meter->begin(); auto begin=std::chrono::steady_clock::now(); unsigned completed=0;
                try {
                    t.numerical.valid=true; t.numerical.shape_equal=true; t.numerical.dtype_equal=true; t.numerical.quality_score=1;
                    for (unsigned repetition=0;repetition<p.repetitions;++repetition) {
                        auto r=session->run(input); require(r.status==InferenceStatus::Success,r.reason);
                        auto v=validateNumerical(reference,r.output_f32,reference_shape,session->outputSpec().shape,"float32",p);
                        t.numerical.maximum_absolute_error=std::max(t.numerical.maximum_absolute_error,v.maximum_absolute_error);
                        t.numerical.maximum_relative_error=std::max(t.numerical.maximum_relative_error,v.maximum_relative_error);
                        t.numerical.quality_score=std::min(t.numerical.quality_score,v.quality_score);
                        if (!v.valid) { t.numerical=v; throw std::runtime_error(v.reason); }
                        ++completed;
                    }
                    checkMemory(c); t.success=true; t.reason="executed_and_validated"; t.numerical.reason=t.reason;
                } catch (const std::exception &e) { t.reason=e.what(); }
                t.latency_ms_per_fu=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/p.units_per_trial;
                t.energy=meter->end(start,std::max<std::uint64_t>(1,std::uint64_t(completed)*p.batch_size));
                if (!t.success) t.energy.reason+="; failed trial excluded from selection; energy retained as search overhead";
            } else {
                auto config=c.training; config.threads=threads; TorchTrainer trainer; auto result=trainer.trainQualification(config,*meter);
                t.success=result.success; t.reason=result.reason; t.energy=result.total_energy;
                t.latency_ms_per_fu=result.wall_seconds*1000/p.units_per_trial;
                t.numerical=validateNumerical(reference,result.parameters,reference_shape,{static_cast<std::int64_t>(result.parameters.size())},"float32",p);
                t.numerical.quality_score=result.validation_accuracy; t.numerical.valid=t.numerical.valid && result.validation_accuracy>=p.quality_threshold;
                try { checkMemory(c); } catch (const std::exception &e) { t.success=false; t.reason=e.what(); }
                training.array.push_back(trainingJson(result));
            }
            q.trials.push_back(std::move(t));
        }
        // Preserve diagnostic detail even when no executable trial was possible.
        if (!candidate.execution_ready && q.trials.empty()) { ExecutionTrial failure; failure.reason=q.reason; q.trials.push_back(failure); }
        candidates.push_back(std::move(q)); training_records.push_back(std::move(training));
    }
    const auto total=meter->end(all_start,1); auto plan=selectEnergyPlan(std::move(candidates),p); J serialized=arr();
    for (std::size_t n=0;n<plan.candidates.size();++n) serialized.array.push_back(candidateJson(plan.candidates[n],training_records[n]));
    J selected; if (plan.selected_candidate) selected=num(*plan.selected_candidate);
    return obj({{"schema",str("shorthand.ai.cpu_qualification.v1")},{"mode",str(c.mode)},{"workload",str(p.workload)},
        {"configuration_sha256",str(c.configuration_sha256)},{"model_sha256",str(c.model_sha256)},
        {"hardware_sha256",str(hardwareFingerprint())},{"compiler_revision",str(SHORTHAND_QUALIFICATION_REVISION)},
        {"precision",str(p.precision)},{"math_policy",str(p.math_policy)},{"device_policy",str(p.device_policy)},
        {"functional_unit",str(p.functional_unit)},{"units_per_trial",num(p.units_per_trial)},{"quality_metric",str(p.quality_metric)},
        {"dataset_class",str(c.mode=="training"?"synthetic_stripe_classification":"synthetic_inputs_no_task_accuracy")},
        {"quality_scope",str("numerical equivalence or synthetic validation only; standard dataset accuracy and external certification remain pending")},
        {"seed",num(c.seed)},{"warmups",num(p.warmups)},{"repetitions",num(p.repetitions)},{"trials",num(p.trials)},
        {"inventory",inventory},{"accelerator_qualification",str("unavailable; CPU execution only")},
        {"candidates",serialized},{"selected_candidate",selected},{"selection_reason",str(plan.selection_reason)},
        {"comparative_energy_claim",flag(plan.comparative_energy_claim)},{"reduction_percent",num(plan.reduction_percent)},
        {"synthetic_energy_test",flag(false)},{"official_certification",flag(false)},{"lowest_carbon_language_claim",flag(false)},
        {"qualification_total_energy",measurementJson(total)},
        {"accounting_boundary",str("qualification total includes preparation, warmup, validation, search and failed attempts; overlaps candidate windows; artifact acquisition/hash validation and report export excluded; no idle subtraction or amortization claim")},
        {"peak_process_memory_bytes",num(peakMemoryBytes())},{"memory_guard",str("input/model limits and Linux process high-water check; controlled runner must also enforce an OS memory limit")},
        {"uncertainty_method",str("instrument uncertainty plus twice sample standard deviation / mean; not a confidence interval")}});
}
J executeQualifiedProfile(const QualificationConfiguration &c,const std::string &path,const std::string &trusted_sha256) {
    const auto r=readTrustedReport(path,trusted_sha256); validateProfile(c,r);
    const auto &selected=jsonMember(r,"selected_candidate"), &candidates=jsonMember(r,"candidates");
    require(selected.kind==J::Kind::Number && selected.number>=0 && std::floor(selected.number)==selected.number &&
        candidates.kind==J::Kind::Array && selected.number<candidates.array.size(),"profile_has_no_selected_candidate");
    const auto &candidate=candidates.array[static_cast<std::size_t>(selected.number)];
    require(jsonBoolean(candidate,"execution_valid") && jsonString(candidate,"device")=="cpu" && jsonString(candidate,"precision")=="float32","profile_candidate_invalid");
    if (c.protocol.require_measured_energy) require(jsonBoolean(candidate,"energy_valid"),"profile_energy_unavailable");
    auto threads=static_cast<unsigned>(integer(candidate,"threads",0,256));
    require(std::find(c.threads.begin(),c.threads.end(),threads)!=c.threads.end(),"profile_thread_contract_mismatch");
    auto meter=collector(c);
    if (c.mode=="training") {
        require(jsonString(candidate,"backend_version")=="reference_cnn_v1","profile_runtime_version_mismatch");
        auto config=c.training; config.threads=threads; TorchTrainer trainer; auto result=trainer.trainQualification(config,*meter);
        require(result.success,result.reason); checkMemory(c); return trainingJson(result);
    }
    AIRuntime runtime(std::make_shared<SystemHardwareProbe>(),cpuPolicy()); std::string error;
    auto session=runtime.prepare(c.model,preparedConfiguration(c,threads),error); require(bool(session),error);
    require(session->runtimeVersion()==jsonString(candidate,"backend_version"),"profile_runtime_version_mismatch");
    const auto input=makeInput(session->inputSpec(),c.seed); const auto start=meter->begin(); J outputs=arr();
    for (unsigned n=0;n<c.protocol.repetitions;++n) { auto result=session->run(input); require(result.status==InferenceStatus::Success,result.reason); outputs.array.push_back(str(parameterHash(result.output_f32))); }
    checkMemory(c); const auto m=meter->end(start,c.protocol.units_per_trial);
    return obj({{"schema",str("shorthand.ai.cpu_execution.v1")},{"profile_sha256",str(trusted_sha256)},
        {"threads",num(threads)},{"profile_search_repeated",flag(false)},{"prepared_session_count",num(1)},
        {"input_class",str("deterministic_synthetic_qualification_input")},{"output_sha256",outputs},{"energy",measurementJson(m)}});
}
void exportQualificationWorkbook(const std::string &report,const std::string &trusted_sha256,const std::string &accounting,const std::string &output) {
    const auto r=readTrustedReport(report,trusted_sha256), a=parseJson(readFile(accounting));
    require(jsonString(r,"schema")=="shorthand.ai.cpu_qualification.v1" && !jsonBoolean(r,"synthetic_energy_test"),"invalid_report_for_workbook");
    requireExactKeys(a,{"record_id","component","allocation_fraction","pue","carbon_factor_gco2e_per_kwh","factor_source","factor_date", "tariff_per_kwh","tariff_currency","tariff_source","measurement_quality","data_quality","evidence_ref"},"qualification workbook accounting");
    // Export only the disjoint outer window once, never both trials and totals.
    const auto &m=jsonMember(r,"qualification_total_energy"), &i=jsonMember(m,"instrument");
    require(jsonBoolean(m,"available") && jsonBoolean(m,"claim_eligible") && jsonString(m,"evidence_class")=="measured","measured_qualified_energy_required_for_export");
    const auto source=jsonString(m,"source_kind"); require(source=="rapl" || source=="physical_meter","unqualified_workbook_source");
    std::vector<std::string> cells;
    auto add=[&](std::string s) { clean(s); require(!s.empty(),"missing_workbook_provenance"); cells.push_back(std::move(s)); };
    auto addNumber=[&](double n) { add(qualificationJson(num(n))); };
    add(jsonString(a,"record_id")); add(jsonString(a,"component")); add(source); add(jsonString(i,"id"));
    add(jsonString(i,"calibration_id")); add(jsonString(i,"calibration_date")); add(energy::isoTimestamp(jsonNumber(m,"end_unix_seconds")));
    addNumber(jsonNumber(m,"joules")); addNumber(jsonNumber(a,"allocation_fraction")); addNumber(jsonNumber(a,"pue"));
    addNumber(jsonNumber(a,"carbon_factor_gco2e_per_kwh")); add(jsonString(a,"factor_source")); add(jsonString(a,"factor_date"));
    addNumber(jsonNumber(a,"tariff_per_kwh")); add(jsonString(a,"tariff_currency")); add(jsonString(a,"tariff_source"));
    addNumber(jsonNumber(i,"uncertainty_percent")); add(jsonString(a,"measurement_quality")); add(jsonString(a,"data_quality")); add(jsonString(a,"evidence_ref"));
    std::ofstream out(output); require(bool(out),"workbook_output_open_failed");
    out<<"record_id\tcomponent\tsource_kind\tinstrument_id\tcalibration_id\tcalibration_date\tmeasured_at\traw_energy_j\tallocation_fraction\tpue\tcarbon_factor_gco2e_per_kwh\tfactor_source\tfactor_date\ttariff_per_kwh\ttariff_currency\ttariff_source\tuncertainty_percent\tmeasurement_quality\tdata_quality\tevidence_ref\n";
    for (std::size_t n=0;n<cells.size();++n) { if (n) out<<'\t'; out<<cells[n]; } out<<'\n'; out.close(); require(bool(out),"workbook_write_failed");
}
}
