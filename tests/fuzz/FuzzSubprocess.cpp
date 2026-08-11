#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef SHORTHAND_FUZZ_STAGE
#define SHORTHAND_FUZZ_STAGE 0
#endif

namespace {

std::string shellQuote(const std::string &value) {
    std::string out = "'";
    for (char ch : value) {
        if (ch == '\'') out += "'\\''";
        else out += ch;
    }
    out += "'";
    return out;
}

bool writeAll(int fd, const std::uint8_t *data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t written = ::write(fd, data + offset, size - offset);
        if (written <= 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool containsFailureMarker(const std::string &text) {
    static const char *markers[] = {
        "AddressSanitizer",
        "LeakSanitizer",
        "UndefinedBehaviorSanitizer",
        "ThreadSanitizer",
        "runtime error:",
        "Segmentation fault",
        "core dumped",
        "stack-overflow"
    };
    for (const char *marker : markers) {
        if (text.find(marker) != std::string::npos) return true;
    }
    return false;
}

[[noreturn]] void fuzzFailure() {
    __builtin_trap();
}

std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

int normalizedExitCode(int status) {
    if (status == -1) return 127;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 127;
}

void runCompilerInput(const std::uint8_t *data, std::size_t size) {
    if (size > 64U * 1024U) return;

    const char *compiler_env = std::getenv("SHORTHAND_FUZZ_BIN");
    if (compiler_env == nullptr || *compiler_env == '\0') fuzzFailure();
    const std::string compiler(compiler_env);

    char work_template[] = "/tmp/shorthand_fuzz_XXXXXX";
    char *work_dir_raw = ::mkdtemp(work_template);
    if (work_dir_raw == nullptr) fuzzFailure();
    const std::string work_dir(work_dir_raw);
    const std::string log_path = work_dir + "/compiler.log";

    std::string source_path;
    std::string mode;

#if SHORTHAND_FUZZ_STAGE == 3
    const std::string src_dir = work_dir + "/src";
    if (::mkdir(src_dir.c_str(), 0700) != 0) fuzzFailure();
    const std::string manifest_path = work_dir + "/shorthand.package";
    int manifest_fd = ::open(manifest_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (manifest_fd < 0 || !writeAll(manifest_fd, data, size)) fuzzFailure();
    ::close(manifest_fd);

    source_path = src_dir + "/app.short";
    const std::string app =
        "package fuzz.pkg;\n"
        "module fuzz.pkg.app;\n"
        "int value;\n"
        "value = 1;\n";
    std::ofstream source(source_path, std::ios::binary);
    source << app;
    source.close();
    mode = "lock";
#else
    source_path = work_dir + "/input.short";
    int source_fd = ::open(source_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (source_fd < 0 || !writeAll(source_fd, data, size)) fuzzFailure();
    ::close(source_fd);
#if SHORTHAND_FUZZ_STAGE == 0
    mode = "parse";
#elif SHORTHAND_FUZZ_STAGE == 1
    mode = "parse";
#elif SHORTHAND_FUZZ_STAGE == 2
    mode = "run";
#elif SHORTHAND_FUZZ_STAGE == 4
    mode = "compile-bc";
#else
#error Unsupported SHORTHAND_FUZZ_STAGE
#endif
#endif

    const std::string command =
        "cd " + shellQuote(work_dir) +
        " && timeout --signal=TERM --kill-after=1 3 " + shellQuote(compiler) + " " +
        shellQuote(source_path) + " " + shellQuote(mode) +
        " >" + shellQuote(log_path) + " 2>&1";

    const int status = std::system(command.c_str());
    const int exit_code = normalizedExitCode(status);
    const std::string log = readFile(log_path);

    // Ordinary language rejection is expected fuzz behavior. A sanitizer finding,
    // signal-style exit, timeout, or launcher failure is never expected.
    if (containsFailureMarker(log) || exit_code == 124 || exit_code == 125 ||
        exit_code == 126 || exit_code == 127 || exit_code >= 128) {
        fuzzFailure();
    }

    const std::string cleanup = "rm -rf " + shellQuote(work_dir);
    (void)std::system(cleanup.c_str());
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    runCompilerInput(data, size);
    return 0;
}
