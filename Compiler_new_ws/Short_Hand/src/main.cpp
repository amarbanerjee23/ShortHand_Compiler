#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./ast/AST.h"
#include "./visitors/AST_Printer.h"
#include "./visitors/Interpreter.h"
#include "./visitors/IR_Generator.h"

FILE * flex_output;
FILE * bison_output;

extern "C" FILE *yyin;
extern "C" int yyparse();
extern "C" void shorthand_release_scanner_strings();
AST_PROGRAM * main_program;
int main(int argc, char *argv[])
{

	if (argc < 3) {
			fprintf(stderr, "Correct usage: short_hand filename [run|print|compile|compile-bc|compile-native]\n");
			exit(1);
		}

	if (argc > 3) {
		fprintf(stderr, "Passing more arguments than necessary.\n");
			fprintf(stderr, "Correct usage: short_hand filename [run|print|compile|compile-bc|compile-native]\n");
	        exit(1);
		}


    flex_output = fopen("flex_output.txt", "w");
    bison_output = fopen("bison_output.txt", "w");

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
    //fprintf(bison_output, "\nRETURN VALUE : %d\n", return_val);



    if(!strcmp(argv[2], "run"))
    {
        //cout << "interpreting" << endl;
        Interpreter v;
        main_program->accept(v);
    }
    else if(!strcmp(argv[2], "print"))
    {
        AST_Printer t;
        //cout << "traversing" << endl;
        main_program->accept(t);
        //cout << "\n\n SUCCESS" << endl;
    }
    else if(!strcmp(argv[2], "compile"))
    {
        //cout << "generating code" << endl;
        IR_Generator c;
        c.setModuleName(file_without_extension);

        main_program->accept(c);

        c.dump();
        c.dumpBitcode();
    }
    else if(!strcmp(argv[2], "compile-bc"))
    {
        IR_Generator c;
        c.setModuleName(file_without_extension);
        main_program->accept(c);
        c.dumpBitcode();
    }
    else if(!strcmp(argv[2], "compile-native"))
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
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf(stderr, "Correct usage: short_hand filename [run|print|compile|compile-bc|compile-native]\n");
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
