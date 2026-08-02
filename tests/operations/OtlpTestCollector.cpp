#ifndef _WIN32

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace {

struct Config {
    std::vector<int> statuses{200};
    std::string request_log;
    int timeout_ms = 5000;
};

bool parseStatuses(const std::string &text, std::vector<int> &statuses) {
    statuses.clear();
    std::stringstream input(text);
    std::string token;
    while (std::getline(input, token, ',')) {
        char *end = nullptr;
        errno = 0;
        const long value = std::strtol(token.c_str(), &end, 10);
        if (errno != 0 || end == token.c_str() || *end != '\0' || value < 100 || value > 599) return false;
        statuses.push_back(static_cast<int>(value));
    }
    return !statuses.empty() && statuses.size() <= 10;
}

bool parseArguments(int argc, char **argv, Config &config) {
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (i + 1 >= argc) return false;
        const std::string value = argv[++i];
        if (argument == "--status-sequence") {
            if (!parseStatuses(value, config.statuses)) return false;
        } else if (argument == "--request-log") {
            config.request_log = value;
        } else if (argument == "--timeout-ms") {
            char *end = nullptr;
            const long parsed = std::strtol(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0' || parsed <= 0 || parsed > 60000) return false;
            config.timeout_ms = static_cast<int>(parsed);
        } else {
            return false;
        }
    }
    return !config.request_log.empty();
}

bool sendAll(int fd, const std::string &data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t count = ::send(fd, data.data() + offset, data.size() - offset, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

std::size_t contentLength(const std::string &request) {
    const std::string marker = "\r\nContent-Length:";
    std::size_t position = request.find(marker);
    if (position == std::string::npos) return 0;
    position += marker.size();
    while (position < request.size() && request[position] == ' ') ++position;
    const std::size_t end = request.find("\r\n", position);
    if (end == std::string::npos) return 0;
    char *parse_end = nullptr;
    const unsigned long value = std::strtoul(request.substr(position, end - position).c_str(), &parse_end, 10);
    return static_cast<std::size_t>(value);
}

std::string reasonPhrase(int status) {
    if (status == 200) return "OK";
    if (status == 202) return "Accepted";
    if (status == 400) return "Bad Request";
    if (status == 429) return "Too Many Requests";
    if (status == 503) return "Service Unavailable";
    return "Test Status";
}

}  // namespace

int main(int argc, char **argv) {
    Config config;
    if (!parseArguments(argc, argv, config)) {
        std::cerr << "usage: otlp_test_collector --status-sequence 200[,503] --request-log PATH [--timeout-ms MS]\n";
        return 2;
    }

    const int server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) return 1;
    const int reuse = 1;
    ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (::bind(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 || ::listen(server, 8) != 0) {
        ::close(server);
        return 1;
    }
    socklen_t length = sizeof(address);
    if (::getsockname(server, reinterpret_cast<sockaddr *>(&address), &length) != 0) {
        ::close(server);
        return 1;
    }

    std::ofstream log(config.request_log, std::ios::binary | std::ios::trunc);
    if (!log) {
        ::close(server);
        return 1;
    }

    std::cout << "OTLP_TEST_COLLECTOR_LISTENING port=" << ntohs(address.sin_port) << "\n";
    std::cout.flush();

    for (std::size_t index = 0; index < config.statuses.size(); ++index) {
        const int client = ::accept(server, nullptr, nullptr);
        if (client < 0) {
            ::close(server);
            return 1;
        }
        timeval timeout{};
        timeout.tv_sec = config.timeout_ms / 1000;
        timeout.tv_usec = (config.timeout_ms % 1000) * 1000;
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        ::setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        std::string request;
        char buffer[4096];
        std::size_t expected_total = 0;
        while (request.size() <= 2 * 1024 * 1024) {
            const ssize_t count = ::recv(client, buffer, sizeof(buffer), 0);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            request.append(buffer, static_cast<std::size_t>(count));
            const std::size_t headers_end = request.find("\r\n\r\n");
            if (headers_end != std::string::npos) {
                expected_total = headers_end + 4 + contentLength(request);
                if (request.size() >= expected_total) break;
            }
        }

        log << "---REQUEST " << (index + 1) << "---\n" << request << "\n";
        log.flush();
        const int status = config.statuses[index];
        const std::string body = status >= 200 && status < 300 ? "{}" : "{\"error\":\"test collector rejection\"}";
        std::ostringstream response;
        response << "HTTP/1.1 " << status << ' ' << reasonPhrase(status) << "\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n" << body;
        sendAll(client, response.str());
        ::shutdown(client, SHUT_RDWR);
        ::close(client);
    }

    ::close(server);
    std::cout << "OTLP_TEST_COLLECTOR_STOPPED requests=" << config.statuses.size() << "\n";
    return 0;
}

#else
#include <iostream>
int main() { std::cerr << "unsupported\n"; return 2; }
#endif
