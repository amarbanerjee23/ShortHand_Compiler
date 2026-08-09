#ifndef SHORTHAND_MODULE_AST_H
#define SHORTHAND_MODULE_AST_H

#include "SourceRange.h"

#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct AST_IMPORT_DECLARATION {
    std::string path;
    std::string alias;
    std::string source_path;
    SourceRange source_range;
};

class AST_MODULE_PREAMBLE {
private:
    std::string source_path_;
    bool has_package_ = false;
    bool has_module_ = false;
    std::string package_name_;
    std::string module_name_;
    SourceRange package_range_;
    SourceRange module_range_;
    std::vector<AST_IMPORT_DECLARATION> imports_;

    static std::string jsonEscape(const std::string &value) {
        std::ostringstream escaped;
        for (unsigned char ch : value) {
            switch (ch) {
                case '\\': escaped << "\\\\"; break;
                case '"': escaped << "\\\""; break;
                case '\n': escaped << "\\n"; break;
                case '\r': escaped << "\\r"; break;
                case '\t': escaped << "\\t"; break;
                default:
                    if (ch < 0x20U) {
                        const char hex[] = "0123456789ABCDEF";
                        escaped << "\\u00" << hex[(ch >> 4U) & 0x0FU] << hex[ch & 0x0FU];
                    } else {
                        escaped << static_cast<char>(ch);
                    }
            }
        }
        return escaped.str();
    }

    static void writeRange(std::ostream &out, const SourceRange &range) {
        out << "{\"begin\":{\"line\":" << range.begin.line
            << ",\"column\":" << range.begin.column
            << "},\"end\":{\"line\":" << range.end.line
            << ",\"column\":" << range.end.column << "}}";
    }

public:
    explicit AST_MODULE_PREAMBLE(std::string source_path)
        : source_path_(std::move(source_path)) {}

    bool hasPackage() const { return has_package_; }
    bool hasModule() const { return has_module_; }
    bool hasImports() const { return !imports_.empty(); }
    bool hasAnyDeclaration() const { return has_package_ || has_module_ || !imports_.empty(); }

    const std::string &packageName() const { return package_name_; }
    const std::string &moduleName() const { return module_name_; }
    const std::string &sourcePath() const { return source_path_; }
    const SourceRange &packageRange() const { return package_range_; }
    const SourceRange &moduleRange() const { return module_range_; }
    const std::vector<AST_IMPORT_DECLARATION> &imports() const { return imports_; }

    bool setPackage(const std::string &name, const SourceRange &range) {
        if (has_package_) return false;
        has_package_ = true;
        package_name_ = name;
        package_range_ = range;
        return true;
    }

    bool setModule(const std::string &name, const SourceRange &range) {
        if (has_module_) return false;
        has_module_ = true;
        module_name_ = name;
        module_range_ = range;
        return true;
    }

    bool hasImportPath(const std::string &path) const {
        for (const AST_IMPORT_DECLARATION &import_decl : imports_) {
            if (import_decl.path == path) return true;
        }
        return false;
    }

    bool hasImportAlias(const std::string &alias) const {
        if (alias.empty()) return false;
        for (const AST_IMPORT_DECLARATION &import_decl : imports_) {
            if (import_decl.alias == alias) return true;
        }
        return false;
    }

    void addImport(const std::string &path,
                   const std::string &alias,
                   const SourceRange &range) {
        imports_.push_back(AST_IMPORT_DECLARATION{path, alias, source_path_, range});
    }

    void writeJson(std::ostream &out) const {
        out << "{\"schema\":\"shorthand.module.ast.v1\""
            << ",\"language_version\":\"beta-0.3\""
            << ",\"source_path\":\"" << jsonEscape(source_path_) << "\""
            << ",\"resolver_status\":\"not_resolved\"";

        out << ",\"package\":";
        if (!has_package_) {
            out << "null";
        } else {
            out << "{\"name\":\"" << jsonEscape(package_name_) << "\",\"source_path\":\""
                << jsonEscape(source_path_) << "\",\"range\":";
            writeRange(out, package_range_);
            out << "}";
        }

        out << ",\"module\":";
        if (!has_module_) {
            out << "null";
        } else {
            out << "{\"name\":\"" << jsonEscape(module_name_) << "\",\"source_path\":\""
                << jsonEscape(source_path_) << "\",\"range\":";
            writeRange(out, module_range_);
            out << "}";
        }

        out << ",\"imports\":[";
        for (std::size_t index = 0; index < imports_.size(); ++index) {
            if (index != 0U) out << ',';
            const AST_IMPORT_DECLARATION &import_decl = imports_[index];
            out << "{\"path\":\"" << jsonEscape(import_decl.path)
                << "\",\"alias\":";
            if (import_decl.alias.empty()) {
                out << "null";
            } else {
                out << "\"" << jsonEscape(import_decl.alias) << "\"";
            }
            out << ",\"source_path\":\"" << jsonEscape(import_decl.source_path)
                << "\",\"range\":";
            writeRange(out, import_decl.source_range);
            out << "}";
        }
        out << "]}";
    }
};

#endif
