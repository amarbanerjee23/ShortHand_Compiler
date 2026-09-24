#include "ApplicationQualification.h"
#include "energy/ExternalMeterCollector.h"
#include "../module/Sha256.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <istream>
#include <ostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>
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
using Clock=std::chrono::steady_clock;
J str(const std::string &v) { J j; j.kind=J::Kind::String; j.string=v; return j; }
J num(double v) { require(std::isfinite(v),"nonfinite_application_report"); J j; j.kind=J::Kind::Number; j.number=v; return j; }
J flag(bool v) { J j; j.kind=J::Kind::Boolean; j.boolean=v; return j; }
J arr() { J j; j.kind=J::Kind::Array; return j; }
J obj(std::initializer_list<std::pair<const std::string,J>> v={}) { J j; j.kind=J::Kind::Object; j.object=v; return j; }
template<class T> J numbers(const std::vector<T> &v) { J j=arr(); for (auto n:v) j.array.push_back(num(n)); return j; }
unsigned integer(const J &j,const char *key,unsigned low,unsigned high) {
    double n=jsonNumber(j,key); require(n>=low && n<=high && std::floor(n)==n,"invalid_application_integer:"+std::string(key)); return static_cast<unsigned>(n);
}
bool digest(const std::string &s) { return s.size()==64 && s.find_first_not_of("0123456789abcdef")==std::string::npos; }
std::string snapshot(const std::string &path,const std::string &hash) {
    namespace fs=std::filesystem;
    require(digest(hash) && !fs::is_symlink(fs::symlink_status(path)) && fs::is_regular_file(path),"invalid_application_artifact");
    const auto bytes=readFile(path);
    require(shorthand::crypto::sha256(bytes)==hash,"application_artifact_sha256_mismatch"); return bytes;
}
void textField(const std::string &s) {
    require(!s.empty() && s.size()<=1024,"invalid_application_provenance");
    for (unsigned char c:s) require(c>=32 && c!=127,"unsafe_application_provenance");
}
std::uint64_t memoryBytes() {
#ifdef __linux__
    rusage r{}; if (getrusage(RUSAGE_SELF,&r)==0) return static_cast<std::uint64_t>(r.ru_maxrss)*1024;
#endif
    return 0;
}
double milliseconds(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
J statistics(std::vector<double> v) {
    require(!v.empty(),"missing_application_timings"); auto d=summarize(v); std::sort(v.begin(),v.end());
    return obj({{"mean",num(d.mean)},{"median",num(d.median)},{"sample_standard_deviation",num(d.standard_deviation)},
        {"p95",num(v[static_cast<std::size_t>(std::ceil(.95*v.size()))-1])},{"raw",numbers(v)}});
}
}
ApplicationConfiguration readApplicationConfiguration(const std::string &path) {
    const auto bytes=shorthand::c3eco::readFile(path); const auto j=shorthand::c3eco::parseJson(bytes);
    requireExactKeys(j,{"schema","qualification_config","qualification_sha256","dataset_path","dataset_sha256","dataset_id",
        "dataset_source","dataset_license","dataset_split","features","classes","top_k","threads","workers","request_timeout_ms",
        "input_min","input_max","offset","scale","minimum_accuracy"},"application configuration");
    require(jsonString(j,"schema")=="shorthand.ai.application.config.v1","invalid_application_schema");
    ApplicationConfiguration c; c.configuration_sha256=shorthand::crypto::sha256(bytes);
    const auto qpath=jsonString(j,"qualification_config"), qhash=jsonString(j,"qualification_sha256"); snapshot(qpath,qhash);
    c.qualification=readQualificationConfiguration(qpath);
    require(c.qualification.configuration_sha256==qhash,"qualification_changed_during_read");
    require(c.qualification.mode=="inference","application_requires_inference");
    c.dataset_path=jsonString(j,"dataset_path"); c.dataset_sha256=jsonString(j,"dataset_sha256");
    require(digest(c.dataset_sha256),"invalid_dataset_sha256");
    c.dataset_id=jsonString(j,"dataset_id"); c.dataset_source=jsonString(j,"dataset_source");
    c.dataset_license=jsonString(j,"dataset_license"); c.dataset_split=jsonString(j,"dataset_split");
    for (const auto &s:{c.dataset_id,c.dataset_source,c.dataset_license,c.dataset_split}) textField(s);
    c.features=integer(j,"features",1,4096); c.classes=integer(j,"classes",2,256); c.top_k=integer(j,"top_k",1,c.classes);
    c.threads=integer(j,"threads",1,256); c.workers=integer(j,"workers",1,16); c.request_timeout_ms=integer(j,"request_timeout_ms",1,30000);
    c.input_min=jsonNumber(j,"input_min"); c.input_max=jsonNumber(j,"input_max"); c.offset=jsonNumber(j,"offset"); c.scale=jsonNumber(j,"scale");
    c.minimum_accuracy=jsonNumber(j,"minimum_accuracy");
    require(c.minimum_accuracy>0 && c.minimum_accuracy<=1 && c.input_min<c.input_max && c.scale!=0,"invalid_application_numeric_contract");
    for (double v:{c.input_min,c.input_max,c.offset,c.scale}) require(std::isfinite(v) && std::abs(v)<=1e6,"invalid_application_preprocessing");
    auto &q=c.qualification; const auto b=q.protocol.batch_size;
    require(q.model.input.shape==std::vector<std::int64_t>{b,c.features} && q.model.output.shape==std::vector<std::int64_t>{b,c.classes},"application_requires_batched_classifier_shapes");
    require(std::find(q.threads.begin(),q.threads.end(),c.threads)!=q.threads.end(),"application_thread_not_in_protocol");
    require(q.protocol.trials<=25 && q.protocol.warmups<=20,"application_trial_limit");
    require(std::uint64_t(c.workers)*b*c.features<=16U*1024U*1024U,"application_worker_memory_limit");
    return c;
}
LabeledDataset readLabeledDataset(const ApplicationConfiguration &c) {
    const auto bytes=snapshot(c.dataset_path,c.dataset_sha256); std::istringstream stream(bytes); std::string line;
    LabeledDataset d; std::vector<bool> represented(c.classes,false);
    while (std::getline(stream,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        require(!line.empty() && line.size()<=65536,"invalid_dataset_line");
        require(d.labels.size()<100000 && (d.labels.size()+1)*c.features<=16U*1024U*1024U,"dataset_size_limit");
        std::istringstream row(line); std::string field; std::vector<double> values;
        while (std::getline(row,field,',')) {
            require(!field.empty() && field.size()<=64 && values.size()<=c.features,"invalid_dataset_columns");
            std::size_t end=0; const double v=std::stod(field,&end);
            require(end==field.size() && std::isfinite(v),"nonfinite_or_malformed_dataset_value"); values.push_back(v);
        }
        require(line.back()!=',' && values.size()==c.features+1,"invalid_dataset_columns");
        const double label=values.back(); require(label>=0 && label<c.classes && std::floor(label)==label,"invalid_dataset_label");
        d.labels.push_back(static_cast<unsigned>(label)); represented[d.labels.back()]=true;
        for (unsigned k=0;k<c.features;++k) { require(values[k]>=c.input_min && values[k]<=c.input_max,"dataset_value_outside_range"); d.values.push_back(static_cast<float>(values[k])); }
    }
    require(d.labels.size()>=c.classes && std::all_of(represented.begin(),represented.end(),[](bool v){return v;}),"dataset_requires_all_classes");
    require(d.labels.size()*std::uint64_t(c.qualification.protocol.repetitions)*c.qualification.protocol.trials<=50000000,"application_work_limit");
    const auto &p=c.qualification.protocol;
    const auto batches=(d.labels.size()+p.batch_size-1)/p.batch_size;
    require(d.labels.size()*c.classes<=262144 && batches*std::uint64_t(p.repetitions)*p.trials<=100000,"application_report_size_limit");
    return d;
}
ClassificationApplication::ClassificationApplication(ApplicationConfiguration c):configuration_(std::move(c)) {
    const auto &limits=configuration_; const auto batch=limits.qualification.protocol.batch_size;
    require(limits.features>0 && limits.features<=4096 && limits.classes>=2 && limits.classes<=256 &&
        limits.top_k>0 && limits.top_k<=limits.classes && batch>0 && batch<=1024 &&
        std::isfinite(limits.input_min) && std::isfinite(limits.input_max) && limits.input_min<limits.input_max &&
        std::isfinite(limits.offset) && std::isfinite(limits.scale) && limits.scale!=0,"invalid_application_host_configuration");
    require(limits.qualification.model.input.shape==std::vector<std::int64_t>{batch,limits.features} &&
        limits.qualification.model.output.shape==std::vector<std::int64_t>{batch,limits.classes},"invalid_application_host_shapes");
    HardwareRoutingPolicy policy; policy.preference={DeviceClass::CPU}; policy.override_device=DeviceClass::CPU;
    AIRuntime runtime(std::make_shared<SystemHardwareProbe>(),policy); std::string error;
    InferenceConfiguration p; p.threads=configuration_.threads;
    session_=runtime.prepare(configuration_.qualification.model,p,error); require(bool(session_),"application_prepare_failed:"+error);
}
std::string ClassificationApplication::runtimeVersion() const { return session_->runtimeVersion(); }
namespace {
template<bool Profiled>
ClassificationBatch classifyBatch(const std::vector<float> &raw,const ApplicationConfiguration &c,
                                  PreparedInference &session,ClassificationProfile *profile) {
    Clock::time_point start{},prepared{},validated{},postprocess{};
    if constexpr (Profiled) start=Clock::now();
    const auto b=c.qualification.protocol.batch_size;
    require(!raw.empty() && raw.size()%c.features==0 && raw.size()<=std::size_t(b)*c.features,"invalid_application_batch");
    TensorBuffer input; input.spec=session.inputSpec(); input.f32_data.assign(std::size_t(b)*c.features,0);
    for (std::size_t i=0;i<raw.size();++i) {
        if (!std::isfinite(raw[i]) || raw[i]<c.input_min || raw[i]>c.input_max)
            throw std::runtime_error("application_input_outside_range");
        input.f32_data[i]=static_cast<float>((double(raw[i])-c.offset)*c.scale);
        if (!std::isfinite(input.f32_data[i])) throw std::runtime_error("application_preprocessing_overflow");
    }
    if constexpr (Profiled) prepared=Clock::now();
    auto result=session.run(input);
    if constexpr (Profiled) validated=Clock::now();
    if (result.status!=InferenceStatus::Success) throw std::runtime_error("application_inference_failed:"+result.reason);
    require(result.output_f32.size()==std::size_t(b)*c.classes,"application_output_count_mismatch");
    for (float v:result.output_f32)
        if (!std::isfinite(v)) throw std::runtime_error("nonfinite_application_output");
    if constexpr (Profiled) postprocess=Clock::now();
    ClassificationBatch out; const auto count=raw.size()/c.features;
    // The result owns these scores. Transfer ownership, then trim padded rows.
    // All backend scores, including padding, were validated above.
    out.scores=std::move(result.output_f32);
    out.scores.resize(count*c.classes);
    out.predictions.reserve(count); out.top_k.reserve(count*c.top_k);
    std::vector<unsigned> order(c.classes);
    for (std::size_t row=0;row<count;++row) {
        std::iota(order.begin(),order.end(),0);
        std::partial_sort(order.begin(),order.begin()+c.top_k,order.end(),[&](unsigned a,unsigned b){
            const float x=out.scores[row*c.classes+a],y=out.scores[row*c.classes+b]; return x==y?a<b:x>y;
        });
        out.predictions.push_back(order.front()); out.top_k.insert(out.top_k.end(),order.begin(),order.begin()+c.top_k);
    }
    if constexpr (Profiled) {
        const auto end=Clock::now();
        auto ns=[](Clock::time_point a,Clock::time_point b) {
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());
        };
        profile->preprocessing_ns=ns(start,prepared);
        profile->prepared_call_ns=ns(prepared,validated);
        profile->output_validation_ns=ns(validated,postprocess);
        profile->postprocessing_ns=ns(postprocess,end);
        profile->total_ns=ns(start,end);
        profile->completed=count;
        profile->success=true;
    }
    return out;
}
} // namespace
ClassificationBatch ClassificationApplication::classify(const std::vector<float> &raw) const {
    return classifyBatch<false>(raw,configuration_,*session_,nullptr);
}
ClassificationBatch ClassificationApplication::classifyProfiled(const std::vector<float> &raw,ClassificationProfile &profile) const {
    profile={}; // A failed call must never leave an earlier successful observation.
    return classifyBatch<true>(raw,configuration_,*session_,&profile);
}
serving::HandlerResult ClassificationApplication::handle(const serving::Request &r,const serving::CancellationToken &token) const {
    try {
        require(!token.stopRequested(),"application_cancelled"); const auto j=parseJson(r.payload);
        requireExactKeys(j,{"values"},"classification request"); const auto &a=jsonMember(j,"values");
        require(a.kind==J::Kind::Array && a.array.size()<=std::size_t(configuration_.qualification.protocol.batch_size)*configuration_.features,"invalid_application_request");
        std::vector<float> values; values.reserve(a.array.size());
        for (const auto &v:a.array) { require(v.kind==J::Kind::Number && std::isfinite(v.number) && v.number>=configuration_.input_min && v.number<=configuration_.input_max,"invalid_application_request_value"); values.push_back(static_cast<float>(v.number)); }
        const auto out=classify(values); require(!token.stopRequested(),"application_cancelled");
        return serving::HandlerResult::succeeded(qualificationJson(obj({{"scores",numbers(out.scores)},{"predictions",numbers(out.predictions)},{"top_k",numbers(out.top_k)}})));
    } catch (const std::exception &e) { return serving::HandlerResult::failed(e.what()); }
}
J describeApplication(const ApplicationConfiguration &c) {
    const auto &q=c.qualification;
    return obj({{"schema",str("shorthand.ai.application.report.v1")},{"configuration_sha256",str(c.configuration_sha256)},
        {"qualification_sha256",str(q.configuration_sha256)},{"model_sha256",str(q.model_sha256)},{"dataset_sha256",str(c.dataset_sha256)},
        {"dataset_id",str(c.dataset_id)},{"dataset_source",str(c.dataset_source)},{"dataset_license",str(c.dataset_license)},{"dataset_split",str(c.dataset_split)},
        {"hardware_fingerprint",str(hardwareFingerprint())},{"compiler_revision",str(SHORTHAND_QUALIFICATION_REVISION)},
        {"precision",str("float32")},{"backend",str("onnxruntime_cpu")},{"threads",num(c.threads)},{"batch_size",num(q.protocol.batch_size)},
        {"functional_unit",str("completed_classification")},{"minimum_accuracy",num(c.minimum_accuracy)},
        {"boundary",str("resident raw FP32 rows through range validation, normalization, inference, finite checks and top-k; disk loading, session preparation, warmup and report I/O separate")},
        {"production_claim",flag(false)},{"comparative_energy_claim",flag(false)},{"official_certification_granted",flag(false)}});
}

J profileApplication(const ApplicationConfiguration &c) {
    const auto &p=c.qualification.protocol;
    require(!p.require_measured_energy,"profiling_cannot_qualify_measured_energy");
    const auto data=readLabeledDataset(c);
    ClassificationApplication app(c);
    auto rows=[&](std::size_t start) {
        const auto end=std::min(data.labels.size(),start+p.batch_size);
        return std::vector<float>(data.values.begin()+start*c.features,data.values.begin()+end*c.features);
    };
    for (unsigned n=0;n<p.warmups;++n) app.classify(rows(0));
    // Untimed reference uses the ordinary path with identical operational checks.
    std::vector<ClassificationBatch> reference;
    std::size_t correct=0;
    for (std::size_t offset=0;offset<data.labels.size();offset+=p.batch_size) {
        reference.push_back(app.classify(rows(offset)));
        const auto &out=reference.back();
        for (std::size_t n=0;n<out.predictions.size();++n) correct+=out.predictions[n]==data.labels[offset+n];
    }
    const double accuracy=double(correct)/data.labels.size();
    require(accuracy>=c.minimum_accuracy,"profile_accuracy_below_threshold");
    J trials=arr();
    for (unsigned trial=0;trial<p.trials;++trial) {
        ClassificationProfile sum; std::uint64_t batches=0;
        const auto observer_start=Clock::now();
        for (unsigned repeat=0;repeat<p.repetitions;++repeat) {
            std::size_t index=0;
            for (std::size_t offset=0;offset<data.labels.size();offset+=p.batch_size,++index) {
                ClassificationProfile sample;
                const auto out=app.classifyProfiled(rows(offset),sample);
                const auto &expected=reference[index];
                require(sample.success && sample.completed==expected.predictions.size(),"incomplete_profile_batch");
                require(out.predictions==expected.predictions && out.top_k==expected.top_k &&
                        out.scores.size()==expected.scores.size(),"profile_prediction_regression");
                for (std::size_t n=0;n<out.scores.size();++n)
                    if (std::abs(double(out.scores[n])-expected.scores[n])>
                        p.absolute_tolerance+p.relative_tolerance*std::abs(double(expected.scores[n])))
                        throw std::runtime_error("profile_numerical_regression");
                sum.preprocessing_ns+=sample.preprocessing_ns; sum.prepared_call_ns+=sample.prepared_call_ns;
                sum.output_validation_ns+=sample.output_validation_ns; sum.postprocessing_ns+=sample.postprocessing_ns;
                sum.total_ns+=sample.total_ns; sum.completed+=sample.completed; ++batches;
            }
        }
        const double observer_ms=milliseconds(observer_start);
        require(sum.completed==data.labels.size()*std::uint64_t(p.repetitions),"incomplete_profile_dataset");
        require(sum.total_ns==sum.preprocessing_ns+sum.prepared_call_ns+sum.output_validation_ns+
                sum.postprocessing_ns,"profile_timing_partition_mismatch");
        trials.array.push_back(obj({{"completed",num(sum.completed)},{"batches",num(batches)},
            {"preprocessing_ns",num(sum.preprocessing_ns)},{"prepared_call_ns",num(sum.prepared_call_ns)},
            {"output_validation_ns",num(sum.output_validation_ns)},{"postprocessing_ns",num(sum.postprocessing_ns)},
            {"total_ns",num(sum.total_ns)},{"observer_elapsed_ms",num(observer_ms)}}));
    }
    J report=describeApplication(c);
    report.object["schema"]=str("shorthand.ai.application.profile.v1");
    report.object["boundary"]=str("instrumented classify calls only; dataset slicing, reference comparisons and accumulation excluded; clock overhead included");
    report.object["prepared_call_boundary"]=str("includes backend input validation, ONNX invocation, output copy and telemetry; not pure model-kernel time");
    report.object["success"]=flag(true); report.object["diagnostic_only"]=flag(true);
    report.object["measured_energy_available"]=flag(false); report.object["latency_claim_eligible"]=flag(false);
    report.object["rows"]=num(data.labels.size()); report.object["repetitions"]=num(p.repetitions);
    report.object["warmups"]=num(p.warmups); report.object["accuracy"]=num(accuracy);
    report.object["backend_version"]=str(app.runtimeVersion()); report.object["trials"]=trials;
    return report;
}

J evaluateApplication(const ApplicationConfiguration &c,bool serve) {
    const auto &q=c.qualification; const auto &p=q.protocol; const auto data=readLabeledDataset(c);
    auto meter=qualificationCollector(q); const auto preparation_start=meter->begin(); const auto prep_clock=Clock::now();
    ClassificationApplication app(c); const auto preparation_ms=milliseconds(prep_clock); const auto prep_energy=meter->end(preparation_start,1);
    std::unique_ptr<serving::ServingRuntime> service;
    if (serve) {
        serving::RuntimeLimits limits; limits.tenant_scope="qualification"; limits.worker_threads=c.workers;
        limits.queue_capacity=2*c.workers; limits.max_in_flight=3*c.workers; limits.completed_result_capacity=4*c.workers;
        limits.max_deadline=std::chrono::milliseconds(c.request_timeout_ms);
        service=std::make_unique<serving::ServingRuntime>(limits,[&](const auto &r,const auto &token){return app.handle(r,token);});
    }
    auto rows=[&](std::size_t start) { const auto end=std::min(data.labels.size(),start+p.batch_size); return std::vector<float>(data.values.begin()+start*c.features,data.values.begin()+end*c.features); };
    auto w=meter->begin(); auto wt=Clock::now(); for (unsigned n=0;n<p.warmups;++n) app.classify(rows(0));
    const auto warmup_ms=milliseconds(wt); const auto warmup_energy=meter->end(w,p.warmups);
    J report=describeApplication(c), trials=arr(); std::vector<double> timings; std::vector<float> reference;
    std::vector<unsigned> predictions, topk; bool all_valid=true; std::string failure; unsigned long long sequence=0;
    for (unsigned trial=0;trial<p.trials;++trial) {
        const auto start=meter->begin(); const double unix_start=energy::unixSeconds(); const auto clock=Clock::now();
        std::vector<double> batch_latency; std::uint64_t completed=0; bool ok=true; std::string reason="executed";
        std::vector<unsigned> trial_predictions;
        try {
            for (unsigned repeat=0;repeat<p.repetitions;++repeat) {
                std::vector<float> scores; std::vector<unsigned> labels, ranked;
                for (std::size_t offset=0;offset<data.labels.size();) {
                    const auto width=serve?c.workers:1U; std::vector<std::pair<std::string,Clock::time_point>> pending;
                    for (unsigned n=0;n<width && offset<data.labels.size();++n) {
                        auto raw=rows(offset); offset+=raw.size()/c.features; const auto batch_clock=Clock::now();
                        if (serve) {
                            const std::string id="batch-"+std::to_string(sequence++); const auto payload=qualificationJson(obj({{"values",numbers(raw)}}));
                            auto admission=service->submit({id,"qualification",payload,std::chrono::milliseconds(c.request_timeout_ms)});
                            require(admission.accepted(),"application_admission_failed:"+admission.reason); pending.emplace_back(id,batch_clock);
                        } else {
                            auto out=app.classify(raw); batch_latency.push_back(milliseconds(batch_clock)/out.predictions.size()); completed+=out.predictions.size();
                            scores.insert(scores.end(),out.scores.begin(),out.scores.end()); labels.insert(labels.end(),out.predictions.begin(),out.predictions.end()); ranked.insert(ranked.end(),out.top_k.begin(),out.top_k.end());
                        }
                    }
                    for (const auto &request:pending) {
                        const auto found=service->wait("qualification",request.first,std::chrono::milliseconds(c.request_timeout_ms+1000));
                        require(found.status==serving::LookupStatus::Ready && found.result.status==serving::TerminalStatus::Succeeded,"application_serving_request_failed");
                        const auto out=parseJson(found.result.output);
                        batch_latency.push_back((found.result.queue_time_us+found.result.service_time_us)/1000.0/jsonMember(out,"predictions").array.size());
                        for (const auto &v:jsonMember(out,"scores").array) scores.push_back(static_cast<float>(v.number));
                        for (const auto &v:jsonMember(out,"predictions").array) { labels.push_back(static_cast<unsigned>(v.number)); ++completed; }
                        for (const auto &v:jsonMember(out,"top_k").array) ranked.push_back(static_cast<unsigned>(v.number));
                    }
                }
                require(labels.size()==data.labels.size(),"incomplete_application_dataset");
                if (reference.empty()) { reference=scores; predictions=labels; topk=ranked; }
                require(scores.size()==reference.size() && labels==predictions,"application_prediction_regression");
                for (std::size_t i=0;i<scores.size();++i) require(std::abs(double(scores[i])-reference[i])<=p.absolute_tolerance+p.relative_tolerance*std::abs(double(reference[i])),"application_numerical_regression");
                trial_predictions=std::move(labels);
            }
        } catch (const std::exception &e) { ok=false; reason=e.what(); }
        const double elapsed=milliseconds(clock), unix_end=energy::unixSeconds(); const auto measurement=meter->end(start,completed);
        std::size_t correct=0; for (std::size_t i=0;i<trial_predictions.size();++i) correct+=trial_predictions[i]==data.labels[i];
        const double accuracy=double(correct)/data.labels.size();
        if (accuracy<c.minimum_accuracy) { ok=false; reason="application_accuracy_below_threshold"; }
        if (p.require_measured_energy && (!measurement.claimEligible() || *measurement.instrument.uncertainty_percent>p.maximum_uncertainty_percent)) { ok=false; reason="application_measured_energy_required"; }
        const auto peak=memoryBytes(); if (peak && peak>p.maximum_memory_bytes) { ok=false; reason="application_memory_limit"; }
        if (p.maximum_latency_ms>0 && !batch_latency.empty()) {
            auto sorted=batch_latency; std::sort(sorted.begin(),sorted.end());
            if (sorted[static_cast<std::size_t>(std::ceil(.95*sorted.size()))-1]>p.maximum_latency_ms) { ok=false; reason="application_p95_latency_limit"; }
        }
        all_valid=all_valid && ok; if (!ok) failure=reason; timings.push_back(elapsed);
        trials.array.push_back(obj({{"success",flag(ok)},{"reason",str(reason)},{"completed",num(completed)},{"accuracy",num(accuracy)},
            {"elapsed_ms",num(elapsed)},{"start_unix_seconds",num(unix_start)},{"end_unix_seconds",num(unix_end)},
            {"latency_ms_per_fu",batch_latency.empty()?J{}:statistics(batch_latency)},{"energy",measurementJson(measurement)}}));
        if (!ok) break;
    }
    if (service) { require(service->shutdown(std::chrono::milliseconds(c.request_timeout_ms+1000)),"application_shutdown_failed"); report.object["serving_metrics"]=parseJson(service->healthJson()); }
    std::vector<unsigned> confusion(c.classes*c.classes,0); std::size_t correct=0,top_correct=0;
    for (std::size_t i=0;i<predictions.size();++i) {
        ++confusion[data.labels[i]*c.classes+predictions[i]]; correct+=predictions[i]==data.labels[i];
        top_correct+=std::find(topk.begin()+i*c.top_k,topk.begin()+(i+1)*c.top_k,data.labels[i])!=topk.begin()+(i+1)*c.top_k;
    }
    report.object["success"]=flag(all_valid); report.object["reason"]=str(all_valid?"qualified_execution":failure);
    report.object["runner"]=str(serve?"shorthand_bounded_serving":"shorthand_native_application"); report.object["backend_version"]=str(app.runtimeVersion());
    report.object["rows"]=num(data.labels.size()); report.object["accuracy"]=num(double(correct)/data.labels.size()); report.object["top_k_accuracy"]=num(double(top_correct)/data.labels.size());
    report.object["confusion_matrix_row_major"]=numbers(confusion); report.object["scores"]=numbers(reference); report.object["predictions"]=numbers(predictions);
    report.object["preparation_ms"]=num(preparation_ms); report.object["preparation_energy"]=measurementJson(prep_energy);
    report.object["warmup_ms"]=num(warmup_ms); report.object["warmup_energy"]=measurementJson(warmup_energy);
    report.object["trial_elapsed_ms"]=statistics(timings); report.object["trials"]=trials;
    report.object["peak_process_memory_bytes"]=memoryBytes()?num(memoryBytes()):J{};
    report.object["padding_policy"]=str("zero-normalized tail slots; excluded from functional units and quality metrics");
    return report;
}

void serveApplicationStream(const ApplicationConfiguration &c,std::istream &input,std::ostream &output) {
    ClassificationApplication app(c); serving::RuntimeLimits limits;
    limits.tenant_scope="application"; limits.worker_threads=c.workers;
    limits.queue_capacity=2*c.workers; limits.max_in_flight=3*c.workers; limits.completed_result_capacity=4*c.workers;
    limits.max_deadline=std::chrono::milliseconds(c.request_timeout_ms);
    serving::ServingRuntime service(limits,[&](const auto &r,const auto &token){return app.handle(r,token);});
    for (;;) {
        std::string line; char ch=0; bool read=false,oversize=false;
        while (input.get(ch)) {
            read=true; if (ch=='\n') break;
            if (line.size()<1024U*1024U) line+=ch; else oversize=true;
        }
        if (!read) { require(input.eof(),"application_stream_read_error"); break; }
        J response;
        try {
            require(!oversize,"application_frame_size_limit"); const auto frame=parseJson(line);
            requireExactKeys(frame,{"requests"},"application frame"); const auto &requests=jsonMember(frame,"requests");
            require(requests.kind==J::Kind::Array && !requests.array.empty() && requests.array.size()<=64,"application_frame_request_limit");
            // Validate the entire frame before accepting any request.
            std::vector<serving::Request> validated;
            for (const auto &j:requests.array) {
                requireExactKeys(j,{"id","tenant","values","timeout_ms"},"application request envelope");
                const auto id=jsonString(j,"id"),tenant=jsonString(j,"tenant");
                const auto timeout=integer(j,"timeout_ms",1,c.request_timeout_ms);
                validated.push_back({id,tenant,qualificationJson(obj({{"values",jsonMember(j,"values")}})),std::chrono::milliseconds(timeout)});
            }
            J results=arr(); std::vector<bool> accepted;
            for (const auto &r:validated) {
                const auto admission=service.submit(r); accepted.push_back(admission.accepted());
                results.array.push_back(obj({{"id",str(r.request_id)},{"admission",str(serving::admissionStatusName(admission.status))},
                    {"detail",str(admission.reason)},{"success",flag(false)}}));
            }
            for (std::size_t n=0;n<validated.size();++n) if (accepted[n]) {
                const auto &r=validated[n]; const auto found=service.wait(r.tenant_id,r.request_id,std::chrono::milliseconds(c.request_timeout_ms+1000));
                require(found.status==serving::LookupStatus::Ready,"application_stream_result_timeout");
                auto &result=results.array[n]; result.object["terminal"]=str(serving::terminalStatusName(found.result.status));
                result.object["detail"]=str(found.result.detail); result.object["success"]=flag(found.result.status==serving::TerminalStatus::Succeeded);
                if (found.result.status==serving::TerminalStatus::Succeeded) result.object["classification"]=parseJson(found.result.output);
            }
            response=obj({{"schema",str("shorthand.ai.application.responses.v1")},{"results",results},
                {"health",parseJson(service.healthJson())},{"production_claim",flag(false)},{"comparative_energy_claim",flag(false)}});
        } catch (const std::exception &e) { response=obj({{"schema",str("shorthand.ai.application.responses.v1")},{"error",str(e.what())}}); }
        output<<qualificationJson(response)<<'\n'; output.flush(); require(bool(output),"application_stream_write_error");
    }
    service.beginDrain(); require(service.shutdown(std::chrono::milliseconds(c.request_timeout_ms+1000)),"application_stream_shutdown_failed");
}

J applicationMeterWindow(const ApplicationConfiguration &c,double start,double end,std::uint64_t units) {
    require(c.qualification.energy_source=="physical_meter","comparison_requires_physical_meter");
    return measurePhysicalWindow(c.qualification.meter_csv,c.qualification.instrument,start,end,units);
}
}
