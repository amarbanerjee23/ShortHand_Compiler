#include "serving/ServingRuntime.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace shorthand::serving;
using namespace std::chrono_literals;

namespace {

[[noreturn]] void fail(const std::string &message) {
    std::cerr << "FAIL serving runtime stress: " << message << '\n';
    std::exit(1);
}

} // namespace

int main() {
    constexpr int kRequests = 1200;
    constexpr int kProducers = 8;
    RuntimeLimits limits;
    limits.tenant_scope = "soak";
    limits.worker_threads = 8;
    limits.queue_capacity = 256;
    limits.max_in_flight = 264;
    limits.completed_result_capacity = 2048;
    limits.max_request_bytes = 4096;
    limits.max_response_bytes = 4096;
    limits.max_deadline = 5s;

    ServingRuntime runtime(limits, [](const Request &request, const CancellationToken &token) {
        for (int i = 0; i < 4; ++i) {
            if (token.stopRequested()) return HandlerResult::failed("cancelled");
            std::this_thread::sleep_for(100us);
        }
        return HandlerResult::succeeded(request.payload);
    });

    std::atomic<int> next{0};
    std::atomic<int> admission_failures{0};
    std::atomic<bool> snapshots_running{true};
    std::atomic<int> snapshot_failures{0};
    std::thread snapshot_reader([&] {
        while (snapshots_running.load(std::memory_order_acquire)) {
            const std::string health = runtime.healthJson();
            const std::string metrics = runtime.prometheusMetrics();
            if (health.find("shorthand.serving.health.v1") == std::string::npos ||
                metrics.find("shorthand_serving_in_flight") == std::string::npos)
                ++snapshot_failures;
            std::this_thread::yield();
        }
    });

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int producer = 0; producer < kProducers; ++producer) {
        producers.emplace_back([&] {
            for (;;) {
                const int id = next.fetch_add(1, std::memory_order_relaxed);
                if (id >= kRequests) return;
                Request request;
                request.request_id = "request-" + std::to_string(id);
                request.tenant_id = "soak";
                request.payload = "payload-" + std::to_string(id);
                request.timeout = 4s;
                for (;;) {
                    const Admission admission = runtime.submit(request);
                    if (admission.accepted()) break;
                    if (admission.status != AdmissionStatus::RejectedCapacity &&
                        admission.status != AdmissionStatus::RejectedQuota) {
                        ++admission_failures;
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto &producer : producers) producer.join();
    if (admission_failures.load() != 0) fail("unexpected admission failure");

    int succeeded = 0;
    for (int id = 0; id < kRequests; ++id) {
        const ResultLookup result = runtime.wait("soak", "request-" + std::to_string(id), 10s);
        if (result.status != LookupStatus::Ready) fail("result lookup did not complete");
        if (result.result.status != TerminalStatus::Succeeded) fail("request did not succeed");
        if (result.result.output != "payload-" + std::to_string(id))
            fail("request correlation or tenant result isolation failed");
        ++succeeded;
    }
    snapshots_running.store(false, std::memory_order_release);
    snapshot_reader.join();
    if (snapshot_failures.load() != 0) fail("health or metrics snapshot was malformed");
    if (succeeded != kRequests) fail("soak request count mismatch");

    const RuntimeMetrics before_shutdown = runtime.metrics();
    if (before_shutdown.accepted != kRequests || before_shutdown.succeeded != kRequests ||
        before_shutdown.active != 0 || before_shutdown.queued != 0 ||
        before_shutdown.in_flight != 0)
        fail("final serving metrics are inconsistent");
    if (!runtime.shutdown(5s)) fail("soak runtime did not drain gracefully");

    std::cout << "SERVING_SOAK contract=" << kServingRuntimeContract
              << " requests=" << kRequests << " producers=" << kProducers
              << " workers=" << limits.worker_threads << '\n';
    std::cout << "PASS serving runtime concurrent load and soak stress\n";
    return 0;
}
