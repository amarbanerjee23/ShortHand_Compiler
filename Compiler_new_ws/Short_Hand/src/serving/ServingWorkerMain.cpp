#include "ServingRuntime.h"

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

using namespace shorthand::serving;

volatile std::sig_atomic_t stop_requested = 0;
volatile std::sig_atomic_t drain_requested = 0;

void handleStop(int) { stop_requested = 1; }

#if defined(SIGUSR1)
void handleDrain(int) { drain_requested = 1; }
#endif

bool parseSize(const char *text, std::size_t &value) {
    if (text == nullptr || *text == '\0' || *text == '-') return false;
    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max()))
        return false;
    value = static_cast<std::size_t>(parsed);
    return true;
}

bool writeStateAtomically(const std::string &path, const std::string &body) {
    if (path.empty() || path.size() > 4096 || body.size() > 65536) return false;
    const std::string serialized = body + '\n';
#if defined(_WIN32)
    const std::string temporary = path + ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output << serialized;
        output.flush();
        if (!output) return false;
    }
    if (!MoveFileExA(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::remove(temporary.c_str());
        return false;
    }
    return true;
#else
    const std::string temporary = path + ".tmp." + std::to_string(getpid());
    const int descriptor = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC
#if defined(O_NOFOLLOW)
                                                     | O_NOFOLLOW
#endif
                                ,
                                S_IRUSR | S_IWUSR);
    if (descriptor < 0) return false;
    std::size_t written = 0;
    bool success = true;
    while (written < serialized.size()) {
        const ssize_t count =
            write(descriptor, serialized.data() + written, serialized.size() - written);
        if (count > 0) {
            written += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            success = false;
            break;
        }
    }
    if (success && fsync(descriptor) != 0) success = false;
    if (close(descriptor) != 0) success = false;
    if (success && std::rename(temporary.c_str(), path.c_str()) == 0) return true;
    std::remove(temporary.c_str());
    return false;
#endif
}

bool readBoundedState(const std::string &path, std::string &body) {
    if (path.empty() || path.size() > 4096) return false;
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::array<char, 65537> buffer{};
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize count = input.gcount();
    if (count < 0 || count > 65536) return false;
    if (!input.eof() && input.fail()) return false;
    body.assign(buffer.data(), static_cast<std::size_t>(count));
    return true;
}

bool consumeLiteral(const std::string &body, std::size_t &offset, const char *literal) {
    const std::size_t length = std::strlen(literal);
    if (offset > body.size() || length > body.size() - offset ||
        body.compare(offset, length, literal) != 0)
        return false;
    offset += length;
    return true;
}

bool consumeBoolean(const std::string &body, std::size_t &offset, bool &value) {
    if (consumeLiteral(body, offset, "true")) {
        value = true;
        return true;
    }
    if (consumeLiteral(body, offset, "false")) {
        value = false;
        return true;
    }
    return false;
}

bool consumeUnsigned(const std::string &body, std::size_t &offset) {
    const std::size_t start = offset;
    while (offset < body.size() && body[offset] >= '0' && body[offset] <= '9') ++offset;
    return offset > start;
}

bool parseCanonicalHealth(std::string body, bool &live, bool &ready) {
    if (!body.empty() && body.back() == '\n') body.pop_back();
    if (body.empty() || body.find('\n') != std::string::npos ||
        body.find('\r') != std::string::npos)
        return false;
    bool ignored = false;
    std::size_t offset = 0;
    return consumeLiteral(body, offset,
                          "{\"schema\":\"shorthand.serving.health.v1\",\"contract\":\""
                          "shorthand.serving.runtime.v1\",\"live\":") &&
           consumeBoolean(body, offset, live) &&
           consumeLiteral(body, offset, ",\"ready\":") &&
           consumeBoolean(body, offset, ready) &&
           consumeLiteral(body, offset, ",\"accepting\":") &&
           consumeBoolean(body, offset, ignored) &&
           consumeLiteral(body, offset, ",\"draining\":") &&
           consumeBoolean(body, offset, ignored) &&
           consumeLiteral(body, offset, ",\"saturated\":") &&
           consumeBoolean(body, offset, ignored) &&
           consumeLiteral(body, offset, ",\"active\":") && consumeUnsigned(body, offset) &&
           consumeLiteral(body, offset, ",\"queued\":") && consumeUnsigned(body, offset) &&
           consumeLiteral(body, offset, ",\"in_flight\":") && consumeUnsigned(body, offset) &&
           consumeLiteral(body, offset, "}") && offset == body.size();
}

int runProbe(int argc, char **argv) {
    std::string state_file;
    enum class Probe { None, Live, Ready } probe = Probe::None;
    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--state-file" && i + 1 < argc) state_file = argv[++i];
        else if (argument == "--live") probe = Probe::Live;
        else if (argument == "--ready") probe = Probe::Ready;
        else {
            std::cerr << "error: unsupported probe argument: " << argument << '\n';
            return 2;
        }
    }
    if (state_file.empty() || probe == Probe::None) {
        std::cerr << "usage: shorthand_serving_worker probe --state-file <path> --live|--ready\n";
        return 2;
    }
    std::string state;
    if (!readBoundedState(state_file, state)) return 1;
    bool live = false;
    bool ready = false;
    if (!parseCanonicalHealth(std::move(state), live, ready)) return 1;
    return (probe == Probe::Live ? live : ready) ? 0 : 1;
}

int runSelfTest() {
    RuntimeLimits limits;
    limits.tenant_scope = "self-test";
    limits.worker_threads = 2;
    limits.queue_capacity = 8;
    limits.max_in_flight = 10;
    limits.completed_result_capacity = 16;
    limits.max_request_bytes = 1024;
    limits.max_response_bytes = 1024;
    limits.max_in_flight_request_bytes = 8192;
    limits.max_retained_result_bytes = 16384;
    limits.max_deadline = std::chrono::milliseconds(1000);
    ServingRuntime runtime(limits, [](const Request &request, const CancellationToken &token) {
        if (token.stopRequested()) return HandlerResult::failed("cancelled");
        return HandlerResult::succeeded(request.payload);
    });
    for (int i = 0; i < 8; ++i) {
        Request request;
        request.request_id = "request-" + std::to_string(i);
        request.tenant_id = "self-test";
        request.payload = "ok-" + std::to_string(i);
        request.timeout = std::chrono::milliseconds(500);
        if (!runtime.submit(std::move(request)).accepted()) return 10;
    }
    for (int i = 0; i < 8; ++i) {
        const auto result = runtime.wait("self-test", "request-" + std::to_string(i),
                                         std::chrono::seconds(2));
        if (result.status != LookupStatus::Ready ||
            result.result.status != TerminalStatus::Succeeded ||
            result.result.output != "ok-" + std::to_string(i))
            return 11;
    }
    const RuntimeMetrics metrics = runtime.metrics();
    if (metrics.accepted != 8 || metrics.succeeded != 8 || metrics.in_flight != 0) return 12;
    if (!runtime.shutdown(std::chrono::seconds(1))) return 13;
    std::cout << "PASS serving worker self-test contract=" << kServingRuntimeContract << '\n';
    return 0;
}

int runServe(int argc, char **argv) {
    RuntimeLimits limits;
    limits.tenant_scope = "default";
    std::string state_file = "/tmp/shorthand-serving-health.json";
    std::size_t grace_ms = 10000;
    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];
        const auto read_value = [&](std::string &target) -> bool {
            if (i + 1 >= argc) return false;
            target = argv[++i];
            return true;
        };
        const auto read_size = [&](std::size_t &target) -> bool {
            if (i + 1 >= argc) return false;
            return parseSize(argv[++i], target);
        };
        if (argument == "--tenant") {
            if (!read_value(limits.tenant_scope)) return 2;
        } else if (argument == "--state-file") {
            if (!read_value(state_file)) return 2;
        } else if (argument == "--workers") {
            if (!read_size(limits.worker_threads)) return 2;
        } else if (argument == "--queue-capacity") {
            if (!read_size(limits.queue_capacity)) return 2;
        } else if (argument == "--max-in-flight") {
            if (!read_size(limits.max_in_flight)) return 2;
        } else if (argument == "--max-request-bytes") {
            if (!read_size(limits.max_request_bytes)) return 2;
        } else if (argument == "--max-response-bytes") {
            if (!read_size(limits.max_response_bytes)) return 2;
        } else if (argument == "--max-in-flight-request-bytes") {
            if (!read_size(limits.max_in_flight_request_bytes)) return 2;
        } else if (argument == "--max-retained-result-bytes") {
            if (!read_size(limits.max_retained_result_bytes)) return 2;
        } else if (argument == "--max-deadline-ms") {
            std::size_t value = 0;
            if (!read_size(value) || value > static_cast<std::size_t>(std::numeric_limits<long long>::max()))
                return 2;
            limits.max_deadline = std::chrono::milliseconds(value);
        } else if (argument == "--grace-ms") {
            if (!read_size(grace_ms)) return 2;
        } else {
            std::cerr << "error: unsupported serve argument: " << argument << '\n';
            return 2;
        }
    }

    try {
        ServingRuntime runtime(limits, [](const Request &request, const CancellationToken &token) {
            if (token.stopRequested()) return HandlerResult::failed("cancelled");
            return HandlerResult::succeeded(request.payload);
        });
        std::signal(SIGTERM, handleStop);
        std::signal(SIGINT, handleStop);
#if defined(SIGUSR1)
        std::signal(SIGUSR1, handleDrain);
#endif
        if (!writeStateAtomically(state_file, runtime.healthJson())) {
            std::cerr << "error: cannot publish serving health state\n";
            return 3;
        }
        std::cout << "SERVING_WORKER_READY contract=" << kServingRuntimeContract
                  << " workers=" << limits.worker_threads
                  << " queue_capacity=" << limits.queue_capacity << '\n';
        std::cout.flush();

        bool draining = false;
        while (!stop_requested) {
            if (drain_requested && !draining) {
                runtime.beginDrain();
                draining = true;
                std::cout << "SERVING_WORKER_DRAINING\n";
                std::cout.flush();
            }
            if (!writeStateAtomically(state_file, runtime.healthJson())) {
                std::cerr << "error: serving health state update failed\n";
                return 4;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        runtime.beginDrain();
        writeStateAtomically(state_file, runtime.healthJson());
        const bool graceful = runtime.shutdown(std::chrono::milliseconds(grace_ms));
        std::cout << "SERVING_WORKER_STOPPED graceful=" << (graceful ? "true" : "false") << '\n';
        std::cout.flush();
        return graceful ? 0 : 5;
    } catch (const std::exception &error) {
        std::cerr << "error: serving worker configuration failed: " << error.what() << '\n';
        return 2;
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: shorthand_serving_worker serve|probe|self-test [options]\n";
        return 2;
    }
    const std::string command = argv[1];
    if (command == "serve") return runServe(argc, argv);
    if (command == "probe") return runProbe(argc, argv);
    if (command == "self-test") return runSelfTest();
    std::cerr << "error: unsupported serving worker command: " << command << '\n';
    return 2;
}
