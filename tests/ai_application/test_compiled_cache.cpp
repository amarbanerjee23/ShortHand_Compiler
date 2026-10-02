#include "runtime/ShorthandRuntime.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

void check(bool condition,const char *reason) { if (!condition) throw std::runtime_error(reason); }
void replace(const char *source,const char *destination) {
    std::ifstream in(source,std::ios::binary); std::ofstream out(destination,std::ios::binary|std::ios::trunc);
    out<<in.rdbuf(); check(bool(out),"fixture_copy");
}
void registerModel(const char *path,const char *task="identity") {
    check(!short_ai_register_model("m","onnx",path,task,"float32","1,1","1,1","onnxruntime_cpu"),"register_model");
    check(!short_ai_register_tensor("x","float32","1,1","2","1"),"register_input");
    check(!short_ai_register_tensor("y","float32","1,1","2","1"),"register_output");
}
void infer(float value,float expected,bool success=true) {
    float output=-999; int count=-1;
    const int status=short_ai_infer_f32("m","x",&value,1,"y",&output,1,&count);
    check((status==0)==success,"inference_status");
    check(success?(count==1 && std::abs(output-expected)<1e-5f):(count==0 && output==-999),"output_count_or_value");
}
bool telemetry(const char *s) { return std::strstr(short_runtime_last_infer_telemetry_json(),s); }

void argumentContracts() {
    float input=5, output[3]={-100,-999,-200}; int count=-1;
    auto rejects=[&](const char *model,const char *in,const float *data,int size,
                     const char *out,float *target,int capacity,int *written) {
        count=-1;
        check(short_ai_infer_f32(model,in,data,size,out,target,capacity,written)!=0,"bad_argument_accepted");
        check(output[0]==-100 && output[1]==-999 && output[2]==-200,"failed_call_overwrote_output");
        check(!written || count==0,"failed_call_count_not_zero");
    };
    rejects(nullptr,"x",&input,1,"y",output+1,1,&count);
    rejects("","x",&input,1,"y",output+1,1,&count);
    rejects("missing","x",&input,1,"y",output+1,1,&count);
    rejects("m",nullptr,&input,1,"y",output+1,1,&count);
    rejects("m","missing",&input,1,"y",output+1,1,&count);
    rejects("m","x",nullptr,1,"y",output+1,1,&count);
    for(int size:{-1,0,2}) rejects("m","x",&input,size,"y",output+1,1,&count);
    rejects("m","x",&input,1,nullptr,output+1,1,&count);
    rejects("m","x",&input,1,"missing",output+1,1,&count);
    rejects("m","x",&input,1,"y",nullptr,1,&count);
    for(int capacity:{-1,0}) rejects("m","x",&input,1,"y",output+1,capacity,&count);
    rejects("m","x",&input,1,"y",output+1,1,nullptr);
    check(!short_ai_infer_f32("m","x",&input,1,"y",output+1,2,&count),"spare_capacity_infer");
    check(count==1 && output[0]==-100 && output[1]==5 && output[2]==-200,"output_canaries");
    check(!short_ai_infer_f32("m","x",&input,1,"y",&input,1,&count) && input==5,"aliased_input_output");
    infer(-std::numeric_limits<float>::infinity(),0,false);
    infer(std::numeric_limits<float>::max(),std::numeric_limits<float>::max());
    check(output[1]==5,"caller_output_lifetime");
    std::cout<<"PASS cache argument matrix, output canaries, aliasing and caller ownership\n";
}

void vectorBoundary(const std::filesystem::path &dir,int size) {
    short_runtime_reset();
    const std::string path=(dir/("vector-"+std::to_string(size)+".onnx")).string();
    const std::string shape="1,"+std::to_string(size), countText=std::to_string(size);
    check(!short_ai_register_model("m","onnx",path.c_str(),"identity","float32",shape.c_str(),shape.c_str(),"onnxruntime_cpu"),"vector_model");
    for(const char *name:{"x","y"})
        check(!short_ai_register_tensor(name,"float32",shape.c_str(),"2",countText.c_str()),"vector_tensor");
    std::vector<float> input(size), output(size+2,-999);
    for(int turn=0;turn<3;++turn) {
        for(int i=0;i<size;++i) input[i]=float((i+turn)%101-50);
        int written=-1;
        check(!short_ai_infer_f32("m","x",input.data(),size,"y",output.data()+1,size,&written),"vector_infer");
        check(written==size && output.front()==-999 && output.back()==-999,"vector_canaries");
        for(int i=0;i<size;++i) check(input[i]==output[i+1],"vector_value_or_stale_scratch");
    }
    check(telemetry("\"hit\":true") && telemetry("\"preparations\":1"),"vector_preparation_count");
    std::cout<<"PASS cache bridge input boundary "<<size<<" floats\n";
}

void snapshotBoundary(const std::filesystem::path &dir) {
    for(const auto &name:{"limit.onnx","over-limit.onnx"}) {
        short_runtime_reset(); const std::string path=(dir/name).string(); registerModel(path.c_str());
        infer(11,11); infer(-3,-3);
        if(std::string(name)=="limit.onnx")
            check(telemetry("\"hit\":true") && telemetry("\"preparations\":1"),"snapshot_at_limit_not_cached");
        else check(!telemetry("shorthand.prepared_cache"),"oversize_snapshot_cached");
    }
    std::cout<<"PASS cache snapshot limit at 16 MiB and uncached 16 MiB plus one byte\n";
}

void lifecycleStress(const char *identity,const char *negative) {
    short_runtime_reset(); registerModel(identity);
    std::promise<void> ready; auto start=ready.get_future().share();
    std::atomic<int> completed{0}, rejected{0};
    std::vector<std::future<void>> workers;
    for(int worker=0;worker<4;++worker) workers.push_back(std::async(std::launch::async,[&,worker]{
        start.wait();
        for(int n=0;n<1000;++n) {
            const float input=float(worker+n+1); float output=-999; int count=-1;
            const int status=short_ai_infer_f32("m","x",&input,1,"y",&output,1,&count);
            if(status==SHORTHAND_RUNTIME_OK) {
                check(count==1 && (output==input || output==-input),"concurrent_lifecycle_torn_output");
                ++completed;
            } else {
                check(status==SHORTHAND_RUNTIME_MODEL_NOT_FOUND || status==SHORTHAND_RUNTIME_TENSOR_NOT_FOUND ||
                      status==SHORTHAND_RUNTIME_OUTPUT_TENSOR_NOT_FOUND,"unexpected_lifecycle_failure");
                check(count==0 && output==-999,"concurrent_failure_output_mutated"); ++rejected;
            }
        }
    }));
    workers.push_back(std::async(std::launch::async,[&]{
        start.wait();
        for(int n=0;n<64;++n) {
            short_runtime_reset(); registerModel(n%2?negative:identity);
            infer(2,n%2?-2:2);
        }
    }));
    ready.set_value(); for(auto &worker:workers) worker.get();
    check(completed+rejected==4000 && completed>0,"incomplete_lifecycle_stress");
    short_runtime_reset(); registerModel(identity); infer(3,3); infer(4,4);
    check(telemetry("\"hit\":true") && telemetry("\"preparations\":1"),"lifecycle_recovery");
    std::cout<<"PASS cache concurrent lifecycle: 4000 calls, 64 resets; success="<<completed<<" registration_gap="<<rejected<<"\n";
}
int main(int argc,char **argv) {
    try {
        check(argc==5,"usage: test identity negate external mutable");
        replace(argv[1],argv[4]); short_runtime_reset(); registerModel(argv[4]);
        infer(42,42); check(telemetry("\"hit\":false"),"first_preparation");
        registerModel(argv[4]); infer(7,7);
        check(telemetry("\"hit\":true") && telemetry("\"preparations\":1"),"idempotent_registration_reuses");
        check(telemetry("hardware_inventory") && telemetry("hardware_selection"),"routing_evidence_preserved");
        registerModel(argv[4],"renamed_task"); infer(7,7);
        check(telemetry("\"hit\":false") && telemetry("\"preparations\":2"),"changed_registration_invalidates");
        const auto stamp=std::filesystem::last_write_time(argv[4]);
        replace(argv[2],argv[4]); std::filesystem::last_write_time(argv[4],stamp);
        infer(7,-7); check(telemetry("\"hit\":false"),"content_change_with_preserved_mtime");
        { std::ofstream bad(argv[4],std::ios::binary|std::ios::trunc); bad<<"corrupt"; }
        infer(7,0,false);
        replace(argv[1],argv[4]); infer(7,7); // Failures do not poison recovery.
        std::filesystem::remove(argv[4]); infer(7,0,false);
        replace(argv[1],argv[4]); infer(7,7);
        infer(std::numeric_limits<float>::quiet_NaN(),0,false);
        infer(std::numeric_limits<float>::infinity(),0,false);
        infer(7,7);
        check(!setenv("SHORTHAND_DEVICE_DENY","cpu",1),"set_policy");
        infer(7,0,false); // A cached session must not bypass current routing policy.
        check(!unsetenv("SHORTHAND_DEVICE_DENY"),"restore_policy");
        infer(7,7);
        float input=3,output=-999; int count=-1;
        check(short_ai_infer_f32("m","x",&input,1,"y",&output,0,&count)!=0 && count==0 && output==-999,"invalid_capacity");
        short_ai_register_tensor("y","float32","1,2","2","2"); infer(3,0,false);
        registerModel(argv[4]); infer(3,3);
        argumentContracts();
        for(int n=0;n<2000;++n) {
            if(n%100==0) registerModel(argv[4]);
            infer(float(n%97-48),float(n%97-48));
        }
        check(telemetry("\"hit\":true"),"sequential_soak_lost_cache");
        std::cout<<"PASS cache sequential soak: 2000 calls with idempotent re-registration\n";
        std::vector<std::future<void>> workers;
        for(int i=0;i<4;++i) workers.push_back(std::async(std::launch::async,[i]{for(int n=0;n<25;++n) infer(float(i+n),float(i+n));}));
        for(auto &worker:workers) worker.get();
        short_runtime_reset(); registerModel(argv[4]); infer(3,3);
        check(telemetry("\"preparations\":1"),"reset_releases_cache");
        // External-data models must use path-based execution on every request.
        registerModel(argv[3]); infer(3,6);
        const auto weights=std::filesystem::path(argv[3]).parent_path()/"weights.bin";
        { std::ofstream file(weights,std::ios::binary|std::ios::trunc); float v=4; file.write(reinterpret_cast<const char *>(&v),sizeof(v)); }
        infer(3,12); check(!telemetry("shorthand.prepared_cache"),"external_weights_not_cached");
        std::filesystem::remove(weights); infer(3,0,false);
        { std::ofstream file(weights,std::ios::binary|std::ios::trunc); float v=5; file.write(reinterpret_cast<const char *>(&v),sizeof(v)); }
        infer(3,15); check(!telemetry("shorthand.prepared_cache"),"external_recovery_cached");
        std::cout<<"PASS cache external-weight deletion and recovery\n";
        lifecycleStress(argv[1],argv[2]);
        const auto fixtures=std::filesystem::path(argv[1]).parent_path();
        vectorBoundary(fixtures,65536); vectorBoundary(fixtures,65537);
        snapshotBoundary(fixtures);
        short_runtime_reset(); registerModel(argv[1]); infer(9,9);
        check(telemetry("\"preparations\":1"),"boundary_recovery");
        std::cout<<"PASS extended ONNX cache boundaries and lifecycle stress\n";
        std::cout<<"PASS real ONNX cache reuse, changed registrations/content, reset, failure recovery, finite/capacity checks, concurrency, external weights\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<"\n"; return 1; }
}
