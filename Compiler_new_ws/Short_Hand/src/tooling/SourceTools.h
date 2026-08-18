#ifndef SHORTHAND_TOOLING_SOURCE_TOOLS_H
#define SHORTHAND_TOOLING_SOURCE_TOOLS_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace shorthand::tooling {

struct LintDiagnostic {
    std::string code;
    std::string message;
    std::size_t line = 1;
    std::size_t column = 1;
    bool fixable = true;
};

std::string formatSource(const std::string &source);
std::vector<LintDiagnostic> lintSource(const std::string &source);
void writeDiagnosticsJson(const std::string &path,
                          const std::vector<LintDiagnostic> &diagnostics,
                          std::ostream &out);

} // namespace shorthand::tooling

#endif
