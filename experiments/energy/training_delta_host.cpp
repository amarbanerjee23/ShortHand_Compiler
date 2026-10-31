// The same host compiles against base and candidate sources. Allocation counts
// use a separate instrumented executable; timing executables omit these hooks.
#include "ai_runtime/training/TrainingQualification.h"
#include "module/Sha256.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>

#ifdef SHORTHAND_COUNT_ALLOCATIONS
namespace {
std::atomic<bool> counting{false};
std::atomic<std::uint64_t> allocations{0},allocated_bytes{0};
}
void *operator new(std::size_t n) {
    if(void *p=std::malloc(n?n:1)) {
        if(counting.load(std::memory_order_relaxed)) {
            allocations.fetch_add(1,std::memory_order_relaxed);
            allocated_bytes.fetch_add(n,std::memory_order_relaxed);
        }
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p,std::size_t) noexcept { std::free(p); }
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete[](void *p,std::size_t) noexcept { ::operator delete(p); }
#endif

int main(int argc,char **argv) {
    try {
        if(argc!=4) return 2;
        shorthand::ai::TrainingConfiguration config;
        config.threads=static_cast<unsigned>(std::stoul(argv[1]));
        config.seed=static_cast<std::uint32_t>(std::stoul(argv[2]));
        const bool inject=std::string(argv[3])=="inject";
        shorthand::energy::UnavailableCollector energy;
#ifdef SHORTHAND_COUNT_ALLOCATIONS
        counting=true;
#endif
        const auto start=std::chrono::steady_clock::now();
        auto result=shorthand::ai::trainCpuQualification(config,energy);
        if(inject) {
            // Direct calls with observable use prevent allocation elision.
            auto *p=static_cast<volatile unsigned char *>(::operator new(64));
            p[0]=42; ::operator delete(const_cast<unsigned char *>(p));
        }
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
#ifdef SHORTHAND_COUNT_ALLOCATIONS
        counting=false;
#endif
        if(!result.success || result.parameters.size()!=shorthand::ai::ReferenceCNN::parameter_count) {
            std::cerr<<result.reason<<'\n'; return 1;
        }
        const std::string bytes(reinterpret_cast<const char *>(result.parameters.data()),result.parameters.size()*sizeof(float));
        std::cout<<std::setprecision(17)<<"{\"seconds\":"<<seconds<<",\"accuracy\":"<<result.validation_accuracy
            <<",\"final_loss\":"<<result.final_loss<<",\"initial_loss\":"<<result.initial_loss
            <<",\"samples\":"<<result.samples_processed<<",\"steps\":"<<result.optimizer_steps
            <<",\"parameters_sha256\":\""<<shorthand::crypto::sha256(bytes)<<"\",\"joules\":null,\"allocations\":";
#ifdef SHORTHAND_COUNT_ALLOCATIONS
        std::cout<<allocations<<",\"allocated_bytes\":"<<allocated_bytes;
#else
        std::cout<<"null,\"allocated_bytes\":null";
#endif
        std::cout<<"}\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
