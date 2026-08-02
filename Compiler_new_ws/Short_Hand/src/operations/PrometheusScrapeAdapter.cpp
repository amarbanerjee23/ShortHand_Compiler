#include <runtime/ShorthandRuntime.h>

#ifdef _WIN32

#include <iostream>

int main() {
    std::cerr << "shorthand_prometheus_adapter is not supported on Windows in this release\n";
    return 2;
}

#else

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t stop_requested = 0;

struct Config {
    std::string listen_address = "127.0.0.1";
    std::uint16_t port = 9464;
    std::size_t max_requests = 0;
    std::size_t request_limit_bytes = 8192;
    int read_timeout_ms = 2000;
};

void requestStop(int) {
    stop_requested = 1;
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

void usage(std::ostream &out) {
    out << "Usage: shorthand_prometheus_adapter [options]\n"
        << "  --listen ADDRESS             IPv4 address (default 127.0.0.1)\n"
        << "  --port PORT                  TCP port, 0 selects an ephemeral port\n"
        << "  --max-requests COUNT         Stop after COUNT requests, 0 is unlimited\n"
        << "  --read-timeout-ms MILLISECONDS\n"
        << "  --request-limit-bytes BYTES  Maximum request-header bytes\n"
        << "  --help\n";
}

bool parseArguments(int argc, char **argv, Config &config) {
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help") {
            usage(std::cout);
            std::exit(0);
        }
        if (i + 1 >= argc) {
            std::cerr << "missing value for " << argument << "\n";
            return false;
        }
        const char *value_text = argv[++i];
        unsigned long value = 0;
        if (argument == "--listen") {
            config.listen_address = value_text;
        } else if (argument == "--port") {
            if (!parseUnsigned(value_text, 65535, value)) return false;
            config.port = static_cast<std::uint16_t>(value);
        } else if (argument == "--max-requests") {
            if (!parseUnsigned(value_text, std::numeric_limits<unsigned long>::max(), value)) return false;
            config.max_requests = static_cast<std::size_t>(value);
        } else if (argument == "--read-timeout-ms") {
            if (!parseUnsigned(value_text, 60000, value) || value == 0) return false;
            config.read_timeout_ms = static_cast<int>(value);
        } else if (argument == "--request-limit-bytes") {
            if (!parseUnsigned(value_text, 1024 * 1024, value) || value < 256) return false;
            config.request_limit_bytes = static_cast<std::size_t>(value);
        } else {
            std::cerr << "unknown option: " << argument << "\n";
            return false;
        }
    }
    return true;
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

std::string response(int status, const std::string &reason, const std::string &content_type,
                     const std::string &body, const std::string &extra_headers = "") {
    std::ostringstream out;
    out << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "Cache-Control: no-store\r\n"
        << "X-Content-Type-Options: nosniff\r\n"
        << extra_headers
        << "\r\n"
        << body;
    return out.str();
}

std::string readRequest(int client, std::size_t limit, bool &too_large) {
    std::string request;
    char buffer[2048];
    too_large = false;
    while (request.find("\r\n\r\n") == std::string::npos) {
        const ssize_t count = ::recv(client, buffer, sizeof(buffer), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        if (request.size() + static_cast<std::size_t>(count) > limit) {
            too_large = true;
            break;
        }
        request.append(buffer, static_cast<std::size_t>(count));
    }
    return request;
}

std::string handleRequest(const std::string &request, bool too_large) {
    if (too_large) {
        return response(431, "Request Header Fields Too Large", "text/plain; charset=utf-8",
                        "request headers too large\n");
    }

    const std::size_t line_end = request.find("\r\n");
    if (line_end == std::string::npos) {
        return response(400, "Bad Request", "text/plain; charset=utf-8", "bad request\n");
    }

    std::istringstream line(request.substr(0, line_end));
    std::string method;
    std::string target;
    std::string version;
    std::string trailing;
    if (!(line >> method >> target >> version) || (line >> trailing) ||
        version.rfind("HTTP/1.", 0) != 0) {
        return response(400, "Bad Request", "text/plain; charset=utf-8", "bad request\n");
    }

    if (method != "GET") {
        return response(405, "Method Not Allowed", "text/plain; charset=utf-8",
                        "method not allowed\n", "Allow: GET\r\n");
    }

    const std::size_t query = target.find('?');
    const std::string path = target.substr(0, query);
    if (path == "/healthz") {
        return response(200, "OK", "text/plain; charset=utf-8", "ok\n");
    }
    if (path == "/metrics") {
        const char *metrics = short_runtime_prometheus_metrics();
        std::string body = metrics == nullptr ? std::string() : std::string(metrics);
        if (body.empty() || body.back() != '\n') body.push_back('\n');
        return response(200, "OK", "text/plain; version=0.0.4; charset=utf-8", body);
    }
    return response(404, "Not Found", "text/plain; charset=utf-8", "not found\n");
}

}  // namespace

int main(int argc, char **argv) {
    Config config;
    if (!parseArguments(argc, argv, config)) {
        usage(std::cerr);
        return 2;
    }

    in_addr bind_address{};
    if (::inet_pton(AF_INET, config.listen_address.c_str(), &bind_address) != 1) {
        std::cerr << "invalid IPv4 listen address: " << config.listen_address << "\n";
        return 2;
    }

    struct sigaction action {};
    action.sa_handler = requestStop;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);

    const int server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    const int reuse = 1;
    if (::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        std::cerr << "setsockopt failed: " << std::strerror(errno) << "\n";
        ::close(server);
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr = bind_address;
    address.sin_port = htons(config.port);
    if (::bind(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        std::cerr << "bind failed: " << std::strerror(errno) << "\n";
        ::close(server);
        return 1;
    }
    if (::listen(server, 16) != 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << "\n";
        ::close(server);
        return 1;
    }

    socklen_t address_length = sizeof(address);
    if (::getsockname(server, reinterpret_cast<sockaddr *>(&address), &address_length) != 0) {
        std::cerr << "getsockname failed: " << std::strerror(errno) << "\n";
        ::close(server);
        return 1;
    }

    const unsigned actual_port = ntohs(address.sin_port);
    if (config.listen_address != "127.0.0.1") {
        std::cerr << "warning: non-loopback binding requires external authentication, TLS, and network policy\n";
    }
    std::cout << "PROMETHEUS_ADAPTER_LISTENING host=" << config.listen_address
              << " port=" << actual_port
              << " metrics_path=/metrics health_path=/healthz\n";
    std::cout.flush();

    std::size_t served = 0;
    while (!stop_requested && (config.max_requests == 0 || served < config.max_requests)) {
        const int client = ::accept(server, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR && stop_requested) break;
            if (errno == EINTR) continue;
            std::cerr << "accept failed: " << std::strerror(errno) << "\n";
            ::close(server);
            return 1;
        }

        timeval timeout{};
        timeout.tv_sec = config.read_timeout_ms / 1000;
        timeout.tv_usec = (config.read_timeout_ms % 1000) * 1000;
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        ::setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        bool too_large = false;
        const std::string request = readRequest(client, config.request_limit_bytes, too_large);
        const std::string reply = handleRequest(request, too_large);
        if (!sendAll(client, reply)) {
            std::cerr << "warning: response write failed\n";
        }
        ::shutdown(client, SHUT_RDWR);
        ::close(client);
        ++served;
    }

    ::close(server);
    std::cout << "PROMETHEUS_ADAPTER_STOPPED requests=" << served << "\n";
    return 0;
}

#endif
