#include "SourceTools.h"

#include <algorithm>
#include <ostream>
#include <sstream>

namespace shorthand::tooling {
namespace {

struct ScanState {
    int braceDepth = 0;
    bool inBlockComment = false;
};

std::string normalizeNewlines(const std::string &source) {
    std::string out;
    out.reserve(source.size());
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\r') {
            if (i + 1 < source.size() && source[i + 1] == '\n') ++i;
            out.push_back('\n');
        } else {
            out.push_back(source[i]);
        }
    }
    return out;
}

std::vector<std::string> splitLines(const std::string &source) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < source.size()) {
        const std::size_t end = source.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(source.substr(start));
            return lines;
        }
        lines.push_back(source.substr(start, end - start));
        start = end + 1;
    }
    if (source.empty()) lines.emplace_back();
    return lines;
}

std::size_t leadingWhitespace(const std::string &line) {
    std::size_t n = 0;
    while (n < line.size() && (line[n] == ' ' || line[n] == '\t')) ++n;
    return n;
}

std::string rtrimHorizontal(std::string line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
    return line;
}

bool beginsWithClosingBrace(const std::string &content) {
    return !content.empty() && content.front() == '}';
}

void scanCode(const std::string &line, ScanState &state) {
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        const char next = i + 1 < line.size() ? line[i + 1] : '\0';

        if (state.inBlockComment) {
            if (c == '*' && next == '/') {
                state.inBlockComment = false;
                ++i;
            }
            continue;
        }

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            continue;
        }
        if (c == '/' && next == '/') break;
        if (c == '#') break;
        if (c == '/' && next == '*') {
            state.inBlockComment = true;
            ++i;
            continue;
        }
        if (c == '{') {
            ++state.braceDepth;
        } else if (c == '}') {
            state.braceDepth = std::max(0, state.braceDepth - 1);
        }
    }
}

std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
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
                if (c < 0x20) {
                    const char hex[] = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0xf] << hex[c & 0xf];
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    return out.str();
}

void addDiagnostic(std::vector<LintDiagnostic> &out,
                   const char *code,
                   const char *message,
                   std::size_t line,
                   std::size_t column) {
    out.push_back({code, message, line, column, true});
}

} // namespace

std::string formatSource(const std::string &source) {
    const std::string normalized = normalizeNewlines(source);
    const std::vector<std::string> lines = splitLines(normalized);
    std::vector<std::string> formatted;
    formatted.reserve(lines.size());

    ScanState state;
    bool previousBlank = false;
    for (const std::string &raw : lines) {
        const std::string trimmedRight = rtrimHorizontal(raw);
        const std::size_t prefixLength = leadingWhitespace(trimmedRight);
        const std::string content = trimmedRight.substr(prefixLength);

        if (content.empty()) {
            if (!formatted.empty() && !previousBlank) formatted.emplace_back();
            previousBlank = true;
            continue;
        }

        previousBlank = false;
        int targetDepth = state.braceDepth;
        if (!state.inBlockComment && beginsWithClosingBrace(content))
            targetDepth = std::max(0, targetDepth - 1);

        formatted.push_back(std::string(static_cast<std::size_t>(targetDepth) * 4, ' ') + content);
        scanCode(content, state);
    }

    while (!formatted.empty() && formatted.back().empty()) formatted.pop_back();

    std::ostringstream out;
    for (const std::string &line : formatted) out << line << '\n';
    return out.str();
}

std::vector<LintDiagnostic> lintSource(const std::string &source) {
    std::vector<LintDiagnostic> diagnostics;

    std::size_t crLine = 1;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\r') {
            addDiagnostic(diagnostics, "SHL006", "non-LF line ending", crLine, 1);
            break;
        }
        if (source[i] == '\n') ++crLine;
    }

    const std::string normalized = normalizeNewlines(source);
    const std::vector<std::string> lines = splitLines(normalized);
    ScanState state;
    bool previousBlank = false;

    for (std::size_t index = 0; index < lines.size(); ++index) {
        const std::string &line = lines[index];
        const std::size_t lineNumber = index + 1;
        const std::size_t prefixLength = leadingWhitespace(line);
        const std::string prefix = line.substr(0, prefixLength);
        const std::string trimmedRight = rtrimHorizontal(line);
        const std::string content = trimmedRight.substr(std::min(prefixLength, trimmedRight.size()));

        if (line.size() != trimmedRight.size())
            addDiagnostic(diagnostics, "SHL002", "trailing whitespace", lineNumber, trimmedRight.size() + 1);

        if (prefix.find('\t') != std::string::npos)
            addDiagnostic(diagnostics, "SHL001", "tab used for indentation; use four spaces per level", lineNumber, 1);

        if (content.empty()) {
            if (previousBlank)
                addDiagnostic(diagnostics, "SHL005", "more than one consecutive blank line", lineNumber, 1);
            previousBlank = true;
            continue;
        }
        previousBlank = false;

        int targetDepth = state.braceDepth;
        if (!state.inBlockComment && beginsWithClosingBrace(content))
            targetDepth = std::max(0, targetDepth - 1);
        const std::string expected(static_cast<std::size_t>(targetDepth) * 4, ' ');
        if (prefix != expected)
            addDiagnostic(diagnostics, "SHL003", "non-canonical indentation", lineNumber, 1);

        scanCode(content, state);
    }

    if (previousBlank && !lines.empty())
        addDiagnostic(diagnostics, "SHL005", "trailing blank line is not canonical", lines.size(), 1);

    if (source.empty() || source.back() != '\n')
        addDiagnostic(diagnostics, "SHL004", "source must end with exactly one newline", lines.size(),
                      lines.empty() ? 1 : lines.back().size() + 1);

    return diagnostics;
}

void writeDiagnosticsJson(const std::string &path,
                          const std::vector<LintDiagnostic> &diagnostics,
                          std::ostream &out) {
    out << "{\"schema\":\"shorthand.lint.v1\",\"path\":\"" << jsonEscape(path)
        << "\",\"diagnostics\":[";
    for (std::size_t i = 0; i < diagnostics.size(); ++i) {
        if (i != 0) out << ',';
        const LintDiagnostic &d = diagnostics[i];
        out << "{\"code\":\"" << jsonEscape(d.code)
            << "\",\"severity\":\"warning\",\"line\":" << d.line
            << ",\"column\":" << d.column
            << ",\"message\":\"" << jsonEscape(d.message)
            << "\",\"fixable\":" << (d.fixable ? "true" : "false") << '}';
    }
    out << "]}\n";
}

} // namespace shorthand::tooling
