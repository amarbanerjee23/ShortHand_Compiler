// Same synthetic FP32 MatMul+Add workload through compiled .short and direct
// prepared ORT. All output validation is inside BOTH timed boundaries.
#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
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
        options.SetExecutionMode(ORT_SEQUENTIAL); options.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
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
        check(argc==6,"usage: probe model batch expected.bin iterations blocks");
        const int batch=std::stoi(argv[2]),iterations=std::stoi(argv[4]),blocks=std::stoi(argv[5]);
        check((batch==1||batch==16||batch==32) && iterations>0 && iterations<=4096 && blocks>=2 && blocks<=20,"invalid_protocol");
        expected.resize(batch*10); std::ifstream file(argv[3],std::ios::binary);
        file.read(reinterpret_cast<char *>(expected.data()),expected.size()*sizeof(float));
        check(bool(file) && file.peek()==std::char_traits<char>::eof(),"invalid_oracle");
        for(float v:expected) check(std::isfinite(v),"nonfinite_oracle");
        const auto cold_start=Clock::now();
#if SHORTHAND_GENERATED
        short_runtime_reset();
        auto once=[] { check(shorthand_entry()==0,"generated_entry_failed"); };
#else
        Direct direct(argv[1],batch); auto once=[&] { direct.run(); };
#endif
        once(); const double cold=ms(cold_start);
        for(int i=0;i<8;++i) once();
        calls=0; std::vector<double> elapsed;
        for(int block=0;block<blocks;++block) {
            const auto begin=Clock::now();
            for(int i=0;i<iterations;++i) once();
            elapsed.push_back(ms(begin));
        }
        check(calls==std::uint64_t(iterations)*blocks,"incomplete_execution");
        std::cout<<std::setprecision(17)<<"{\"schema\":\"shorthand.generated_infer.sample.v1\",\"success\":true,\"cold_session_ms\":"<<cold
                 <<",\"completed_calls\":"<<calls<<",\"completed_vectors\":"<<calls*batch
                 <<",\"iterations\":"<<iterations<<",\"batch\":"<<batch<<",\"threads\":1,\"warmups\":8,\"block_elapsed_ms\":[";
        for(std::size_t i=0;i<elapsed.size();++i) std::cout<<(i?",":"")<<elapsed[i];
        std::cout<<"],\"last_runtime_telemetry\":";
#if SHORTHAND_GENERATED
        std::cout<<short_runtime_last_infer_telemetry_json();
#else
        std::cout<<"null";
#endif
        std::cout<<"}\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<"\n"; return 1; }
}
