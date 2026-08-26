#ifndef SHORTHAND_SERVING_RUNTIME_H
#define SHORTHAND_SERVING_RUNTIME_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace shorthand::serving {

constexpr const char *kServingRuntimeContract = "shorthand.serving.runtime.v1";

enum class AdmissionStatus {
    Accepted,
    RejectedNotReady,
    RejectedCapacity,
    RejectedQuota,
    RejectedIsolation,
    RejectedDeadline,
    RejectedInvalid,
    RejectedDuplicate
};

enum class TerminalStatus {
    Succeeded,
    HandlerError,
    Cancelled,
    DeadlineExceeded,
    Shutdown
};

enum class LookupStatus { Ready, Timeout, NotFound };

const char *admissionStatusName(AdmissionStatus status) noexcept;
const char *terminalStatusName(TerminalStatus status) noexcept;

struct RuntimeLimits {
    std::string tenant_scope;
    std::size_t worker_threads = 4;
    std::size_t queue_capacity = 128;
    std::size_t max_in_flight = 132;
    std::size_t completed_result_capacity = 512;
    std::size_t max_request_bytes = 1024 * 1024;
    std::size_t max_response_bytes = 1024 * 1024;
    std::size_t max_in_flight_request_bytes = 64 * 1024 * 1024;
    std::size_t max_retained_result_bytes = 64 * 1024 * 1024;
    std::chrono::milliseconds max_deadline{30000};
};

struct Request {
    std::string request_id;
    std::string tenant_id;
    std::string payload;
    std::chrono::milliseconds timeout{1000};
};

struct Admission {
    AdmissionStatus status = AdmissionStatus::RejectedInvalid;
    std::string reason;

    bool accepted() const noexcept { return status == AdmissionStatus::Accepted; }
};

struct RequestResult {
    TerminalStatus status = TerminalStatus::HandlerError;
    std::string output;
    std::string detail;
    std::uint64_t queue_time_us = 0;
    std::uint64_t service_time_us = 0;
};

struct ResultLookup {
    LookupStatus status = LookupStatus::NotFound;
    RequestResult result;
};

struct HandlerResult {
    bool success = false;
    std::string output;
    std::string detail;

    static HandlerResult succeeded(std::string value);
    static HandlerResult failed(std::string reason);
};

struct CancellationTokenState;

class CancellationToken {
  public:
    bool stopRequested() const noexcept;
    bool deadlineExceeded() const noexcept;

  private:
    friend struct ServingRuntimeImpl;
    CancellationToken(std::shared_ptr<CancellationTokenState> state,
                      std::chrono::steady_clock::time_point deadline) noexcept;

    std::shared_ptr<CancellationTokenState> state_;
    std::chrono::steady_clock::time_point deadline_;
};

using RequestHandler = std::function<HandlerResult(const Request &, const CancellationToken &)>;

struct RuntimeHealth {
    bool live = false;
    bool ready = false;
    bool accepting = false;
    bool draining = false;
    bool saturated = false;
    std::size_t active = 0;
    std::size_t queued = 0;
    std::size_t in_flight = 0;
};

struct RuntimeMetrics {
    std::uint64_t submitted = 0;
    std::uint64_t accepted = 0;
    std::uint64_t rejected_not_ready = 0;
    std::uint64_t rejected_capacity = 0;
    std::uint64_t rejected_quota = 0;
    std::uint64_t rejected_isolation = 0;
    std::uint64_t rejected_deadline = 0;
    std::uint64_t rejected_invalid = 0;
    std::uint64_t rejected_duplicate = 0;
    std::uint64_t succeeded = 0;
    std::uint64_t handler_error = 0;
    std::uint64_t cancelled = 0;
    std::uint64_t deadline_exceeded = 0;
    std::uint64_t shutdown = 0;
    std::uint64_t result_evictions = 0;
    std::uint64_t saturation_events = 0;
    std::size_t active = 0;
    std::size_t queued = 0;
    std::size_t in_flight = 0;
    std::size_t retained_results = 0;
    std::size_t in_flight_request_bytes = 0;
    std::size_t retained_result_bytes = 0;
};

struct ServingRuntimeImpl;

// Thread-safe bounded scheduler. The handler may run concurrently on up to
// worker_threads threads and must poll its token at bounded intervals. The host
// must outlive all API calls and must not destroy the runtime from a handler.
class ServingRuntime {
  public:
    ServingRuntime(RuntimeLimits limits, RequestHandler handler);
    ~ServingRuntime();

    ServingRuntime(const ServingRuntime &) = delete;
    ServingRuntime &operator=(const ServingRuntime &) = delete;
    ServingRuntime(ServingRuntime &&) = delete;
    ServingRuntime &operator=(ServingRuntime &&) = delete;

    // Admission never waits for capacity. Request ids stay reserved while live
    // or retained and may be reused only after consumption or bounded eviction.
    Admission submit(Request request);
    bool cancel(const std::string &tenant_id, const std::string &request_id);
    ResultLookup wait(const std::string &tenant_id, const std::string &request_id,
                      std::chrono::milliseconds max_wait);

    bool beginDrain();
    bool shutdown(std::chrono::milliseconds grace_period);

    RuntimeHealth health() const;
    RuntimeMetrics metrics() const;
    std::string healthJson() const;
    std::string prometheusMetrics() const;

  private:
    std::unique_ptr<ServingRuntimeImpl> impl_;
};

} // namespace shorthand::serving

#endif
