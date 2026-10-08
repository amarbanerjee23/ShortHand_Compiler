#include "ast/AST.h"
#include "ast/ModuleAST.h"
#include "ast/SourceRange.h"
#include "parser/ParseSession.h"
#include "parser/ParserLimits.h"
#include "visitors/SemanticAnalyzer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

FILE *flex_output = nullptr;
FILE *bison_output = nullptr;
const char *shorthand_source_path = "<parser-lifetime>";
AST_PROGRAM *main_program = nullptr;
AST_MODULE_PREAMBLE *main_module_preamble = nullptr;
extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_reset_scanner_location();
extern void yyrestart(FILE *);
extern int yylex_destroy();

namespace {
void require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL parser lifetime: %s\n", message);
        std::exit(1);
    }
}

void parse(const char *source, bool valid) {
    FILE *input = std::tmpfile();
    require(input != nullptr, "temporary source stream");
    require(std::fwrite(source, 1, std::strlen(source), input) == std::strlen(source), "write source");
    std::rewind(input);
    main_program = nullptr;
    main_module_preamble = nullptr;
    shorthand_reset_scanner_location();
    yyin = input;
    yyrestart(input);
    const int status = yyparse();
    std::fclose(input);
    yyin = nullptr;
    require((status == 0 && !shorthand::parser::hasParserGuardFailure()) == valid, "parse result");
    if (valid) require(main_program != nullptr, "successful parse owns an AST");
}

long peakRssKiB() {
#if defined(__linux__) || defined(__APPLE__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return -1;
#if defined(__APPLE__)
    return usage.ru_maxrss / 1024;
#else
    return usage.ru_maxrss;
#endif
#else
    return -1;
#endif
}
} // namespace

int main() {
    flex_output = std::tmpfile();
    bison_output = std::tmpfile();
    require(flex_output && bison_output, "diagnostic streams");
    const long initial_rss = peakRssKiB();
    std::size_t peak_live = 0;
    constexpr unsigned iterations = 25000;
    for (unsigned i = 0; i < iterations; ++i) {
        {
            // The entry AST must remain usable after another unit is parsed and
            // temporary successful/failed parses have both been destroyed.
            shorthand::parser::ParseSession graph;
            parse("package lifetime; module lifetime.entry; import lifetime.dep; "
                  "int value; def int keep(int n;) { return n; }; value = keep(1);", true);
            AST_PROGRAM *entry = main_program;
            AST_MODULE_PREAMBLE *entry_preamble = main_module_preamble;
            const SourceRange entry_range = shorthand_get_ast_source_range(entry);
            parse("package lifetime; module lifetime.dep; int other; "
                  "def int helper(int n;) { return n + 1; }; other = helper(1);", true);
            AST_PROGRAM *dependency = main_program;
            AST_MODULE_PREAMBLE *dependency_preamble = main_module_preamble;
            const std::size_t graph_nodes = shorthand::parser::liveParserAllocationCount();
            const std::size_t graph_ranges = shorthand_ast_source_range_count();
            const std::size_t graph_strings = shorthand::parser::liveScannerStringCount();
            for (bool valid : {false, true}) {
                {
                    shorthand::parser::ParseSession temporary;
                    parse(valid ? "int temp; temp = 2;" : "int temp; temp = (1 + ;", valid);
                    require(shorthand::parser::liveParserAllocationCount() > graph_nodes,
                            "temporary parse allocates nodes, including on failure");
                    peak_live = std::max(peak_live, shorthand::parser::liveParserAllocationCount());
                }
                require(main_program == dependency && main_module_preamble == dependency_preamble,
                        "nested scope restores outer parser roots");
                require(shorthand::parser::liveParserAllocationCount() == graph_nodes,
                        "temporary AST reclaimed without losing graph");
                require(shorthand_ast_source_range_count() == graph_ranges,
                        "temporary source ranges reclaimed without losing graph locations");
                require(shorthand::parser::liveScannerStringCount() == graph_strings,
                        "temporary tokens reclaimed without losing borrowed graph names");
            }
            require(entry_preamble->moduleName() == "lifetime.entry" &&
                    entry_preamble->imports().at(0).path == "lifetime.dep" &&
                    dependency_preamble->moduleName() == "lifetime.dep", "module graph still readable");
            require(SemanticAnalyzer::functionNames(entry).count("keep") == 1 &&
                    SemanticAnalyzer::functionNames(dependency).count("helper") == 1,
                    "borrowed function names survive subsequent and nested parses");
            require(shorthand_get_ast_source_range(entry).toString() == entry_range.toString() &&
                    entry_range.valid(), "outer source location survives nested cleanup");
            SemanticAnalyzer semantic;
            entry->accept(semantic);
            require(!semantic.diagnostics.hasErrors(), "entry remains traversable after nested cleanup");
        }
        require(shorthand::parser::liveParserAllocationCount() == 0, "no AST retained across sessions");
        require(shorthand_ast_source_range_count() == 0, "no source ranges retained across sessions");
        require(shorthand::parser::liveScannerStringCount() == 0, "no tokens retained across sessions");
        require(main_program == nullptr && main_module_preamble == nullptr, "no dangling parser roots");
    }
    yylex_destroy();
    std::fclose(flex_output);
    std::fclose(bison_output);
    flex_output = nullptr;
    bison_output = nullptr;
    std::printf("PARSER_LIFETIME sessions=%u parses=%u peak_live_nodes=%zu initial_rss_kib=%ld peak_rss_kib=%ld\n",
                iterations, iterations * 4, peak_live, initial_rss, peakRssKiB());
    std::puts("PASS parser lifetime: valid/invalid repeated parses, module retention, nested scopes, tokens and locations");
}
