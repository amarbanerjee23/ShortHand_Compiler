#include "Diagnostics.h"
#include "../ast/SourceRange.cpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

namespace {
bool contains(const std::string &line, const std::string &needle) {
    return !needle.empty() && line.find(needle) != std::string::npos;
}

int firstNonSpaceColumn(const std::string &line) {
    for (size_t i = 0; i < line.size(); ++i) {
        if (!std::isspace(static_cast<unsigned char>(line[i]))) return static_cast<int>(i + 1);
    }
    return 1;
}

SourceRange pointRange(int line, int column) {
    SourceRange range;
    range.begin.line = line;
    range.begin.column = column;
    range.end = range.begin;
    return range;
}

void printCode(const std::string &code) {
    if (!code.empty()) std::cerr << '[' << code << "] ";
}
}

void Diagnostics::setSourceFile(const std::string &path) {
    source_file_ = path;
    source_lines_.clear();
    std::ifstream in(path.c_str());
    std::string line;
    while (std::getline(in, line)) source_lines_.push_back(line);
}

void Diagnostics::add(Severity severity,
                      const std::string &code,
                      const std::string &message,
                      const SourceRange &range) {
    records_.push_back({severity, code, message, range});
    if (severity == Severity::Error) has_errors_ = true;
    if (severity == Severity::Warning) has_warnings_ = true;
}

void Diagnostics::error(const std::string &message) { add(Severity::Error, "", message); }
void Diagnostics::warning(const std::string &message) { add(Severity::Warning, "", message); }
void Diagnostics::error(const std::string &code, const std::string &message) { add(Severity::Error, code, message); }
void Diagnostics::warning(const std::string &code, const std::string &message) { add(Severity::Warning, code, message); }

void Diagnostics::errorAt(const std::string &anchor_kind,
                          const std::string &anchor_name,
                          const std::string &message) {
    auto loc = locateAnchor(anchor_kind, anchor_name);
    add(Severity::Error, "", message, pointRange(loc.first, loc.second));
}

void Diagnostics::warningAt(const std::string &anchor_kind,
                            const std::string &anchor_name,
                            const std::string &message) {
    auto loc = locateAnchor(anchor_kind, anchor_name);
    add(Severity::Warning, "", message, pointRange(loc.first, loc.second));
}

void Diagnostics::errorAtRange(const SourceRange &range, const std::string &message) {
    add(Severity::Error, "", message, range);
}

void Diagnostics::warningAtRange(const SourceRange &range, const std::string &message) {
    add(Severity::Warning, "", message, range);
}

void Diagnostics::errorAtRange(const SourceRange &range,
                               const std::string &code,
                               const std::string &message) {
    add(Severity::Error, code, message, range);
}

void Diagnostics::warningAtRange(const SourceRange &range,
                                 const std::string &code,
                                 const std::string &message) {
    add(Severity::Warning, code, message, range);
}

void Diagnostics::errorAtNode(const void *node, const std::string &message) {
    errorAtRange(shorthand_get_ast_source_range(node), message);
}

void Diagnostics::warningAtNode(const void *node, const std::string &message) {
    warningAtRange(shorthand_get_ast_source_range(node), message);
}

void Diagnostics::errorAtNode(const void *node,
                              const std::string &code,
                              const std::string &message) {
    errorAtRange(shorthand_get_ast_source_range(node), code, message);
}

void Diagnostics::warningAtNode(const void *node,
                                const std::string &code,
                                const std::string &message) {
    warningAtRange(shorthand_get_ast_source_range(node), code, message);
}

bool Diagnostics::hasErrors() const { return has_errors_; }
bool Diagnostics::hasWarnings() const { return has_warnings_; }
bool Diagnostics::hasDiagnostics() const { return !records_.empty(); }

std::pair<int, int> Diagnostics::locateAnchor(const std::string &anchor_kind,
                                               const std::string &anchor_name) const {
    std::vector<std::string> patterns;
    if (anchor_kind == "model") patterns.push_back("model " + anchor_name);
    else if (anchor_kind == "tensor") patterns.push_back("tensor " + anchor_name);
    else if (anchor_kind == "infer") {
        patterns.push_back("infer " + anchor_name + "(");
        patterns.push_back("infer ");
    } else if (anchor_kind == "greenai_contract") patterns.push_back("greenai_contract " + anchor_name);
    else if (anchor_kind == "greenai_measure") patterns.push_back("greenai_measure " + anchor_name);
    else if (anchor_kind == "keyword") patterns.push_back(anchor_name);

    for (size_t i = 0; i < source_lines_.size(); ++i) {
        const std::string &line = source_lines_[i];
        for (const auto &pattern : patterns) {
            if (contains(line, pattern)) return {static_cast<int>(i + 1), firstNonSpaceColumn(line)};
        }
    }
    return {0, 0};
}

void Diagnostics::print() const {
    for (const auto &record : records_) {
        const char *severity = record.severity == Severity::Warning ? "warning" : "error";
        const SourceRange &range = record.range;
        if (!source_file_.empty() && range.valid()) {
            std::cerr << source_file_ << ':' << range.begin.line << ':' << range.begin.column
                      << ": " << severity << ": ";
            printCode(record.code);
            std::cerr << record.message << " [range " << range.toString() << "]\n";
            if (range.begin.line <= static_cast<int>(source_lines_.size())) {
                const std::string &line = source_lines_[range.begin.line - 1];
                std::cerr << "  " << line << "\n  ";
                for (int i = 1; i < range.begin.column; ++i) std::cerr << ' ';
                int width = 1;
                if (range.begin.line == range.end.line)
                    width = std::max(1, range.end.column - range.begin.column + 1);
                for (int i = 0; i < width; ++i) std::cerr << '^';
                std::cerr << "\n";
            }
        } else {
            std::cerr << severity << ": ";
            printCode(record.code);
            std::cerr << record.message << "\n";
        }
    }
}
