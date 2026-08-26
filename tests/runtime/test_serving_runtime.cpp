#include "serving/ServingRuntime.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

using namespace shorthand::serving;
using namespace std::chrono_literals;

namespace {

[[noreturn]] void fail(const std::string &message) {
    std::cerr << "FAIL serving runtime unit: " << message << '\n';
    std::exit(1);
}

void require(bool condition, const std::string &message) {
    if (!condition) fail(message);
}

RuntimeLimits limitsFor(const std::string &tenant) {
    RuntimeLimits limits;
    limits.tenant_scope = tenant;
    limits.worker_threads = 1;
    limits.queue_capacity = 2;
    limits.max_in_flight = 3;
    limits.completed_result_capacity = 8;
    limits.max_request_bytes = 32;
    limits.max_response_bytes = 32;
    limits.max_deadline = 2s;
    return limits;
}

Request request(std::string id, std::string tenant, std::string payload = "value",
                std::chrono::milliseconds timeout = 1s) {
    return Request{std::move(id), std::move(tenant), std::move(payload), timeout};
}

void validationAndIsolation() {
    bool rejected_config = false;
    try {
        RuntimeLimits invalid;
        invalid.tenant_scope = "bad tenant";
        ServingRuntime runtime(invalid, [](const Request &, const CancellationToken &) {
            return HandlerResult::succeeded("unexpected");
        });
    } catch (const std::invalid_argument &) {
        rejected_config = true;
    }
    require(rejected_config, "invalid configuration was accepted");

    rejected_config = false;
    try {
        RuntimeLimits invalid = limitsFor("aggregate");
        invalid.max_in_flight_request_bytes = invalid.max_request_bytes - 1;
        ServingRuntime runtime(invalid, [](const Request &, const CancellationToken &) {
            return HandlerResult::succeeded("unexpected");
        });
    } catch (const std::invalid_argument &) {
        rejected_config = true;
    }
    require(rejected_config, "invalid aggregate byte configuration was accepted");

    rejected_config = false;
    try {
        ServingRuntime runtime(limitsFor("handler"), RequestHandler{});
    } catch (const std::invalid_argument &) {
        rejected_config = true;
    }
    require(rejected_config, "missing handler configuration was accepted");

    ServingRuntime runtime(limitsFor("tenant-a"), [](const Request &value, const CancellationToken &) {
        return HandlerResult::succeeded(value.payload);
    });
    require(runtime.submit(request("cross", "tenant-b")).status ==
                AdmissionStatus::RejectedIsolation,
            "cross-tenant request was admitted");
    require(runtime.submit(request("bad/id", "tenant-a")).status ==
                AdmissionStatus::RejectedInvalid,
            "invalid request identity was admitted");
    require(runtime.submit(request("large", "tenant-a", std::string(33, 'x'))).status ==
                AdmissionStatus::RejectedQuota,
            "oversized request was admitted");
    require(runtime.submit(request("deadline", "tenant-a", "x", 2001ms)).status ==
                AdmissionStatus::RejectedDeadline,
            "out-of-policy deadline was admitted");
    require(runtime.submit(request("ok", "tenant-a")).accepted(), "valid request rejected");
    require(runtime.submit(request("ok", "tenant-a")).status ==
                AdmissionStatus::RejectedDuplicate,
            "duplicate request id was admitted");
    require(runtime.wait("tenant-b", "ok", 1ms).status == LookupStatus::NotFound,
            "cross-tenant result lookup was visible");
    const ResultLookup result = runtime.wait("tenant-a", "ok", 1s);
    require(result.status == LookupStatus::Ready &&
                result.result.status == TerminalStatus::Succeeded && result.result.output == "value",
            "valid request did not complete");
    require(runtime.shutdown(1s), "clean runtime did not drain");
}

void capacityCancellationAndDrain() {
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool first_started = false;
    bool release_first = false;
    ServingRuntime runtime(limitsFor("bounded"), [&](const Request &value,
                                                     const CancellationToken &token) {
        if (value.request_id == "first") {
            std::unique_lock<std::mutex> lock(gate_mutex);
            first_started = true;
            gate_cv.notify_all();
            gate_cv.wait(lock, [&] { return release_first || token.stopRequested(); });
        }
        if (token.stopRequested()) return HandlerResult::failed("cancelled");
        return HandlerResult::succeeded(value.payload);
    });
    require(runtime.submit(request("first", "bounded")).accepted(), "first request rejected");
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        require(gate_cv.wait_for(lock, 1s, [&] { return first_started; }), "first request did not start");
    }
    require(runtime.submit(request("second", "bounded")).accepted(), "second request rejected");
    require(runtime.submit(request("third", "bounded")).accepted(), "third request rejected");
    const RuntimeHealth saturated = runtime.health();
    require(saturated.live && !saturated.ready && saturated.saturated,
            "saturated runtime remained ready");
    require(runtime.submit(request("fourth", "bounded")).status ==
                AdmissionStatus::RejectedCapacity,
            "queue capacity did not reject saturation");
    require(runtime.cancel("bounded", "second"), "queued cancellation failed");
    const ResultLookup cancelled = runtime.wait("bounded", "second", 1s);
    require(cancelled.status == LookupStatus::Ready &&
                cancelled.result.status == TerminalStatus::Cancelled,
            "queued cancellation produced the wrong terminal status");
    require(runtime.beginDrain(), "drain did not transition the runtime");
    require(runtime.submit(request("after-drain", "bounded")).status ==
                AdmissionStatus::RejectedNotReady,
            "draining runtime accepted new work");
    const RuntimeHealth draining = runtime.health();
    require(draining.live && !draining.ready && draining.draining && !draining.accepting,
            "drain health state is inconsistent");
    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release_first = true;
    }
    gate_cv.notify_all();
    require(runtime.wait("bounded", "first", 1s).result.status == TerminalStatus::Succeeded,
            "first request did not complete after release");
    require(runtime.wait("bounded", "third", 1s).result.status == TerminalStatus::Succeeded,
            "third request did not complete after release");
    require(runtime.shutdown(1s), "draining runtime did not shut down gracefully");
}

void inFlightQuotaAndRunningCancellation() {
    RuntimeLimits limits = limitsFor("quota");
    limits.queue_capacity = 4;
    limits.max_in_flight = 2;
    limits.max_request_bytes = 4;
    limits.max_in_flight_request_bytes = 8;
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool first_started = false;
    ServingRuntime runtime(limits, [&](const Request &value, const CancellationToken &token) {
        if (value.request_id == "first") {
            std::unique_lock<std::mutex> lock(gate_mutex);
            first_started = true;
            gate_cv.notify_all();
            while (!token.stopRequested()) gate_cv.wait_for(lock, 1ms);
        }
        if (token.stopRequested()) return HandlerResult::failed("cancelled");
        return HandlerResult::succeeded(value.payload);
    });
    require(runtime.submit(request("first", "quota", "1234")).accepted(),
            "first quota request rejected");
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        require(gate_cv.wait_for(lock, 1s, [&] { return first_started; }),
                "quota handler did not start");
    }
    require(runtime.submit(request("second", "quota", "5678")).accepted(),
            "second quota request rejected");
    require(runtime.submit(request("count-quota", "quota", "x")).status ==
                AdmissionStatus::RejectedQuota,
            "in-flight count quota was not enforced");
    require(runtime.cancel("quota", "first"), "running cancellation failed");
    const ResultLookup cancelled = runtime.wait("quota", "first", 1s);
    require(cancelled.status == LookupStatus::Ready &&
                cancelled.result.status == TerminalStatus::Cancelled,
            "running cancellation produced the wrong terminal status");
    require(runtime.wait("quota", "second", 1s).result.status == TerminalStatus::Succeeded,
            "queued request did not continue after cancellation");

    RuntimeLimits byte_limits = limitsFor("byte-quota");
    byte_limits.queue_capacity = 4;
    byte_limits.max_in_flight = 5;
    byte_limits.max_request_bytes = 4;
    byte_limits.max_in_flight_request_bytes = 4;
    std::mutex byte_mutex;
    std::condition_variable byte_cv;
    bool byte_started = false;
    ServingRuntime byte_runtime(byte_limits,
                                [&](const Request &value, const CancellationToken &token) {
        if (value.request_id == "byte-first") {
            std::unique_lock<std::mutex> lock(byte_mutex);
            byte_started = true;
            byte_cv.notify_all();
            while (!token.stopRequested()) byte_cv.wait_for(lock, 1ms);
        }
        if (token.stopRequested()) return HandlerResult::failed("cancelled");
        return HandlerResult::succeeded(value.payload);
    });
    require(byte_runtime.submit(request("byte-first", "byte-quota", "1234")).accepted(),
            "aggregate byte fixture rejected");
    {
        std::unique_lock<std::mutex> lock(byte_mutex);
        require(byte_cv.wait_for(lock, 1s, [&] { return byte_started; }),
                "aggregate byte handler did not start");
    }
    require(byte_runtime.submit(request("byte-second", "byte-quota", "x")).status ==
                AdmissionStatus::RejectedQuota,
            "aggregate in-flight request byte quota was not enforced");
    require(byte_runtime.cancel("byte-quota", "byte-first"),
            "aggregate byte fixture cancellation failed");
    require(byte_runtime.wait("byte-quota", "byte-first", 1s).result.status ==
                TerminalStatus::Cancelled,
            "aggregate byte fixture did not cancel");
    require(byte_runtime.shutdown(1s), "aggregate byte runtime did not shut down");
    require(runtime.shutdown(1s), "quota runtime did not shut down");
}

void deadlinesFaultsAndResponseQuota() {
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool started = false;
    bool release = false;
    ServingRuntime runtime(limitsFor("faults"), [&](const Request &value,
                                                    const CancellationToken &token) {
        if (value.request_id == "block") {
            std::unique_lock<std::mutex> lock(gate_mutex);
            started = true;
            gate_cv.notify_all();
            gate_cv.wait(lock, [&] { return release || token.stopRequested(); });
        }
        if (value.payload == "throw") throw std::runtime_error("fixture");
        if (value.payload == "fail") return HandlerResult::failed("fixture_failure");
        if (value.payload == "large") return HandlerResult::succeeded(std::string(33, 'r'));
        if (value.payload == "late") {
            std::this_thread::sleep_for(30ms);
            return HandlerResult::succeeded("too-late");
        }
        if (token.stopRequested()) return HandlerResult::failed("stopped");
        return HandlerResult::succeeded(value.payload);
    });
    require(runtime.submit(request("block", "faults", "block", 1s)).accepted(),
            "blocking request rejected");
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        require(gate_cv.wait_for(lock, 1s, [&] { return started; }), "blocking request did not start");
    }
    require(runtime.submit(request("queued-deadline", "faults", "queued", 20ms)).accepted(),
            "deadline fixture rejected");
    std::this_thread::sleep_for(40ms);
    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release = true;
    }
    gate_cv.notify_all();
    require(runtime.wait("faults", "block", 1s).result.status == TerminalStatus::Succeeded,
            "blocking fixture failed");
    const ResultLookup expired = runtime.wait("faults", "queued-deadline", 1s);
    require(expired.status == LookupStatus::Ready &&
                expired.result.status == TerminalStatus::DeadlineExceeded,
            "queued deadline was not enforced");

    require(runtime.submit(request("throw", "faults", "throw")).accepted(),
            "throwing handler fixture rejected");
    require(runtime.wait("faults", "throw", 1s).result.status == TerminalStatus::HandlerError,
            "handler exception escaped the serving boundary");
    require(runtime.submit(request("fail", "faults", "fail")).accepted(),
            "failed handler fixture rejected");
    require(runtime.wait("faults", "fail", 1s).result.detail == "fixture_failure",
            "handler failure detail was not deterministic");
    require(runtime.submit(request("response", "faults", "large")).accepted(),
            "response quota fixture rejected");
    const ResultLookup response = runtime.wait("faults", "response", 1s);
    require(response.result.status == TerminalStatus::HandlerError &&
                response.result.detail == "response_quota_exceeded",
            "response byte quota was not enforced");
    require(runtime.submit(request("late", "faults", "late", 10ms)).accepted(),
            "late handler fixture rejected");
    require(runtime.wait("faults", "late", 1s).result.status ==
                TerminalStatus::DeadlineExceeded,
            "late handler success bypassed the deadline");

    const std::string health = runtime.healthJson();
    const std::string metrics = runtime.prometheusMetrics();
    require(health.find("shorthand.serving.health.v1") != std::string::npos,
            "health schema missing");
    require(metrics.find("shorthand_serving_deadline_exceeded_total 2") != std::string::npos,
            "deadline metric missing");
    require(metrics.find("request_id") == std::string::npos &&
                metrics.find("tenant_id") == std::string::npos,
            "high-cardinality or tenant labels leaked into metrics");
    require(runtime.shutdown(1s), "fault runtime did not shut down");
}

void boundedResultRetention() {
    RuntimeLimits limits = limitsFor("retention");
    limits.queue_capacity = 4;
    limits.max_in_flight = 5;
    limits.completed_result_capacity = 2;
    limits.max_response_bytes = 3;
    limits.max_retained_result_bytes = 6;
    ServingRuntime runtime(limits, [](const Request &, const CancellationToken &) {
        return HandlerResult::succeeded("abc");
    });
    require(runtime.submit(request("one", "retention")).accepted(), "retention one rejected");
    require(runtime.submit(request("two", "retention")).accepted(), "retention two rejected");
    require(runtime.submit(request("three", "retention")).accepted(), "retention three rejected");
    const ResultLookup third = runtime.wait("retention", "three", 1s);
    require(third.status == LookupStatus::Ready &&
                third.result.status == TerminalStatus::Succeeded,
            "retention completion did not finish");
    require(runtime.wait("retention", "one", 1ms).status == LookupStatus::NotFound,
            "oldest retained result was not evicted deterministically");
    require(runtime.wait("retention", "two", 1s).status == LookupStatus::Ready,
            "newer retained result was incorrectly evicted");
    const RuntimeMetrics metrics = runtime.metrics();
    require(metrics.result_evictions == 1 && metrics.retained_results == 0 &&
                metrics.retained_result_bytes == 0 && metrics.in_flight_request_bytes == 0,
            "result retention count or byte accounting is inconsistent");
    require(runtime.shutdown(1s), "retention runtime did not shut down");
}

void forcedShutdownAndRestart() {
    std::atomic<bool> started{false};
    ServingRuntime first(limitsFor("restart"), [&](const Request &, const CancellationToken &token) {
        started.store(true, std::memory_order_release);
        while (!token.stopRequested()) std::this_thread::sleep_for(1ms);
        return HandlerResult::failed("stopped");
    });
    require(first.submit(request("long", "restart", "x", 1500ms)).accepted(),
            "long request rejected");
    for (int i = 0; i < 100 && !started.load(std::memory_order_acquire); ++i)
        std::this_thread::sleep_for(2ms);
    require(started.load(std::memory_order_acquire), "long handler did not start");
    require(!first.shutdown(5ms), "forced shutdown was misreported as graceful");
    const ResultLookup stopped = first.wait("restart", "long", 1s);
    require(stopped.status == LookupStatus::Ready &&
                stopped.result.status == TerminalStatus::Cancelled,
            "forced shutdown did not cancel running work");

    ServingRuntime second(limitsFor("restart"), [](const Request &value, const CancellationToken &) {
        return HandlerResult::succeeded(value.payload);
    });
    require(second.submit(request("after-restart", "restart", "recovered")).accepted(),
            "replacement runtime rejected work");
    require(second.wait("restart", "after-restart", 1s).result.output == "recovered",
            "replacement runtime did not execute work");
    require(second.shutdown(1s), "replacement runtime did not shut down");
}

void destructorCancellation() {
    std::atomic<bool> started{false};
    std::atomic<bool> stopped{false};
    {
        ServingRuntime runtime(limitsFor("destructor"),
                               [&](const Request &, const CancellationToken &token) {
            started.store(true, std::memory_order_release);
            while (!token.stopRequested()) std::this_thread::sleep_for(1ms);
            stopped.store(true, std::memory_order_release);
            return HandlerResult::failed("stopped");
        });
        require(runtime.submit(request("implicit", "destructor")).accepted(),
                "destructor fixture rejected");
        for (int i = 0; i < 100 && !started.load(std::memory_order_acquire); ++i)
            std::this_thread::sleep_for(2ms);
        require(started.load(std::memory_order_acquire), "destructor handler did not start");
    }
    require(stopped.load(std::memory_order_acquire),
            "runtime destructor did not cancel and join running work");
}

} // namespace

int main() {
    validationAndIsolation();
    capacityCancellationAndDrain();
    inFlightQuotaAndRunningCancellation();
    deadlinesFaultsAndResponseQuota();
    boundedResultRetention();
    forcedShutdownAndRestart();
    destructorCancellation();
    std::cout << "PASS serving runtime deadlines cancellation backpressure quotas isolation health metrics restart and graceful shutdown\n";
    return 0;
}
