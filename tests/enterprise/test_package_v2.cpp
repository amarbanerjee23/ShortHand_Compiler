#include "Compiler_new_ws/Short_Hand/src/module/ModuleResolver.h"
#include "Compiler_new_ws/Short_Hand/src/module/Sha256.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using shorthand::modules::ModuleResolver;
using shorthand::modules::ModuleUnitDescriptor;

namespace {

int failures = 0;

void expect(bool condition, const std::string &message) {
    if (condition) return;
    std::cerr << "FAIL " << message << '\n';
    ++failures;
}

bool writeFile(const fs::path &path, const std::string &contents) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << contents;
    return out.good();
}

}  // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: test_package_v2 <temporary-directory>\n";
        return 2;
    }
    const fs::path root = fs::path(argv[1]) / "package-v2-unit";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "src", ec);
    fs::create_directories(root / "vendor/acme.math/src", ec);
    if (ec) {
        std::cerr << "FAIL unable to create package v2 unit fixture\n";
        return 2;
    }

    const std::string dependency_manifest =
        "format shorthand.package.v2\n"
        "package acme.math\n"
        "version 2.1.0\n"
        "license MIT\n"
        "module acme.math.ops src/ops.short\n";
    expect(writeFile(root / "vendor/acme.math/shorthand.package", dependency_manifest),
           "write dependency manifest");
    expect(writeFile(root / "vendor/acme.math/src/ops.short", "package acme.math;\nmodule acme.math.ops;\n"),
           "write dependency source");
    expect(writeFile(root / "src/main.short", "package acme.app;\nmodule acme.app.main;\n"),
           "write root source");

    std::string dependency_digest;
    expect(shorthand::crypto::sha256File((root / "vendor/acme.math/shorthand.package").string(), dependency_digest),
           "hash dependency manifest");
    const std::string root_manifest =
        "format shorthand.package.v2\n"
        "package acme.app\n"
        "version 1.4.0\n"
        "license Apache-2.0\n"
        "module acme.app.main src/main.short\n"
        "dependency acme.math 2.1.0 vendor/acme.math sha256:" + dependency_digest + " MIT\n";
    expect(writeFile(root / "shorthand.package", root_manifest), "write root manifest");

    ModuleResolver resolver;
    std::string code;
    std::string message;
    expect(resolver.loadForEntry((root / "src/main.short").string(), code, message),
           "load exact offline package v2 graph: " + code + " " + message);
    expect(resolver.manifest().format == "shorthand.package.v2", "retain package v2 format");
    expect(resolver.manifest().dependencies.size() == 1U, "retain one exact dependency");

    std::string root_source;
    std::string dependency_source;
    expect(resolver.resolveModule("acme.app.main", root_source, code, message), "resolve root module");
    expect(resolver.resolveModule("acme.math.ops", dependency_source, code, message), "resolve dependency module");

    ModuleUnitDescriptor root_unit{"acme.app.main", root_source, {"acme.math.ops"}};
    ModuleUnitDescriptor dependency_unit{"acme.math.ops", dependency_source, {}};
    expect(resolver.validateUnitIdentity(root_unit, "acme.app", code, message), "validate root ownership");
    expect(resolver.validateUnitIdentity(dependency_unit, "acme.math", code, message),
           "validate dependency ownership");
    std::map<std::string, ModuleUnitDescriptor> units{
        {root_unit.module_name, root_unit},
        {dependency_unit.module_name, dependency_unit},
    };
    std::vector<std::string> order;
    expect(resolver.topologicalOrder(units, root_unit.module_name, order, code, message),
           "order package v2 graph");
    expect(order.size() == 2U && order[0] == "acme.math.ops" && order[1] == "acme.app.main",
           "dependency-first deterministic order");
    expect(resolver.writeLockfile(units, root_unit.module_name, code, message), "write package v2 lock");
    expect(resolver.verifyLockfile(units, root_unit.module_name, code, message), "verify package v2 lock");

    std::ostringstream sbom;
    resolver.writePackageSbom("2026-08-22T00:00:00Z", sbom);
    expect(sbom.str().find("\"creationInfo\":{\"created\":\"2026-08-22T00:00:00Z\"") != std::string::npos,
           "emit reproducible SPDX creation information");
    expect(sbom.str().find("\"filesAnalyzed\":false") != std::string::npos,
           "avoid unsupported SPDX file-analysis claim");
    expect(sbom.str().find("\"copyrightText\":\"NOASSERTION\"") != std::string::npos,
           "emit required SPDX package copyright field");
    expect(sbom.str().find("\"referenceLocator\":\"sha256:" + dependency_digest + "\"") != std::string::npos,
           "emit dependency manifest integrity reference");

    // LLVM's redistribution exception is a single allowlisted SPDX expression,
    // including in dependency identity checks, exact lockfiles and SPDX output.
    const std::string llvm_license = "Apache-2.0 WITH LLVM-exception";
    auto licensedFixture = [&](const std::string &root_license,
                               const std::string &declared_license,
                               const std::string &actual_license) {
        std::string vendored = dependency_manifest;
        vendored.replace(vendored.find("license MIT"), std::string("license MIT").size(),
                         "license " + actual_license);
        expect(writeFile(root / "vendor/acme.math/shorthand.package", vendored), "write licensed dependency");
        std::string digest;
        expect(shorthand::crypto::sha256File((root / "vendor/acme.math/shorthand.package").string(), digest),
               "hash licensed dependency");
        std::string manifest = root_manifest;
        manifest.replace(manifest.find("license Apache-2.0"), std::string("license Apache-2.0").size(),
                         "license " + root_license);
        manifest.replace(manifest.find(dependency_digest + " MIT"), dependency_digest.size() + 4U,
                         digest + " " + declared_license);
        expect(writeFile(root / "shorthand.package", manifest), "write licensed root");
    };
    licensedFixture("Apache-2.0\t WITH  LLVM-exception", llvm_license, llvm_license);
    ModuleResolver licensed;
    expect(licensed.loadForEntry((root / "src/main.short").string(), code, message), "load LLVM exception");
    expect(licensed.manifest().license_spdx == llvm_license, "canonical root license expression");
    auto licensed_dependency = licensed.manifest().dependencies.find("acme.math");
    expect(licensed_dependency != licensed.manifest().dependencies.end() &&
           licensed_dependency->second.license_spdx == llvm_license, "retain full dependency license expression");
    expect(licensed.writeLockfile(units, root_unit.module_name, code, message), "lock LLVM license expression");
    expect(licensed.verifyLockfile(units, root_unit.module_name, code, message), "verify LLVM license expression");
    std::ifstream lock_file(root / "shorthand.lock", std::ios::binary);
    std::ostringstream lock_contents;
    lock_contents << lock_file.rdbuf();
    lock_file.close();
    const std::string llvm_lock = lock_contents.str();
    expect(llvm_lock.find("package acme.app 1.4.0 " + llvm_license + "\n") != std::string::npos,
           "lock preserves complete root license");
    expect(llvm_lock.find(" " + llvm_license + "\nentry ") != std::string::npos,
           "lock preserves complete dependency license");
    std::ostringstream licensed_sbom;
    licensed.writePackageSbom("2026-08-22T00:00:00Z", licensed_sbom);
    const std::string license_pair = "\"licenseConcluded\":\"" + llvm_license +
        "\",\"licenseDeclared\":\"" + llvm_license + "\"";
    const std::size_t first_license = licensed_sbom.str().find(license_pair);
    expect(first_license != std::string::npos &&
           licensed_sbom.str().find(license_pair, first_license + 1U) != std::string::npos,
           "SPDX retains root and dependency LLVM exceptions");
    licensedFixture("MIT", llvm_license, llvm_license);
    ModuleResolver changed_license;
    expect(changed_license.loadForEntry((root / "src/main.short").string(), code, message), "load changed root license");
    expect(!changed_license.verifyLockfile(units, root_unit.module_name, code, message) && code == "SHD2028",
           "reject license changed after locking");
    licensedFixture(llvm_license, llvm_license, "MIT");
    ModuleResolver mismatched_license;
    expect(!mismatched_license.loadForEntry((root / "src/main.short").string(), code, message) && code == "SHD2031",
           "reject dependency license mismatch despite valid manifest hash");
    for (const std::string &invalid : {std::string("Apache-2.0 WITH Unknown-exception"),
             std::string("Apache-2.0 WITH LLVM-exception OR MIT"), std::string("MIT trailing"),
             std::string("GPL-3.0-only"), std::string("Apache-2.0 WITH"), std::string("")}) {
        licensedFixture(invalid, "MIT", "MIT");
        ModuleResolver invalid_root_license;
        expect(!invalid_root_license.loadForEntry((root / "src/main.short").string(), code, message) && code == "SHD2032",
               "reject unallowlisted root license: " + invalid);
        licensedFixture("MIT", invalid, "MIT");
        ModuleResolver invalid_dependency_license;
        expect(!invalid_dependency_license.loadForEntry((root / "src/main.short").string(), code, message) &&
               code == (invalid.empty() ? "SHD2031" : "SHD2032"), "reject unallowlisted dependency license: " + invalid);
        licensedFixture("MIT", "MIT", invalid);
        ModuleResolver invalid_vendored_license;
        expect(!invalid_vendored_license.loadForEntry((root / "src/main.short").string(), code, message) && code == "SHD2031",
               "reject unallowlisted vendored license: " + invalid);
    }
    expect(writeFile(root / "vendor/acme.math/shorthand.package", dependency_manifest), "restore licensed dependency");
    expect(writeFile(root / "shorthand.package", root_manifest), "restore licensed root");
    expect(resolver.writeLockfile(units, root_unit.module_name, code, message), "restore original lock");

    expect(writeFile(root / "vendor/acme.math/src/ops.short", "tampered\n"), "tamper dependency source");
    expect(!resolver.verifyLockfile(units, root_unit.module_name, code, message) && code == "SHD2028",
           "reject a dependency source changed after locking");
    expect(writeFile(root / "vendor/acme.math/shorthand.package", dependency_manifest + "# tampered\n"),
           "tamper dependency manifest");
    ModuleResolver reloaded;
    expect(!reloaded.loadForEntry((root / "src/main.short").string(), code, message) && code == "SHD2031",
           "reject a dependency manifest SHA-256 mismatch");

    expect(writeFile(root / "vendor/acme.math/shorthand.package", dependency_manifest),
           "restore dependency manifest");
    std::string invalid_version_manifest = root_manifest;
    invalid_version_manifest.replace(invalid_version_manifest.find("version 1.4.0"),
                                     std::string("version 1.4.0").size(),
                                     "version 01.4.0");
    expect(writeFile(root / "shorthand.package", invalid_version_manifest), "write leading-zero version");
    ModuleResolver invalid_version;
    expect(!invalid_version.loadForEntry((root / "src/main.short").string(), code, message) && code == "SHD2033",
           "reject semantic version core leading zero");

    std::string build_version_manifest = root_manifest;
    build_version_manifest.replace(build_version_manifest.find("version 1.4.0"),
                                   std::string("version 1.4.0").size(),
                                   "version 1.4.0+build.7");
    expect(writeFile(root / "shorthand.package", build_version_manifest), "write exact build version");
    ModuleResolver build_version;
    expect(build_version.loadForEntry((root / "src/main.short").string(), code, message),
           "accept exact semantic version build metadata");

    const fs::path oversized = root / "oversized";
    fs::create_directories(oversized / "src", ec);
    expect(!ec && writeFile(oversized / "src/main.short", "package too.large;\nmodule too.large.main;\n"),
           "write oversized-manifest entry");
    expect(writeFile(oversized / "shorthand.package", std::string(1024U * 1024U + 1U, 'x')),
           "write oversized manifest");
    ModuleResolver oversized_resolver;
    expect(!oversized_resolver.loadForEntry((oversized / "src/main.short").string(), code, message) && code == "SHD2021",
           "reject root manifest beyond resource ceiling");

    fs::remove_all(root, ec);
    if (failures != 0) return 1;
    std::cout << "PASS package v2 exact offline integrity lock and SPDX contracts\n";
    return 0;
}
