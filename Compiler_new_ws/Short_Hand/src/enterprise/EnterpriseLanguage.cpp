#include "EnterpriseLanguage.h"

#include "../parser/ParserLimits.h"
#include "../type_system/ProductionTypeSystem.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace shorthand::enterprise {
namespace {

namespace fs = std::filesystem;

using shorthand::types::Field;
using shorthand::types::OwnershipTracker;
using shorthand::types::TypeDescriptor;
using shorthand::types::TypeKind;

struct Borrow {
    std::string owner;
    bool mutable_borrow = false;
};

std::string trim(const std::string &value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return value.substr(begin, end - begin);
}

std::string withoutComment(const std::string &value) {
    std::size_t marker = value.find('#');
    const std::size_t slash = value.find("//");
    if (slash != std::string::npos && (marker == std::string::npos || slash < marker)) marker = slash;
    return trim(marker == std::string::npos ? value : value.substr(0, marker));
}

std::vector<std::string> words(std::string value) {
    for (char &ch : value) {
        if (ch == ';' || ch == '{' || ch == '}' || ch == '<' || ch == '>' || ch == ',') ch = ' ';
    }
    std::istringstream in(value);
    std::vector<std::string> result;
    std::string word;
    while (in >> word) result.push_back(word);
    return result;
}

bool validIdentifier(const std::string &value) {
    if (value.empty()) return false;
    const unsigned char first = static_cast<unsigned char>(value.front());
    if (!(std::isalpha(first) || first == '_')) return false;
    for (unsigned char ch : value) {
        if (!(std::isalnum(ch) || ch == '_')) return false;
    }
    return true;
}

bool validNamespace(const std::string &value) {
    if (value.empty() || value.front() == '.' || value.back() == '.') return false;
    std::size_t begin = 0;
    while (begin < value.size()) {
        const std::size_t dot = value.find('.', begin);
        const std::size_t end = dot == std::string::npos ? value.size() : dot;
        if (!validIdentifier(value.substr(begin, end - begin))) return false;
        if (dot == std::string::npos) break;
        begin = dot + 1;
    }
    return true;
}

bool scalarKind(const std::string &name, TypeKind &kind) {
    if (name == "bool") kind = TypeKind::Bool;
    else if (name == "int32") kind = TypeKind::Int32;
    else if (name == "float64") kind = TypeKind::Float64;
    else if (name == "string") kind = TypeKind::String;
    else return false;
    return true;
}

bool fail(std::string &code,
          std::string &message,
          std::size_t &failure_line,
          std::size_t current_line,
          const std::string &failure_code,
          const std::string &failure_message) {
    code = failure_code;
    message = failure_message;
    failure_line = current_line;
    return false;
}

std::string diagnosticCode(const std::string &diagnostic, const std::string &fallback) {
    if (diagnostic.size() >= 7U && diagnostic.compare(0, 3, "SHD") == 0) return diagnostic.substr(0, 7);
    return fallback;
}

std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << static_cast<char>(ch); break;
        }
    }
    return out.str();
}

}  // namespace

bool validateSourceFile(const std::string &path,
                        ValidationSummary &summary,
                        std::string &code,
                        std::string &message,
                        std::size_t &line) {
    summary = ValidationSummary{};
    code.clear();
    message.clear();
    line = 1;

    std::error_code source_error;
    if (!fs::is_regular_file(fs::path(path), source_error) || source_error)
        return fail(code, message, line, 1, "SHD3024", "enterprise source must be a readable regular file");
    const std::uintmax_t source_bytes = fs::file_size(fs::path(path), source_error);
    if (source_error)
        return fail(code, message, line, 1, "SHD3024", "cannot inspect enterprise source size");
    if (source_bytes > shorthand::parser::currentParserLimits().max_source_bytes)
        return fail(code, message, line, 1, "SHD2004", "enterprise source exceeds the source-size limit");

    std::ifstream in(path);
    if (!in) return fail(code, message, line, 1, "SHD3024", "cannot read enterprise source");

    bool contract_seen = false;
    bool namespace_seen = false;
    bool in_record = false;
    bool in_enum = false;
    std::string active_name;
    std::vector<Field> active_fields;
    std::vector<std::string> active_variants;
    std::map<std::string, TypeDescriptor> types;
    std::map<std::string, std::string> value_types;
    std::map<std::string, Borrow> borrows;
    OwnershipTracker ownership;

    auto register_type = [&](const std::string &name,
                             TypeDescriptor descriptor,
                             std::size_t current_line) -> bool {
        if (!validIdentifier(name))
            return fail(code, message, line, current_line, "SHD3024", "invalid enterprise type name `" + name + "`");
        if (types.count(name) != 0U)
            return fail(code, message, line, current_line, "SHD3025", "duplicate enterprise type `" + name + "`");
        std::string diagnostic;
        if (!descriptor.validate(diagnostic))
            return fail(code, message, line, current_line, diagnosticCode(diagnostic, "SHD3024"), diagnostic);
        types.emplace(name, std::move(descriptor));
        return true;
    };

    std::string raw;
    std::size_t current_line = 0;
    std::size_t statement_lines = 0;
    while (std::getline(in, raw)) {
        ++current_line;
        if (raw.size() > shorthand::parser::currentParserLimits().max_token_bytes)
            return fail(code, message, line, current_line, "SHD2005", "enterprise source line exceeds the token-size limit");
        const std::string text = withoutComment(raw);
        if (text.empty()) continue;
        ++statement_lines;
        if (statement_lines > shorthand::parser::currentParserLimits().max_scanner_matches)
            return fail(code, message, line, current_line, "SHD2006", "enterprise source exceeds the statement-work limit");
        const std::vector<std::string> parts = words(text);

        if (in_record) {
            if (text == "};") {
                if (!register_type(active_name,
                                   TypeDescriptor::record(summary.namespace_name + "." + active_name,
                                                          active_fields),
                                   current_line)) return false;
                in_record = false;
                active_name.clear();
                active_fields.clear();
                continue;
            }
            if (parts.size() != 2U || text.back() != ';')
                return fail(code, message, line, current_line, "SHD3024", "record field must be `<scalar> <name>;`");
            TypeKind field_kind = TypeKind::Void;
            if (!scalarKind(parts[0], field_kind) || !validIdentifier(parts[1]))
                return fail(code, message, line, current_line, "SHD3024", "record fields require bool, int32, float64, or string and a valid name");
            active_fields.push_back(Field{parts[1], field_kind});
            continue;
        }

        if (in_enum) {
            if (text == "};") {
                if (!register_type(active_name,
                                   TypeDescriptor::enumeration(summary.namespace_name + "." + active_name,
                                                               active_variants),
                                   current_line)) return false;
                in_enum = false;
                active_name.clear();
                active_variants.clear();
                continue;
            }
            if (parts.size() != 1U || text.back() != ';' || !validIdentifier(parts[0]))
                return fail(code, message, line, current_line, "SHD3024", "enum variant must be `<name>;`");
            active_variants.push_back(parts[0]);
            continue;
        }

        if (parts.empty())
            return fail(code, message, line, current_line, "SHD3024", "unsupported enterprise punctuation");

        if (!contract_seen) {
            if (parts.size() != 2U || parts[0] != "language" || parts[1] != kContractVersion || text.back() != ';')
                return fail(code, message, line, current_line, "SHD3024", "first declaration must be `language shorthand.enterprise_language.v1;`");
            contract_seen = true;
            continue;
        }
        if (!namespace_seen) {
            if (parts.size() != 2U || parts[0] != "namespace" || !validNamespace(parts[1]) || text.back() != ';')
                return fail(code, message, line, current_line, "SHD3024", "second declaration must be a valid namespace");
            summary.namespace_name = parts[1];
            namespace_seen = true;
            continue;
        }

        if (parts[0] == "record") {
            if (parts.size() != 2U || text.back() != '{' || !validIdentifier(parts[1]))
                return fail(code, message, line, current_line, "SHD3024", "record declaration must be `record <name> {`");
            in_record = true;
            active_name = parts[1];
            active_fields.clear();
            continue;
        }
        if (parts[0] == "enum") {
            if (parts.size() != 2U || text.back() != '{' || !validIdentifier(parts[1]))
                return fail(code, message, line, current_line, "SHD3024", "enum declaration must be `enum <name> {`");
            in_enum = true;
            active_name = parts[1];
            active_variants.clear();
            continue;
        }
        if (parts[0] == "slice" || parts[0] == "option") {
            if (parts.size() != 3U || text.back() != ';')
                return fail(code, message, line, current_line, "SHD3024", "slice/option declaration requires a name and scalar payload");
            TypeKind payload = TypeKind::Void;
            if (!scalarKind(parts[2], payload))
                return fail(code, message, line, current_line, "SHD3024", "slice/option payload must be scalar");
            const TypeDescriptor descriptor = parts[0] == "slice"
                ? TypeDescriptor::slice(payload)
                : TypeDescriptor::option(payload);
            if (!register_type(parts[1], descriptor, current_line)) return false;
            continue;
        }
        if (parts[0] == "result") {
            if (parts.size() != 4U || text.back() != ';')
                return fail(code, message, line, current_line, "SHD3024", "result declaration requires a name, ok scalar, and error scalar");
            TypeKind ok = TypeKind::Void;
            TypeKind error = TypeKind::Void;
            if (!scalarKind(parts[2], ok) || !scalarKind(parts[3], error))
                return fail(code, message, line, current_line, "SHD3024", "result payloads must be scalar");
            if (!register_type(parts[1], TypeDescriptor::result(ok, error), current_line)) return false;
            continue;
        }
        if (parts[0] == "owned") {
            if (parts.size() != 3U || text.back() != ';' || types.count(parts[1]) == 0U || !validIdentifier(parts[2]))
                return fail(code, message, line, current_line, "SHD3024", "owned declaration requires a declared enterprise type and value name");
            if (value_types.count(parts[2]) != 0U || borrows.count(parts[2]) != 0U)
                return fail(code, message, line, current_line, "SHD3025", "duplicate ownership value `" + parts[2] + "`");
            std::string diagnostic;
            if (!ownership.declareValue(parts[2], diagnostic) || !ownership.initialize(parts[2], diagnostic))
                return fail(code, message, line, current_line, "SHD3016", diagnostic);
            value_types[parts[2]] = parts[1];
            ++summary.ownership_operations;
            continue;
        }
        if (parts[0] == "move") {
            if (parts.size() != 4U || parts[2] != "to" || text.back() != ';' || value_types.count(parts[1]) == 0U || !validIdentifier(parts[3]))
                return fail(code, message, line, current_line, "SHD3024", "move must be `move <owner> to <new_owner>;`");
            if (value_types.count(parts[3]) != 0U || borrows.count(parts[3]) != 0U)
                return fail(code, message, line, current_line, "SHD3025", "duplicate ownership value `" + parts[3] + "`");
            std::string diagnostic;
            if (!ownership.moveValue(parts[1], diagnostic) ||
                !ownership.declareValue(parts[3], diagnostic) ||
                !ownership.initialize(parts[3], diagnostic))
                return fail(code, message, line, current_line, "SHD3016", diagnostic);
            value_types[parts[3]] = value_types[parts[1]];
            ++summary.ownership_operations;
            continue;
        }
        if (parts[0] == "borrow") {
            const bool shared = parts.size() == 5U && parts[1] == "shared";
            const bool mutable_borrow = parts.size() == 5U && parts[1] == "mutable";
            if ((!shared && !mutable_borrow) || parts[3] != "as" || text.back() != ';' ||
                value_types.count(parts[2]) == 0U || !validIdentifier(parts[4]))
                return fail(code, message, line, current_line, "SHD3024", "borrow must be `borrow shared|mutable <owner> as <view>;`");
            if (borrows.count(parts[4]) != 0U || value_types.count(parts[4]) != 0U)
                return fail(code, message, line, current_line, "SHD3025", "duplicate ownership identity `" + parts[4] + "`");
            std::string diagnostic;
            const bool accepted = mutable_borrow
                ? ownership.borrowMutable(parts[2], diagnostic)
                : ownership.borrowShared(parts[2], diagnostic);
            if (!accepted) return fail(code, message, line, current_line, "SHD3016", diagnostic);
            borrows.emplace(parts[4], Borrow{parts[2], mutable_borrow});
            ++summary.ownership_operations;
            continue;
        }
        if (parts[0] == "release") {
            if (parts.size() != 2U || text.back() != ';' || borrows.count(parts[1]) == 0U)
                return fail(code, message, line, current_line, "SHD3016", "release references an unknown borrow");
            const Borrow borrow = borrows.at(parts[1]);
            std::string diagnostic;
            const bool accepted = borrow.mutable_borrow
                ? ownership.releaseMutable(borrow.owner, diagnostic)
                : ownership.releaseShared(borrow.owner, diagnostic);
            if (!accepted) return fail(code, message, line, current_line, "SHD3016", diagnostic);
            borrows.erase(parts[1]);
            ++summary.ownership_operations;
            continue;
        }
        if (parts[0] == "read" || parts[0] == "assign") {
            if (parts.size() != 2U || text.back() != ';' || value_types.count(parts[1]) == 0U)
                return fail(code, message, line, current_line, "SHD3024", "read/assign requires a declared ownership value");
            std::string diagnostic;
            const bool accepted = parts[0] == "read"
                ? ownership.read(parts[1], diagnostic)
                : ownership.assign(parts[1], diagnostic);
            if (!accepted) return fail(code, message, line, current_line, "SHD3016", diagnostic);
            ++summary.ownership_operations;
            continue;
        }

        return fail(code, message, line, current_line, "SHD3024", "unsupported enterprise declaration or ownership operation");
    }

    if (!contract_seen || !namespace_seen)
        return fail(code, message, line, current_line == 0 ? 1 : current_line, "SHD3024", "enterprise source is missing its contract or namespace declaration");
    if (in_record || in_enum)
        return fail(code, message, line, current_line, "SHD3024", "unterminated composite declaration");
    if (!borrows.empty())
        return fail(code, message, line, current_line, "SHD3016", "all borrows must be released before end of source");
    if (types.empty())
        return fail(code, message, line, current_line, "SHD3024", "enterprise source must declare at least one composite type");

    for (const auto &entry : types)
        summary.canonical_types.push_back(summary.namespace_name + "::" + entry.first + "=" + entry.second.canonicalName());
    std::sort(summary.canonical_types.begin(), summary.canonical_types.end());
    summary.ownership_values = value_types.size();
    return true;
}

void writeSummaryJson(const ValidationSummary &summary, std::ostream &out) {
    out << "{\"schema\":\"" << kContractVersion << "\""
        << ",\"namespace\":\"" << jsonEscape(summary.namespace_name) << "\""
        << ",\"types\":[";
    for (std::size_t i = 0; i < summary.canonical_types.size(); ++i) {
        if (i != 0U) out << ',';
        out << '"' << jsonEscape(summary.canonical_types[i]) << '"';
    }
    out << "],\"ownership_values\":" << summary.ownership_values
        << ",\"ownership_operations\":" << summary.ownership_operations
        << ",\"production_claim\":false}";
}

}  // namespace shorthand::enterprise
