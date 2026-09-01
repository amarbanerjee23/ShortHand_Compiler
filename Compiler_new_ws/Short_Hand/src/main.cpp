#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <limits>
#include <set>
#include <string>
#include <vector>
#include "./ast/AST.h"
#include "./ast/ModuleAST.h"
#include "./module/ModuleResolver.h"
#include "./parser/ParserLimits.h"
#include "./visitors/AST_Printer.h"
#include "./visitors/DiagnosticCodes.h"
#include "./visitors/Interpreter.h"
#include "./visitors/IR_Generator.h"
#include "./visitors/SemanticAnalyzer.h"
#include "./evidence/EvidenceEmitter.h"
#include "./enterprise/EnterpriseLanguage.h"

namespace fs = std::filesystem;
namespace diag = shorthand::diagnostics;
namespace shmod = shorthand::modules;

FILE * flex_output;
FILE * bison_output;
const char *shorthand_source_path = nullptr;

extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_release_scanner_strings();
extern "C" void shorthand_reset_scanner_location();
extern void yyrestart(FILE *input_file);
AST_PROGRAM * main_program;
AST_MODULE_PREAMBLE * main_module_preamble;

struct ParsedSourceUnit {
    std::string source_path;
    AST_PROGRAM *program = nullptr;
    AST_MODULE_PREAMBLE *preamble = nullptr;
};

static void print_usage() {
    fprintf(stderr, "Correct usage: short_hand filename [parse|enterprise-check|module-info|module-graph|package-sbom|lock|run|print|compile|compile-bc|compile-native|evidence|c3eco-report|c3eco-check|c3eco-workbook|c3eco-migrate] [--output file]\n");
}

static bool has_output_arg(int argc, char *argv[]) {
    return argc == 5 && !strcmp(argv[3], "--output");
}

static bool supported_mode(const std::string &mode) {
    return mode == "parse" || mode == "enterprise-check" || mode == "module-info" || mode == "module-graph" || mode == "package-sbom" || mode == "lock" ||
           mode == "run" || mode == "print" || mode == "compile" || mode == "compile-bc" ||
           mode == "compile-native" || mode == "evidence" || mode == "c3eco-report" ||
           mode == "c3eco-check" || mode == "c3eco-workbook" || mode == "c3eco-migrate";
}

static void cleanup_parser_resources() {
    shorthand_release_scanner_strings();
    if (flex_output) {
        fclose(flex_output);
        flex_output = nullptr;
    }
    if (bison_output) {
        fclose(bison_output);
        bison_output = nullptr;
    }
    if (yyin) {
        fclose(yyin);
        yyin = nullptr;
    }
}

static int finish_with(int status) {
    cleanup_parser_resources();
    return status;
}

static bool validate_regular_file_size(const char *path) {
    struct stat source_stat {};
    if (stat(path, &source_stat) != 0 || !S_ISREG(source_stat.st_mode)) return true;
    if (shorthand::parser::validateSourceBytes(static_cast<std::uint64_t>(source_stat.st_size))) return true;
    shorthand::parser::emitParserGuardFailure(path, 1, 1, 1, 1);
    return false;
}

static void emit_module_error(const std::string &source_path,
                              const std::string &code,
                              const std::string &message) {
    if (!source_path.empty()) std::cerr << source_path << ":1:1: ";
    std::cerr << "error: [" << code << "] " << message << " [range 1:1-1:1]\n";
}

static bool source_date_epoch_timestamp(std::string &timestamp) {
    const char *raw = std::getenv("SOURCE_DATE_EPOCH");
    if (raw == nullptr || *raw == '\0') return false;
    for (const char *digit = raw; *digit != '\0'; ++digit)
        if (*digit < '0' || *digit > '9') return false;
    errno = 0;
    char *end = nullptr;
    const unsigned long long seconds = std::strtoull(raw, &end, 10);
    if (errno != 0 || end == raw || *end != '\0' ||
        seconds > static_cast<unsigned long long>(std::numeric_limits<std::time_t>::max())) return false;
    const std::time_t epoch = static_cast<std::time_t>(seconds);
    std::tm utc{};
#if defined(_WIN32)
    if (gmtime_s(&utc, &epoch) != 0) return false;
#else
    if (gmtime_r(&epoch, &utc) == nullptr) return false;
#endif
    char rendered[21]{};
    if (std::strftime(rendered, sizeof(rendered), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0U) return false;
    timestamp = rendered;
    return true;
}

static bool parse_source_unit(const std::string &path, ParsedSourceUnit &out) {
    if (!validate_regular_file_size(path.c_str())) return false;

    shorthand_source_path = path.c_str();
    main_program = nullptr;
    main_module_preamble = nullptr;
    shorthand_reset_scanner_location();

    yyin = fopen(path.c_str(), "r");
    if (!yyin) {
        fprintf(stderr, "Could not open source file: %s\n", path.c_str());
        return false;
    }
    yyrestart(yyin);
    const int return_val = yyparse();
    fclose(yyin);
    yyin = nullptr;
    if (return_val || shorthand::parser::hasParserGuardFailure() || main_program == nullptr) return false;

    out.source_path = path;
    out.program = main_program;
    out.preamble = main_module_preamble;
    return true;
}

static shmod::ModuleUnitDescriptor descriptor_for(const ParsedSourceUnit &unit) {
    shmod::ModuleUnitDescriptor descriptor;
    descriptor.source_path = unit.source_path;
    if (unit.preamble == nullptr) return descriptor;
    descriptor.module_name = unit.preamble->moduleName();
    for (const AST_IMPORT_DECLARATION &import_decl : unit.preamble->imports())
        descriptor.imports.push_back(import_decl.path);
    return descriptor;
}

static std::string package_for(const ParsedSourceUnit &unit) {
    if (unit.preamble == nullptr || !unit.preamble->hasPackage()) return std::string();
    return unit.preamble->packageName();
}

static bool load_module_graph(const ParsedSourceUnit &entry,
                              shmod::ModuleResolver &resolver,
                              std::map<std::string, ParsedSourceUnit> &parsed_units,
                              std::map<std::string, shmod::ModuleUnitDescriptor> &descriptors,
                              std::vector<std::string> &ordered_modules,
                              std::string &code,
                              std::string &message) {
    if (!resolver.loadForEntry(entry.source_path, code, message)) return false;

    shmod::ModuleUnitDescriptor entry_descriptor = descriptor_for(entry);
    if (!resolver.validateUnitIdentity(entry_descriptor, package_for(entry), code, message)) return false;
    const std::string entry_module = entry_descriptor.module_name;
    parsed_units[entry_module] = entry;
    descriptors[entry_module] = entry_descriptor;

    std::set<std::string> expanded;
    std::set<std::string> expanding;
    std::function<bool(const std::string &)> expand = [&](const std::string &module_name) -> bool {
        if (expanded.count(module_name) != 0U) return true;
        if (expanding.count(module_name) != 0U) return true;
        expanding.insert(module_name);

        const std::vector<std::string> imports = descriptors[module_name].imports;
        for (const std::string &import_name : imports) {
            if (parsed_units.count(import_name) == 0U) {
                std::string dependency_path;
                if (!resolver.resolveModule(import_name, dependency_path, code, message)) return false;
                ParsedSourceUnit dependency;
                if (!parse_source_unit(dependency_path, dependency)) {
                    code = diag::ModuleIdentityMismatch;
                    message = "failed to parse resolved module source: " + dependency_path;
                    return false;
                }
                shmod::ModuleUnitDescriptor dependency_descriptor = descriptor_for(dependency);
                if (dependency_descriptor.module_name != import_name) {
                    code = diag::ModuleIdentityMismatch;
                    message = "import " + import_name + " resolved to source declaring module " + dependency_descriptor.module_name;
                    return false;
                }
                if (!resolver.validateUnitIdentity(dependency_descriptor, package_for(dependency), code, message))
                    return false;
                parsed_units[import_name] = dependency;
                descriptors[import_name] = dependency_descriptor;
            }
            if (!expand(import_name)) return false;
        }

        expanding.erase(module_name);
        expanded.insert(module_name);
        return true;
    };

    if (!expand(entry_module)) return false;
    return resolver.topologicalOrder(descriptors, entry_module, ordered_modules, code, message);
}

static bool validate_graph_symbols(const std::map<std::string, ParsedSourceUnit> &units,
                                   const std::vector<std::string> &ordered_modules,
                                   std::string &code,
                                   std::string &message) {
    std::map<std::string, std::string> owners;
    for (const std::string &module_name : ordered_modules) {
        const ParsedSourceUnit &unit = units.at(module_name);
        std::set<std::string> symbols = SemanticAnalyzer::functionNames(unit.program);
        const std::set<std::string> globals = SemanticAnalyzer::globalNames(unit.program);
        symbols.insert(globals.begin(), globals.end());
        for (const std::string &symbol : symbols) {
            auto existing = owners.find(symbol);
            if (existing != owners.end()) {
                code = diag::ModuleSymbolCollision;
                message = "package graph symbol collision for `" + symbol + "` between " + existing->second + " and " + module_name;
                return false;
            }
            owners[symbol] = module_name;
        }
    }
    return true;
}

static bool analyze_module_graph(const std::map<std::string, ParsedSourceUnit> &units,
                                 const std::map<std::string, shmod::ModuleUnitDescriptor> &descriptors,
                                 const std::vector<std::string> &ordered_modules) {
    std::map<std::string, std::map<std::string, SemanticAnalyzer::FunctionSignature>> function_signatures;
    for (const std::string &module_name : ordered_modules)
        function_signatures[module_name] = SemanticAnalyzer::functionSignatures(units.at(module_name).program);

    bool failed = false;
    for (const std::string &module_name : ordered_modules) {
        std::map<std::string, SemanticAnalyzer::FunctionSignature> imported;
        for (const std::string &dependency : descriptors.at(module_name).imports) {
            const auto &dependency_functions = function_signatures[dependency];
            imported.insert(dependency_functions.begin(), dependency_functions.end());
        }
        SemanticAnalyzer semantic;
        semantic.setImportedFunctions(imported, true);
        semantic.diagnostics.setSourceFile(units.at(module_name).source_path);
        units.at(module_name).program->accept(semantic);
        if (semantic.diagnostics.hasDiagnostics()) semantic.diagnostics.print();
        if (semantic.diagnostics.hasErrors()) failed = true;
    }
    return !failed;
}

static void add_imported_programs(IR_Generator &generator,
                                  const std::map<std::string, ParsedSourceUnit> &units,
                                  const std::vector<std::string> &ordered_modules,
                                  const std::string &entry_module) {
    for (const std::string &module_name : ordered_modules) {
        if (module_name == entry_module) continue;
        generator.addLibraryProgram(units.at(module_name).program);
    }
}

static void add_imported_programs(Interpreter &interpreter,
                                  const std::map<std::string, ParsedSourceUnit> &units,
                                  const std::vector<std::string> &ordered_modules,
                                  const std::string &entry_module) {
    for (const std::string &module_name : ordered_modules) {
        if (module_name == entry_module) continue;
        const ParsedSourceUnit &unit = units.at(module_name);
        interpreter.addLibraryProgram(unit.program, unit.source_path);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        print_usage();
        return 1;
    }

    if (argc != 3 && !has_output_arg(argc, argv)) {
        fprintf(stderr, "Invalid arguments.\n");
        print_usage();
        return 1;
    }

    std::string mode(argv[2]);
    if (!supported_mode(mode)) {
        fprintf(stderr, "Unsupported mode: %s\n", argv[2]);
        print_usage();
        return 1;
    }

    if (mode == "enterprise-check") {
        shorthand::parser::resetParserGuard();
        if (!validate_regular_file_size(argv[1])) return 1;
        shorthand::enterprise::ValidationSummary summary;
        std::string code;
        std::string message;
        std::size_t line = 1;
        if (!shorthand::enterprise::validateSourceFile(argv[1], summary, code, message, line)) {
            fprintf(stderr,
                    "%s:%zu:1: error: [%s] %s [range %zu:1-%zu:1]\n",
                    argv[1], line, code.c_str(), message.c_str(), line, line);
            return 1;
        }
        if (has_output_arg(argc, argv)) {
            std::ofstream out(argv[4]);
            if (!out) {
                fprintf(stderr, "Could not open output file: %s\n", argv[4]);
                return 1;
            }
            shorthand::enterprise::writeSummaryJson(summary, out);
            out << '\n';
            if (!out.good()) {
                fprintf(stderr, "Could not write output file: %s\n", argv[4]);
                return 1;
            }
        } else {
            shorthand::enterprise::writeSummaryJson(summary, std::cout);
            std::cout << '\n';
        }
        return 0;
    }

    flex_output = tmpfile();
    bison_output = tmpfile();
    if (!flex_output || !bison_output) {
        fprintf(stderr, "Could not initialize parser diagnostic streams.\n");
        return finish_with(1);
    }

    std::error_code path_error;
    std::string path = fs::weakly_canonical(fs::path(argv[1]), path_error).generic_string();
    if (path_error || path.empty()) path = argv[1];
    std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
    std::string::size_type const p(base_filename.find_last_of('.'));
    std::string file_without_extension = base_filename.substr(0, p);

    ParsedSourceUnit entry;
    if (!parse_source_unit(path, entry)) return finish_with(1);

    if (mode == "parse") return finish_with(0);

    if (mode == "module-info") {
        if (entry.preamble == nullptr) {
            fprintf(stderr, "Module AST scaffold was not initialized.\n");
            return finish_with(1);
        }
        entry.preamble->writeJson(std::cout);
        std::cout << '\n';
        return finish_with(0);
    }

    const bool has_module_contract = entry.preamble != nullptr && entry.preamble->hasAnyDeclaration();
    std::map<std::string, ParsedSourceUnit> parsed_units;
    std::map<std::string, shmod::ModuleUnitDescriptor> descriptors;
    std::vector<std::string> ordered_modules;
    shmod::ModuleResolver resolver;
    std::string module_code;
    std::string module_message;
    std::string entry_module;

    if (has_module_contract) {
        if (!load_module_graph(entry, resolver, parsed_units, descriptors, ordered_modules,
                               module_code, module_message)) {
            emit_module_error(path, module_code, module_message);
            return finish_with(1);
        }
        entry_module = descriptor_for(entry).module_name;
        if (!validate_graph_symbols(parsed_units, ordered_modules, module_code, module_message)) {
            emit_module_error(path, module_code, module_message);
            return finish_with(1);
        }

        if (mode == "lock") {
            if (!resolver.writeLockfile(descriptors, entry_module, module_code, module_message)) {
                emit_module_error(path, module_code, module_message);
                return finish_with(1);
            }
            std::cout << resolver.lockfilePath() << '\n';
            return finish_with(0);
        }

        if (mode == "module-graph") {
            std::string lock_status = "not_generated";
            if (fs::exists(fs::path(resolver.lockfilePath()))) {
                if (!resolver.verifyLockfile(descriptors, entry_module, module_code, module_message)) {
                    emit_module_error(path, module_code, module_message);
                    return finish_with(1);
                }
                lock_status = "verified";
            }
            resolver.writeGraphJson(descriptors, ordered_modules, entry_module, lock_status, std::cout);
            std::cout << '\n';
            return finish_with(0);
        }

        if (!resolver.verifyLockfile(descriptors, entry_module, module_code, module_message)) {
            emit_module_error(path, module_code, module_message);
            return finish_with(1);
        }
        if (mode == "package-sbom") {
            if (resolver.manifest().format != "shorthand.package.v2") {
                emit_module_error(path, diag::ModuleManifestInvalid,
                                  "package-sbom requires shorthand.package.v2");
                return finish_with(1);
            }
            std::string created_timestamp;
            if (!source_date_epoch_timestamp(created_timestamp)) {
                emit_module_error(path, diag::ModuleSbomReproducibility,
                                  "package-sbom requires a valid non-negative SOURCE_DATE_EPOCH");
                return finish_with(1);
            }
            if (has_output_arg(argc, argv)) {
                std::ofstream out(argv[4]);
                if (!out) {
                    fprintf(stderr, "Could not open output file: %s\n", argv[4]);
                    return finish_with(1);
                }
                resolver.writePackageSbom(created_timestamp, out);
                out << '\n';
                if (!out.good()) {
                    fprintf(stderr, "Could not write output file: %s\n", argv[4]);
                    return finish_with(1);
                }
            } else {
                resolver.writePackageSbom(created_timestamp, std::cout);
                std::cout << '\n';
            }
            return finish_with(0);
        }
        if (!analyze_module_graph(parsed_units, descriptors, ordered_modules)) return finish_with(1);
    } else {
        if (mode == "lock" || mode == "module-graph" || mode == "package-sbom") {
            emit_module_error(path, diag::ModuleIdentityMismatch,
                              "lock, module-graph and package-sbom modes require a package/module source unit");
            return finish_with(1);
        }
        SemanticAnalyzer semantic;
        semantic.diagnostics.setSourceFile(path);
        entry.program->accept(semantic);
        if (semantic.diagnostics.hasDiagnostics()) semantic.diagnostics.print();
        if (semantic.diagnostics.hasErrors()) return finish_with(1);
    }

    if(mode == "evidence" || mode == "c3eco-report" || mode == "c3eco-check" ||
       mode == "c3eco-workbook" || mode == "c3eco-migrate")
    {
        EvidenceEmitter emitter(path);
        if (has_output_arg(argc, argv)) {
            std::ofstream out(argv[4]);
            if (!out) {
                fprintf(stderr, "Could not open output file: %s\n", argv[4]);
                return finish_with(1);
            }
            if (mode == "c3eco-check") emitter.writeCheck(entry.program, out);
            else if (mode == "c3eco-workbook") emitter.writeWorkbookCsv(entry.program, out);
            else if (mode == "c3eco-migrate") emitter.writeProfileMigration(entry.program, out);
            else emitter.writeCandidateReport(entry.program, out);
        } else {
            if (mode == "c3eco-check") emitter.writeCheck(entry.program, std::cout);
            else if (mode == "c3eco-workbook") emitter.writeWorkbookCsv(entry.program, std::cout);
            else if (mode == "c3eco-migrate") emitter.writeProfileMigration(entry.program, std::cout);
            else emitter.writeCandidateReport(entry.program, std::cout);
        }
    }
    else if(mode == "run")
    {
        Interpreter interpreter(path);
        if (has_module_contract) add_imported_programs(interpreter, parsed_units, ordered_modules, entry_module);
        if (entry.program->accept(interpreter) != 0 || !interpreter.ok()) return finish_with(1);
    }
    else if(mode == "print")
    {
        AST_Printer t;
        entry.program->accept(t);
    }
    else if(mode == "compile")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        if (has_module_contract) add_imported_programs(c, parsed_units, ordered_modules, entry_module);
        entry.program->accept(c);
        c.dump();
        if (!c.dumpBitcode()) return finish_with(1);
    }
    else if(mode == "compile-bc")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        if (has_module_contract) add_imported_programs(c, parsed_units, ordered_modules, entry_module);
        entry.program->accept(c);
        if (!c.dumpBitcode()) return finish_with(1);
    }
    else if(mode == "compile-native")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        if (has_module_contract) add_imported_programs(c, parsed_units, ordered_modules, entry_module);
        entry.program->accept(c);
        c.dump();
        if (!c.dumpNativeBinary())
        {
            fprintf(stderr, "Native build failed. Install llc/clang and retry.\n");
            return finish_with(1);
        }
    }
    return finish_with(0);
}
