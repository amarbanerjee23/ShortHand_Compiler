#include "runtime/ShorthandRuntime.h"
#include "runtime/RuntimePhaseProfile.h"
#include <cmath>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>
using namespace shorthand::runtime_profile;
void check(bool ok,const char *reason) { if (!ok) throw std::runtime_error(reason); }
void partition(const Sample &sample) {
    check(sample.total_ns>0 && std::accumulate(sample.ns.begin(),sample.ns.end(),std::uint64_t{0})==sample.total_ns,
          "profile_partition");
}
unsigned visits(const Sample &sample,Phase phase) { return sample.visits[static_cast<unsigned>(phase)]; }
void registerModel(const char *path) {
    check(!short_ai_register_model("m","onnx",path,"profile","float32","16,64","16,10","onnxruntime_cpu"),"register_model");
    check(!short_ai_register_tensor("x","float32","16,64","2","1024"),"register_input");
    check(!short_ai_register_tensor("y","float32","16,10","2","160"),"register_output");
}
Sample invoke(bool success=true,bool nonfinite=false) {
    std::vector<float> input(1024,0),output(160,-999);
    if (nonfinite) input[7]=std::numeric_limits<float>::quiet_NaN();
    int count=-1;
    Recorder recorder;
    const int status=short_ai_infer_f32("m","x",input.data(),1024,"y",output.data(),160,&count);
    const auto sample=recorder.finish(); partition(sample);
    check((status==0)==success,"profile_status");
    check(count==(success?160:0),"profile_output_count");
    for(float value:output) check(success?std::isfinite(value):value==-999,"profile_output_or_rollback");
    check(visits(sample,Phase::ort_run)==(success?1U:0U),"profile_execution_count");
    check(active==nullptr,"capture_leaked");
    return sample;
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: profile-test model");
        check(!short_runtime_reset(),"reset"); registerModel(argv[1]);
        check(visits(invoke(),Phase::session_prepare)==1,"cold_preparation_missing");
        check(visits(invoke(),Phase::session_prepare)==0,"warm_preparation_unexpected");
        invoke(false,true);
        check(!setenv("SHORTHAND_DEVICE_DENY","cpu",1),"deny_policy");
        invoke(false);
        check(!unsetenv("SHORTHAND_DEVICE_DENY"),"restore_policy");
        invoke();
        check(!short_runtime_reset(),"reset_again"); registerModel(argv[1]);
        check(visits(invoke(),Phase::session_prepare)==1,"reset_preparation_missing");
        auto worker=[] { for(int i=0;i<32;++i) invoke(); };
        auto first=std::async(std::launch::async,worker),second=std::async(std::launch::async,worker);
        first.get(); second.get();
        {
            Recorder recorder;
            bool rejected=false;
            try { Recorder nested; } catch(const std::logic_error &) { rejected=true; }
            check(rejected,"nested_capture_accepted");
            try {
                Scope outer(Phase::registration);
                Scope inner(Phase::oracle);
                throw std::runtime_error("expected_unwind");
            } catch(const std::runtime_error &) {}
            auto sample=recorder.finish(); partition(sample);
            check(visits(sample,Phase::registration)==1 && visits(sample,Phase::oracle)==1,"unwind_accounting");
        }
        check(active==nullptr,"unwind_leaked_capture");
        short_runtime_reset();
        std::cout<<"PASS compiled phase profiling: cold/warm/reset, failure rollback, policy, thread isolation and exception unwind\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
