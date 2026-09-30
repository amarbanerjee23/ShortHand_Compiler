#include "runtime/ShorthandRuntime.h"
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
        std::cout<<"PASS real ONNX cache reuse, changed registrations/content, reset, failure recovery, finite/capacity checks, concurrency, external weights\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<"\n"; return 1; }
}
