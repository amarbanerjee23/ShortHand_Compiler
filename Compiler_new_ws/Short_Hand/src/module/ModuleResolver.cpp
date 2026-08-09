#include "ModuleResolver.h"

#include "../visitors/DiagnosticCodes.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace fs = std::filesystem;
namespace diag = shorthand::diagnostics;

namespace shorthand {
namespace modules {
namespace {

std::string trim(const std::string &value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return value.substr(begin, end - begin);
}

std::vector<std::string> words(const std::string &line) {
    std::istringstream in(line);
    std::vector<std::string> out;
    std::string value;
    while (in >> value) out.push_back(value);
    return out;
}

bool validIdentifierSegment(const std::string &segment) {
    if (segment.empty()) return false;
    const unsigned char first = static_cast<unsigned char>(segment.front());
    if (!(std::isalpha(first) || first == '_')) return false;
    for (unsigned char ch : segment) {
        if (!(std::isalnum(ch) || ch == '_')) return false;
    }
    return true;
}

bool validModuleName(const std::string &value) {
    if (value.empty() || value.front() == '.' || value.back() == '.') return false;
    std::size_t begin = 0;
    while (begin < value.size()) {
        const std::size_t dot = value.find('.', begin);
        const std::size_t end = dot == std::string::npos ? value.size() : dot;
        if (!validIdentifierSegment(value.substr(begin, end - begin))) return false;
        if (dot == std::string::npos) break;
        begin = dot + 1;
    }
    return true;
}

bool startsWithPackage(const std::string &module_name, const std::string &package_name) {
    if (module_name == package_name) return true;
    return module_name.size() > package_name.size() &&
           module_name.compare(0, package_name.size(), package_name) == 0 &&
           module_name[package_name.size()] == '.';
}

bool relativePathIsSafe(const fs::path &path) {
    if (path.empty() || path.is_absolute()) return false;
    for (const fs::path &part : path) {
        if (part == "..") return false;
    }
    return path.lexically_normal().extension() == ".short";
}

bool pathWithinRoot(const fs::path &root, const fs::path &candidate) {
    std::error_code ec;
    fs::path relative = fs::relative(candidate, root, ec);
    if (ec || relative.empty()) return !ec && candidate == root;
    if (relative.is_absolute()) return false;
    for (const fs::path &part : relative) {
        if (part == "..") return false;
    }
    return true;
}

std::string canonicalString(const fs::path &path, std::error_code &ec) {
    fs::path value = fs::weakly_canonical(path, ec);
    if (ec) return std::string();
    return value.generic_string();
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
            default:
                if (ch < 0x20U) {
                    const char hex[] = "0123456789ABCDEF";
                    out << "\\u00" << hex[(ch >> 4U) & 0x0FU] << hex[ch & 0x0FU];
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

bool fingerprintFile(const std::string &path, std::string &fingerprint) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::uint64_t hash = 14695981039346656037ULL;
    char buffer[8192];
    while (in) {
        in.read(buffer, sizeof(buffer));
        const std::streamsize count = in.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= 1099511628211ULL;
        }
    }
    std::ostringstream out;
    out << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    fingerprint = out.str();
    return true;
}

std::string relativeToRoot(const PackageManifest &manifest, const std::string &source_path) {
    std::error_code ec;
    fs::path relative = fs::relative(fs::path(source_path), fs::path(manifest.package_root), ec);
    if (ec) return std::string();
    return relative.generic_string();
}

}  // namespace

bool ModuleResolver::loadForEntry(const std::string &entry_source,
                                  std::string &code,
                                  std::string &message) {
    manifest_ = PackageManifest();
    std::error_code ec;
    fs::path entry = fs::weakly_canonical(fs::path(entry_source), ec);
    if (ec || entry.empty()) {
        code = diag::ModuleManifestInvalid;
        message = "cannot canonicalize entry source: " + entry_source;
        return false;
    }

    fs::path current = entry.parent_path();
    fs::path manifest_path;
    while (!current.empty()) {
        fs::path candidate = current / "shorthand.package";
        if (fs::exists(candidate, ec) && !ec) {
            manifest_path = candidate;
            break;
        }
        fs::path parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    if (manifest_path.empty()) {
        code = diag::ModuleManifestNotFound;
        message = "no shorthand.package found for module entry: " + entry_source;
        return false;
    }

    manifest_.manifest_path = canonicalString(manifest_path, ec);
    if (ec || manifest_.manifest_path.empty()) {
        code = diag::ModuleManifestInvalid;
        message = "cannot canonicalize package manifest";
        return false;
    }
    manifest_.package_root = fs::path(manifest_.manifest_path).parent_path().generic_string();

    std::ifstream in(manifest_.manifest_path);
    if (!in) {
        code = diag::ModuleManifestInvalid;
        message = "cannot read package manifest: " + manifest_.manifest_path;
        return false;
    }

    bool format_seen = false;
    bool package_seen = false;
    std::map<std::string, std::string> path_to_module;
    std::string raw;
    std::size_t line_number = 0;
    while (std::getline(in, raw)) {
        ++line_number;
        std::size_t comment = raw.find('#');
        if (comment != std::string::npos) raw.erase(comment);
        raw = trim(raw);
        if (raw.empty()) continue;
        const std::vector<std::string> parts = words(raw);
        if (parts.size() == 2U && parts[0] == "format") {
            if (format_seen || parts[1] != "shorthand.package.v1") {
                code = diag::ModuleManifestInvalid;
                message = "invalid or duplicate manifest format at line " + std::to_string(line_number);
                return false;
            }
            format_seen = true;
            continue;
        }
        if (parts.size() == 2U && parts[0] == "package") {
            if (package_seen || !validModuleName(parts[1])) {
                code = diag::ModuleManifestInvalid;
                message = "invalid or duplicate package declaration at line " + std::to_string(line_number);
                return false;
            }
            manifest_.package_name = parts[1];
            package_seen = true;
            continue;
        }
        if (parts.size() == 3U && parts[0] == "module") {
            if (!validModuleName(parts[1])) {
                code = diag::ModuleManifestInvalid;
                message = "invalid module identity in manifest at line " + std::to_string(line_number);
                return false;
            }
            if (!relativePathIsSafe(fs::path(parts[2]))) {
                code = diag::ModulePathEscape;
                message = "unsafe module path in manifest at line " + std::to_string(line_number) + ": " + parts[2];
                return false;
            }
            if (manifest_.module_paths.count(parts[1]) != 0U) {
                code = diag::ModuleAmbiguousMapping;
                message = "module appears more than once in manifest: " + parts[1];
                return false;
            }
            fs::path candidate = fs::path(manifest_.package_root) / fs::path(parts[2]);
            std::string canonical = canonicalString(candidate, ec);
            if (ec || canonical.empty() || !pathWithinRoot(fs::path(manifest_.package_root), fs::path(canonical))) {
                code = diag::ModulePathEscape;
                message = "module path escapes package root: " + parts[2];
                return false;
            }
            if (path_to_module.count(canonical) != 0U) {
                code = diag::ModuleAmbiguousMapping;
                message = "multiple modules map to the same source path: " + parts[1] + " and " + path_to_module[canonical];
                return false;
            }
            manifest_.module_paths[parts[1]] = parts[2];
            path_to_module[canonical] = parts[1];
            continue;
        }
        code = diag::ModuleManifestInvalid;
        message = "invalid shorthand.package record at line " + std::to_string(line_number);
        return false;
    }

    if (!format_seen || !package_seen || manifest_.module_paths.empty()) {
        code = diag::ModuleManifestInvalid;
        message = "manifest requires format, package, and at least one module record";
        return false;
    }
    for (const auto &entry_pair : manifest_.module_paths) {
        if (!startsWithPackage(entry_pair.first, manifest_.package_name)) {
            code = diag::ModulePackageMismatch;
            message = "module is outside declared package namespace: " + entry_pair.first;
            return false;
        }
    }
    return true;
}

bool ModuleResolver::resolveModule(const std::string &module_name,
                                   std::string &source_path,
                                   std::string &code,
                                   std::string &message) const {
    auto it = manifest_.module_paths.find(module_name);
    if (it == manifest_.module_paths.end()) {
        code = diag::ModuleNotFound;
        message = "module is not declared in shorthand.package: " + module_name;
        return false;
    }
    std::error_code ec;
    fs::path root(manifest_.package_root);
    fs::path candidate = root / fs::path(it->second);
    std::string canonical = canonicalString(candidate, ec);
    if (ec || canonical.empty() || !pathWithinRoot(root, fs::path(canonical))) {
        code = diag::ModulePathEscape;
        message = "resolved module path escapes package root: " + module_name;
        return false;
    }
    if (!fs::is_regular_file(fs::path(canonical), ec) || ec) {
        code = diag::ModuleNotFound;
        message = "resolved module file does not exist: " + canonical;
        return false;
    }
    source_path = canonical;
    return true;
}

bool ModuleResolver::validateUnitIdentity(const ModuleUnitDescriptor &unit,
                                          const std::string &declared_package,
                                          std::string &code,
                                          std::string &message) const {
    if (unit.module_name.empty()) {
        code = diag::ModuleIdentityMismatch;
        message = "resolved package source is missing a module declaration: " + unit.source_path;
        return false;
    }
    if (declared_package.empty() || declared_package != manifest_.package_name) {
        code = diag::ModulePackageMismatch;
        message = "source package declaration does not match shorthand.package for module " + unit.module_name;
        return false;
    }
    std::string expected_path;
    if (!resolveModule(unit.module_name, expected_path, code, message)) return false;
    std::error_code ec;
    std::string actual = canonicalString(fs::path(unit.source_path), ec);
    if (ec || actual != expected_path) {
        code = diag::ModuleIdentityMismatch;
        message = "module declaration and manifest path disagree for " + unit.module_name;
        return false;
    }
    return true;
}

bool ModuleResolver::topologicalOrder(const std::map<std::string, ModuleUnitDescriptor> &units,
                                      const std::string &entry_module,
                                      std::vector<std::string> &ordered_modules,
                                      std::string &code,
                                      std::string &message) const {
    ordered_modules.clear();
    if (units.count(entry_module) == 0U) {
        code = diag::ModuleIdentityMismatch;
        message = "entry module is absent from resolved graph: " + entry_module;
        return false;
    }

    std::map<std::string, int> state;
    std::vector<std::string> stack;
    std::function<bool(const std::string &)> visit = [&](const std::string &name) -> bool {
        if (state[name] == 2) return true;
        if (state[name] == 1) {
            std::ostringstream cycle;
            bool emit = false;
            for (const std::string &item : stack) {
                if (item == name) emit = true;
                if (emit) cycle << item << " -> ";
            }
            cycle << name;
            code = diag::ModuleImportCycle;
            message = "module import cycle: " + cycle.str();
            return false;
        }
        auto unit_it = units.find(name);
        if (unit_it == units.end()) {
            code = diag::ModuleNotFound;
            message = "resolved graph is missing imported module: " + name;
            return false;
        }
        state[name] = 1;
        stack.push_back(name);
        std::vector<std::string> imports = unit_it->second.imports;
        std::sort(imports.begin(), imports.end());
        for (const std::string &dependency : imports) {
            if (!visit(dependency)) return false;
        }
        stack.pop_back();
        state[name] = 2;
        ordered_modules.push_back(name);
        return true;
    };

    return visit(entry_module);
}

std::string ModuleResolver::expectedLockfile(
    const std::map<std::string, ModuleUnitDescriptor> &units,
    const std::string &entry_module,
    std::string &code,
    std::string &message) const {
    std::ostringstream out;
    out << "format shorthand.lock.v1\n";
    out << "package " << manifest_.package_name << "\n";
    out << "entry " << entry_module << "\n";
    for (const auto &pair : units) {
        std::string relative = relativeToRoot(manifest_, pair.second.source_path);
        if (relative.empty()) {
            code = diag::ModulePathEscape;
            message = "cannot express module path relative to package root: " + pair.first;
            return std::string();
        }
        std::string fingerprint;
        if (!fingerprintFile(pair.second.source_path, fingerprint)) {
            code = diag::ModuleNotFound;
            message = "cannot fingerprint module source: " + pair.second.source_path;
            return std::string();
        }
        out << "module " << pair.first << ' ' << relative << ' ' << fingerprint << "\n";
    }
    return out.str();
}

std::string ModuleResolver::lockfilePath() const {
    return (fs::path(manifest_.package_root) / "shorthand.lock").generic_string();
}

bool ModuleResolver::writeLockfile(const std::map<std::string, ModuleUnitDescriptor> &units,
                                   const std::string &entry_module,
                                   std::string &code,
                                   std::string &message) const {
    const std::string expected = expectedLockfile(units, entry_module, code, message);
    if (expected.empty()) return false;
    const fs::path target(lockfilePath());
    const fs::path temp = target.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            code = diag::ModuleLockfileMismatch;
            message = "cannot create lockfile: " + target.generic_string();
            return false;
        }
        out << expected;
        if (!out.good()) {
            code = diag::ModuleLockfileMismatch;
            message = "failed writing lockfile: " + target.generic_string();
            return false;
        }
    }
    std::error_code ec;
    fs::remove(target, ec);
    ec.clear();
    fs::rename(temp, target, ec);
    if (ec) {
        fs::remove(temp, ec);
        code = diag::ModuleLockfileMismatch;
        message = "cannot publish lockfile: " + target.generic_string();
        return false;
    }
    return true;
}

bool ModuleResolver::verifyLockfile(const std::map<std::string, ModuleUnitDescriptor> &units,
                                    const std::string &entry_module,
                                    std::string &code,
                                    std::string &message) const {
    const std::string expected = expectedLockfile(units, entry_module, code, message);
    if (expected.empty()) return false;
    std::ifstream in(lockfilePath(), std::ios::binary);
    if (!in) {
        code = diag::ModuleLockfileMismatch;
        message = "missing shorthand.lock; run `short_hand <entry.short> lock`";
        return false;
    }
    std::ostringstream actual;
    actual << in.rdbuf();
    if (actual.str() != expected) {
        code = diag::ModuleLockfileMismatch;
        message = "shorthand.lock is stale or does not match the resolved module graph";
        return false;
    }
    return true;
}

void ModuleResolver::writeGraphJson(const std::map<std::string, ModuleUnitDescriptor> &units,
                                    const std::vector<std::string> &ordered_modules,
                                    const std::string &entry_module,
                                    const std::string &lock_status,
                                    std::ostream &out) const {
    out << "{\"schema\":\"shorthand.module.graph.v1\""
        << ",\"package\":\"" << jsonEscape(manifest_.package_name) << "\""
        << ",\"manifest\":\"" << jsonEscape(manifest_.manifest_path) << "\""
        << ",\"entry\":\"" << jsonEscape(entry_module) << "\""
        << ",\"lock_status\":\"" << jsonEscape(lock_status) << "\""
        << ",\"modules\":[";
    bool first = true;
    for (const auto &pair : units) {
        if (!first) out << ',';
        first = false;
        out << "{\"name\":\"" << jsonEscape(pair.first)
            << "\",\"source_path\":\"" << jsonEscape(pair.second.source_path)
            << "\",\"imports\":[";
        for (std::size_t i = 0; i < pair.second.imports.size(); ++i) {
            if (i != 0U) out << ',';
            out << '"' << jsonEscape(pair.second.imports[i]) << '"';
        }
        out << "]}";
    }
    out << "],\"order\":[";
    for (std::size_t i = 0; i < ordered_modules.size(); ++i) {
        if (i != 0U) out << ',';
        out << '"' << jsonEscape(ordered_modules[i]) << '"';
    }
    out << "]}";
}

}  // namespace modules
}  // namespace shorthand
