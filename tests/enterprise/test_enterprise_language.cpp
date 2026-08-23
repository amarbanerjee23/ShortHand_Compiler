#include "enterprise/EnterpriseLanguage.h"

#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string &message) {
    if (condition) return;
    std::cerr << "FAIL " << message << '\n';
    ++failures;
}

void expectFailure(const std::string &path, const std::string &expected_code) {
    shorthand::enterprise::ValidationSummary summary;
    std::string code;
    std::string message;
    std::size_t line = 0;
    expect(!shorthand::enterprise::validateSourceFile(path, summary, code, message, line),
           path + " must fail");
    expect(code == expected_code, path + " stable diagnostic code");
    expect(line > 0U && !message.empty(), path + " source line and message");
}

}  // namespace

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    const std::string root(argv[1]);
    shorthand::enterprise::ValidationSummary summary;
    std::string code;
    std::string message;
    std::size_t line = 0;
    expect(shorthand::enterprise::validateSourceFile(root + "/valid.enterprise.short",
                                                     summary,
                                                     code,
                                                     message,
                                                     line),
           "valid enterprise language surface");
    expect(summary.namespace_name == "policyclub.sustainable_ai", "stable namespace");
    expect(summary.canonical_types.size() == 5U, "all five composite families");
    expect(summary.ownership_values == 2U, "move creates a distinct owner");
    expect(summary.ownership_operations == 9U, "ownership operation evidence count");
    expectFailure(root + "/invalid_use_after_move.enterprise.short", "SHD3016");
    expectFailure(root + "/invalid_borrow_conflict.enterprise.short", "SHD3016");
    expectFailure(root + "/invalid_duplicate_field.enterprise.short", "SHD3010");
    expectFailure(root + "/invalid_unreleased_borrow.enterprise.short", "SHD3016");
    expectFailure(root + "/invalid_enterprise_syntax.enterprise.short", "SHD3024");
    expectFailure(root + "/invalid_duplicate_type.enterprise.short", "SHD3025");
    expectFailure(root + "/invalid_stray_close.enterprise.short", "SHD3024");
    expectFailure(root + "/invalid_identity_collision.enterprise.short", "SHD3025");
    const std::string oversized_path = std::string(argv[2]) + "/oversized-line.enterprise.short";
    {
        std::ofstream out(oversized_path, std::ios::binary | std::ios::trunc);
        out << "language shorthand.enterprise_language.v1;\n"
            << "namespace policyclub.invalid;\n"
            << std::string(16U * 1024U + 1U, 'x') << '\n';
    }
    expectFailure(oversized_path, "SHD2005");
    if (failures != 0) return 1;
    std::cout << "PASS enterprise composites namespaces and ownership validation\n";
    return 0;
}
