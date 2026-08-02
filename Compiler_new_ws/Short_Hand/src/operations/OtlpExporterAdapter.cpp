#include <runtime/ShorthandRuntime.h>

#ifdef _WIN32

#include <iostream>

int main() {
    std::cerr << "shorthand_otlp_exporter is not supported on Windows in this release\n";
    return 2;
}

#else

#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

namespace {

constexpr std::size_t kResponseLimitBytes = 64 * 1024;
constexpr const char *kSnapshotSchema = "shorthand.runtime.otlp_spans.v1";

struct Config {
    std::string host = "127.0.0.1";
    std::uint16_t port = 4318;
    std::string path = "/v1/traces";
    std::string service_name = "shorthand-runtime";
    unsigned max_attempts = 3;
    unsigned connect_timeout_ms = 2000;
    unsigned io_timeout_ms = 5000;
    unsigned retry_backoff_ms = 250;
    std::size_t snapshot_limit_bytes = 1024 * 1024;
    std::string input_file;
    bool read_stdin = false;
    bool dry_run = false;
    std::string authorization_env;
};

struct DeliveryResult {
    bool transport_ok = false;
    int http_status = 0;
    std::string reason;
};

bool containsNewline(const std::string &value) {
    return value.find('\r') != std::string::npos || value.find('\n') != std::string::npos;
}

bool parseUnsigned(const char *text, unsigned long maximum, unsigned long &value) {
    if (text == nullptr || *text == '\0' || *text == '-') return false;
    errno = 0;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > maximum) return false;
    value = parsed;
    return true;
}

bool validEnvironmentName(const std::string &name) {
    if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_')) return false;
    for (char c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    }
    return true;
}

void usage(std::ostream &out) {
    out << "Usage: shorthand_otlp_exporter [options]\n"
        << "  --host HOST                     Collector host (default 127.0.0.1)\n"
        << "  --port PORT                     OTLP/HTTP port (default 4318)\n"
        << "  --path PATH                     Trace endpoint path (default /v1/traces)\n"
        << "  --service-name NAME             OTLP service.name resource attribute\n"
        << "  --input-file PATH               Read a bounded runtime snapshot from PATH\n"
        << "  --stdin                         Read a bounded runtime snapshot from stdin\n"
        << "  --max-attempts COUNT            Total delivery attempts, 1 to 10\n"
        << "  --connect-timeout-ms MS         Per-attempt connect timeout\n"
        << "  --io-timeout-ms MS              Per-attempt send/receive timeout\n"
        << "  --retry-backoff-ms MS           Initial exponential retry backoff\n"
        << "  --snapshot-limit-bytes BYTES    Maximum source snapshot size\n"
        << "  --authorization-env NAME        Read Authorization header value from env\n"
        << "  --dry-run                       Print OTLP JSON without network delivery\n"
        << "  --help\n";
}

bool parseArguments(int argc, char **argv, Config &config) {
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help") {
            usage(std::cout);
            std::exit(0);
        }
        if (argument == "--stdin") {
            config.read_stdin = true;
            continue;
        }
        if (argument == "--dry-run") {
            config.dry_run = true;
            continue;
        }
        if (i + 1 >= argc) {
            std::cerr << "missing value for " << argument << "\n";
            return false;
        }
        const char *value_text = argv[++i];
        const std::string value_string = value_text;
        unsigned long value = 0;
        if (argument == "--host") {
            config.host = value_string;
        } else if (argument == "--port") {
            if (!parseUnsigned(value_text, 65535, value) || value == 0) return false;
            config.port = static_cast<std::uint16_t>(value);
        } else if (argument == "--path") {
            config.path = value_string;
        } else if (argument == "--service-name") {
            config.service_name = value_string;
        } else if (argument == "--input-file") {
            config.input_file = value_string;
        } else if (argument == "--max-attempts") {
            if (!parseUnsigned(value_text, 10, value) || value == 0) return false;
            config.max_attempts = static_cast<unsigned>(value);
        } else if (argument == "--connect-timeout-ms") {
            if (!parseUnsigned(value_text, 60000, value) || value == 0) return false;
            config.connect_timeout_ms = static_cast<unsigned>(value);
        } else if (argument == "--io-timeout-ms") {
            if (!parseUnsigned(value_text, 60000, value) || value == 0) return false;
            config.io_timeout_ms = static_cast<unsigned>(value);
        } else if (argument == "--retry-backoff-ms") {
            if (!parseUnsigned(value_text, 60000, value)) return false;
            config.retry_backoff_ms = static_cast<unsigned>(value);
        } else if (argument == "--snapshot-limit-bytes") {
            if (!parseUnsigned(value_text, 16UL * 1024UL * 1024UL, value) || value < 256) return false;
            config.snapshot_limit_bytes = static_cast<std::size_t>(value);
        } else if (argument == "--authorization-env") {
            config.authorization_env = value_string;
        } else {
            std::cerr << "unknown option: " << argument << "\n";
            return false;
        }
    }

    if (config.host.empty() || config.service_name.empty() || config.path.empty() || config.path[0] != '/') {
        std::cerr << "host, service name, and absolute endpoint path are required\n";
        return false;
    }
    if (containsNewline(config.host) || containsNewline(config.path) || containsNewline(config.service_name)) {
        std::cerr << "host, path, and service name must not contain newlines\n";
        return false;
    }
    if (config.read_stdin && !config.input_file.empty()) {
        std::cerr << "--stdin and --input-file are mutually exclusive\n";
        return false;
    }
    if (!config.authorization_env.empty() && !validEnvironmentName(config.authorization_env)) {
        std::cerr << "invalid authorization environment variable name\n";
        return false;
    }
    return true;
}

bool readBounded(std::istream &input, std::size_t limit, std::string &output) {
    output.clear();
    char buffer[4096];
    while (input.good()) {
        input.read(buffer, sizeof(buffer));
        const std::streamsize count = input.gcount();
        if (count <= 0) break;
        if (output.size() + static_cast<std::size_t>(count) > limit) return false;
        output.append(buffer, static_cast<std::size_t>(count));
    }
    return !input.bad();
}

std::string trim(const std::string &value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return value.substr(begin, end - begin);
}

bool loadSnapshot(const Config &config, std::string &snapshot, std::string &source, std::string &reason) {
    if (!config.input_file.empty()) {
        std::ifstream input(config.input_file, std::ios::binary);
        if (!input) {
            reason = "input_file_open_failed";
            return false;
        }
        if (!readBounded(input, config.snapshot_limit_bytes, snapshot)) {
            reason = "snapshot_limit_exceeded_or_read_failed";
            return false;
        }
        source = "file";
    } else if (config.read_stdin) {
        if (!readBounded(std::cin, config.snapshot_limit_bytes, snapshot)) {
            reason = "snapshot_limit_exceeded_or_read_failed";
            return false;
        }
        source = "stdin";
    } else {
        const char *runtime_snapshot = short_runtime_otlp_spans_json();
        snapshot = runtime_snapshot == nullptr ? std::string() : std::string(runtime_snapshot);
        if (snapshot.size() > config.snapshot_limit_bytes) {
            reason = "snapshot_limit_exceeded";
            return false;
        }
        source = "runtime";
    }

    snapshot = trim(snapshot);
    if (snapshot.size() < 2 || snapshot.front() != '{' || snapshot.back() != '}') {
        reason = "snapshot_not_json_object";
        return false;
    }
    if (snapshot.find(kSnapshotSchema) == std::string::npos) {
        reason = "unsupported_snapshot_schema";
        return false;
    }
    return true;
}

std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) out << "\\u" << std::setw(4) << static_cast<unsigned>(c);
                else out << static_cast<char>(c);
        }
    }
    return out.str();
}

std::uint64_t fnv1a(const std::string &value, std::uint64_t seed) {
    std::uint64_t hash = seed;
    for (unsigned char c : value) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

std::string buildOtlpPayload(const Config &config, const std::string &snapshot, const std::string &source) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const std::uint64_t unix_nano = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    const std::string identity = snapshot + std::to_string(unix_nano) + std::to_string(::getpid());
    const std::uint64_t trace_high = fnv1a(identity, 1469598103934665603ULL);
    const std::uint64_t trace_low = fnv1a(identity, 1099511628211ULL);
    const std::uint64_t span_id = fnv1a(identity, 7809847782465536322ULL);

    std::ostringstream out;
    out << "{\"resourceSpans\":[{\"resource\":{\"attributes\":["
        << "{\"key\":\"service.name\",\"value\":{\"stringValue\":\"" << jsonEscape(config.service_name) << "\"}},"
        << "{\"key\":\"service.version\",\"value\":{\"stringValue\":\"1.0.0\"}},"
        << "{\"key\":\"telemetry.sdk.name\",\"value\":{\"stringValue\":\"shorthand-otlp-exporter\"}}"
        << "]},\"scopeSpans\":[{\"scope\":{\"name\":\"shorthand.runtime.otlp_exporter\",\"version\":\"1.0.0\"},"
        << "\"spans\":[{\"traceId\":\"" << hex64(trace_high) << hex64(trace_low)
        << "\",\"spanId\":\"" << hex64(span_id)
        << "\",\"name\":\"shorthand.runtime.snapshot\",\"kind\":1"
        << ",\"startTimeUnixNano\":\"" << unix_nano << "\""
        << ",\"endTimeUnixNano\":\"" << (unix_nano + 1) << "\""
        << ",\"attributes\":["
        << "{\"key\":\"shorthand.runtime.snapshot.schema\",\"value\":{\"stringValue\":\"" << kSnapshotSchema << "\"}},"
        << "{\"key\":\"shorthand.runtime.snapshot.source\",\"value\":{\"stringValue\":\"" << source << "\"}},"
        << "{\"key\":\"shorthand.runtime.snapshot\",\"value\":{\"stringValue\":\"" << jsonEscape(snapshot) << "\"}}"
        << "],\"status\":{\"code\":1}}]}]}]}";
    return out.str();
}

int connectWithTimeout(const Config &config, std::string &reason) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo *addresses = nullptr;
    const std::string port_text = std::to_string(config.port);
    const int resolve_status = ::getaddrinfo(config.host.c_str(), port_text.c_str(), &hints, &addresses);
    if (resolve_status != 0) {
        reason = "name_resolution_failed";
        return -1;
    }

    int connected = -1;
    for (addrinfo *address = addresses; address != nullptr; address = address->ai_next) {
        const int fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) continue;
        const int original_flags = ::fcntl(fd, F_GETFL, 0);
        if (original_flags < 0 || ::fcntl(fd, F_SETFL, original_flags | O_NONBLOCK) != 0) {
            ::close(fd);
            continue;
        }

        int status = ::connect(fd, address->ai_addr, address->ai_addrlen);
        if (status != 0 && errno == EINPROGRESS) {
            pollfd poll_fd{};
            poll_fd.fd = fd;
            poll_fd.events = POLLOUT;
            do {
                status = ::poll(&poll_fd, 1, static_cast<int>(config.connect_timeout_ms));
            } while (status < 0 && errno == EINTR);
            if (status > 0) {
                int socket_error = 0;
                socklen_t length = sizeof(socket_error);
                if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &length) != 0 || socket_error != 0) {
                    status = -1;
                } else {
                    status = 0;
                }
            } else {
                status = -1;
            }
        }

        if (status == 0) {
            ::fcntl(fd, F_SETFL, original_flags);
            timeval timeout{};
            timeout.tv_sec = config.io_timeout_ms / 1000;
            timeout.tv_usec = (config.io_timeout_ms % 1000) * 1000;
            ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            connected = fd;
            break;
        }
        ::close(fd);
    }

    ::freeaddrinfo(addresses);
    if (connected < 0) reason = "connect_failed_or_timed_out";
    return connected;
}

bool sendAll(int fd, const std::string &data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
#ifdef MSG_NOSIGNAL
        const ssize_t sent = ::send(fd, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
#else
        const ssize_t sent = ::send(fd, data.data() + offset, data.size() - offset, 0);
#endif
        if (sent < 0 && errno == EINTR) continue;
        if (sent <= 0) return false;
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

std::string hostHeader(const Config &config) {
    if (config.host.find(':') != std::string::npos && config.host.front() != '[') {
        return "[" + config.host + "]:" + std::to_string(config.port);
    }
    return config.host + ":" + std::to_string(config.port);
}

DeliveryResult deliverOnce(const Config &config, const std::string &payload, const std::string &authorization) {
    DeliveryResult result;
    std::string connect_reason;
    const int fd = connectWithTimeout(config, connect_reason);
    if (fd < 0) {
        result.reason = connect_reason;
        return result;
    }

    std::ostringstream request;
    request << "POST " << config.path << " HTTP/1.1\r\n"
            << "Host: " << hostHeader(config) << "\r\n"
            << "User-Agent: shorthand-otlp-exporter/1.0.0\r\n"
            << "Content-Type: application/json\r\n"
            << "Accept: application/json\r\n"
            << "Content-Length: " << payload.size() << "\r\n"
            << "Connection: close\r\n";
    if (!authorization.empty()) request << "Authorization: " << authorization << "\r\n";
    request << "\r\n" << payload;

    if (!sendAll(fd, request.str())) {
        result.reason = "request_send_failed_or_timed_out";
        ::close(fd);
        return result;
    }

    std::string response;
    char buffer[4096];
    while (response.size() <= kResponseLimitBytes) {
        const ssize_t count = ::recv(fd, buffer, sizeof(buffer), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) {
            result.reason = "response_read_failed_or_timed_out";
            ::close(fd);
            return result;
        }
        if (count == 0) break;
        response.append(buffer, static_cast<std::size_t>(count));
    }
    ::close(fd);

    if (response.size() > kResponseLimitBytes) {
        result.reason = "response_limit_exceeded";
        return result;
    }
    const std::size_t line_end = response.find("\r\n");
    if (line_end == std::string::npos) {
        result.reason = "invalid_http_response";
        return result;
    }
    std::istringstream status_line(response.substr(0, line_end));
    std::string version;
    int status = 0;
    if (!(status_line >> version >> status) || version.rfind("HTTP/1.", 0) != 0 || status < 100 || status > 599) {
        result.reason = "invalid_http_status";
        return result;
    }
    result.transport_ok = true;
    result.http_status = status;
    result.reason = status >= 200 && status < 300 ? "http_accepted" : "http_rejected";
    return result;
}

bool retryable(const DeliveryResult &result) {
    if (!result.transport_ok) return true;
    return result.http_status == 408 || result.http_status == 429 || result.http_status >= 500;
}

unsigned retryDelay(const Config &config, unsigned completed_attempts) {
    std::uint64_t delay = config.retry_backoff_ms;
    for (unsigned i = 1; i < completed_attempts && delay < 60000; ++i) delay *= 2;
    if (delay > 60000) delay = 60000;
    return static_cast<unsigned>(delay);
}

}  // namespace

int main(int argc, char **argv) {
    Config config;
    if (!parseArguments(argc, argv, config)) {
        usage(std::cerr);
        return 2;
    }

    std::string snapshot;
    std::string source;
    std::string reason;
    if (!loadSnapshot(config, snapshot, source, reason)) {
        std::cerr << "OTLP_EXPORT_DELIVERY status=not_attempted delivered=false reason=" << reason << "\n";
        return 2;
    }

    std::string authorization;
    if (!config.authorization_env.empty()) {
        const char *value = std::getenv(config.authorization_env.c_str());
        if (value == nullptr || *value == '\0') {
            std::cerr << "OTLP_EXPORT_DELIVERY status=not_attempted delivered=false reason=authorization_env_missing\n";
            return 2;
        }
        authorization = value;
        if (authorization.size() > 4096 || containsNewline(authorization)) {
            std::cerr << "OTLP_EXPORT_DELIVERY status=not_attempted delivered=false reason=authorization_value_invalid\n";
            return 2;
        }
    }

    const std::string payload = buildOtlpPayload(config, snapshot, source);
    if (config.dry_run) {
        std::cout << payload << "\n";
        std::cerr << "OTLP_EXPORT_DELIVERY status=dry_run delivered=false attempts=0 source=" << source << "\n";
        return 0;
    }

    DeliveryResult last;
    unsigned attempts = 0;
    for (unsigned attempt = 1; attempt <= config.max_attempts; ++attempt) {
        attempts = attempt;
        last = deliverOnce(config, payload, authorization);
        if (last.transport_ok && last.http_status >= 200 && last.http_status < 300) {
            std::cout << "OTLP_EXPORT_DELIVERY status=delivered delivered=true attempts=" << attempts
                      << " http_status=" << last.http_status << " source=" << source << "\n";
            return 0;
        }

        const bool may_retry = retryable(last) && attempt < config.max_attempts;
        std::cerr << "OTLP_EXPORT_ATTEMPT attempt=" << attempt
                  << " outcome=" << (may_retry ? "retryable_failure" : "failure")
                  << " http_status=" << last.http_status
                  << " reason=" << last.reason << "\n";
        if (!may_retry) break;
        const unsigned delay = retryDelay(config, attempt);
        if (delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    std::cerr << "OTLP_EXPORT_DELIVERY status=failed delivered=false attempts=" << attempts
              << " http_status=" << last.http_status << " reason=" << last.reason << " source=" << source << "\n";
    return 1;
}

#endif
