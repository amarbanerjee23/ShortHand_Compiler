%{
#include "./ast/AST.h"
#include "parser.tab.hh"
#include <cstdlib>
#include <cstring>
#include <vector>
#define YY_DECL extern "C" int yylex()
extern FILE * flex_output;
extern union _NODE_ yylval;
static std::vector<char *> shorthand_scanner_strings;
static char *shorthand_strdup_token(const char *text) {
    char *copy = strdup(text);
    if (copy) shorthand_scanner_strings.push_back(copy);
    return copy;
}
extern "C" void shorthand_release_scanner_strings() {
    for (char *value : shorthand_scanner_strings) free(value);
    shorthand_scanner_strings.clear();
}

%}


%%


"def"    {
                   fprintf(flex_output, "def keyword from scanner\n");
                   return DEF;
               }


"int"      return INT;
"float"      return FLOAT;
"string"      return STRING;
"bool"      return BOOL;
"void"		return VOID;

"loop"      return LOOP;
"while"    return WHILE;
"if"       return IF;
"else"     return ELSE;
"goto"     return GOTO;
"print"    return PRINT;
"read"     return READ;
"break"     return BREAK;


"+"    return '+';
"-"    return '-';
"*"    return '*';
"/"    return '/';
"%"    return '%';

";"    return ';';
","    return ',';

":"    return ':';

"{"    return '{';
"}"    return '}';
"="    return '=';
"["    return '[';
"]"    return ']';
"("    return '(';
")"    return ')';

"<"     return LESS;
">"     return GREATER;
"<="    return LESS_OR_EQUAL;
">="    return GREATER_OR_EQUAL;
"=="    return EQUAL;
"!="    return NOT_EQUAL;
"||"    return OR;
"&&"    return AND;


[0-9][0-9]*    {
                   yylval.int_val = atoi(yytext);
                   fprintf(flex_output, "integer literal: %s\n", yytext);
                   return INT_LITERAL;
               }


[a-zA-Z_][a-zA-Z0-9_]*    {
                            yylval.string_val = shorthand_strdup_token(yytext);
                            fprintf(flex_output, "identifier: %s\n", yytext);
                            return IDENTIFIER;
                        }


\"(\.|[^\"])*\"    {
                       yylval.string_val = shorthand_strdup_token(yytext);
                       fprintf(flex_output, "string literal: %s\n", yytext);
                       return STRING_LITERAL;
                   }


[ \t\n]    { /* Do nothing */ }

.    {
         fprintf(flex_output, "Unexpected token encountered: %s\n", yytext);
         return ETOK;
     }
