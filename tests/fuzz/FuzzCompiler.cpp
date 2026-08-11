#include "ast/AST.h"
#include "ast/ModuleAST.h"
#include "module/ModuleResolver.h"
#include "parser/ParserLimits.h"
#include "visitors/IR_Generator.h"
#include "visitors/SemanticAnalyzer.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef SHORTHAND_FUZZ_STAGE
#error "SHORTHAND_FUZZ_STAGE must be defined"
#endif

namespace fs = std::filesystem;
namespace shmod = shorthand::modules;

FILE *flex_output = nullptr;
FILE *bison_output = nullptr;
const char *shorthand_source_path = "<fuzz>";
AST_PROGRAM *main_program = nullptr;
AST_MODULE_PREAMBLE *main_module_preamble = nullptr;

extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_release_scanner_strings();
extern "C" void shorthand_reset_scanner_location();
extern void yyrestart(FILE *input_file);

namespace {
constexpr std::size_t kMaxFuzzInputBytes = 4096U;
fs::path module_root;

void closeDiagnosticStreams() {
    shorthand_release_scanner_strings();
    if (flex_output != nullptr) {
        std::fclose(flex_output);
        flex_output = nullptr;
    }
    if (bison_output != nullptr) {
        std::fclose(bison_output);
        bison_output = nullptr;
    }
    if (!module_root.empty()) {
        std::error_code ignored;
        fs::remove_all(module_root, ignored);
    }
}

AST_PROGRAM *parseBuffer(const std::uint8_t *data, std::size_t size) {
    if (data == nullptr || size > kMaxFuzzInputBytes) return nullptr;

    FILE *input = std::tmpfile();
    if (input == nullptr) return nullptr;
    if (size != 0U && std::fwrite(data, 1U, size, input) != size) {
        std::fclose(input);
        return nullptr;
    }
    std::rewind(input);

    main_program = nullptr;
    main_module_preamble = nullptr;
    shorthand_source_path = "<fuzz>";
    shorthand_reset_scanner_location();
    yyin = input;
    yyrestart(input);
    const int status = yyparse();
    std::fclose(input);
    yyin = nullptr;

    AST_PROGRAM *program = nullptr;
    if (status == 0 && !shorthand::parser::hasParserGuardFailure()) {
        program = main_program;
    }
    return program;
}

void finishParseIteration() {
    shorthand_release_scanner_strings();
    main_program = nullptr;
    main_module_preamble = nullptr;
}

int fuzzParser(const std::uint8_t *data, std::size_t size) {
    (void)parseBuffer(data, size);
    finishParseIteration();
    return 0;
}

int fuzzSemantic(const std::uint8_t *data, std::size_t size) {
    AST_PROGRAM *program = parseBuffer(data, size);
    if (program != nullptr) {
        SemanticAnalyzer semantic;
        program->accept(semantic);
    }
    finishParseIteration();
    return 0;
}

int fuzzLowering(const std::uint8_t *data, std::size_t size) {
    AST_PROGRAM *program = parseBuffer(data, size);
    if (program != nullptr) {
        SemanticAnalyzer semantic;
        program->accept(semantic);
        if (!semantic.diagnostics.hasErrors()) {
            IR_Generator generator;
            generator.setModuleName("shorthand_fuzz");
            program->accept(generator);
        }
    }
    finishParseIteration();
    return 0;
}

void ensureModuleFixture() {
    if (!module_root.empty()) return;
    module_root = fs::temp_directory_path() / "shorthand-pr73-module-fuzz";
    std::error_code ignored;
    fs::remove_all(module_root, ignored);
    fs::create_directories(module_root / "src");

    std::ofstream app(module_root / "src" / "app.short");
    app << "package fuzz.pkg;\nmodule fuzz.pkg.app;\nint value;\nvalue = 1;\n";
    std::ofstream lib(module_root / "src" / "lib.short");
    lib << "package fuzz.pkg;\nmodule fuzz.pkg.lib;\nint value;\nvalue = 2;\n";
}

int fuzzModuleResolver(const std::uint8_t *data, std::size_t size) {
    if (data == nullptr || size > kMaxFuzzInputBytes) return 0;
    ensureModuleFixture();

    {
        std::ofstream manifest(module_root / "shorthand.package", std::ios::binary | std::ios::trunc);
        if (!manifest) return 0;
        manifest.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
    }

    shmod::ModuleResolver resolver;
    std::string code;
    std::string message;
    const std::string entry = (module_root / "src" / "app.short").string();
    if (resolver.loadForEntry(entry, code, message)) {
        for (const auto &mapping : resolver.manifest().module_paths) {
            std::string resolved;
            std::string resolve_code;
            std::string resolve_message;
            (void)resolver.resolveModule(mapping.first, resolved, resolve_code, resolve_message);
        }
    }
    return 0;
}
}  // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
    flex_output = std::fopen("/dev/null", "w");
    bison_output = std::fopen("/dev/null", "w");
    if (flex_output == nullptr || bison_output == nullptr) std::abort();
    std::atexit(closeDiagnosticStreams);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
#if SHORTHAND_FUZZ_STAGE == 1
    return fuzzParser(data, size);
#elif SHORTHAND_FUZZ_STAGE == 2
    return fuzzModuleResolver(data, size);
#elif SHORTHAND_FUZZ_STAGE == 3
    return fuzzSemantic(data, size);
#elif SHORTHAND_FUZZ_STAGE == 4
    return fuzzLowering(data, size);
#else
#error "Unsupported SHORTHAND_FUZZ_STAGE"
#endif
}
