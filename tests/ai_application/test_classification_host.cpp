#include "Compiler_new_ws/Short_Hand/src/ai_runtime/ApplicationQualification.h"
#include <cmath>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace shorthand::ai;
thread_local int fault=0;
namespace shorthand::c3eco {
void require(bool ok,const std::string &message) { if (!ok) throw std::runtime_error(message); }
}
class Session final: public PreparedInference {
public:
TensorSpec inputSpec() const override { TensorSpec s; s.shape={2,2}; s.element_count=4; s.element_type=ElementType::Float32; return s; }
TensorSpec outputSpec() const override { TensorSpec s; s.shape={2,3}; s.element_count=6; return s; }
std::string runtimeVersion() const override { return "test-double"; }
InferenceResult run(const TensorBuffer &input) override {
    InferenceResult r; r.status=InferenceStatus::Success;
    r.output_f32={input.f32_data[0],1,1,input.f32_data[2],1,1};
    if(fault==1) r.output_f32.back()=std::numeric_limits<float>::quiet_NaN();
    if(fault==2) r.output_f32.pop_back();
    if(fault==3) {r.status=InferenceStatus::RuntimeError; r.reason="test_failure";}
    return r;
}
InferenceResult runProfiled(const TensorBuffer &input,PreparedInferenceProfile &p) override {
    if (fault==4) return PreparedInference::runProfiled(input,p);
    p={}; const auto start=std::chrono::steady_clock::now();
    auto r=run(input);
    p.session_run_ns=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count());
    p.total_ns=p.session_run_ns; p.success=r.status==InferenceStatus::Success;
    if (fault==5) ++p.total_ns; // Corrupt a synthetic partition to test rejection.
    return r;
}
};
namespace shorthand::ai {
AIRuntime::AIRuntime(std::shared_ptr<HardwareProbe>,HardwareRoutingPolicy) {}
std::unique_ptr<PreparedInference> AIRuntime::prepare(const ModelSpec &,const InferenceConfiguration &,std::string &) { return std::make_unique<Session>(); }
}
void check(bool ok) { if(!ok) throw std::runtime_error("regression"); }
template<class F> void rejects(F f,const std::string &reason) {
    try { f(); } catch(const std::runtime_error &e) { check(e.what()==reason); return; }
    throw std::runtime_error("expected rejection: "+reason);
}
int main() {
    ApplicationConfiguration c; c.features=2;c.classes=3;c.top_k=3;c.input_max=16;c.scale=.5;
    c.qualification.protocol.batch_size=2;c.qualification.model.input.shape={2,2};c.qualification.model.output.shape={2,3};
    ClassificationApplication app(c);
    auto full=app.classify({4,0,0,0});
    check(full.scores==std::vector<float>({2,1,1,0,1,1}));
    check(full.predictions==std::vector<unsigned>({0,1}));
    check(full.top_k==std::vector<unsigned>({0,1,2,1,2,0}));
    auto partial=app.classify({0,0});check(partial.scores.size()==3 && partial.top_k.size()==3);
    check(app.classify({4,0,0,0}).scores==full.scores);
    for(auto raw: {std::vector<float>{}, {1}, {1,2,3,4,5,6}})
        rejects([&]{app.classify(raw);},"invalid_application_batch");
    for(float bad:{-1.f,17.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        rejects([&]{app.classify({bad,0});},"application_input_outside_range");
    fault=1;rejects([&]{app.classify({0,0});},"nonfinite_application_output");
    fault=2;rejects([&]{app.classify({0,0});},"application_output_count_mismatch");
    fault=3;rejects([&]{app.classify({0,0});},"application_inference_failed:test_failure");fault=0;
    ClassificationProfile profile;
    auto profiled=app.classifyProfiled({4,0,0,0},profile);
    check(profile.success && profile.completed==2);
    check(profile.backend.success && profile.backend.total_ns<=profile.prepared_call_ns);
    check(profile.total_ns==profile.preprocessing_ns+profile.prepared_call_ns+
          profile.output_validation_ns+profile.postprocessing_ns);
    check(profiled.scores==full.scores && profiled.predictions==full.predictions && profiled.top_k==full.top_k);
    check(app.classifyProfiled({0,0},profile).scores.size()==3 && profile.completed==1);
    rejects([&]{app.classifyProfiled({17,0},profile);},"application_input_outside_range");
    check(!profile.success && !profile.completed && !profile.total_ns);
    for (int mode:{1,2,3}) {
        fault=mode; profile.success=true;profile.total_ns=123;
        const std::string reason=mode==1?"nonfinite_application_output":
            mode==2?"application_output_count_mismatch":"application_inference_failed:test_failure";
        rejects([&]{app.classifyProfiled({0,0},profile);},reason);
        check(!profile.success && !profile.completed && !profile.total_ns && !profile.backend.success && !profile.backend.total_ns);
    }
    fault=4;
    check(app.classify({0,0}).scores.size()==3); // Ordinary calls never invoke profiling.
    rejects([&]{app.classifyProfiled({0,0},profile);},"application_inference_failed:prepared_profiling_unavailable");
    check(!profile.success && !profile.backend.success);
    fault=5;
    rejects([&]{app.classifyProfiled({0,0},profile);},"invalid_prepared_profile");
    check(!profile.success && !profile.backend.success);
    fault=0;
    std::vector<std::future<void>> workers;
    for(int i=0;i<4;++i) workers.push_back(std::async(std::launch::async,[&]{for(int n=0;n<100;++n) {ClassificationProfile local;check(app.classifyProfiled({4,0,0,0},local).scores==full.scores && local.success);}}));
    for(auto &worker:workers) worker.get();
    std::cout<<"PASS host classification: full/partial batches, ties, error preservation, padding validation, concurrent calls (test backend; not ONNX or energy evidence)\n";
}
