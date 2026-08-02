#ifndef SHORTHAND_DIAGNOSTICS_H
#define SHORTHAND_DIAGNOSTICS_H

#include "../ast/SourceRange.h"

#include <string>
#include <vector>

class Diagnostics {
public:
    void setSourceFile(const std::string &path);

    void error(const std::string &message);
    void warning(const std::string &message);
    void errorAt(const std::string &anchor_kind, const std::string &anchor_name, const std::string &message);
    void warningAt(const std::string &anchor_kind, const std::string &anchor_name, const std::string &message);
    void errorAtRange(const SourceRange &range, const std::string &message);
    void warningAtRange(const SourceRange &range, const std::string &message);
    void errorAtNode(const void *node, const std::string &message);
    void warningAtNode(const void *node, const std::string &message);

    bool hasErrors() const;
    void print() const;

private:
    enum class Severity { Warning, Error };
    struct DiagnosticRecord {
        Severity severity;
        std::string message;
        SourceRange range;
    };

    std::string source_file_;
    std::vector<std::string> source_lines_;
    std::vector<DiagnosticRecord> records_;
    bool has_errors_ = false;

    void add(Severity severity, const std::string &message, const SourceRange &range = {});
    std::pair<int, int> locateAnchor(const std::string &anchor_kind, const std::string &anchor_name) const;
};

#endif
