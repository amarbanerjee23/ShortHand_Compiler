#include "AotSupport.h"
#include "../ai_runtime/ProductionBackendQualification.h"
#include <cfenv>
#include <cstring>
#include <limits>

namespace {
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,
              "AOT v1 requires IEEE-754 binary32");
struct Region { uintptr_t begin=0,end=0; };
bool region(const void *pointer,uint64_t count,Region &r) {
    if(!pointer || count==0 || count>65536) return false;
    r.begin=reinterpret_cast<uintptr_t>(pointer);
    if(r.begin%alignof(float) || count>(UINTPTR_MAX-r.begin)/sizeof(float)) return false;
    r.end=r.begin+static_cast<uintptr_t>(count)*sizeof(float);
    return true;
}
bool overlaps(const Region &a,const Region &b) { return a.begin<b.end && b.begin<a.end; }
bool finite(const float *values,uint64_t count) {
    for(uint64_t i=0;i<count;++i) {
        uint32_t bits; std::memcpy(&bits,values+i,sizeof(bits));
        if((bits&0x7f800000U)==0x7f800000U) return false;
    }
    return true;
}
bool cpuAllowed() {
    using namespace shorthand::ai;
    // Reuse the runtime's platform override and policy parser. No backend is
    // advertised as ORT: this is an explicitly requested experimental kernel.
    if(!productionBackendPlatformQualified() && !allowUnqualifiedBackendHardwareFromEnvironment()) return false;
    bool configured=false;
    for(const char *name:hardwareRoutingPolicyEnvironmentNames()) {
        const char *value=std::getenv(name); configured|=value && *value;
    }
    if(!configured) return true; // default path allocates nothing and reads no files
    const auto policy=hardwareRoutingPolicyFromEnvironment();
    const auto order=hardware_detail::routingOrder(policy);
    if(std::find(order.begin(),order.end(),DeviceClass::CPU)==order.end()) return false;
    HardwareDeviceCapability cpu;
    cpu.device_class=DeviceClass::CPU;
    if(policy.minimum_memory_mb) cpu.memory_mb=hardware_detail::hostMemoryMb();
    return hardware_detail::policyAllowsDevice(policy,cpu);
}
bool fpEnvironment() {
    if(std::fegetround()!=FE_TONEAREST) return false;
    // Detect input/output flushing without architecture-specific control words.
    volatile float tiny=std::numeric_limits<float>::denorm_min();
    volatile float one=1.0f;
    const float value=tiny*one;
    uint32_t bits; std::memcpy(&bits,&value,sizeof(bits));
    return bits==1;
}
}
extern "C" int32_t sh_aot_v1_prepare(const float *input,uint64_t input_count,
                                    float *output,uint64_t output_count,
                                    float *workspace,uint64_t workspace_count,
                                    uint64_t expected_input,uint64_t expected_output) {
    Region in,out,scratch;
    if(input_count!=expected_input || output_count!=expected_output || workspace_count!=expected_output ||
       !region(input,input_count,in) || !region(output,output_count,out) ||
       !region(workspace,workspace_count,scratch) || overlaps(in,scratch) || overlaps(out,scratch)) return 1;
    try { if(!cpuAllowed()) return 3; } catch(...) { return 3; }
    if(!fpEnvironment()) return 4;
    return finite(input,input_count)?0:2;
}
extern "C" int32_t sh_aot_v1_commit(float *output,uint64_t output_count,
                                   const float *workspace,uint64_t workspace_count) {
    Region out,scratch;
    if(output_count!=workspace_count || !region(output,output_count,out) ||
       !region(workspace,workspace_count,scratch) || overlaps(out,scratch)) return 1;
    if(!finite(workspace,workspace_count)) return 2;
    std::memcpy(output,workspace,static_cast<size_t>(output_count)*sizeof(float));
    return 0;
}
