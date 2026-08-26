#include <serving/ServingRuntime.h>

#include <chrono>
#include <string>
#include <utility>

int main() {
    using namespace shorthand::serving;
    RuntimeLimits limits;
    limits.tenant_scope = "installed";
    limits.worker_threads = 1;
    limits.queue_capacity = 2;
    limits.max_in_flight = 3;
    ServingRuntime runtime(limits, [](const Request &request, const CancellationToken &) {
        return HandlerResult::succeeded(request.payload);
    });
    Request request{"installed-1", "installed", "ready", std::chrono::milliseconds(500)};
    if (!runtime.submit(std::move(request)).accepted()) return 1;
    const ResultLookup result = runtime.wait("installed", "installed-1", std::chrono::seconds(1));
    if (result.status != LookupStatus::Ready || result.result.status != TerminalStatus::Succeeded ||
        result.result.output != "ready")
        return 2;
    return runtime.shutdown(std::chrono::seconds(1)) ? 0 : 3;
}
