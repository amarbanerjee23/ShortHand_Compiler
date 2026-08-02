#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "./ast/AST.h"
#include "./parser/ParserLimits.h"
#include "./visitors/AST_Printer.h"
#include "./visitors/Interpreter.h"
#include "./visitors/IR_Generator.h"
#include "./visitors/SemanticAnalyzer.h"
#include "./evidence/EvidenceEmitter.h"
#include <fstream>

FILE * flex_output;
FILE * bison_output;
const char *shorthand_source_path = nullptr;

extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_release_scanner_strings();
extern "C" void shorthand_reset_scanner_location();
AST_PROGRAM * main_program;

static void print_usage() {
    fprintf(stderr, "Correct usage: short_hand filename [parse|run|print|compile|compile-bc|compile-native|evidence|c3eco-report|c3eco-check|c3eco-workbook] [--output file]\n");
}

static bool has_output_arg(int argc, char *argv[]) {
    return argc == 5 && !strcmp(argv[3], "--output");
}

static bool supported_mode(const std::string &mode) {
    return mode == "parse" || mode == "run" || mode == "print" ||
           mode == "compile" || mode == "compile-bc" ||
           mode == "compile-native" || mode == "evidence" ||
           mode == "c3eco-report" || mode == "c3eco-check" ||
           mode == "c3eco-workbook";
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

static bool validate_regular_file_size(const char *path) {
    struct stat source_stat {};
    if (stat(path, &source_stat) != 0 || !S_ISREG(source_stat.st_mode)) {
        return true;
    }
    if (shorthand::parser::validateSourceBytes(
            static_cast<std::uint64_t>(source_stat.st_size))) {
        return true;
    }
    shorthand::parser::emitParserGuardFailure(path, 1, 1, 1, 1);
    return false;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        print_usage();
        exit(1);
    }

    if (argc != 3 && !has_output_arg(argc, argv)) {
        fprintf(stderr, "Invalid arguments.\n");
        print_usage();
        exit(1);
    }

    std::string mode(argv[2]);
    if (!supported_mode(mode)) {
        fprintf(stderr, "Unsupported mode: %s\n", argv[2]);
        print_usage();
        exit(1);
    }

    flex_output = fopen("/dev/null", "w");
    bison_output = fopen("/dev/null", "w");
    if (!flex_output || !bison_output) {
        fprintf(stderr, "Could not initialize parser diagnostic streams.\n");
        cleanup_parser_resources();
        exit(1);
    }

    std::string path(argv[1]);
    shorthand_source_path = argv[1];
    std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
    std::string::size_type const p(base_filename.find_last_of('.'));
    std::string file_without_extension = base_filename.substr(0, p);

    shorthand_reset_scanner_location();
    if (!validate_regular_file_size(argv[1])) {
        cleanup_parser_resources();
        exit(1);
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Could not open source file: %s\n", argv[1]);
        cleanup_parser_resources();
        exit(1);
    }

    int return_val = yyparse();
    if(return_val || shorthand::parser::hasParserGuardFailure())
    {
        cleanup_parser_resources();
        exit(1);
    }

    if (mode == "parse") {
        cleanup_parser_resources();
        return 0;
    }

    SemanticAnalyzer semantic;
    semantic.diagnostics.setSourceFile(argv[1]);
    main_program->accept(semantic);
    if (semantic.diagnostics.hasDiagnostics()) semantic.diagnostics.print();
    if (semantic.diagnostics.hasErrors()) {
        cleanup_parser_resources();
        exit(1);
    }

    if(mode == "evidence" || mode == "c3eco-report" || mode == "c3eco-check" || mode == "c3eco-workbook")
    {
        EvidenceEmitter emitter(argv[1]);
        if (has_output_arg(argc, argv)) {
            std::ofstream out(argv[4]);
            if (!out) {
                fprintf(stderr, "Could not open output file: %s\n", argv[4]);
                cleanup_parser_resources();
                exit(1);
            }
            if (mode == "c3eco-check") emitter.writeCheck(main_program, out);
            else if (mode == "c3eco-workbook") emitter.writeWorkbookCsv(main_program, out);
            else emitter.writeCandidateReport(main_program, out);
        } else {
            if (mode == "c3eco-check") emitter.writeCheck(main_program, std::cout);
            else if (mode == "c3eco-workbook") emitter.writeWorkbookCsv(main_program, std::cout);
            else emitter.writeCandidateReport(main_program, std::cout);
        }
    }
    else if(mode == "run")
    {
        Interpreter v;
        main_program->accept(v);
    }
    else if(mode == "print")
    {
        AST_Printer t;
        main_program->accept(t);
    }
    else if(mode == "compile")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        main_program->accept(c);
        c.dump();
        c.dumpBitcode();
    }
    else if(mode == "compile-bc")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        main_program->accept(c);
        c.dumpBitcode();
    }
    else if(mode == "compile-native")
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        main_program->accept(c);
        c.dump();
        if (!c.dumpNativeBinary())
        {
            fprintf(stderr, "Native build failed. Install llc/clang and retry.\n");
            cleanup_parser_resources();
            exit(1);
        }
    }
    cleanup_parser_resources();
    return 0;
}
