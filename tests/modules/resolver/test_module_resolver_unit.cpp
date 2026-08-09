#include "module/ModuleResolver.h"
#include "visitors/DiagnosticCodes.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace modules = shorthand::modules;
namespace diag = shorthand::diagnostics;

static int fail(const std::string &message) {
    std::cerr << message << '\n';
    return 1;
}

int main() {
    modules::ModuleResolver resolver;
    std::string code;
    std::string message;
    std::vector<std::string> order;

    std::map<std::string, modules::ModuleUnitDescriptor> graph;
    graph["pkg.app"] = modules::ModuleUnitDescriptor{
        "pkg.app", "/tmp/app.short", {"pkg.zeta", "pkg.alpha"}};
    graph["pkg.alpha"] = modules::ModuleUnitDescriptor{
        "pkg.alpha", "/tmp/alpha.short", {"pkg.core"}};
    graph["pkg.zeta"] = modules::ModuleUnitDescriptor{
        "pkg.zeta", "/tmp/zeta.short", {}};
    graph["pkg.core"] = modules::ModuleUnitDescriptor{
        "pkg.core", "/tmp/core.short", {}};

    if (!resolver.topologicalOrder(graph, "pkg.app", order, code, message))
        return fail("valid graph unexpectedly failed: " + code + " " + message);

    const std::vector<std::string> expected{
        "pkg.core", "pkg.alpha", "pkg.zeta", "pkg.app"};
    if (order != expected)
        return fail("dependency-first deterministic topological order changed");

    graph["pkg.core"].imports.push_back("pkg.app");
    order.clear();
    code.clear();
    message.clear();
    if (resolver.topologicalOrder(graph, "pkg.app", order, code, message))
        return fail("cycle unexpectedly accepted");
    if (code != diag::ModuleImportCycle)
        return fail("cycle did not emit SHD2025");
    if (message.find("pkg.app") == std::string::npos ||
        message.find("pkg.core") == std::string::npos)
        return fail("cycle diagnostic lost graph provenance");

    std::cout << "PASS module resolver unit graph ordering and cycle detection\n";
    return 0;
}
