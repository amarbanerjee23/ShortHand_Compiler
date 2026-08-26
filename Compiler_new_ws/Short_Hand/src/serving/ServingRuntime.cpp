#include "ServingRuntime.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace shorthand::serving {
namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kMaxWorkers = 64;
constexpr std::size_t kMaxQueueCapacity = 65536;
constexpr std::size_t kMaxResultCapacity = 65536;
constexpr std::size_t kMaxBodyBytes = 16 * 1024 * 1024;
constexpr std::size_t kMaxAggregateBodyBytes = 1024ULL * 1024 * 1024;
constexpr std::size_t kMaxIdentityBytes = 64;
constexpr auto kMaxSupportedDeadline = std::chrono::hours(1);

bool validIdentity(const std::string &value) {
    if (value.empty() || value.size() > kMaxIdentityBytes) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    });
}

std::uint64_t elapsedMicros(Clock::time_point start, Clock::time_point end) {
    if (end <= start) return 0;
    const auto value = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return value < 0 ? 0 : static_cast<std::uint64_t>(value);
}

bool exceedsCombinedLimit(const std::string &first, const std::string &second,
                          std::size_t limit) {
    return first.size() > limit || second.size() > limit - first.size();
}

void validateLimits(const RuntimeLimits &limits) {
    if (!validIdentity(limits.tenant_scope))
        throw std::invalid_argument("tenant_scope must be a bounded portable identifier");
    if (limits.worker_threads == 0 || limits.worker_threads > kMaxWorkers)
        throw std::invalid_argument("worker_threads must be in the range 1..64");
    if (limits.queue_capacity == 0 || limits.queue_capacity > kMaxQueueCapacity)
        throw std::invalid_argument("queue_capacity must be in the range 1..65536");
    if (limits.max_in_flight == 0 ||
        limits.max_in_flight > limits.queue_capacity + limits.worker_threads)
        throw std::invalid_argument("max_in_flight must fit the configured workers and queue");
    if (limits.completed_result_capacity == 0 ||
        limits.completed_result_capacity > kMaxResultCapacity)
        throw std::invalid_argument("completed_result_capacity must be in the range 1..65536");
    if (limits.max_request_bytes == 0 || limits.max_request_bytes > kMaxBodyBytes ||
        limits.max_response_bytes == 0 || limits.max_response_bytes > kMaxBodyBytes)
        throw std::invalid_argument("request and response limits must be in the range 1..16 MiB");
    if (limits.max_in_flight_request_bytes < limits.max_request_bytes ||
        limits.max_in_flight_request_bytes > kMaxAggregateBodyBytes ||
        limits.max_retained_result_bytes < limits.max_response_bytes ||
        limits.max_retained_result_bytes > kMaxAggregateBodyBytes)
        throw std::invalid_argument(
            "aggregate request and result limits must contain one body and not exceed 1 GiB");
    if (limits.max_deadline.count() <= 0 || limits.max_deadline > kMaxSupportedDeadline)
        throw std::invalid_argument("max_deadline must be in the range 1ms..1h");
}

struct RequestState {
    Request request;
    Clock::time_point submitted_at;
    Clock::time_point deadline;
    Clock::time_point started_at;
    std::shared_ptr<CancellationTokenState> cancellation;
    bool started = false;
    bool done = false;
    RequestResult result;
};

} // namespace

struct CancellationTokenState {
    std::atomic<bool> stop_requested{false};
};

struct ServingRuntimeImpl {
    RuntimeLimits limits;
    RequestHandler handler;

    mutable std::mutex mutex;
    std::mutex shutdown_mutex;
    std::condition_variable work_available;
    std::condition_variable state_changed;
    std::deque<std::shared_ptr<RequestState>> queue;
    std::unordered_map<std::string, std::shared_ptr<RequestState>> requests;
    std::deque<std::string> completed_order;
    std::vector<std::thread> workers;
    RuntimeMetrics counters;
    bool live = true;
    bool accepting = true;
    bool draining = false;
    bool stopping = false;
    bool joined = false;
    std::size_t active = 0;
    std::size_t in_flight = 0;
    std::size_t in_flight_request_bytes = 0;
    std::size_t retained_result_bytes = 0;

    ServingRuntimeImpl(RuntimeLimits configured, RequestHandler configured_handler)
        : limits(std::move(configured)), handler(std::move(configured_handler)) {
        validateLimits(limits);
        if (!handler) throw std::invalid_argument("serving request handler is required");
        workers.reserve(limits.worker_threads);
        try {
            for (std::size_t i = 0; i < limits.worker_threads; ++i)
                workers.emplace_back([this] { workerLoop(); });
        } catch (...) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                accepting = false;
                stopping = true;
                work_available.notify_all();
            }
            for (auto &worker : workers)
                if (worker.joinable()) worker.join();
            throw;
        }
    }

    void recordTerminalLocked(TerminalStatus status) {
        switch (status) {
        case TerminalStatus::Succeeded: ++counters.succeeded; break;
        case TerminalStatus::HandlerError: ++counters.handler_error; break;
        case TerminalStatus::Cancelled: ++counters.cancelled; break;
        case TerminalStatus::DeadlineExceeded: ++counters.deadline_exceeded; break;
        case TerminalStatus::Shutdown: ++counters.shutdown; break;
        }
    }

    static std::size_t resultBytes(const RequestResult &result) {
        return result.output.size() + result.detail.size();
    }

    void retainCompletedLocked(const std::shared_ptr<RequestState> &state) {
        completed_order.push_back(state->request.request_id);
        retained_result_bytes += resultBytes(state->result);
        while (completed_order.size() > limits.completed_result_capacity ||
               retained_result_bytes > limits.max_retained_result_bytes) {
            const std::string oldest = std::move(completed_order.front());
            completed_order.pop_front();
            const auto found = requests.find(oldest);
            if (found != requests.end() && found->second->done) {
                const std::size_t evicted_bytes = resultBytes(found->second->result);
                retained_result_bytes = retained_result_bytes >= evicted_bytes
                                            ? retained_result_bytes - evicted_bytes
                                            : 0;
                requests.erase(found);
                ++counters.result_evictions;
            }
        }
    }

    void completeLocked(const std::shared_ptr<RequestState> &state, TerminalStatus status,
                        std::string output, std::string detail, Clock::time_point completed_at) {
        if (state->done) return;
        state->done = true;
        state->result.status = status;
        state->result.output = std::move(output);
        state->result.detail = std::move(detail);
        const Clock::time_point service_start = state->started ? state->started_at : completed_at;
        state->result.queue_time_us = elapsedMicros(state->submitted_at, service_start);
        state->result.service_time_us = elapsedMicros(service_start, completed_at);
        if (state->started && active > 0) --active;
        if (in_flight > 0) --in_flight;
        const std::size_t request_bytes = state->request.payload.size();
        in_flight_request_bytes = in_flight_request_bytes >= request_bytes
                                      ? in_flight_request_bytes - request_bytes
                                      : 0;
        std::string{}.swap(state->request.payload);
        recordTerminalLocked(status);
        retainCompletedLocked(state);
        state_changed.notify_all();
    }

    void expireQueuedLocked(Clock::time_point now) {
        auto current = queue.begin();
        while (current != queue.end()) {
            const auto &state = *current;
            if (state->deadline <= now) {
                current = queue.erase(current);
                completeLocked(state, TerminalStatus::DeadlineExceeded, {},
                               "deadline_exceeded_while_queued", now);
            } else if (state->cancellation->stop_requested.load(std::memory_order_acquire)) {
                current = queue.erase(current);
                completeLocked(state, TerminalStatus::Cancelled, {},
                               "cancelled_while_queued", now);
            } else {
                ++current;
            }
        }
    }

    void workerLoop() {
        for (;;) {
            std::shared_ptr<RequestState> state;
            {
                std::unique_lock<std::mutex> lock(mutex);
                work_available.wait(lock, [this] { return stopping || !queue.empty(); });
                if (stopping && queue.empty()) return;
                const auto now = Clock::now();
                expireQueuedLocked(now);
                if (queue.empty()) {
                    if (stopping) return;
                    continue;
                }
                state = queue.front();
                queue.pop_front();
                if (state->cancellation->stop_requested.load(std::memory_order_acquire)) {
                    completeLocked(state, TerminalStatus::Cancelled, {},
                                   "cancelled_before_execution", Clock::now());
                    continue;
                }
                if (state->deadline <= Clock::now()) {
                    completeLocked(state, TerminalStatus::DeadlineExceeded, {},
                                   "deadline_exceeded_before_execution", Clock::now());
                    continue;
                }
                state->started = true;
                state->started_at = Clock::now();
                ++active;
                state_changed.notify_all();
            }

            HandlerResult handler_result;
            bool handler_threw = false;
            try {
                const CancellationToken token(state->cancellation, state->deadline);
                handler_result = handler(state->request, token);
            } catch (const std::exception &error) {
                handler_threw = true;
                handler_result = HandlerResult::failed(std::string("handler_exception:") + error.what());
            } catch (...) {
                handler_threw = true;
                handler_result = HandlerResult::failed("handler_exception:unknown");
            }

            std::lock_guard<std::mutex> lock(mutex);
            const auto completed_at = Clock::now();
            if (state->cancellation->stop_requested.load(std::memory_order_acquire)) {
                completeLocked(state, TerminalStatus::Cancelled, {}, "cancelled_during_execution",
                               completed_at);
            } else if (completed_at >= state->deadline) {
                completeLocked(state, TerminalStatus::DeadlineExceeded, {},
                               "deadline_exceeded_during_execution", completed_at);
            } else if (exceedsCombinedLimit(handler_result.output, handler_result.detail,
                                            limits.max_response_bytes)) {
                completeLocked(state, TerminalStatus::HandlerError, {},
                               "response_quota_exceeded", completed_at);
            } else if (handler_threw || !handler_result.success) {
                completeLocked(state, TerminalStatus::HandlerError, {},
                               handler_result.detail.empty() ? "handler_error" : handler_result.detail,
                               completed_at);
            } else {
                completeLocked(state, TerminalStatus::Succeeded, std::move(handler_result.output),
                               std::move(handler_result.detail), completed_at);
            }
        }
    }

    RuntimeHealth healthLocked() const {
        RuntimeHealth result;
        result.live = live;
        result.accepting = accepting && !stopping;
        result.draining = draining;
        result.saturated = queue.size() >= limits.queue_capacity ||
                           in_flight >= limits.max_in_flight;
        result.ready = result.live && result.accepting && !result.saturated;
        result.active = active;
        result.queued = queue.size();
        result.in_flight = in_flight;
        return result;
    }

    RuntimeMetrics metricsLocked() const {
        RuntimeMetrics result = counters;
        result.active = active;
        result.queued = queue.size();
        result.in_flight = in_flight;
        result.retained_results = completed_order.size();
        result.in_flight_request_bytes = in_flight_request_bytes;
        result.retained_result_bytes = retained_result_bytes;
        return result;
    }
};

const char *admissionStatusName(AdmissionStatus status) noexcept {
    switch (status) {
    case AdmissionStatus::Accepted: return "accepted";
    case AdmissionStatus::RejectedNotReady: return "rejected_not_ready";
    case AdmissionStatus::RejectedCapacity: return "rejected_capacity";
    case AdmissionStatus::RejectedQuota: return "rejected_quota";
    case AdmissionStatus::RejectedIsolation: return "rejected_isolation";
    case AdmissionStatus::RejectedDeadline: return "rejected_deadline";
    case AdmissionStatus::RejectedInvalid: return "rejected_invalid";
    case AdmissionStatus::RejectedDuplicate: return "rejected_duplicate";
    }
    return "rejected_invalid";
}

const char *terminalStatusName(TerminalStatus status) noexcept {
    switch (status) {
    case TerminalStatus::Succeeded: return "succeeded";
    case TerminalStatus::HandlerError: return "handler_error";
    case TerminalStatus::Cancelled: return "cancelled";
    case TerminalStatus::DeadlineExceeded: return "deadline_exceeded";
    case TerminalStatus::Shutdown: return "shutdown";
    }
    return "handler_error";
}

HandlerResult HandlerResult::succeeded(std::string value) {
    return HandlerResult{true, std::move(value), {}};
}

HandlerResult HandlerResult::failed(std::string reason) {
    return HandlerResult{false, {}, std::move(reason)};
}

CancellationToken::CancellationToken(std::shared_ptr<CancellationTokenState> state,
                                     Clock::time_point deadline) noexcept
    : state_(std::move(state)), deadline_(deadline) {}

bool CancellationToken::stopRequested() const noexcept {
    return state_->stop_requested.load(std::memory_order_acquire) || deadlineExceeded();
}

bool CancellationToken::deadlineExceeded() const noexcept { return Clock::now() >= deadline_; }

ServingRuntime::ServingRuntime(RuntimeLimits limits, RequestHandler handler)
    : impl_(std::make_unique<ServingRuntimeImpl>(std::move(limits), std::move(handler))) {}

ServingRuntime::~ServingRuntime() {
    if (!impl_) return;
    std::lock_guard<std::mutex> lifecycle_lock(impl_->shutdown_mutex);
    if (impl_->joined) return;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->accepting = false;
        impl_->draining = true;
        impl_->stopping = true;
        for (auto &entry : impl_->requests)
            if (!entry.second->done)
                entry.second->cancellation->stop_requested.store(true,
                                                                  std::memory_order_release);
        const auto now = Clock::now();
        while (!impl_->queue.empty()) {
            const auto state = impl_->queue.front();
            impl_->queue.pop_front();
            impl_->completeLocked(state, TerminalStatus::Shutdown, {},
                                  "runtime_destructor_shutdown", now);
        }
        impl_->work_available.notify_all();
    }
    for (auto &worker : impl_->workers)
        if (worker.joinable()) worker.join();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->joined = true;
        impl_->live = false;
        impl_->state_changed.notify_all();
    }
}

Admission ServingRuntime::submit(Request request) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    ++impl_->counters.submitted;
    const auto reject = [this](AdmissionStatus status, const char *reason) {
        switch (status) {
        case AdmissionStatus::RejectedNotReady: ++impl_->counters.rejected_not_ready; break;
        case AdmissionStatus::RejectedCapacity:
            ++impl_->counters.rejected_capacity;
            ++impl_->counters.saturation_events;
            break;
        case AdmissionStatus::RejectedQuota:
            ++impl_->counters.rejected_quota;
            ++impl_->counters.saturation_events;
            break;
        case AdmissionStatus::RejectedIsolation: ++impl_->counters.rejected_isolation; break;
        case AdmissionStatus::RejectedDeadline: ++impl_->counters.rejected_deadline; break;
        case AdmissionStatus::RejectedInvalid: ++impl_->counters.rejected_invalid; break;
        case AdmissionStatus::RejectedDuplicate: ++impl_->counters.rejected_duplicate; break;
        case AdmissionStatus::Accepted: break;
        }
        return Admission{status, reason};
    };

    const auto now = Clock::now();
    impl_->expireQueuedLocked(now);
    if (!impl_->accepting || impl_->stopping)
        return reject(AdmissionStatus::RejectedNotReady, "runtime_not_accepting");
    if (!validIdentity(request.request_id) || !validIdentity(request.tenant_id))
        return reject(AdmissionStatus::RejectedInvalid, "invalid_request_identity");
    if (request.tenant_id != impl_->limits.tenant_scope)
        return reject(AdmissionStatus::RejectedIsolation, "tenant_scope_mismatch");
    if (request.payload.size() > impl_->limits.max_request_bytes)
        return reject(AdmissionStatus::RejectedQuota, "request_byte_quota_exceeded");
    if (request.timeout.count() <= 0 || request.timeout > impl_->limits.max_deadline)
        return reject(AdmissionStatus::RejectedDeadline, "invalid_request_deadline");
    if (impl_->requests.find(request.request_id) != impl_->requests.end())
        return reject(AdmissionStatus::RejectedDuplicate, "duplicate_request_id");
    if (impl_->queue.size() >= impl_->limits.queue_capacity)
        return reject(AdmissionStatus::RejectedCapacity, "queue_capacity_exceeded");
    if (impl_->in_flight >= impl_->limits.max_in_flight)
        return reject(AdmissionStatus::RejectedQuota, "in_flight_quota_exceeded");
    if (request.payload.size() >
        impl_->limits.max_in_flight_request_bytes - impl_->in_flight_request_bytes)
        return reject(AdmissionStatus::RejectedQuota, "in_flight_request_byte_quota_exceeded");

    auto state = std::make_shared<RequestState>();
    state->request = std::move(request);
    state->submitted_at = now;
    state->deadline = now + state->request.timeout;
    state->cancellation = std::make_shared<CancellationTokenState>();
    impl_->requests.emplace(state->request.request_id, state);
    impl_->queue.push_back(state);
    ++impl_->in_flight;
    impl_->in_flight_request_bytes += state->request.payload.size();
    ++impl_->counters.accepted;
    impl_->work_available.notify_one();
    impl_->state_changed.notify_all();
    return Admission{AdmissionStatus::Accepted, "accepted"};
}

bool ServingRuntime::cancel(const std::string &tenant_id, const std::string &request_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (tenant_id != impl_->limits.tenant_scope) return false;
    const auto found = impl_->requests.find(request_id);
    if (found == impl_->requests.end() || found->second->done) return false;
    const auto &state = found->second;
    state->cancellation->stop_requested.store(true, std::memory_order_release);
    const auto queued = std::find(impl_->queue.begin(), impl_->queue.end(), state);
    if (queued != impl_->queue.end()) {
        impl_->queue.erase(queued);
        impl_->completeLocked(state, TerminalStatus::Cancelled, {}, "cancelled_while_queued",
                              Clock::now());
    }
    impl_->work_available.notify_all();
    return true;
}

ResultLookup ServingRuntime::wait(const std::string &tenant_id, const std::string &request_id,
                                  std::chrono::milliseconds max_wait) {
    if (max_wait.count() < 0) return ResultLookup{LookupStatus::Timeout, {}};
    std::unique_lock<std::mutex> lock(impl_->mutex);
    if (tenant_id != impl_->limits.tenant_scope)
        return ResultLookup{LookupStatus::NotFound, {}};
    const auto found = impl_->requests.find(request_id);
    if (found == impl_->requests.end()) return ResultLookup{LookupStatus::NotFound, {}};
    const auto state = found->second;
    if (!state->done && !impl_->state_changed.wait_for(lock, max_wait, [&state] { return state->done; }))
        return ResultLookup{LookupStatus::Timeout, {}};
    const RequestResult result = state->result;
    const auto current = impl_->requests.find(request_id);
    if (current != impl_->requests.end() && current->second == state) impl_->requests.erase(current);
    const auto retained = std::find(impl_->completed_order.begin(), impl_->completed_order.end(), request_id);
    if (retained != impl_->completed_order.end()) {
        const std::size_t result_bytes = impl_->resultBytes(state->result);
        impl_->retained_result_bytes = impl_->retained_result_bytes >= result_bytes
                                          ? impl_->retained_result_bytes - result_bytes
                                          : 0;
        impl_->completed_order.erase(retained);
    }
    return ResultLookup{LookupStatus::Ready, result};
}

bool ServingRuntime::beginDrain() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->stopping || !impl_->live) return false;
    const bool changed = impl_->accepting;
    impl_->accepting = false;
    impl_->draining = true;
    impl_->state_changed.notify_all();
    return changed;
}

bool ServingRuntime::shutdown(std::chrono::milliseconds grace_period) {
    if (grace_period.count() < 0) grace_period = std::chrono::milliseconds(0);
    std::lock_guard<std::mutex> lifecycle_lock(impl_->shutdown_mutex);
    if (impl_->joined) return true;

    std::unique_lock<std::mutex> lock(impl_->mutex);
    impl_->accepting = false;
    impl_->draining = true;
    const bool graceful = impl_->state_changed.wait_for(
        lock, grace_period, [this] { return impl_->in_flight == 0; });
    if (!graceful) {
        for (auto &entry : impl_->requests)
            if (!entry.second->done)
                entry.second->cancellation->stop_requested.store(true, std::memory_order_release);
        const auto now = Clock::now();
        while (!impl_->queue.empty()) {
            const auto state = impl_->queue.front();
            impl_->queue.pop_front();
            impl_->completeLocked(state, TerminalStatus::Shutdown, {},
                                  "grace_period_exhausted", now);
        }
    }
    impl_->stopping = true;
    impl_->work_available.notify_all();
    lock.unlock();

    for (auto &worker : impl_->workers)
        if (worker.joinable()) worker.join();

    lock.lock();
    impl_->joined = true;
    impl_->live = false;
    impl_->state_changed.notify_all();
    return graceful;
}

RuntimeHealth ServingRuntime::health() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->healthLocked();
}

RuntimeMetrics ServingRuntime::metrics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->metricsLocked();
}

std::string ServingRuntime::healthJson() const {
    const RuntimeHealth value = health();
    std::ostringstream out;
    out << "{\"schema\":\"shorthand.serving.health.v1\",\"contract\":\""
        << kServingRuntimeContract << "\",\"live\":" << (value.live ? "true" : "false")
        << ",\"ready\":" << (value.ready ? "true" : "false")
        << ",\"accepting\":" << (value.accepting ? "true" : "false")
        << ",\"draining\":" << (value.draining ? "true" : "false")
        << ",\"saturated\":" << (value.saturated ? "true" : "false")
        << ",\"active\":" << value.active << ",\"queued\":" << value.queued
        << ",\"in_flight\":" << value.in_flight << "}";
    return out.str();
}

std::string ServingRuntime::prometheusMetrics() const {
    const RuntimeMetrics value = metrics();
    std::ostringstream out;
    out << "# HELP shorthand_serving_requests_total Submission attempts at the serving runtime.\n"
        << "# TYPE shorthand_serving_requests_total counter\n"
        << "shorthand_serving_requests_total " << value.submitted << '\n'
        << "shorthand_serving_requests_accepted_total " << value.accepted << '\n'
        << "shorthand_serving_rejected_not_ready_total " << value.rejected_not_ready << '\n'
        << "shorthand_serving_rejected_capacity_total " << value.rejected_capacity << '\n'
        << "shorthand_serving_rejected_quota_total " << value.rejected_quota << '\n'
        << "shorthand_serving_rejected_isolation_total " << value.rejected_isolation << '\n'
        << "shorthand_serving_rejected_deadline_total " << value.rejected_deadline << '\n'
        << "shorthand_serving_rejected_invalid_total " << value.rejected_invalid << '\n'
        << "shorthand_serving_rejected_duplicate_total " << value.rejected_duplicate << '\n'
        << "shorthand_serving_succeeded_total " << value.succeeded << '\n'
        << "shorthand_serving_handler_error_total " << value.handler_error << '\n'
        << "shorthand_serving_cancelled_total " << value.cancelled << '\n'
        << "shorthand_serving_deadline_exceeded_total " << value.deadline_exceeded << '\n'
        << "shorthand_serving_shutdown_total " << value.shutdown << '\n'
        << "shorthand_serving_result_evictions_total " << value.result_evictions << '\n'
        << "shorthand_serving_saturation_events_total " << value.saturation_events << '\n'
        << "# TYPE shorthand_serving_active gauge\n"
        << "shorthand_serving_active " << value.active << '\n'
        << "shorthand_serving_queued " << value.queued << '\n'
        << "shorthand_serving_in_flight " << value.in_flight << '\n'
        << "shorthand_serving_retained_results " << value.retained_results << '\n'
        << "shorthand_serving_in_flight_request_bytes " << value.in_flight_request_bytes << '\n'
        << "shorthand_serving_retained_result_bytes " << value.retained_result_bytes << '\n';
    return out.str();
}

} // namespace shorthand::serving
