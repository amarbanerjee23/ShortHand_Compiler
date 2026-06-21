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


"def" return DEF;


"double" return DOUBLE;
"return" return RETURN;
"continue" return CONTINUE;
"true" return TRUE;
"false" return FALSE;
"model" return MODEL;
"format" return FORMAT;
"path" return PATH;
"task" return TASK;
"precision" return PRECISION;
"input_shape" return INPUT_SHAPE;
"output_shape" return OUTPUT_SHAPE;
"backend_preference" return BACKEND_PREFERENCE;
"compact" return COMPACT;
"quality_guardrail" return QUALITY_GUARDRAIL;
"greenai_contract" return GREENAI_CONTRACT_T;
"functional_unit" return FUNCTIONAL_UNIT;
"success_criteria" return SUCCESS_CRITERIA;
"boundary" return BOUNDARY;
"measurement_quality" return MEASUREMENT_QUALITY;
"data_quality" return DATA_QUALITY;
"carbon_factor" return CARBON_FACTOR;
"energy_budget_j" return ENERGY_BUDGET_J;
"carbon_budget_gco2e" return CARBON_BUDGET_GCO2E;
"evidence_retention" return EVIDENCE_RETENTION;
"claims_mode" return CLAIMS_MODE;
"evidence_only" return EVIDENCE_ONLY;
"greenai_measure" return GREENAI_MEASURE;
"infer" return INFER;
"tensor" return TENSOR;
"int8" return INT8;
"fp16" return FP16;
"fp32" return FP32;
"bf16" return BF16;
"int4" return INT4;
"fp64" return FP64;
"onnx" return ONNX;
"engine" return ENGINE;
"torchscript" return TORCHSCRIPT;
"openvino_ir" return OPENVINO_IR;
"gguf" return GGUF;
"tensorrt" return TENSORRT;
"onnxruntime_tensorrt" return ONNXRUNTIME_TENSORRT;
"onnxruntime_cuda" return ONNXRUNTIME_CUDA;
"onnxruntime_cpu" return ONNXRUNTIME_CPU;
"openvino" return OPENVINO;
"libtorch" return LIBTORCH;
"llamacpp" return LLAMACPP;
"fallback" return FALLBACK;
"MQ1" return MQ1;
"MQ2" return MQ2;
"MQ3" return MQ3;
"MQ4" return MQ4;
"DQ1" return DQ1;
"DQ2" return DQ2;
"DQ3" return DQ3;
"DQ4" return DQ4;
"location" return LOCATION;
"ci_cd" return CI_CD;
"thirdparty" return THIRDPARTY;
"accelerator" return ACCELERATOR;
"compute" return COMPUTE;
"storage" return STORAGE;
"network" return NETWORK;

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
"//"[^\n]* ;
"#"[^\n]* ;
"/*"([^*]|\*+[^*/])*"*/" ;
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
"->"   { return GREATER; }
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


[0-9]+"."[0-9]+ { yylval.float_val = atof(yytext); return FLOAT_LITERAL; }

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


[ \t\r\n]    { /* Do nothing */ }

.    {
         fprintf(flex_output, "Unexpected token encountered: %s\n", yytext);
         return ETOK;
     }
