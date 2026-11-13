#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <thread>
#include <vector>
static std::atomic<bool> track{false};static std::atomic<unsigned> allocations{0};
void *operator new(std::size_t n){if(track.load()) ++allocations;if(void *p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] void operator delete(void *p) noexcept{std::free(p);}
[[gnu::noinline]] void operator delete[](void *p) noexcept{std::free(p);}
[[gnu::noinline]] void operator delete(void *p,std::size_t) noexcept{std::free(p);}
[[gnu::noinline]] void operator delete[](void *p,std::size_t) noexcept{std::free(p);}
#ifndef AOT_SYMBOL
#error AOT_SYMBOL required
#endif
extern "C" int32_t AOT_SYMBOL(const float *,uint64_t,float *,uint64_t,float *,uint64_t);
static std::vector<float> read(const char *path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);assert(f);const auto size=f.tellg();assert(size>0 && size%4==0);
    std::vector<float> values(static_cast<std::size_t>(size)/4);f.seekg(0);f.read(reinterpret_cast<char *>(values.data()),size);assert(f);return values;
}
int main(int argc,char **argv){
    assert(argc==3 || argc==4);const auto input=read(argv[1]),expected=read(argv[2]);const auto ni=input.size(),no=expected.size();
    std::vector<float> output(no,999),workspace(no);
    auto call=[&](const float *in,float *out,float *scratch){return AOT_SYMBOL(in,ni,out,no,scratch,no);};
    auto equal=[&](const float *v){return std::memcmp(v,expected.data(),no*sizeof(float))==0;};
    if(argc==4){const auto saved=output;assert(call(input.data(),output.data(),workspace.data())==2 && output==saved);puts("PASS finite overflow rollback");return 0;}
    allocations=0;track=true;for(int i=0;i<100;++i){assert(call(input.data(),output.data(),workspace.data())==0);assert(equal(output.data()));}
    track=false;assert(allocations==0);track=true;auto *control=::operator new(13);track=false;::operator delete(control);assert(allocations>0);
    std::vector<float> alias(std::max(ni,no)+1);std::copy(input.begin(),input.end(),alias.begin());assert(call(alias.data(),alias.data(),workspace.data())==0 && equal(alias.data()));
    std::copy(input.begin(),input.end(),alias.begin());assert(call(alias.data(),alias.data()+1,workspace.data())==0 && equal(alias.data()+1));
    output.assign(no,999);const auto saved=output;assert(AOT_SYMBOL(input.data(),ni,output.data(),no,workspace.data(),no-1)==1 && output==saved);
    assert(call(input.data(),output.data(),output.data())==1 && output==saved);
    auto invalid=input;invalid[0]=std::numeric_limits<float>::infinity();assert(call(invalid.data(),output.data(),workspace.data())==2 && output==saved);
    invalid[0]=std::numeric_limits<float>::quiet_NaN();assert(call(invalid.data(),output.data(),workspace.data())==2 && output==saved);
    std::vector<std::thread> workers;for(int t=0;t<4;++t) workers.emplace_back([&]{std::vector<float> out(no),scratch(no);for(int i=0;i<20;++i) assert(call(input.data(),out.data(),scratch.data())==0 && equal(out.data()));});
    for(auto &worker:workers) worker.join();puts("PASS same AOT artifact C++: exact FP32, alias, rollback, allocation control and concurrency");
}
