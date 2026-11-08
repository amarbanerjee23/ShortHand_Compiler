// Same synthetic FP32 MatMul+Add workload through compiled .short and direct
// prepared ORT. All output validation is inside BOTH timed boundaries.
#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include "runtime/RuntimePhaseProfile.h"
#include "PerfEventDiagnostics.h"
#if SHORTHAND_RUNTIME_EVENT_PROFILE && SHORTHAND_RUNTIME_PHASE_PROFILE
#error Event and phase diagnostics require separate executables
#endif
#ifndef SHORTHAND_ORT_ALL
#define SHORTHAND_ORT_ALL 0
#endif
#if SHORTHAND_GENERATED
#include "runtime/ShorthandRuntime.h"
extern "C" int shorthand_entry();
extern "C" int __real_short_ai_infer_f32(const char *,const char *,const float *,int,const char *,float *,int,int *);
#endif

namespace {
using Clock=std::chrono::steady_clock;
std::vector<float> expected;
std::uint64_t calls=0;
void check(bool ok,const char *reason) { if (!ok) throw std::runtime_error(reason); }
bool valid(const float *input,std::size_t inputs,const float *output,std::size_t count) {
    SHORTHAND_PHASE_SCOPE(oracle);
    if (inputs!=expected.size()/10*64 || count!=expected.size()) return false;
    for(std::size_t i=0;i<inputs;++i) if(input[i]!=0) return false;
    for(std::size_t i=0;i<count;++i)
        if(!std::isfinite(output[i]) || std::abs(output[i]-expected[i])>1e-5f+1e-5f*std::abs(expected[i])) return false;
    ++calls; return true;
}
double ms(Clock::time_point begin) { return std::chrono::duration<double,std::milli>(Clock::now()-begin).count(); }
#if !SHORTHAND_GENERATED
class Direct {
    Ort::Env env_{ORT_LOGGING_LEVEL_ERROR,"direct"};
    Ort::MemoryInfo memory_=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
    std::unique_ptr<Ort::Session> session_;
    std::vector<float> input_,output_;
    std::vector<std::int64_t> in_shape_,out_shape_;
    std::string in_name_,out_name_;
    Ort::Value in_{nullptr},out_{nullptr};
public:
    Direct(const char *model,int batch):input_(batch*64,0),output_(batch*10,0),in_shape_{batch,64},out_shape_{batch,10} {
        Ort::SessionOptions options; options.SetIntraOpNumThreads(1); options.SetInterOpNumThreads(1);
        options.SetExecutionMode(ORT_SEQUENTIAL);
        options.SetGraphOptimizationLevel(SHORTHAND_ORT_ALL ? ORT_ENABLE_ALL : ORT_ENABLE_BASIC);
        options.AddConfigEntry("session.intra_op.allow_spinning","0"); options.AddConfigEntry("session.inter_op.allow_spinning","0");
        session_=std::make_unique<Ort::Session>(env_,model,options);
        Ort::AllocatorWithDefaultOptions allocator;
        in_name_=session_->GetInputNameAllocated(0,allocator).get();
        out_name_=session_->GetOutputNameAllocated(0,allocator).get();
        in_=Ort::Value::CreateTensor<float>(memory_,input_.data(),input_.size(),in_shape_.data(),2);
        out_=Ort::Value::CreateTensor<float>(memory_,output_.data(),output_.size(),out_shape_.data(),2);
    }
    void run() {
        const char *inputs[]={in_name_.c_str()},*outputs[]={out_name_.c_str()};
        session_->Run(Ort::RunOptions{nullptr},inputs,&in_,1,outputs,&out_,1);
        check(valid(input_.data(),input_.size(),output_.data(),output_.size()),"direct_output_mismatch");
    }
};
#endif
}
#if SHORTHAND_GENERATED
extern "C" int __wrap_short_ai_infer_f32(const char *model,const char *name,const float *input,int size,
                                          const char *out_name,float *output,int capacity,int *count) {
    const int status=__real_short_ai_infer_f32(model,name,input,size,out_name,output,capacity,count);
    if(status || !count || *count<=0 || !valid(input,size,output,*count)) return -1;
    return status;
}
#endif
int main(int argc,char **argv) {
    try {
        check(argc==6 || (argc==7 && std::string(argv[6])=="--observe-every-call"),
              "usage: probe model batch expected.bin iterations blocks [--observe-every-call]");
        const bool observe=argc==7;
#if !SHORTHAND_GENERATED || SHORTHAND_RUNTIME_PHASE_PROFILE
        check(!observe,"observer_control_requires_uninstrumented_generated_runner");
#endif
        const int batch=std::stoi(argv[2]),iterations=std::stoi(argv[4]),blocks=std::stoi(argv[5]);
        check((batch==1||batch==16||batch==32) && iterations>0 && iterations<=4096 && blocks>=2 && blocks<=20,"invalid_protocol");
        expected.resize(batch*10); std::ifstream file(argv[3],std::ios::binary);
        file.read(reinterpret_cast<char *>(expected.data()),expected.size()*sizeof(float));
        check(bool(file) && file.peek()==std::char_traits<char>::eof(),"invalid_oracle");
        for(float v:expected) check(std::isfinite(v),"nonfinite_oracle");
#if SHORTHAND_RUNTIME_EVENT_PROFILE
        check(!observe,"event_profile_cannot_be_an_observer_control");
        shorthand::event_profile::Capture<> empty_capture;
        const auto empty_events=empty_capture.finish();
        shorthand::event_profile::Capture<> cold_capture;
#endif
        const auto cold_start=Clock::now();
#if SHORTHAND_GENERATED
        short_runtime_reset();
        auto once=[&] {
            check(shorthand_entry()==0,"generated_entry_failed");
            if (observe) {
                // Consume every exporter inside the timed call. This control
                // detects work moved to lazy observers rather than eliminated.
                for (auto query : {short_runtime_last_infer_telemetry_json,
                                   short_runtime_infer_bridge_request_json,
                                   short_runtime_observability_json,
                                   short_runtime_otlp_spans_json}) {
                    const char *json=query();
                    check(json && std::strstr(json,"\"schema\":") && std::strlen(json)>2,
                          "missing_observer_payload");
                }
            }
        };
#else
        Direct direct(argv[1],batch); auto once=[&] { direct.run(); };
#endif
        once(); const double cold=ms(cold_start);
#if SHORTHAND_RUNTIME_EVENT_PROFILE
        const auto cold_events=cold_capture.finish();
        std::vector<shorthand::event_profile::Sample> event_blocks;
        event_blocks.reserve(blocks);
#endif
        for(int i=0;i<8;++i) once();
        calls=0; std::vector<double> elapsed;
#if SHORTHAND_RUNTIME_PHASE_PROFILE
        std::vector<shorthand::runtime_profile::Sample> profiles;
        profiles.reserve(iterations*blocks);
        std::vector<double> clock_pairs;
        for(int i=0;i<1000;++i) {
            const auto begin=Clock::now(),end=Clock::now();
            clock_pairs.push_back(std::chrono::duration<double,std::nano>(end-begin).count());
        }
        std::sort(clock_pairs.begin(),clock_pairs.end());
#endif
        for(int block=0;block<blocks;++block) {
#if SHORTHAND_RUNTIME_EVENT_PROFILE
            shorthand::event_profile::Capture<> capture;
#endif
            const auto begin=Clock::now();
            for(int i=0;i<iterations;++i) {
#if SHORTHAND_RUNTIME_PHASE_PROFILE
                shorthand::runtime_profile::Recorder recorder;
                once();
                profiles.push_back(recorder.finish());
#else
                once();
#endif
            }
            elapsed.push_back(ms(begin));
#if SHORTHAND_RUNTIME_EVENT_PROFILE
            event_blocks.push_back(capture.finish());
#endif
        }
        check(calls==std::uint64_t(iterations)*blocks,"incomplete_execution");
#if SHORTHAND_RUNTIME_EVENT_PROFILE
        (void)cold;
        std::cout<<"{\"schema\":\"shorthand.generated_infer.events.v1\",\"success\":true"
                 <<",\"diagnostic_only\":true,\"instrumented\":true,\"energy_claim_authorized\":false"
                 <<",\"latency_claim_authorized\":false,\"hardware_measured_joules\":null"
                 <<",\"batch\":"<<batch<<",\"iterations\":"<<iterations<<",\"completed_calls\":"<<calls
                 <<",\"threads\":1,\"warmups\":8,\"cold_calls\":1,\"empty_calls\":0,\"empty_boundary\":";
        shorthand::event_profile::json(std::cout,empty_events);
        std::cout<<",\"cold_boundary\":"; shorthand::event_profile::json(std::cout,cold_events);
        std::cout<<",\"warm_blocks\":[";
        for(std::size_t i=0;i<event_blocks.size();++i) {
            if(i) std::cout<<',';
            shorthand::event_profile::json(std::cout,event_blocks[i]);
        }
        std::cout<<"]}\n";
#else
        std::cout<<std::setprecision(17)<<"{\"schema\":\"shorthand.generated_infer.sample.v2\",\"success\":true,\"cold_session_ms\":"<<cold
                 <<",\"observation_mode\":\""<<(observe?"every_call":"last_call_after_timing")<<"\""
                 <<",\"completed_calls\":"<<calls<<",\"completed_vectors\":"<<calls*batch
                 <<",\"iterations\":"<<iterations<<",\"batch\":"<<batch<<",\"threads\":1,\"warmups\":8,\"block_elapsed_ms\":[";
        for(std::size_t i=0;i<elapsed.size();++i) std::cout<<(i?",":"")<<elapsed[i];
        std::cout<<"],\"last_runtime_telemetry\":";
#if SHORTHAND_GENERATED
        std::cout<<short_runtime_last_infer_telemetry_json();
#else
        std::cout<<"null";
#endif
        std::cout<<",\"configuration\":{\"graph_optimization\":\""<<(SHORTHAND_ORT_ALL?"all":"basic")
                 <<"\",\"execution_mode\":\"sequential\",\"spinning\":false,\"intra_threads\":1,\"inter_threads\":1}"
                 <<",\"instrumented\":";
#if SHORTHAND_RUNTIME_PHASE_PROFILE
        std::cout<<"true,\"clock_pair_median_ns\":"<<clock_pairs[clock_pairs.size()/2]<<",\"phase_samples\":[";
        for(std::size_t i=0;i<profiles.size();++i) {
            const auto &p=profiles[i];
            std::cout<<(i?",":"")<<"{\"total_ns\":"<<p.total_ns<<",\"clock_reads\":"<<p.clock_reads<<",\"phases\":{";
            for(std::size_t k=0;k<p.ns.size();++k)
                std::cout<<(k?",":"")<<'"'<<shorthand::runtime_profile::names[k]<<"\":"<<p.ns[k];
            std::cout<<"},\"visits\":{";
            for(std::size_t k=0;k<p.visits.size();++k)
                std::cout<<(k?",":"")<<'"'<<shorthand::runtime_profile::names[k]<<"\":"<<p.visits[k];
            std::cout<<"}}";
        }
        std::cout<<"]";
#else
        std::cout<<"false";
#endif
        std::cout<<"}\n";
#endif
    } catch(const std::exception &e) { std::cerr<<e.what()<<"\n"; return 1; }
}
