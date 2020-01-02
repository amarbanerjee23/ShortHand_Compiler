#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./ast/AST.h"
#include "./visitors/AST_Printer.cpp"
#include "./visitors/AST_Printer.h"
#include "./visitors/Interpreter.cpp"
#include "./visitors/Interpreter.h"
#include "./visitors/IR_Generator.cpp"
#include "./visitors/IR_Generator.h"

FILE * flex_output;
FILE * bison_output;

extern "C" FILE *yyin;
extern "C" int yyparse();
AST_PROGRAM * main_program;
int main(int argc, char *argv[])
{

	if (argc < 3) {
			fprintf(stderr, "Correct usage: short_hand filename [run|print|compile]\n");
			exit(1);
		}

	if (argc < 3) {
		fprintf(stderr, "Correct usage: short_hand filename [run|print|compile]\n");
		exit(1);
	}

	if (argc > 3) {
		fprintf(stderr, "Passing more arguments than necessary.\n");
		fprintf(stderr, "Correct usage: short_hand filename\n");
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
    }
    else
    {
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf(stderr, "Correct usage: short_hand filename run|print|compile]\n");
        exit(1);
    }
    return 0;
}
