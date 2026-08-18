#include "SourceTools.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void usage() {
    std::cerr << "usage: shorthand_tool <source.short> <format|lint|fix> [--output <path>]\n";
}

bool readFile(const std::string &path, std::string &content) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    content = buffer.str();
    return static_cast<bool>(in) || in.eof();
}

bool writeFile(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << content;
    return static_cast<bool>(out);
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3 && argc != 5) {
        usage();
        return 2;
    }
    if (argc == 5 && std::string(argv[3]) != "--output") {
        usage();
        return 2;
    }

    const std::string path = argv[1];
    const std::string mode = argv[2];
    if (mode != "format" && mode != "lint" && mode != "fix") {
        usage();
        return 2;
    }
    if (mode == "fix" && argc != 5) {
        std::cerr << "error: fix mode requires --output so source files are never mutated implicitly\n";
        return 2;
    }

    std::string source;
    if (!readFile(path, source)) {
        std::cerr << "error: could not read source file: " << path << '\n';
        return 2;
    }

    if (mode == "lint") {
        const auto diagnostics = shorthand::tooling::lintSource(source);
        if (argc == 5) {
            std::ofstream out(argv[4], std::ios::binary | std::ios::trunc);
            if (!out) {
                std::cerr << "error: could not open output file: " << argv[4] << '\n';
                return 2;
            }
            shorthand::tooling::writeDiagnosticsJson(path, diagnostics, out);
        } else {
            shorthand::tooling::writeDiagnosticsJson(path, diagnostics, std::cout);
        }
        return diagnostics.empty() ? 0 : 1;
    }

    const std::string formatted = shorthand::tooling::formatSource(source);
    if (argc == 5) {
        if (!writeFile(argv[4], formatted)) {
            std::cerr << "error: could not write output file: " << argv[4] << '\n';
            return 2;
        }
    } else {
        std::cout << formatted;
    }
    return 0;
}
