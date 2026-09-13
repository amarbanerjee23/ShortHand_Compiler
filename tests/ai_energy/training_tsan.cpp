#include "ai_runtime/ExecutionPlan.h"
#include "ai_runtime/training/TrainingQualification.h"
#include <iostream>
int main() {
    using namespace shorthand::ai;
    if (availableCpuThreads()<2) { std::cerr<<"two CPU threads are mandatory for training race qualification\n"; return 1; }
    shorthand::energy::UnavailableCollector collector;
    TrainingConfiguration config;
    auto serial=trainCpuQualification(config,collector);
    config.threads=2;
    for (unsigned n=0;n<3;++n) {
        auto parallel=trainCpuQualification(config,collector);
        if (!serial.success || !parallel.success || serial.parameters!=parallel.parameters) return 1;
    }
    std::cout<<"PASS deterministic CPU training parallel worker stress\n";
}
