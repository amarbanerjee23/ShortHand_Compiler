#include "../../Compiler_new_ws/Short_Hand/src/runtime/ImmutableModelInternal.h"
#include "../../Compiler_new_ws/Short_Hand/src/runtime/ShorthandRuntime.h"
#include "../../Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace iv=shorthand::runtime::immutable_v1;
namespace ai=shorthand::ai;
using Path=std::filesystem::path;
void check(bool good,const char *why) { if(!good) throw std::runtime_error(why); }
std::string read(const Path &path) {
    std::ifstream file(path,std::ios::binary);
    check(bool(file),"fixture_open");
    return {std::istreambuf_iterator<char>(file),{}};
}
ai::ModelSpec spec() {
    ai::ModelSpec model;
    model.name="owned"; model.path="deliberately-missing.onnx";
    model.format=ai::ModelFormat::Onnx; model.precision="float32";
    model.backend_preference={ai::BackendKind::OnnxRuntimeCPU}; model.allow_fallback=false;
    model.input.name="X"; model.input.element_type=ai::ElementType::Float32;
    model.input.shape={1,4}; model.input.element_count=4;
    model.output=model.input; model.output.name="Y";
    return model;
}
iv::Binding bindFile(const Path &path) {
    const auto bytes=read(path), digest=read(path.string()+".sha256");
    return iv::bind(spec(),bytes.data(),bytes.size(),digest);
}
void infer(iv::Handle handle,float sign=1) {
    const float in[]={3,-7,1.25f,-0.125f}; float out[6]={-111,-999,-999,-999,-999,-222};
    const auto result=iv::infer(handle,in,4,out+1,4);
    check(result.status==ai::InferenceStatus::Success && result.output_elements==4,"immutable_numerics_status");
    check(out[0]==-111 && out[5]==-222,"immutable_output_canaries");
    for(int n=0;n<4;++n) check(out[n+1]==sign*in[n],"immutable_numerics_value");
    check(result.evidence_json_fragment.find("shorthand.immutable_model.v1")!=std::string::npos,"immutable_identity_evidence");
}
void stale(iv::Handle handle) {
    float in[4]={1,2,3,4},out[4]={8,8,8,8};
    auto result=iv::infer(handle,in,4,out,4);
    check(result.status==ai::InferenceStatus::InvalidInput && result.reason=="stale_immutable_handle" &&
          result.output_elements==0 && out[0]==8 && out[3]==8,"stale_handle_not_rejected");
}
void setDeny(const char *value) {
#ifdef _WIN32
    check(_putenv_s("SHORTHAND_DEVICE_DENY",value?value:"")==0,"policy_environment");
#else
    check((value?setenv("SHORTHAND_DEVICE_DENY",value,1):unsetenv("SHORTHAND_DEVICE_DENY"))==0,"policy_environment");
#endif
}
void contracts(const Path &dir) {
    short_runtime_reset(); stale({});
    auto model=spec(); auto bytes=read(dir/"identity.onnx");
    const auto digest=read(dir/"identity.onnx.sha256");
    auto bound=iv::bind(model,bytes.data(),bytes.size(),digest);
    check(bound.handle.generation!=0,bound.error.c_str());
    std::fill(bytes.begin(),bytes.end(),'x'); bytes.clear(); bytes.shrink_to_fit();
    model.input.shape={99}; model.path="another-missing.onnx";
    infer(bound.handle); // Neither the original bytes nor ModelSpec remain authoritative.
    auto rejects=[&](const ai::ModelSpec &m,const void *data,std::size_t size,const std::string &hash) {
        const auto failed=iv::bind(m,data,size,hash);
        check(!failed.handle.generation && !failed.error.empty(),"invalid_binding_accepted");
        infer(bound.handle); // Failed replacement must preserve old ownership.
    };
    bytes=read(dir/"identity.onnx");
    rejects(spec(),nullptr,bytes.size(),digest);
    rejects(spec(),bytes.data(),0,digest);
    rejects(spec(),bytes.data(),16U*1024U*1024U+1,digest); // Bounds checked before dereferencing.
    rejects(spec(),bytes.data(),bytes.size(),std::string(64,'0'));
    rejects(spec(),bytes.data(),bytes.size(),"abc");
    rejects(spec(),bytes.data(),bytes.size(),std::string(64,'A'));
    for(int n=0;n<7;++n) {
        auto invalid=spec();
        if(n==0) invalid.allow_fallback=true;
        if(n==1) invalid.precision="float16";
        if(n==2) invalid.input.shape={1,5};
        if(n==3) invalid.output.shape={1,-1};
        if(n==4) invalid.backend_preference={ai::BackendKind::Fallback};
        if(n==5) invalid.input.element_count=0;
        if(n==6) invalid.output.dynamic=true;
        rejects(invalid,bytes.data(),bytes.size(),digest);
    }
    unsigned rejected=0;
    for(const auto &entry:std::filesystem::directory_iterator(dir)) {
        const auto name=entry.path().filename().string();
        if(name.rfind("reject-",0)!=0 || entry.path().extension()!=".onnx") continue;
        const auto bad=read(entry.path());
        rejects(spec(),bad.data(),bad.size(),read(entry.path().string()+".sha256"));
        ++rejected;
    }
    check(rejected>=15,"missing_envelope_negatives");
    float in[4]={1,2,3,4},out[6]={8,8,8,8,8,8};
    auto ioRejects=[&](const float *data,std::size_t count,float *target,std::size_t capacity) {
        const auto result=iv::infer(bound.handle,data,count,target,capacity);
        check(result.status!=ai::InferenceStatus::Success && result.output_elements==0,"invalid_io_accepted");
        for(float value:out) check(value==8,"rejection_changed_output");
    };
    ioRejects(nullptr,4,out+1,4); ioRejects(in,0,out+1,4); ioRejects(in,3,out+1,4);
    ioRejects(in,4,nullptr,4); ioRejects(in,4,out+1,3); ioRejects(in,4,out+1,5);
    ioRejects(reinterpret_cast<const float *>(reinterpret_cast<const char *>(in)+1),4,out+1,4);
    ioRejects(in,4,reinterpret_cast<float *>(reinterpret_cast<char *>(out)+1),4);
    in[0]=std::numeric_limits<float>::quiet_NaN(); ioRejects(in,4,out+1,4);
    in[0]=std::numeric_limits<float>::infinity(); ioRejects(in,4,out+1,4);
    float overlap[6]={1,-2,3,-4,99,100};
    check(iv::infer(bound.handle,overlap,4,overlap+1,4).status==ai::InferenceStatus::Success,"partial_overlap_status");
    check(overlap[1]==1 && overlap[2]==-2 && overlap[3]==3 && overlap[4]==-4 && overlap[5]==100,"partial_overlap_values");
    check(iv::infer(bound.handle,overlap+1,4,overlap+1,4).status==ai::InferenceStatus::Success,"exact_overlap");
    const char *original=std::getenv("SHORTHAND_DEVICE_DENY");
    const std::string saved=original?original:"";
    setDeny("cpu");
    const auto denied=iv::infer(bound.handle,overlap,4,out+1,4);
    setDeny(original?saved.c_str():nullptr);
    check(denied.status==ai::InferenceStatus::BackendUnavailable && out[1]==8,"immutable_policy_bypassed");
    infer(bound.handle);
    auto changed=bindFile(dir/"negative.onnx"); check(changed.handle.generation>bound.handle.generation,"generation_not_advanced");
    stale(bound.handle); infer(changed.handle,-1);
    check(!iv::release(bound.handle),"stale_release_removed_new_slot"); infer(changed.handle,-1);
    check(iv::release(changed.handle) && !iv::release(changed.handle),"release_lifecycle"); stale(changed.handle);
    auto limit=bindFile(dir/"limit.onnx"); check(limit.handle.generation!=0,limit.error.c_str()); infer(limit.handle);
    short_runtime_reset(); stale(limit.handle);
    auto overflow=bindFile(dir/"overflow.onnx"); check(overflow.handle.generation!=0,overflow.error.c_str());
    float extreme[4]={std::numeric_limits<float>::max(),1,2,3};
    const auto nonfinite=iv::infer(overflow.handle,extreme,4,out+1,4);
    check(nonfinite.status==ai::InferenceStatus::RuntimeError && nonfinite.output_elements==0 && out[1]==8,"nonfinite_output_rollback");
    short_runtime_reset();
    std::atomic<std::uint64_t> current{0}; std::atomic<int> successes{0},rejections{0};
    auto first=bindFile(dir/"identity.onnx"); check(first.handle.generation!=0,first.error.c_str()); current=first.handle.generation;
    std::promise<void> start; auto ready=start.get_future().share();
    std::vector<std::future<void>> workers;
    for(int worker=0;worker<4;++worker) workers.push_back(std::async(std::launch::async,[&]{
        ready.wait();
        for(int n=0;n<500;++n) {
            float x[4]={1,-2,3,-4}, y[4]={8,8,8,8};
            const auto r=iv::infer({current.load()},x,4,y,4);
            if(r.status==ai::InferenceStatus::Success) {
                check(r.output_elements==4 && (y[0]==1 || y[0]==-1),"concurrent_output_status");
                const float sign=y[0];
                for(int k=0;k<4;++k) check(y[k]==sign*x[k],"concurrent_torn_output");
                ++successes;
            } else { check(r.reason=="stale_immutable_handle" && y[0]==8 && y[3]==8,"concurrent_reset_rejection"); ++rejections; }
        }
    }));
    start.set_value();
    for(int turn=0;turn<32;++turn) {
        short_runtime_reset();
        const auto next=bindFile(dir/(turn%2?"negative.onnx":"identity.onnx"));
        check(next.handle.generation!=0,next.error.c_str()); current=next.handle.generation;
        infer(next.handle,turn%2?-1:1);
    }
    for(auto &worker:workers) worker.get();
    check(successes+rejections==2000,"incomplete_reset_stress");
    infer({current.load()},-1); short_runtime_reset(); stale({current.load()});
    std::cout<<"PASS immutable ownership, digest, envelope, rollback, policy and stale handles; rejected="<<rejected<<"\n";
    std::cout<<"PASS immutable concurrent lifecycle: 2000 calls, 32 resets; success="<<successes<<" stale="<<rejections<<"\n";
}

void probe(const std::string &mode,const Path &path,int iterations) {
    check(mode=="immutable" || mode=="mutable" || mode=="injected-read","invalid_probe_mode");
    check(iterations>0 && iterations<=1000000,"invalid_iterations");
    const auto bytes=read(path),digest=read(path.string()+".sha256");
    auto model=spec(); model.path=path.string();
    ai::AIRuntime runtime; ai::PreparedInferenceCache cache;
    ai::TensorBuffer input; input.spec=model.input; input.f32_data={3,-7,1.25f,-0.125f};
    float scratch[4]={}, output[4]={}; iv::Binding binding;
    const auto begin=std::chrono::steady_clock::now();
    if(mode!="mutable") { binding=iv::bind(model,bytes.data(),bytes.size(),digest); check(binding.handle.generation!=0,binding.error.c_str()); }
    auto run=[&] {
        if(mode=="injected-read") { std::ifstream file(path,std::ios::binary); check(file.get()>=0,"injected_read_failed"); }
        ai::InferenceResult result;
        if(mode=="mutable") {
            runtime.refreshPolicyFromEnvironment();
            result=runtime.inferCachedInto(model,input,cache,scratch,4);
            if(result.status==ai::InferenceStatus::Success) {
                for(float v:scratch) check(std::isfinite(v),"nonfinite_control_output");
                std::copy(scratch,scratch+4,output);
            }
        } else result=iv::infer(binding.handle,input.f32_data.data(),4,output,4);
        check(result.status==ai::InferenceStatus::Success && result.output_elements==4,"probe_execution_failed");
        for(int k=0;k<4;++k) check(output[k]==input.f32_data[k],"probe_oracle_failed");
    };
    run();
    const auto prepared=std::chrono::steady_clock::now();
    for(int warm=0;warm<8;++warm) run();
    std::cout<<"MODEL_WARM_BEGIN\n"<<std::flush;
    const auto start=std::chrono::steady_clock::now();
    for(int i=0;i<iterations;++i) run();
    const auto finish=std::chrono::steady_clock::now();
    std::cout<<"MODEL_WARM_END\n"<<std::flush;
    const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
    const auto cold=std::chrono::duration_cast<std::chrono::nanoseconds>(prepared-begin).count();
    std::cout<<"{\"mode\":\""<<mode<<"\",\"iterations\":"<<iterations
             <<",\"verified_calls\":"<<iterations+9<<",\"warm_ns\":"<<ns<<",\"cold_ns\":"<<cold
             <<",\"mutable_preparations\":"<<cache.preparations()<<"}\n";
    short_runtime_reset();
}
int main(int argc,char **argv) {
    try {
        if(argc==3 && std::string(argv[1])=="contracts") contracts(argv[2]);
        else if(argc==4) probe(argv[1],argv[2],std::stoi(argv[3]));
        else throw std::runtime_error("usage: test contracts DIR | MODE MODEL ITERATIONS");
        return 0;
    } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
