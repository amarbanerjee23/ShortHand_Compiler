#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./ast/AST.h"
#include "./visitors/AST_Printer.h"
#include "./visitors/Interpreter.h"
#include "./visitors/IR_Generator.h"
#include "./visitors/SemanticAnalyzer.h"
#include "./evidence/EvidenceEmitter.h"
#include <fstream>

FILE * flex_output;
FILE * bison_output;

extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_release_scanner_strings();
AST_PROGRAM * main_program;

static void print_usage() {
    fprintf(stderr, "Correct usage: short_hand filename [run|print|compile|compile-bc|compile-native|evidence|c3eco-report|c3eco-check|c3eco-workbook] [--output file]\n");
}

static bool has_output_arg(int argc, char *argv[]) {
    return argc == 5 && !strcmp(argv[3], "--output");
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

    flex_output = fopen("/dev/null", "w");
    bison_output = fopen("/dev/null", "w");

    std::string path(argv[1]);
    std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
    std::string::size_type const p(base_filename.find_last_of('.'));
    std::string file_without_extension = base_filename.substr(0, p);

    yyin = fopen(argv[1], "r");
    int return_val = yyparse();
    if(return_val)
    {
        shorthand_release_scanner_strings();
        fclose(flex_output);
        fclose(bison_output);
        if (yyin) fclose(yyin);
        exit(1);
    }

    SemanticAnalyzer semantic;
    main_program->accept(semantic);
    if (semantic.diagnostics.hasErrors()) {
        semantic.diagnostics.print();
        shorthand_release_scanner_strings();
        fclose(flex_output);
        fclose(bison_output);
        if (yyin) fclose(yyin);
        exit(1);
    }

    std::string mode(argv[2]);

    if(mode == "evidence" || mode == "c3eco-report" || mode == "c3eco-check" || mode == "c3eco-workbook")
    {
        EvidenceEmitter emitter(argv[1]);
        if (has_output_arg(argc, argv)) {
            std::ofstream out(argv[4]);
            if (!out) {
                fprintf(stderr, "Could not open output file: %s\n", argv[4]);
                shorthand_release_scanner_strings();
                fclose(flex_output);
                fclose(bison_output);
                if (yyin) fclose(yyin);
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
            shorthand_release_scanner_strings();
            fclose(flex_output);
            fclose(bison_output);
            if (yyin) fclose(yyin);
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "----------------ERROR----------------\n");
        print_usage();
        shorthand_release_scanner_strings();
        fclose(flex_output);
        fclose(bison_output);
        if (yyin) fclose(yyin);
        exit(1);
    }
    shorthand_release_scanner_strings();
    fclose(flex_output);
    fclose(bison_output);
    if (yyin) fclose(yyin);
    return 0;
}
