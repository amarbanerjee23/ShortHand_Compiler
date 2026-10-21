// The same prepared C++ host and oracle for AOT and both direct ORT controls.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#if SHORTHAND_AOT
#include "model.h"
#else
#include <onnxruntime_cxx_api.h>
#endif

namespace {
using Clock=std::chrono::steady_clock;
void check(bool ok,const char *reason) { if(!ok) throw std::runtime_error(reason); }
std::vector<float> read(const char *path,std::size_t count) {
    std::vector<float> values(count);
    std::ifstream in(path,std::ios::binary);
    in.read(reinterpret_cast<char*>(values.data()),count*sizeof(float));
    check(bool(in) && in.peek()==std::char_traits<char>::eof(),"bad fixture byte count");
    for(float v:values) check(std::isfinite(v),"nonfinite fixture");
    return values;
}
double ms(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
}
int main(int argc,char **argv) {
    try {
        check(argc==8,"probe model input expected batch inputs outputs iterations");
        const int m=std::stoi(argv[4]),k=std::stoi(argv[5]),n=std::stoi(argv[6]),iterations=std::stoi(argv[7]);
        check(m>0 && m<=32 && k>0 && k<=1024 && n>0 && n<=1024 && iterations>0 && iterations<=65536,"bad protocol");
        auto input=read(argv[2],m*k),expected=read(argv[3],m*n);
        std::vector<float> output(m*n),workspace(m*n);
        const auto cold=Clock::now();
#if !SHORTHAND_AOT
        Ort::Env env(ORT_LOGGING_LEVEL_ERROR,"aot_control");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1); options.SetInterOpNumThreads(1);
        options.SetExecutionMode(ORT_SEQUENTIAL);
        options.SetGraphOptimizationLevel(SHORTHAND_ORT_ALL?ORT_ENABLE_ALL:ORT_ENABLE_BASIC);
        options.AddConfigEntry("session.intra_op.allow_spinning","0");
        options.AddConfigEntry("session.inter_op.allow_spinning","0");
        Ort::Session session(env,argv[1],options);
        auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
        const std::int64_t in_shape[]={m,k},out_shape[]={m,n};
        auto in=Ort::Value::CreateTensor<float>(memory,input.data(),input.size(),in_shape,2);
        auto out=Ort::Value::CreateTensor<float>(memory,output.data(),output.size(),out_shape,2);
        const char *in_names[]={"X"},*out_names[]={"Y"};
#endif
        std::uint64_t completed=0;
        const auto once=[&] {
            for(float v:input) check(std::isfinite(v),"nonfinite input");
#if SHORTHAND_AOT
            check(sh_aot_infer(input.data(),input.size(),output.data(),output.size(),workspace.data(),workspace.size())==0,"AOT rejected input");
#else
            session.Run(Ort::RunOptions{nullptr},in_names,&in,1,out_names,&out,1);
#endif
            for(std::size_t i=0;i<output.size();++i)
                check(std::isfinite(output[i]) && std::abs(double(output[i])-expected[i])<=1e-5+1e-4*std::abs(double(expected[i])),"numerical mismatch");
            ++completed;
        };
        once(); const double cold_ms=ms(cold);
        for(int i=0;i<8;++i) once();
        const auto start=Clock::now();
        for(int i=0;i<iterations;++i) once();
        const double elapsed=ms(start);
        check(completed==std::uint64_t(iterations)+9,"incomplete calls");
        std::cout<<std::setprecision(17)<<"{\"schema\":\"shorthand.aot.sample.v1\",\"success\":true,"
                 <<"\"runner\":\""<<(SHORTHAND_AOT?"aot":(SHORTHAND_ORT_ALL?"ort_all":"ort_basic"))<<"\","
                 <<"\"batch\":"<<m<<",\"inputs\":"<<k<<",\"outputs\":"<<n<<",\"iterations\":"<<iterations
                 <<",\"warmups\":8,\"completed_calls\":"<<completed<<",\"cold_ms\":"<<cold_ms
                 <<",\"elapsed_ms\":"<<elapsed<<",\"verified_vectors\":"<<completed*m<<"}\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
