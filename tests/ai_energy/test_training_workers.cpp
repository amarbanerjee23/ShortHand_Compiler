#include "Compiler_new_ws/Short_Hand/src/ai_runtime/training/PersistentWorkers.h"
#include <array>
#include <atomic>
#include <iostream>
#include <limits>
using shorthand::ai::training_detail::PersistentWorkers;
void require(bool value) { if(!value) throw std::runtime_error("training_worker_contract"); }
template<class F> void rejects(F f) {
    bool caught=false; try { f(); } catch(const std::exception &) { caught=true; }
    require(caught);
}
int main() {
    rejects([] { PersistentWorkers w(0,[](unsigned){}); });
    rejects([] { PersistentWorkers w(std::numeric_limits<unsigned>::max(),[](unsigned){}); });
    rejects([] { PersistentWorkers w(2,{}); });
    rejects([] { PersistentWorkers w(2,[](unsigned){},[](std::function<void()>){return std::thread{};}); });
    std::atomic<unsigned> starts{0},live{0};
    auto factory=[&](std::function<void()> work) {
        ++starts;
        return std::thread([&,work=std::move(work)] { ++live; work(); --live; });
    };
    std::array<unsigned,4> counts{};
    bool fail=false;
    {
        PersistentWorkers workers(4,[&](unsigned i) {
            ++counts[i]; if(fail && i==1) throw std::runtime_error("injected_worker_failure");
        },factory);
        require(workers.size()==4);
        for(unsigned i=0;i<200;++i) workers.run();
        for(auto count:counts) require(count==200);
        require(starts==4);
        fail=true; rejects([&] { workers.run(); });
        fail=false; workers.run();
        for(auto count:counts) require(count==202);
        workers.stop(); workers.stop();
        rejects([&] { workers.run(); }); require(live==0);
    }
    unsigned requested=0;
    rejects([&] {
        PersistentWorkers workers(4,[](unsigned){},[&](std::function<void()> work) {
            if(++requested==3) throw std::runtime_error("injected_thread_creation_failure");
            return factory(std::move(work));
        });
    });
    require(live==0 && starts==6);
    std::array<std::atomic<unsigned>,2> calls{};
    {
        PersistentWorkers workers(2,[&](unsigned i){++calls[i];});
        std::thread a([&] { for(unsigned i=0;i<100;++i) workers.run(); });
        std::thread b([&] { for(unsigned i=0;i<100;++i) workers.run(); });
        a.join(); b.join();
        require(calls[0]==200 && calls[1]==200);
    }
    std::cout<<"PASS persistent workers: bounded creation, 402 batches, exception recovery, partial construction and shutdown\n";
}
