#include "Diagnostics.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
std::string trimLeft(const std::string &value) {
    size_t pos = 0;
    while (pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos]))) ++pos;
    return value.substr(pos);
}

bool contains(const std::string &line, const std::string &needle) {
    return !needle.empty() && line.find(needle) != std::string::npos;
}

int firstNonSpaceColumn(const std::string &line) {
    for (size_t i = 0; i < line.size(); ++i) {
        if (!std::isspace(static_cast<unsigned char>(line[i]))) return static_cast<int>(i + 1);
    }
    return 1;
}
}

void Diagnostics::setSourceFile(const std::string &path) {
    source_file_ = path;
    source_lines_.clear();

    std::ifstream in(path.c_str());
    std::string line;
    while (std::getline(in, line)) {
        source_lines_.push_back(line);
    }
}

void Diagnostics::add(Severity severity, const std::string &message, int line, int column) {
    DiagnosticRecord record;
    record.severity = severity;
    record.message = message;
    record.line = line;
    record.column = column > 0 ? column : 1;
    records_.push_back(record);
    if (severity == Severity::Error) has_errors_ = true;
}

void Diagnostics::error(const std::string &message) {
    add(Severity::Error, message);
}

void Diagnostics::warning(const std::string &message) {
    add(Severity::Warning, message);
}

void Diagnostics::errorAt(const std::string &anchor_kind, const std::string &anchor_name, const std::string &message) {
    auto loc = locateAnchor(anchor_kind, anchor_name);
    add(Severity::Error, message, loc.first, loc.second);
}

void Diagnostics::warningAt(const std::string &anchor_kind, const std::string &anchor_name, const std::string &message) {
    auto loc = locateAnchor(anchor_kind, anchor_name);
    add(Severity::Warning, message, loc.first, loc.second);
}

bool Diagnostics::hasErrors() const {
    return has_errors_;
}

std::pair<int, int> Diagnostics::locateAnchor(const std::string &anchor_kind, const std::string &anchor_name) const {
    std::vector<std::string> patterns;

    if (anchor_kind == "model") {
        patterns.push_back("model " + anchor_name);
    } else if (anchor_kind == "tensor") {
        patterns.push_back("tensor " + anchor_name);
    } else if (anchor_kind == "infer") {
        patterns.push_back("infer " + anchor_name + "(");
        patterns.push_back("infer ");
    } else if (anchor_kind == "greenai_contract") {
        patterns.push_back("greenai_contract " + anchor_name);
    } else if (anchor_kind == "greenai_measure") {
        patterns.push_back("greenai_measure " + anchor_name);
    } else if (anchor_kind == "keyword") {
        patterns.push_back(anchor_name);
    }

    for (size_t i = 0; i < source_lines_.size(); ++i) {
        const std::string &line = source_lines_[i];
        for (const auto &pattern : patterns) {
            if (contains(line, pattern)) {
                return {static_cast<int>(i + 1), firstNonSpaceColumn(line)};
            }
        }
    }

    return {0, 0};
}

void Diagnostics::print() const {
    for (const auto &record : records_) {
        const char *severity = record.severity == Severity::Warning ? "warning" : "error";
        if (!source_file_.empty() && record.line > 0) {
            std::cerr << source_file_ << ":" << record.line << ":" << record.column
                      << ": " << severity << ": " << record.message << "\n";
            if (record.line <= static_cast<int>(source_lines_.size())) {
                const std::string &line = source_lines_[record.line - 1];
                std::cerr << "  " << line << "\n";
                std::cerr << "  ";
                for (int i = 1; i < record.column; ++i) std::cerr << ' ';
                std::cerr << "^\n";
            }
        } else {
            std::cerr << severity << ": " << record.message << "\n";
        }
    }
}
