#include "aot/AotSupport.h"
#include "ai_runtime/ProductionBackendQualification.h"
#include <atomic>
#include <cassert>
#include <cfenv>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <thread>
#include <vector>

static std::atomic<bool> track{false};
static std::atomic<unsigned> allocations{0};
void *operator new(std::size_t n) {
    if(track.load()) ++allocations;
    if(void *p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] void operator delete(void *p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete[](void *p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void *p,std::size_t) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete[](void *p,std::size_t) noexcept { std::free(p); }
static void env(const char *name,const char *value) {
#ifdef _WIN32
    _putenv_s(name,value?value:"");
#else
    if(value) setenv(name,value,1); else unsetenv(name);
#endif
}
int main() {
    using namespace shorthand::ai;
    for(const char *key:hardwareRoutingPolicyEnvironmentNames()) env(key,nullptr);
    env("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE",nullptr);
    float input[4]={1,2,3,4}, output[4]={9,9,9,9}, work[4]={5,6,7,8};
    auto prepare=[&] {return sh_aot_v1_prepare(input,4,output,4,work,4,4,4);};
    assert(prepare()==(productionBackendPlatformQualified()?0:3));
    env("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE","true");
    assert(prepare()==0);
    assert(sh_aot_v1_prepare(input,4,input,4,work,4,4,4)==0);
    float partial[6]={1,2,3,4,5,6};
    assert(sh_aot_v1_prepare(partial,4,partial+1,4,work,4,4,4)==0);
    assert(sh_aot_v1_prepare(input,4,output,4,input,4,4,4)==1);
    assert(sh_aot_v1_prepare(input,4,output,4,output,4,4,4)==1);
    assert(sh_aot_v1_prepare(nullptr,4,output,4,work,4,4,4)==1);
    assert(sh_aot_v1_prepare(input,0,output,4,work,4,0,4)==1);
    assert(sh_aot_v1_prepare(input,4,output,4,work,3,4,4)==1);
    assert(sh_aot_v1_prepare(input,4,output,4,work,4,3,4)==1);
    assert(sh_aot_v1_prepare(input,UINT64_MAX,output,4,work,4,UINT64_MAX,4)==1);
    assert(sh_aot_v1_prepare(reinterpret_cast<float *>(UINTPTR_MAX-3),4,output,4,work,4,4,4)==1);
    assert(sh_aot_v1_prepare(reinterpret_cast<float *>(reinterpret_cast<char *>(input)+1),4,output,4,work,4,4,4)==1);
    for(float value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        input[2]=value; assert(prepare()==2); input[2]=3;
        work[2]=value; assert(sh_aot_v1_commit(output,4,work,4)==2); assert(output[0]==9 && output[3]==9); work[2]=7;
    }
    assert(sh_aot_v1_commit(output,4,work,3)==1);
    assert(sh_aot_v1_commit(work,4,work,4)==1);
    for(const char *deny:{"cpu","CPU","gpu,cpu"}) {env("SHORTHAND_DEVICE_DENY",deny);assert(prepare()==3);}
    env("SHORTHAND_DEVICE_DENY",nullptr);
    env("SHORTHAND_ALLOW_CPU_FALLBACK","false"); assert(prepare()==3);
    env("SHORTHAND_DEVICE_OVERRIDE","cpu"); assert(prepare()==3);
    env("SHORTHAND_ALLOW_CPU_FALLBACK","true"); assert(prepare()==0);
    env("SHORTHAND_DEVICE_OVERRIDE","gpu"); assert(prepare()==0);
    env("SHORTHAND_ALLOW_CPU_FALLBACK","false"); assert(prepare()==3);
    for(const char *key:hardwareRoutingPolicyEnvironmentNames()) env(key,nullptr);
    env("SHORTHAND_MIN_DEVICE_MEMORY_MB","18446744073709551615"); assert(prepare()==3);
    env("SHORTHAND_MIN_DEVICE_MEMORY_MB",nullptr);
    std::fesetround(FE_DOWNWARD); assert(prepare()==4); std::fesetround(FE_TONEAREST);
    // Default CPU policy is refreshed every call, with no heap allocation.
    allocations=0; track=true;
    for(int i=0;i<1000;++i) { assert(prepare()==0); assert(sh_aot_v1_commit(output,4,work,4)==0); }
    track=false; assert(allocations==0);
    // Live allocation positive control guards the instrumentation itself.
    track=true; auto *control=::operator new(13); track=false; ::operator delete(control); assert(allocations>0);
    std::vector<std::thread> threads;
    for(int t=0;t<4;++t) threads.emplace_back([] {
        float in[4]={1,2,3,4},out[4]={},scratch[4]={4,3,2,1};
        for(int i=0;i<1000;++i) {
            assert(sh_aot_v1_prepare(in,4,out,4,scratch,4,4,4)==0);
            assert(sh_aot_v1_commit(out,4,scratch,4)==0 && out[0]==4 && out[3]==1);
        }
    });
    for(auto &thread:threads) thread.join();
    puts("PASS native AOT support: regions, rollback, policy refresh, FP environment, allocation control and concurrency");
}
