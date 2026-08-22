#ifndef SHORTHAND_ENTERPRISE_LANGUAGE_H
#define SHORTHAND_ENTERPRISE_LANGUAGE_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace shorthand::enterprise {

inline constexpr const char *kContractVersion = "shorthand.enterprise_language.v1";

struct ValidationSummary {
    std::string namespace_name;
    std::vector<std::string> canonical_types;
    std::size_t ownership_values = 0;
    std::size_t ownership_operations = 0;
};

bool validateSourceFile(const std::string &path,
                        ValidationSummary &summary,
                        std::string &code,
                        std::string &message,
                        std::size_t &line);

void writeSummaryJson(const ValidationSummary &summary, std::ostream &out);

}  // namespace shorthand::enterprise

#endif
