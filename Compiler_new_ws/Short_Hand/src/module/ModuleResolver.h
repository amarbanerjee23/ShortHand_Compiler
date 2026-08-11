#ifndef SHORTHAND_MODULE_RESOLVER_H
#define SHORTHAND_MODULE_RESOLVER_H

#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace shorthand {
namespace modules {

struct ModuleUnitDescriptor {
    std::string module_name;
    std::string source_path;
    std::vector<std::string> imports;
};

struct PackageManifest {
    std::string manifest_path;
    std::string package_root;
    std::string package_name;
    std::map<std::string, std::string> module_paths;
};

class ModuleResolver {
public:
    bool loadForEntry(const std::string &entry_source,
                      std::string &code,
                      std::string &message);

    bool resolveModule(const std::string &module_name,
                       std::string &source_path,
                       std::string &code,
                       std::string &message) const;

    bool validateUnitIdentity(const ModuleUnitDescriptor &unit,
                              const std::string &declared_package,
                              std::string &code,
                              std::string &message) const;

    bool topologicalOrder(const std::map<std::string, ModuleUnitDescriptor> &units,
                          const std::string &entry_module,
                          std::vector<std::string> &ordered_modules,
                          std::string &code,
                          std::string &message) const;

    bool writeLockfile(const std::map<std::string, ModuleUnitDescriptor> &units,
                       const std::string &entry_module,
                       std::string &code,
                       std::string &message) const;

    bool verifyLockfile(const std::map<std::string, ModuleUnitDescriptor> &units,
                        const std::string &entry_module,
                        std::string &code,
                        std::string &message) const;

    void writeGraphJson(const std::map<std::string, ModuleUnitDescriptor> &units,
                        const std::vector<std::string> &ordered_modules,
                        const std::string &entry_module,
                        const std::string &lock_status,
                        std::ostream &out) const;

    const PackageManifest &manifest() const { return manifest_; }
    std::string lockfilePath() const;

private:
    PackageManifest manifest_;

    std::string expectedLockfile(const std::map<std::string, ModuleUnitDescriptor> &units,
                                 const std::string &entry_module,
                                 std::string &code,
                                 std::string &message) const;
};

}  // namespace modules
}  // namespace shorthand

#endif
