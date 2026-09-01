/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "scanner_parser/parser.yy"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "./ast/AST.h"
#include "./ast/ModuleAST.h"
#include "./ast/SourceRange.h"
#include "./visitors/DiagnosticCodes.h"
#include <vector>
#include <string>
#include <unordered_set>
static ModelDeclarationData current_model;
static GreenAIContractData current_contract;
static GreenAIMeasurementData current_measure;
static C3EcoDeclarationData current_c3eco;
static std::string current_module_path;
static std::string current_import_alias;
using namespace std;
extern "C" int yylex();
extern "C" int yyparse();
extern "C" void yyerror(char const *s);
extern "C" int yywrap(void){return 1;}
extern "C" int yydebug;
extern union _NODE_ yylval;
extern class AST_PROGRAM * main_program;
extern class AST_MODULE_PREAMBLE * main_module_preamble;
extern const char *shorthand_source_path;

#line 100 "parser.tab.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ETOK = 3,                       /* ETOK  */
  YYSYMBOL_4_ = 4,                         /* '='  */
  YYSYMBOL_OR = 5,                         /* OR  */
  YYSYMBOL_AND = 6,                        /* AND  */
  YYSYMBOL_EQUAL = 7,                      /* EQUAL  */
  YYSYMBOL_NOT_EQUAL = 8,                  /* NOT_EQUAL  */
  YYSYMBOL_LESS = 9,                       /* LESS  */
  YYSYMBOL_LESS_OR_EQUAL = 10,             /* LESS_OR_EQUAL  */
  YYSYMBOL_GREATER = 11,                   /* GREATER  */
  YYSYMBOL_GREATER_OR_EQUAL = 12,          /* GREATER_OR_EQUAL  */
  YYSYMBOL_ARROW = 13,                     /* ARROW  */
  YYSYMBOL_14_ = 14,                       /* '+'  */
  YYSYMBOL_15_ = 15,                       /* '-'  */
  YYSYMBOL_16_ = 16,                       /* '*'  */
  YYSYMBOL_17_ = 17,                       /* '/'  */
  YYSYMBOL_18_ = 18,                       /* '%'  */
  YYSYMBOL_UMINUS = 19,                    /* UMINUS  */
  YYSYMBOL_STRING_LITERAL = 20,            /* STRING_LITERAL  */
  YYSYMBOL_IDENTIFIER = 21,                /* IDENTIFIER  */
  YYSYMBOL_AI_INFER_BUILTIN = 22,          /* AI_INFER_BUILTIN  */
  YYSYMBOL_GREENAI_REPORT_BUILTIN = 23,    /* GREENAI_REPORT_BUILTIN  */
  YYSYMBOL_INT_LITERAL = 24,               /* INT_LITERAL  */
  YYSYMBOL_FLOAT_LITERAL = 25,             /* FLOAT_LITERAL  */
  YYSYMBOL_READ = 26,                      /* READ  */
  YYSYMBOL_PRINT = 27,                     /* PRINT  */
  YYSYMBOL_GOTO = 28,                      /* GOTO  */
  YYSYMBOL_BREAK = 29,                     /* BREAK  */
  YYSYMBOL_WHILE = 30,                     /* WHILE  */
  YYSYMBOL_LOOP = 31,                      /* LOOP  */
  YYSYMBOL_ELSE = 32,                      /* ELSE  */
  YYSYMBOL_IF = 33,                        /* IF  */
  YYSYMBOL_DEF = 34,                       /* DEF  */
  YYSYMBOL_INT = 35,                       /* INT  */
  YYSYMBOL_FLOAT = 36,                     /* FLOAT  */
  YYSYMBOL_STRING = 37,                    /* STRING  */
  YYSYMBOL_VOID = 38,                      /* VOID  */
  YYSYMBOL_BOOL = 39,                      /* BOOL  */
  YYSYMBOL_DOUBLE = 40,                    /* DOUBLE  */
  YYSYMBOL_RETURN = 41,                    /* RETURN  */
  YYSYMBOL_CONTINUE = 42,                  /* CONTINUE  */
  YYSYMBOL_TRUE = 43,                      /* TRUE  */
  YYSYMBOL_FALSE = 44,                     /* FALSE  */
  YYSYMBOL_PACKAGE = 45,                   /* PACKAGE  */
  YYSYMBOL_MODULE = 46,                    /* MODULE  */
  YYSYMBOL_IMPORT = 47,                    /* IMPORT  */
  YYSYMBOL_AS = 48,                        /* AS  */
  YYSYMBOL_MODEL = 49,                     /* MODEL  */
  YYSYMBOL_FORMAT = 50,                    /* FORMAT  */
  YYSYMBOL_PATH = 51,                      /* PATH  */
  YYSYMBOL_TASK = 52,                      /* TASK  */
  YYSYMBOL_PRECISION = 53,                 /* PRECISION  */
  YYSYMBOL_INPUT_SHAPE = 54,               /* INPUT_SHAPE  */
  YYSYMBOL_OUTPUT_SHAPE = 55,              /* OUTPUT_SHAPE  */
  YYSYMBOL_BACKEND_PREFERENCE = 56,        /* BACKEND_PREFERENCE  */
  YYSYMBOL_COMPACT = 57,                   /* COMPACT  */
  YYSYMBOL_QUALITY_GUARDRAIL = 58,         /* QUALITY_GUARDRAIL  */
  YYSYMBOL_GREENAI_CONTRACT_T = 59,        /* GREENAI_CONTRACT_T  */
  YYSYMBOL_FUNCTIONAL_UNIT = 60,           /* FUNCTIONAL_UNIT  */
  YYSYMBOL_SUCCESS_CRITERIA = 61,          /* SUCCESS_CRITERIA  */
  YYSYMBOL_BOUNDARY = 62,                  /* BOUNDARY  */
  YYSYMBOL_MEASUREMENT_QUALITY = 63,       /* MEASUREMENT_QUALITY  */
  YYSYMBOL_DATA_QUALITY = 64,              /* DATA_QUALITY  */
  YYSYMBOL_CARBON_FACTOR = 65,             /* CARBON_FACTOR  */
  YYSYMBOL_ENERGY_BUDGET_J = 66,           /* ENERGY_BUDGET_J  */
  YYSYMBOL_CARBON_BUDGET_GCO2E = 67,       /* CARBON_BUDGET_GCO2E  */
  YYSYMBOL_EVIDENCE_RETENTION = 68,        /* EVIDENCE_RETENTION  */
  YYSYMBOL_CLAIMS_MODE = 69,               /* CLAIMS_MODE  */
  YYSYMBOL_EVIDENCE_ONLY = 70,             /* EVIDENCE_ONLY  */
  YYSYMBOL_GREENAI_MEASURE = 71,           /* GREENAI_MEASURE  */
  YYSYMBOL_INFER = 72,                     /* INFER  */
  YYSYMBOL_TENSOR = 73,                    /* TENSOR  */
  YYSYMBOL_CERTIFICATION_PROFILE = 74,     /* CERTIFICATION_PROFILE  */
  YYSYMBOL_CERTIFICATION = 75,             /* CERTIFICATION  */
  YYSYMBOL_WORKLOAD = 76,                  /* WORKLOAD  */
  YYSYMBOL_MEASUREMENT_PLAN = 77,          /* MEASUREMENT_PLAN  */
  YYSYMBOL_AI_LIFECYCLE = 78,              /* AI_LIFECYCLE  */
  YYSYMBOL_RAG_PIPELINE = 79,              /* RAG_PIPELINE  */
  YYSYMBOL_TOKEN_BUDGET = 80,              /* TOKEN_BUDGET  */
  YYSYMBOL_MODEL_ROUTING = 81,             /* MODEL_ROUTING  */
  YYSYMBOL_GUARDRAILS = 82,                /* GUARDRAILS  */
  YYSYMBOL_INT8 = 83,                      /* INT8  */
  YYSYMBOL_FP16 = 84,                      /* FP16  */
  YYSYMBOL_FP32 = 85,                      /* FP32  */
  YYSYMBOL_BF16 = 86,                      /* BF16  */
  YYSYMBOL_INT4 = 87,                      /* INT4  */
  YYSYMBOL_FP64 = 88,                      /* FP64  */
  YYSYMBOL_ONNX = 89,                      /* ONNX  */
  YYSYMBOL_ENGINE = 90,                    /* ENGINE  */
  YYSYMBOL_TORCHSCRIPT = 91,               /* TORCHSCRIPT  */
  YYSYMBOL_OPENVINO_IR = 92,               /* OPENVINO_IR  */
  YYSYMBOL_GGUF = 93,                      /* GGUF  */
  YYSYMBOL_TENSORRT = 94,                  /* TENSORRT  */
  YYSYMBOL_ONNXRUNTIME_TENSORRT = 95,      /* ONNXRUNTIME_TENSORRT  */
  YYSYMBOL_ONNXRUNTIME_CUDA = 96,          /* ONNXRUNTIME_CUDA  */
  YYSYMBOL_ONNXRUNTIME_CPU = 97,           /* ONNXRUNTIME_CPU  */
  YYSYMBOL_OPENVINO = 98,                  /* OPENVINO  */
  YYSYMBOL_LIBTORCH = 99,                  /* LIBTORCH  */
  YYSYMBOL_LLAMACPP = 100,                 /* LLAMACPP  */
  YYSYMBOL_FALLBACK = 101,                 /* FALLBACK  */
  YYSYMBOL_MQ1 = 102,                      /* MQ1  */
  YYSYMBOL_MQ2 = 103,                      /* MQ2  */
  YYSYMBOL_MQ3 = 104,                      /* MQ3  */
  YYSYMBOL_MQ4 = 105,                      /* MQ4  */
  YYSYMBOL_DQ1 = 106,                      /* DQ1  */
  YYSYMBOL_DQ2 = 107,                      /* DQ2  */
  YYSYMBOL_DQ3 = 108,                      /* DQ3  */
  YYSYMBOL_DQ4 = 109,                      /* DQ4  */
  YYSYMBOL_LOCATION = 110,                 /* LOCATION  */
  YYSYMBOL_CI_CD = 111,                    /* CI_CD  */
  YYSYMBOL_THIRDPARTY = 112,               /* THIRDPARTY  */
  YYSYMBOL_ACCELERATOR = 113,              /* ACCELERATOR  */
  YYSYMBOL_COMPUTE = 114,                  /* COMPUTE  */
  YYSYMBOL_STORAGE = 115,                  /* STORAGE  */
  YYSYMBOL_NETWORK = 116,                  /* NETWORK  */
  YYSYMBOL_117_ = 117,                     /* ';'  */
  YYSYMBOL_118_ = 118,                     /* '.'  */
  YYSYMBOL_119_ = 119,                     /* '('  */
  YYSYMBOL_120_ = 120,                     /* ')'  */
  YYSYMBOL_121_ = 121,                     /* ','  */
  YYSYMBOL_122_ = 122,                     /* '['  */
  YYSYMBOL_123_ = 123,                     /* ']'  */
  YYSYMBOL_124_ = 124,                     /* '{'  */
  YYSYMBOL_125_ = 125,                     /* '}'  */
  YYSYMBOL_126_ = 126,                     /* ':'  */
  YYSYMBOL_YYACCEPT = 127,                 /* $accept  */
  YYSYMBOL_PROGRAMME_RULE = 128,           /* PROGRAMME_RULE  */
  YYSYMBOL_MODULE_PREAMBLE_RULE = 129,     /* MODULE_PREAMBLE_RULE  */
  YYSYMBOL_PACKAGE_DECLARATION = 130,      /* PACKAGE_DECLARATION  */
  YYSYMBOL_MODULE_DECLARATION = 131,       /* MODULE_DECLARATION  */
  YYSYMBOL_IMPORT_DECLARATION = 132,       /* IMPORT_DECLARATION  */
  YYSYMBOL_MODULE_PATH = 133,              /* MODULE_PATH  */
  YYSYMBOL_IMPORT_ALIAS_OPT = 134,         /* IMPORT_ALIAS_OPT  */
  YYSYMBOL_FUNCTION_LIST_RULE = 135,       /* FUNCTION_LIST_RULE  */
  YYSYMBOL_FUNCTION_RULE = 136,            /* FUNCTION_RULE  */
  YYSYMBOL_FUNCTION_PARAMETER_LIST_RULE = 137, /* FUNCTION_PARAMETER_LIST_RULE  */
  YYSYMBOL_DECLARATION_STATEMENT_LIST_RULE = 138, /* DECLARATION_STATEMENT_LIST_RULE  */
  YYSYMBOL_ShortType = 139,                /* ShortType  */
  YYSYMBOL_DECLARATION_STATEMENT_RULE = 140, /* DECLARATION_STATEMENT_RULE  */
  YYSYMBOL_DECLARATION_VARIABLE_LIST_RULE = 141, /* DECLARATION_VARIABLE_LIST_RULE  */
  YYSYMBOL_LOGIC_BLOCK = 142,              /* LOGIC_BLOCK  */
  YYSYMBOL_TOP_LEVEL_STATEMENT_LIST_RULE = 143, /* TOP_LEVEL_STATEMENT_LIST_RULE  */
  YYSYMBOL_STATEMENT_BLOCK_RULE = 144,     /* STATEMENT_BLOCK_RULE  */
  YYSYMBOL_STATEMENT_LIST_RULE = 145,      /* STATEMENT_LIST_RULE  */
  YYSYMBOL_STATEMENT_RULE = 146,           /* STATEMENT_RULE  */
  YYSYMBOL_EXECUTABLE_STATEMENT_RULE = 147, /* EXECUTABLE_STATEMENT_RULE  */
  YYSYMBOL_RETURN_STATEMENT = 148,         /* RETURN_STATEMENT  */
  YYSYMBOL_INFER_STATEMENT = 149,          /* INFER_STATEMENT  */
  YYSYMBOL_TENSOR_DECLARATION = 150,       /* TENSOR_DECLARATION  */
  YYSYMBOL_MODEL_DECLARATION = 151,        /* MODEL_DECLARATION  */
  YYSYMBOL_152_1 = 152,                    /* $@1  */
  YYSYMBOL_MODEL_FIELD_LIST = 153,         /* MODEL_FIELD_LIST  */
  YYSYMBOL_MODEL_FIELD = 154,              /* MODEL_FIELD  */
  YYSYMBOL_BACKEND_LIST = 155,             /* BACKEND_LIST  */
  YYSYMBOL_GREENAI_CONTRACT = 156,         /* GREENAI_CONTRACT  */
  YYSYMBOL_157_2 = 157,                    /* $@2  */
  YYSYMBOL_CONTRACT_FIELD_LIST = 158,      /* CONTRACT_FIELD_LIST  */
  YYSYMBOL_CONTRACT_FIELD = 159,           /* CONTRACT_FIELD  */
  YYSYMBOL_BOUNDARY_LIST = 160,            /* BOUNDARY_LIST  */
  YYSYMBOL_GREENAI_MEASUREMENT = 161,      /* GREENAI_MEASUREMENT  */
  YYSYMBOL_162_3 = 162,                    /* $@3  */
  YYSYMBOL_MEASURE_FIELD_LIST = 163,       /* MEASURE_FIELD_LIST  */
  YYSYMBOL_MEASURE_FIELD = 164,            /* MEASURE_FIELD  */
  YYSYMBOL_C3ECO_DECLARATION = 165,        /* C3ECO_DECLARATION  */
  YYSYMBOL_166_4 = 166,                    /* $@4  */
  YYSYMBOL_167_5 = 167,                    /* $@5  */
  YYSYMBOL_168_6 = 168,                    /* $@6  */
  YYSYMBOL_169_7 = 169,                    /* $@7  */
  YYSYMBOL_170_8 = 170,                    /* $@8  */
  YYSYMBOL_171_9 = 171,                    /* $@9  */
  YYSYMBOL_172_10 = 172,                   /* $@10  */
  YYSYMBOL_173_11 = 173,                   /* $@11  */
  YYSYMBOL_174_12 = 174,                   /* $@12  */
  YYSYMBOL_175_13 = 175,                   /* $@13  */
  YYSYMBOL_176_14 = 176,                   /* $@14  */
  YYSYMBOL_C3ECO_FIELD_LIST = 177,         /* C3ECO_FIELD_LIST  */
  YYSYMBOL_C3ECO_FIELD = 178,              /* C3ECO_FIELD  */
  YYSYMBOL_C3ECO_FIELD_NAME = 179,         /* C3ECO_FIELD_NAME  */
  YYSYMBOL_C3ECO_IDENTIFIER_VALUE = 180,   /* C3ECO_IDENTIFIER_VALUE  */
  YYSYMBOL_FORMAT_NAME = 181,              /* FORMAT_NAME  */
  YYSYMBOL_PRECISION_NAME = 182,           /* PRECISION_NAME  */
  YYSYMBOL_BACKEND_NAME = 183,             /* BACKEND_NAME  */
  YYSYMBOL_MQ_NAME = 184,                  /* MQ_NAME  */
  YYSYMBOL_DQ_NAME = 185,                  /* DQ_NAME  */
  YYSYMBOL_BOUNDARY_NAME = 186,            /* BOUNDARY_NAME  */
  YYSYMBOL_AI_INFER_RULE = 187,            /* AI_INFER_RULE  */
  YYSYMBOL_GREENAI_REPORT_RULE = 188,      /* GREENAI_REPORT_RULE  */
  YYSYMBOL_EXPRESSION_RULE = 189,          /* EXPRESSION_RULE  */
  YYSYMBOL_EXPRESSION_LIST_OPT = 190,      /* EXPRESSION_LIST_OPT  */
  YYSYMBOL_EXPRESSION_LIST_RULE = 191,     /* EXPRESSION_LIST_RULE  */
  YYSYMBOL_VARIABLE_RULE = 192,            /* VARIABLE_RULE  */
  YYSYMBOL_READ_VARIABLE_LIST_RULE = 193,  /* READ_VARIABLE_LIST_RULE  */
  YYSYMBOL_PRINT_VARIABLE_LIST_RULE = 194  /* PRINT_VARIABLE_LIST_RULE  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;



/* Unqualified %code blocks.  */
#line 32 "scanner_parser/parser.yy"

struct ShorthandParserAllocation {
    void *pointer;
    void (*destroy)(void *);
};

class ShorthandParserAllocationRegistry {
private:
    vector<ShorthandParserAllocation> allocations;
    unordered_set<void *> tracked;

public:
    template <typename T>
    T *track(T *node) {
        if (node == nullptr) return node;
        if (tracked.insert(static_cast<void *>(node)).second) {
            allocations.push_back(
                ShorthandParserAllocation{
                    static_cast<void *>(node),
                    [](void *pointer) { delete static_cast<T *>(pointer); }});
        }
        return node;
    }

    ~ShorthandParserAllocationRegistry() {
        for (vector<ShorthandParserAllocation>::reverse_iterator it = allocations.rbegin();
             it != allocations.rend(); ++it) {
            it->destroy(it->pointer);
        }
        main_program = nullptr;
        main_module_preamble = nullptr;
    }
};

static ShorthandParserAllocationRegistry &shorthand_parser_allocation_registry() {
    static ShorthandParserAllocationRegistry registry;
    return registry;
}

template <typename T>
static T *shorthand_track_parser_node(T *node) {
    return shorthand_parser_allocation_registry().track(node);
}

static SourceRange shorthand_range(const YYLTYPE &loc) {
    SourceRange range;
    range.begin.line = loc.first_line;
    range.begin.column = loc.first_column;
    range.end.line = loc.last_line;
    range.end.column = loc.last_column;
    return range;
}

static void shorthand_parser_diagnostic(const char *code,
                                        const char *message,
                                        const YYLTYPE &loc) {
    const char *path = shorthand_source_path == nullptr ? "<input>" : shorthand_source_path;
    fprintf(stderr, "----------------ERROR----------------\n");
    fprintf(stderr,
            "%s:%d:%d: error: [%s] %s [range %d:%d-%d:%d]\n",
            path,
            loc.first_line,
            loc.first_column,
            code,
            message,
            loc.first_line,
            loc.first_column,
            loc.last_line,
            loc.last_column);
}

template <typename T>
static T *located(T *node, const YYLTYPE &loc) {
    shorthand_track_parser_node(node);
    shorthand_set_ast_source_range(node, shorthand_range(loc));
    return node;
}

static AST_MODULE_PREAMBLE *shorthand_ensure_module_preamble(const YYLTYPE &loc) {
    if (main_module_preamble == nullptr) {
        const char *path = shorthand_source_path == nullptr ? "<input>" : shorthand_source_path;
        main_module_preamble = shorthand_track_parser_node(new AST_MODULE_PREAMBLE(path));
        shorthand_set_ast_source_range(main_module_preamble, shorthand_range(loc));
    }
    return main_module_preamble;
}

static void shorthand_begin_c3eco(C3EcoDeclarationKind kind, const char *name) {
    current_c3eco = C3EcoDeclarationData();
    current_c3eco.kind = kind;
    current_c3eco.name = name == nullptr ? "" : std::string(name);
}

static std::string shorthand_unquote(const char *value) {
    if (value == nullptr) return "";
    std::string text(value);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
        return text.substr(1, text.size() - 2);
    return text;
}

static void shorthand_add_c3eco_field(const char *name,
                                      C3EcoValueKind kind,
                                      const std::string &value) {
    const std::string field_name = name == nullptr ? "" : std::string(name);
    for (auto &field : current_c3eco.fields) {
        if (field.name == field_name) {
            field.values.push_back({kind, value});
            return;
        }
    }
    C3EcoFieldData field;
    field.name = field_name;
    field.values.push_back({kind, value});
    current_c3eco.fields.push_back(field);
}

static std::string shorthand_decimal_text(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.15g", value);
    return std::string(buffer);
}

#line 453 "parser.tab.cc"

#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   837

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  127
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  68
/* YYNRULES -- Number of rules.  */
#define YYNRULES  220
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  454

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   365


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    18,     2,     2,
     119,   120,    16,    14,   121,    15,   118,    17,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   126,   117,
       2,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   122,     2,   123,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   124,     2,   125,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   207,   207,   222,   223,   224,   225,   227,   243,   259,
     281,   282,   285,   286,   289,   290,   292,   296,   297,   300,
     301,   303,   303,   303,   303,   303,   303,   305,   308,   309,
     310,   311,   312,   321,   323,   324,   325,   327,   328,   331,
     332,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   362,   363,   366,   367,
     369,   375,   375,   376,   376,   377,   377,   377,   377,   377,
     377,   377,   377,   377,   377,   378,   378,   380,   380,   381,
     381,   382,   382,   382,   382,   382,   382,   382,   382,   382,
     382,   382,   383,   383,   385,   385,   386,   386,   387,   387,
     390,   390,   391,   391,   392,   392,   393,   393,   394,   394,
     395,   395,   396,   396,   397,   397,   398,   398,   399,   399,
     400,   400,   401,   401,   403,   404,   405,   406,   407,   408,
     410,   411,   412,   413,   414,   415,   418,   419,   420,   421,
     422,   423,   424,   426,   426,   426,   426,   426,   427,   427,
     427,   427,   427,   427,   427,   428,   428,   428,   428,   428,
     428,   428,   428,   429,   429,   429,   429,   430,   430,   430,
     430,   431,   431,   431,   431,   431,   431,   433,   442,   452,
     453,   454,   455,   456,   457,   458,   459,   460,   461,   462,
     463,   464,   465,   466,   467,   468,   469,   470,   471,   472,
     473,   476,   477,   480,   481,   484,   485,   488,   489,   492,
     493
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ETOK", "'='", "OR",
  "AND", "EQUAL", "NOT_EQUAL", "LESS", "LESS_OR_EQUAL", "GREATER",
  "GREATER_OR_EQUAL", "ARROW", "'+'", "'-'", "'*'", "'/'", "'%'", "UMINUS",
  "STRING_LITERAL", "IDENTIFIER", "AI_INFER_BUILTIN",
  "GREENAI_REPORT_BUILTIN", "INT_LITERAL", "FLOAT_LITERAL", "READ",
  "PRINT", "GOTO", "BREAK", "WHILE", "LOOP", "ELSE", "IF", "DEF", "INT",
  "FLOAT", "STRING", "VOID", "BOOL", "DOUBLE", "RETURN", "CONTINUE",
  "TRUE", "FALSE", "PACKAGE", "MODULE", "IMPORT", "AS", "MODEL", "FORMAT",
  "PATH", "TASK", "PRECISION", "INPUT_SHAPE", "OUTPUT_SHAPE",
  "BACKEND_PREFERENCE", "COMPACT", "QUALITY_GUARDRAIL",
  "GREENAI_CONTRACT_T", "FUNCTIONAL_UNIT", "SUCCESS_CRITERIA", "BOUNDARY",
  "MEASUREMENT_QUALITY", "DATA_QUALITY", "CARBON_FACTOR",
  "ENERGY_BUDGET_J", "CARBON_BUDGET_GCO2E", "EVIDENCE_RETENTION",
  "CLAIMS_MODE", "EVIDENCE_ONLY", "GREENAI_MEASURE", "INFER", "TENSOR",
  "CERTIFICATION_PROFILE", "CERTIFICATION", "WORKLOAD", "MEASUREMENT_PLAN",
  "AI_LIFECYCLE", "RAG_PIPELINE", "TOKEN_BUDGET", "MODEL_ROUTING",
  "GUARDRAILS", "INT8", "FP16", "FP32", "BF16", "INT4", "FP64", "ONNX",
  "ENGINE", "TORCHSCRIPT", "OPENVINO_IR", "GGUF", "TENSORRT",
  "ONNXRUNTIME_TENSORRT", "ONNXRUNTIME_CUDA", "ONNXRUNTIME_CPU",
  "OPENVINO", "LIBTORCH", "LLAMACPP", "FALLBACK", "MQ1", "MQ2", "MQ3",
  "MQ4", "DQ1", "DQ2", "DQ3", "DQ4", "LOCATION", "CI_CD", "THIRDPARTY",
  "ACCELERATOR", "COMPUTE", "STORAGE", "NETWORK", "';'", "'.'", "'('",
  "')'", "','", "'['", "']'", "'{'", "'}'", "':'", "$accept",
  "PROGRAMME_RULE", "MODULE_PREAMBLE_RULE", "PACKAGE_DECLARATION",
  "MODULE_DECLARATION", "IMPORT_DECLARATION", "MODULE_PATH",
  "IMPORT_ALIAS_OPT", "FUNCTION_LIST_RULE", "FUNCTION_RULE",
  "FUNCTION_PARAMETER_LIST_RULE", "DECLARATION_STATEMENT_LIST_RULE",
  "ShortType", "DECLARATION_STATEMENT_RULE",
  "DECLARATION_VARIABLE_LIST_RULE", "LOGIC_BLOCK",
  "TOP_LEVEL_STATEMENT_LIST_RULE", "STATEMENT_BLOCK_RULE",
  "STATEMENT_LIST_RULE", "STATEMENT_RULE", "EXECUTABLE_STATEMENT_RULE",
  "RETURN_STATEMENT", "INFER_STATEMENT", "TENSOR_DECLARATION",
  "MODEL_DECLARATION", "$@1", "MODEL_FIELD_LIST", "MODEL_FIELD",
  "BACKEND_LIST", "GREENAI_CONTRACT", "$@2", "CONTRACT_FIELD_LIST",
  "CONTRACT_FIELD", "BOUNDARY_LIST", "GREENAI_MEASUREMENT", "$@3",
  "MEASURE_FIELD_LIST", "MEASURE_FIELD", "C3ECO_DECLARATION", "$@4", "$@5",
  "$@6", "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13", "$@14",
  "C3ECO_FIELD_LIST", "C3ECO_FIELD", "C3ECO_FIELD_NAME",
  "C3ECO_IDENTIFIER_VALUE", "FORMAT_NAME", "PRECISION_NAME",
  "BACKEND_NAME", "MQ_NAME", "DQ_NAME", "BOUNDARY_NAME", "AI_INFER_RULE",
  "GREENAI_REPORT_RULE", "EXPRESSION_RULE", "EXPRESSION_LIST_OPT",
  "EXPRESSION_LIST_RULE", "VARIABLE_RULE", "READ_VARIABLE_LIST_RULE",
  "PRINT_VARIABLE_LIST_RULE", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-305)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -305,    16,   780,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
      24,    24,    24,  -305,  -305,  -305,   153,    52,  -104,  -305,
    -111,   -84,   -31,   464,   -17,     4,    25,  -305,  -305,   155,
    -305,   164,    77,   576,  -305,     9,    85,   106,  -305,  -305,
     210,   576,   223,   123,   576,   576,   153,   507,   151,  -305,
    -305,   312,   321,   322,   333,   335,   348,   350,   353,   354,
     357,   358,   359,   360,   361,   366,   368,  -305,   576,   395,
     268,  -305,   533,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,   273,   274,   241,   388,  -305,   369,   386,  -305,
    -305,  -305,   -62,  -305,  -305,   576,   576,  -305,   389,   391,
     290,  -305,     6,   776,     8,   -24,  -305,    32,   409,    32,
     393,  -305,   255,  -305,   301,   303,   305,   316,   318,   326,
     -32,   323,   327,   328,   332,   356,   370,   372,   375,   376,
     191,   336,   324,  -305,  -305,  -305,  -305,  -305,  -305,   576,
     576,   576,   576,   576,   576,   576,   576,   576,   576,   576,
     576,   576,  -305,   576,   355,   379,   776,   330,   325,    60,
     362,   381,  -305,   210,  -305,   576,   576,  -305,  -305,   576,
     449,   363,  -305,  -305,  -305,  -305,  -305,  -305,   482,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,   484,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
     721,    87,   819,   819,   219,   219,   219,   219,   136,   136,
    -305,  -305,  -305,   271,  -305,   485,  -305,   576,  -305,   490,
     576,  -305,   776,   291,    74,   387,   153,  -305,  -305,  -305,
    -305,  -305,   396,   398,  -305,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,   394,   776,   397,   133,  -305,   576,
    -305,   400,   153,   107,   252,   269,   563,     5,   317,  -305,
     569,   577,   578,   596,   609,   624,   625,   647,   655,  -305,
     501,   576,    13,   387,   233,   505,   509,   -32,   510,   513,
     364,   231,   526,   417,  -305,   528,   532,   543,    99,   190,
     229,   455,   544,   545,   547,   500,   454,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,   456,  -305,   698,   461,   221,   462,
    -305,   551,   559,   468,   469,   470,   472,   477,   486,   499,
     515,   516,   498,   163,   576,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,   524,   527,   530,   531,   534,   536,  -305,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,    65,  -305,   538,   542,
     590,  -305,   610,   546,   548,  -305,  -305,  -305,  -305,  -305,
    -305,   122,  -305,  -305,  -305,  -305,  -305,   549,  -305,  -305,
    -305,  -305,   555,   636,   556,   558,   560,   564,  -305,  -305,
     574,  -305,   575,   579,   581,   582,  -305,  -305,  -305,   583,
    -305,  -305,  -305,  -305,   584,   587,  -305,   589,   591,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,   576,
      32,  -305,  -305,  -305,  -305,  -305,  -305,  -305,   364,  -305,
    -305,   638,   656,  -305,  -305,  -305,    99,  -305,  -305,   594,
    -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,   212,  -305,  -305,   597,   599,  -305,
    -305,  -305,  -305,  -305
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     1,    21,    22,    24,    25,    26,    23,
       0,     0,     0,     4,     5,     6,    15,    32,     0,    10,
       0,     0,    12,     0,     0,    30,    27,    20,     7,     0,
       8,     0,     0,     0,   208,   215,     0,     0,   206,   207,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   209,
     210,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    65,     0,     0,
       0,     2,    33,    51,    35,    47,    46,    42,    41,    43,
      44,    45,     0,     0,     0,   205,    19,     0,     0,    11,
      13,     9,   215,   202,   205,   212,     0,    64,     0,     0,
     215,   218,     0,   220,     0,     0,    60,     0,   205,     0,
       0,    67,     0,    48,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,    39,    14,    34,    63,    62,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,     0,     0,    28,   214,     0,   211,     0,
       0,     0,    59,     0,    61,     0,     0,    57,    56,     0,
      52,     0,    66,    71,    87,   114,   118,   104,     0,   164,
     158,   160,   161,   162,   159,   163,     0,   110,   112,   116,
     120,   122,   124,   126,   128,   130,   203,    40,    36,    37,
     200,   201,   198,   199,   194,   195,   196,   197,   189,   190,
     191,   192,   193,     0,    31,     0,   204,     0,   216,     0,
       0,   217,   219,     0,     0,     0,    18,    74,    90,   133,
     133,   107,     0,     0,   133,   133,   133,   133,   133,   133,
     133,   133,   133,    50,     0,   213,     0,     0,    58,     0,
      53,     0,    17,     0,     0,     0,     0,     0,     0,    70,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    29,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    89,   140,   143,
     144,   145,   141,   142,     0,   132,     0,     0,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    54,    16,   153,   154,   155,
     156,   157,     0,     0,     0,     0,     0,     0,   165,   166,
     167,   168,   169,   170,   171,   172,     0,    86,     0,     0,
       0,    72,     0,     0,     0,   185,   186,   182,   181,   183,
     184,     0,   103,   173,   174,   175,   176,     0,   177,   178,
     179,   180,     0,     0,     0,     0,     0,     0,    88,   115,
       0,   146,     0,     0,     0,     0,   151,   152,   150,     0,
     148,   149,   147,   119,     0,     0,   105,     0,     0,   111,
     113,   117,   121,   123,   125,   127,   129,   131,   187,     0,
       0,    75,    76,    77,    78,    79,    80,    81,     0,    82,
      83,     0,     0,    91,    92,    93,     0,    94,    95,     0,
      97,    98,   100,   101,   134,   136,   137,   138,   139,   135,
     109,   108,    69,    68,     0,    55,    85,     0,     0,   102,
      96,   188,    84,    99
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -305,  -305,  -305,  -305,  -305,  -305,   329,  -305,  -305,  -305,
    -305,   423,   678,   -11,  -305,  -305,  -305,  -106,  -305,   561,
     -13,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
    -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,  -305,
     523,  -305,  -305,  -305,  -305,   463,   334,   437,   438,  -304,
    -305,  -305,   -33,  -305,  -305,    -8,  -305,  -305
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,     2,    13,    14,    15,    20,    32,    23,    70,
     251,    16,    17,    18,    26,    71,    72,    73,   132,   133,
     134,    75,    76,    77,    78,   227,   253,   284,   346,    79,
     228,   254,   297,   361,    80,   231,   257,   310,    81,   234,
     235,   229,   236,   230,   237,   238,   239,   240,   241,   242,
     255,   305,   306,   389,   332,   186,   347,   367,   372,   362,
      82,    83,    84,   157,   158,    94,   102,   104
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      93,   168,   392,   170,   179,    24,    28,    29,   103,   166,
      74,   107,   109,    27,   112,    85,     3,    31,   139,   140,
     141,   142,   143,   144,   145,   146,   308,   147,   148,   149,
     150,   151,   101,    30,    29,   130,   108,   139,   140,   141,
     142,   143,   144,   145,   146,    19,   147,   148,   149,   150,
     151,   180,   181,   182,   183,   184,   185,    95,   131,   136,
      96,    85,   156,   159,    85,   139,   140,   141,   142,   143,
     144,   145,   146,    25,   147,   148,   149,   150,   151,   139,
     140,   141,   142,   143,   144,   145,   146,    29,   147,   148,
     149,   150,   151,   167,   141,   142,   143,   144,   145,   146,
      86,   147,   148,   149,   150,   151,   200,   201,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   250,
     213,   131,   449,   162,    85,   164,    87,   163,    95,   165,
     309,    96,   222,   223,   324,    97,   224,    69,   139,   140,
     141,   142,   143,   144,   145,   146,    88,   147,   148,   149,
     150,   151,   149,   150,   151,   221,    69,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   325,   326,   139,   140,
     141,   142,   143,   144,   145,   146,    89,   147,   148,   149,
     150,   151,   417,   218,   245,    90,   418,   247,     4,     5,
       6,     7,     8,     9,    91,   249,   139,   140,   141,   142,
     143,   144,   145,   146,    98,   147,   148,   149,   150,   151,
     355,   356,   357,   358,   359,   360,   272,   139,   140,   141,
     142,   143,   144,   145,   146,    99,   147,   148,   149,   150,
     151,   100,   283,   147,   148,   149,   150,   151,   323,   425,
     106,    24,   394,   426,   105,   395,   139,   140,   141,   142,
     143,   144,   145,   146,   271,   147,   148,   149,   150,   151,
     139,   140,   141,   142,   143,   144,   145,   146,   113,   147,
     148,   149,   150,   151,   348,   349,   139,   140,   141,   142,
     143,   144,   145,   146,   409,   147,   148,   149,   150,   151,
     298,   410,   363,   364,   365,   366,   139,   140,   141,   142,
     143,   144,   145,   146,   445,   147,   148,   149,   150,   151,
     285,   196,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   327,   328,   329,   330,   331,   299,   311,   300,
     312,   301,   451,   114,   302,   368,   369,   370,   371,    33,
      21,    22,   115,   116,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,   117,    44,   118,    45,   152,     4,
       5,     6,     7,     8,     9,    47,    48,    49,    50,   119,
     303,   120,   172,    51,   121,   122,   444,   296,   123,   124,
     125,   126,   127,    52,    53,   135,    54,   128,   243,   129,
     137,   138,   153,   154,   304,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,   155,   248,   160,
      33,   161,    96,   169,   171,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,   173,    44,   174,    45,   175,
       4,     5,     6,     7,     8,     9,    47,    48,    49,    50,
     176,    67,   177,    68,    51,   178,   217,   187,    69,   198,
     216,   188,   189,   197,    52,    53,   190,    54,   338,   339,
     340,   341,   342,   343,   344,   345,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,   214,    33,
     191,   225,   226,   219,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,   192,    44,   193,    45,    46,   194,
     195,   215,   220,   232,   233,    47,    48,    49,    50,   244,
     246,    69,    67,    51,    68,   259,   258,   269,   270,    69,
     273,   322,    33,    52,    53,   333,    54,    34,    92,   334,
     336,    38,    39,   337,   351,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,   350,    33,   352,
      49,    50,   353,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,   354,    44,   373,    45,   376,   374,   375,
     377,   378,   397,   379,    47,    48,    49,    50,   393,   396,
     398,    67,    51,    68,   298,   399,   400,   401,    69,   402,
     298,    33,    52,    53,   403,    54,    34,    92,   298,   298,
      38,    39,   421,   404,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,   405,   298,   408,    49,
      50,   299,   422,   300,   111,   301,    68,   299,   302,   300,
     298,   301,   406,   407,   302,   299,   299,   300,   300,   301,
     301,   411,   302,   302,   412,   298,   298,   413,   414,   252,
      67,   415,    68,   416,   299,   419,   300,    69,   301,   420,
     429,   302,   447,   423,   303,   424,   427,   299,   298,   300,
     303,   301,   428,   430,   302,   431,   298,   432,   303,   303,
     448,   433,   299,   299,   300,   300,   301,   301,   307,   302,
     302,   434,   435,   199,   313,    68,   436,   303,   437,   438,
     439,   440,   314,   315,   441,   299,   442,   300,   443,   301,
     303,   450,   302,   299,   452,   300,   453,   301,   380,   381,
     302,   316,   382,   383,   110,   303,   303,   140,   141,   142,
     143,   144,   145,   146,   317,   147,   148,   149,   150,   151,
     335,   384,   385,   390,   391,     0,     0,     0,   303,   318,
     319,     0,   446,   256,     0,     0,   303,   260,   261,   262,
     263,   264,   265,   266,   267,   268,     0,     0,   386,     0,
       0,     0,   320,     0,     0,     0,     0,     0,     0,     0,
     321,   139,   140,   141,   142,   143,   144,   145,   146,     0,
     147,   148,   149,   150,   151,     0,     0,     0,     0,   387,
     363,   364,   365,   366,   368,   369,   370,   371,   388,   355,
     356,   357,   358,   359,   360,     4,     5,     6,     7,     8,
       9,     0,     0,     0,     0,    10,    11,    12,   143,   144,
     145,   146,     0,   147,   148,   149,   150,   151
};

static const yytype_int16 yycheck[] =
{
      33,   107,   306,   109,    36,    16,   117,   118,    41,    33,
      23,    44,    45,   117,    47,    23,     0,    48,     5,     6,
       7,     8,     9,    10,    11,    12,    21,    14,    15,    16,
      17,    18,    40,   117,   118,    68,    44,     5,     6,     7,
       8,     9,    10,    11,    12,    21,    14,    15,    16,    17,
      18,    83,    84,    85,    86,    87,    88,   119,    69,    72,
     122,    69,    95,    96,    72,     5,     6,     7,     8,     9,
      10,    11,    12,    21,    14,    15,    16,    17,    18,     5,
       6,     7,     8,     9,    10,    11,    12,   118,    14,    15,
      16,    17,    18,   117,     7,     8,     9,    10,    11,    12,
     117,    14,    15,    16,    17,    18,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   225,
     153,   132,   426,   117,   132,   117,   122,   121,   119,   121,
     125,   122,   165,   166,   121,   126,   169,   124,     5,     6,
       7,     8,     9,    10,    11,    12,   121,    14,    15,    16,
      17,    18,    16,    17,    18,   163,   124,    50,    51,    52,
      53,    54,    55,    56,    57,    58,   272,   273,     5,     6,
       7,     8,     9,    10,    11,    12,    21,    14,    15,    16,
      17,    18,   117,   123,   217,    21,   121,   220,    35,    36,
      37,    38,    39,    40,   117,   121,     5,     6,     7,     8,
       9,    10,    11,    12,   119,    14,    15,    16,    17,    18,
     111,   112,   113,   114,   115,   116,   249,     5,     6,     7,
       8,     9,    10,    11,    12,   119,    14,    15,    16,    17,
      18,    21,   125,    14,    15,    16,    17,    18,   271,   117,
     117,   252,    21,   121,    21,    24,     5,     6,     7,     8,
       9,    10,    11,    12,   121,    14,    15,    16,    17,    18,
       5,     6,     7,     8,     9,    10,    11,    12,   117,    14,
      15,    16,    17,    18,    43,    44,     5,     6,     7,     8,
       9,    10,    11,    12,   121,    14,    15,    16,    17,    18,
      21,   324,   102,   103,   104,   105,     5,     6,     7,     8,
       9,    10,    11,    12,   410,    14,    15,    16,    17,    18,
      58,   120,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    89,    90,    91,    92,    93,    58,    11,    60,
      13,    62,   120,    21,    65,   106,   107,   108,   109,    15,
      11,    12,    21,    21,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    21,    31,    21,    33,   117,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    21,
     101,    21,   117,    49,    21,    21,   409,   125,    21,    21,
      21,    21,    21,    59,    60,   117,    62,    21,   117,    21,
     117,   117,     4,    24,   125,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    21,   117,    20,
      15,    20,   122,     4,    21,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,   124,    31,   124,    33,   124,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
     124,   117,   124,   119,    49,   119,   121,   124,   124,   125,
     120,   124,   124,   117,    59,    60,   124,    62,    94,    95,
      96,    97,    98,    99,   100,   101,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,   123,    15,
     124,    32,   119,   121,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,   124,    31,   124,    33,    34,   124,
     124,   122,   121,    21,    20,    41,    42,    43,    44,    24,
      20,   124,   117,    49,   119,   117,   120,   123,   121,   124,
     120,    20,    15,    59,    60,    20,    62,    20,    21,    20,
      20,    24,    25,    20,   117,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    21,    15,    21,
      43,    44,    20,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    20,    31,   110,    33,    20,    24,    24,
      70,   117,    21,   117,    41,    42,    43,    44,   117,   117,
      21,   117,    49,   119,    21,   117,   117,   117,   124,   117,
      21,    15,    59,    60,   117,    62,    20,    21,    21,    21,
      24,    25,    12,   117,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,   117,    21,   120,    43,
      44,    58,    12,    60,   117,    62,   119,    58,    65,    60,
      21,    62,   117,   117,    65,    58,    58,    60,    60,    62,
      62,   117,    65,    65,   117,    21,    21,   117,   117,   226,
     117,   117,   119,   117,    58,   117,    60,   124,    62,   117,
      24,    65,    24,   117,   101,   117,   117,    58,    21,    60,
     101,    62,   117,   117,    65,   117,    21,   117,   101,   101,
      24,   117,    58,    58,    60,    60,    62,    62,   125,    65,
      65,   117,   117,   132,   125,   119,   117,   101,   117,   117,
     117,   117,   125,   125,   117,    58,   117,    60,   117,    62,
     101,   117,    65,    58,   117,    60,   117,    62,    20,    21,
      65,   125,    24,    25,    46,   101,   101,     6,     7,     8,
       9,    10,    11,    12,   125,    14,    15,    16,    17,    18,
     277,    43,    44,   306,   306,    -1,    -1,    -1,   101,   125,
     125,    -1,   418,   230,    -1,    -1,   101,   234,   235,   236,
     237,   238,   239,   240,   241,   242,    -1,    -1,    70,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     125,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    15,    16,    17,    18,    -1,    -1,    -1,    -1,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    45,    46,    47,     9,    10,
      11,    12,    -1,    14,    15,    16,    17,    18
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   128,   129,     0,    35,    36,    37,    38,    39,    40,
      45,    46,    47,   130,   131,   132,   138,   139,   140,    21,
     133,   133,   133,   135,   140,    21,   141,   117,   117,   118,
     117,    48,   134,    15,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    31,    33,    34,    41,    42,    43,
      44,    49,    59,    60,    62,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   117,   119,   124,
     136,   142,   143,   144,   147,   148,   149,   150,   151,   156,
     161,   165,   187,   188,   189,   192,   117,   122,   121,    21,
      21,   117,    21,   189,   192,   119,   122,   126,   119,   119,
      21,   192,   193,   189,   194,    21,   117,   189,   192,   189,
     139,   117,   189,   117,    21,    21,    21,    21,    21,    21,
      21,    21,    21,    21,    21,    21,    21,    21,    21,    21,
     189,   140,   145,   146,   147,   117,   147,   117,   117,     5,
       6,     7,     8,     9,    10,    11,    12,    14,    15,    16,
      17,    18,   117,     4,    24,    21,   189,   190,   191,   189,
      20,    20,   117,   121,   117,   121,    33,   117,   144,     4,
     144,    21,   117,   124,   124,   124,   124,   124,   119,    36,
      83,    84,    85,    86,    87,    88,   182,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   120,   117,   125,   146,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   123,   122,   120,   121,   123,   121,
     121,   192,   189,   189,   189,    32,   119,   152,   157,   168,
     170,   162,    21,    20,   166,   167,   169,   171,   172,   173,
     174,   175,   176,   117,    24,   189,    20,   189,   117,   121,
     144,   137,   138,   153,   158,   177,   177,   163,   120,   117,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   123,
     121,   121,   189,   120,    50,    51,    52,    53,    54,    55,
      56,    57,    58,   125,   154,    58,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,   125,   159,    21,    58,
      60,    62,    65,   101,   125,   178,   179,   125,    21,   125,
     164,    11,    13,   125,   125,   125,   125,   125,   125,   125,
     125,   125,    20,   189,   121,   144,   144,    89,    90,    91,
      92,    93,   181,    20,    20,   182,    20,    20,    94,    95,
      96,    97,    98,    99,   100,   101,   155,   183,    43,    44,
      21,   117,    21,    20,    20,   111,   112,   113,   114,   115,
     116,   160,   186,   102,   103,   104,   105,   184,   106,   107,
     108,   109,   185,   110,    24,    24,    20,    70,   117,   117,
      20,    21,    24,    25,    43,    44,    70,   101,   110,   180,
     184,   185,   186,   117,    21,    24,   117,    21,    21,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   120,   121,
     189,   117,   117,   117,   117,   117,   117,   117,   121,   117,
     117,    12,    12,   117,   117,   117,   121,   117,   117,    24,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   189,   144,   183,    24,    24,   186,
     117,   120,   117,   117
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   127,   128,   129,   129,   129,   129,   130,   131,   132,
     133,   133,   134,   134,   135,   135,   136,   137,   137,   138,
     138,   139,   139,   139,   139,   139,   139,   140,   141,   141,
     141,   141,   141,   142,   143,   143,   144,   145,   145,   146,
     146,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   148,   148,   149,   149,
     150,   152,   151,   153,   153,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   155,   155,   157,   156,   158,
     158,   159,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   160,   160,   162,   161,   163,   163,   164,   164,
     166,   165,   167,   165,   168,   165,   169,   165,   170,   165,
     171,   165,   172,   165,   173,   165,   174,   165,   175,   165,
     176,   165,   177,   177,   178,   178,   178,   178,   178,   178,
     179,   179,   179,   179,   179,   179,   180,   180,   180,   180,
     180,   180,   180,   181,   181,   181,   181,   181,   182,   182,
     182,   182,   182,   182,   182,   183,   183,   183,   183,   183,
     183,   183,   183,   184,   184,   184,   184,   185,   185,   185,
     185,   186,   186,   186,   186,   186,   186,   187,   188,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   190,   190,   191,   191,   192,   192,   193,   193,   194,
     194
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     4,     0,     2,     2,     2,     3,     3,     4,
       1,     3,     0,     2,     3,     0,     7,     1,     0,     3,
       2,     1,     1,     1,     1,     1,     1,     2,     3,     6,
       1,     4,     0,     1,     2,     1,     3,     2,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       4,     1,     3,     5,     7,     9,     3,     3,     5,     3,
       2,     3,     2,     2,     2,     1,     3,     2,     8,     8,
       5,     0,     7,     2,     0,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     5,     3,     1,     0,     7,     2,
       0,     3,     3,     3,     3,     3,     4,     3,     3,     5,
       3,     3,     3,     1,     0,     7,     2,     0,     3,     3,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     0,     7,     0,     7,     0,     7,     0,     7,
       0,     7,     2,     0,     3,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     8,    10,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     4,     1,     1,     1,     1,     1,
       1,     1,     0,     3,     1,     1,     4,     3,     1,     3,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* PROGRAMME_RULE: MODULE_PREAMBLE_RULE DECLARATION_STATEMENT_LIST_RULE FUNCTION_LIST_RULE LOGIC_BLOCK  */
#line 208 "scanner_parser/parser.yy"
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble((yylsp[-3]));
        if (preamble->hasAnyDeclaration() && !preamble->hasModule()) {
            shorthand_parser_diagnostic(
                shorthand::diagnostics::ParserModuleRequired,
                "a module declaration is required when package or import declarations are present",
                (yylsp[-3]));
            YYERROR;
        }
        (yyval.program) = located(new AST_PROGRAM((yyvsp[-2].decl_block),(yyvsp[-1].functions),(yyvsp[0].code_block)), (yyloc));
        main_program = (yyval.program);
    }
#line 1951 "parser.tab.cc"
    break;

  case 3: /* MODULE_PREAMBLE_RULE: %empty  */
#line 222 "scanner_parser/parser.yy"
             { shorthand_ensure_module_preamble((yyloc)); }
#line 1957 "parser.tab.cc"
    break;

  case 7: /* PACKAGE_DECLARATION: PACKAGE MODULE_PATH ';'  */
#line 228 "scanner_parser/parser.yy"
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble((yyloc));
        if (preamble->hasPackage()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicatePackageDeclaration,
                                        "duplicate package declaration", (yyloc));
            YYERROR;
        }
        if (preamble->hasModule() || preamble->hasImports()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "package declaration must precede module and import declarations", (yyloc));
            YYERROR;
        }
        preamble->setPackage(current_module_path, shorthand_range((yyloc)));
    }
#line 1976 "parser.tab.cc"
    break;

  case 8: /* MODULE_DECLARATION: MODULE MODULE_PATH ';'  */
#line 244 "scanner_parser/parser.yy"
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble((yyloc));
        if (preamble->hasModule()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateModuleDeclaration,
                                        "duplicate module declaration", (yyloc));
            YYERROR;
        }
        if (preamble->hasImports()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "module declaration must precede import declarations", (yyloc));
            YYERROR;
        }
        preamble->setModule(current_module_path, shorthand_range((yyloc)));
    }
#line 1995 "parser.tab.cc"
    break;

  case 9: /* IMPORT_DECLARATION: IMPORT MODULE_PATH IMPORT_ALIAS_OPT ';'  */
#line 260 "scanner_parser/parser.yy"
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble((yyloc));
        if (!preamble->hasModule()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "import declaration requires a preceding module declaration", (yyloc));
            YYERROR;
        }
        if (preamble->hasImportPath(current_module_path)) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateImportPath,
                                        "duplicate import path", (yyloc));
            YYERROR;
        }
        if (preamble->hasImportAlias(current_import_alias)) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateImportAlias,
                                        "duplicate import alias", (yyloc));
            YYERROR;
        }
        preamble->addImport(current_module_path, current_import_alias, shorthand_range((yyloc)));
    }
#line 2019 "parser.tab.cc"
    break;

  case 10: /* MODULE_PATH: IDENTIFIER  */
#line 281 "scanner_parser/parser.yy"
                 { current_module_path = string((yyvsp[0].string_val)); }
#line 2025 "parser.tab.cc"
    break;

  case 11: /* MODULE_PATH: MODULE_PATH '.' IDENTIFIER  */
#line 282 "scanner_parser/parser.yy"
                                 { current_module_path += "."; current_module_path += string((yyvsp[0].string_val)); }
#line 2031 "parser.tab.cc"
    break;

  case 12: /* IMPORT_ALIAS_OPT: %empty  */
#line 285 "scanner_parser/parser.yy"
             { current_import_alias.clear(); }
#line 2037 "parser.tab.cc"
    break;

  case 13: /* IMPORT_ALIAS_OPT: AS IDENTIFIER  */
#line 286 "scanner_parser/parser.yy"
                    { current_import_alias = string((yyvsp[0].string_val)); }
#line 2043 "parser.tab.cc"
    break;

  case 14: /* FUNCTION_LIST_RULE: FUNCTION_LIST_RULE FUNCTION_RULE ';'  */
#line 289 "scanner_parser/parser.yy"
                                           { (yyval.functions)=(yyvsp[-2].functions); (yyval.functions)->push_back((yyvsp[-1].function)); located((yyval.functions), (yyloc)); }
#line 2049 "parser.tab.cc"
    break;

  case 15: /* FUNCTION_LIST_RULE: %empty  */
#line 290 "scanner_parser/parser.yy"
             { (yyval.functions)=located(new AST_FUNCTION_LIST_RULE(), (yyloc)); }
#line 2055 "parser.tab.cc"
    break;

  case 16: /* FUNCTION_RULE: DEF ShortType IDENTIFIER '(' FUNCTION_PARAMETER_LIST_RULE ')' STATEMENT_BLOCK_RULE  */
#line 293 "scanner_parser/parser.yy"
    { (yyvsp[0].block_statement)->setLexicalScope(false); (yyval.function)=located(new AST_FUNCTION_RULE((yyvsp[-5].type),(yyvsp[-4].string_val),(yyvsp[-2].decl_block),(yyvsp[0].block_statement)), (yyloc)); }
#line 2061 "parser.tab.cc"
    break;

  case 17: /* FUNCTION_PARAMETER_LIST_RULE: DECLARATION_STATEMENT_LIST_RULE  */
#line 296 "scanner_parser/parser.yy"
                                      { (yyval.decl_block)=(yyvsp[0].decl_block); }
#line 2067 "parser.tab.cc"
    break;

  case 18: /* FUNCTION_PARAMETER_LIST_RULE: %empty  */
#line 297 "scanner_parser/parser.yy"
             { (yyval.decl_block)=located(new AST_DATA_DECLARATION_BLOCK(), (yyloc)); }
#line 2073 "parser.tab.cc"
    break;

  case 19: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';'  */
#line 300 "scanner_parser/parser.yy"
                                                                     { (yyval.decl_block)=(yyvsp[-2].decl_block); (yyval.decl_block)->push_back((yyvsp[-1].decl_block)); located((yyval.decl_block), (yyloc)); }
#line 2079 "parser.tab.cc"
    break;

  case 20: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_RULE ';'  */
#line 301 "scanner_parser/parser.yy"
                                     { (yyval.decl_block)=(yyvsp[-1].decl_block); located((yyval.decl_block), (yyloc)); }
#line 2085 "parser.tab.cc"
    break;

  case 21: /* ShortType: INT  */
#line 303 "scanner_parser/parser.yy"
               {(yyval.type)=ShortType::Int;}
#line 2091 "parser.tab.cc"
    break;

  case 22: /* ShortType: FLOAT  */
#line 303 "scanner_parser/parser.yy"
                                            {(yyval.type)=ShortType::Float;}
#line 2097 "parser.tab.cc"
    break;

  case 23: /* ShortType: DOUBLE  */
#line 303 "scanner_parser/parser.yy"
                                                                            {(yyval.type)=ShortType::Float;}
#line 2103 "parser.tab.cc"
    break;

  case 24: /* ShortType: STRING  */
#line 303 "scanner_parser/parser.yy"
                                                                                                            {(yyval.type)=ShortType::String;}
#line 2109 "parser.tab.cc"
    break;

  case 25: /* ShortType: VOID  */
#line 303 "scanner_parser/parser.yy"
                                                                                                                                           {(yyval.type)=ShortType::Void;}
#line 2115 "parser.tab.cc"
    break;

  case 26: /* ShortType: BOOL  */
#line 303 "scanner_parser/parser.yy"
                                                                                                                                                                        {(yyval.type)=ShortType::Boolean;}
#line 2121 "parser.tab.cc"
    break;

  case 27: /* DECLARATION_STATEMENT_RULE: ShortType DECLARATION_VARIABLE_LIST_RULE  */
#line 305 "scanner_parser/parser.yy"
                                                                     { (yyval.decl_block)=(yyvsp[0].decl_block); (yyval.decl_block)->setType((yyvsp[-1].type)); located((yyval.decl_block), (yyloc)); }
#line 2127 "parser.tab.cc"
    break;

  case 28: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER  */
#line 308 "scanner_parser/parser.yy"
                                                    { (yyval.decl_block)=(yyvsp[-2].decl_block); (yyval.decl_block)->push_back(string((yyvsp[0].string_val))); located((yyval.decl_block), (yyloc)); }
#line 2133 "parser.tab.cc"
    break;

  case 29: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']'  */
#line 309 "scanner_parser/parser.yy"
                                                                        { (yyval.decl_block)=(yyvsp[-5].decl_block); (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)),(yyvsp[-1].int_val)); located((yyval.decl_block), (yyloc)); }
#line 2139 "parser.tab.cc"
    break;

  case 30: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER  */
#line 310 "scanner_parser/parser.yy"
                 { (yyval.decl_block)=located(new AST_DATA_DECLARATION_BLOCK(), (yyloc)); (yyval.decl_block)->push_back(string((yyvsp[0].string_val))); }
#line 2145 "parser.tab.cc"
    break;

  case 31: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER '[' INT_LITERAL ']'  */
#line 311 "scanner_parser/parser.yy"
                                     { (yyval.decl_block)=located(new AST_DATA_DECLARATION_BLOCK(), (yyloc)); (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)),(yyvsp[-1].int_val)); }
#line 2151 "parser.tab.cc"
    break;

  case 32: /* DECLARATION_VARIABLE_LIST_RULE: %empty  */
#line 312 "scanner_parser/parser.yy"
             { (yyval.decl_block)=located(new AST_DATA_DECLARATION_BLOCK(), (yyloc)); }
#line 2157 "parser.tab.cc"
    break;

  case 33: /* LOGIC_BLOCK: TOP_LEVEL_STATEMENT_LIST_RULE  */
#line 321 "scanner_parser/parser.yy"
                                           { (yyval.code_block)=located(new AST_LOGIC_BLOCK((yyvsp[0].block_statement)), (yyloc)); }
#line 2163 "parser.tab.cc"
    break;

  case 34: /* TOP_LEVEL_STATEMENT_LIST_RULE: TOP_LEVEL_STATEMENT_LIST_RULE EXECUTABLE_STATEMENT_RULE  */
#line 323 "scanner_parser/parser.yy"
                                                              { (yyval.block_statement)=(yyvsp[-1].block_statement); (yyval.block_statement)->push_back((yyvsp[0].statement)); located((yyval.block_statement), (yyloc)); }
#line 2169 "parser.tab.cc"
    break;

  case 35: /* TOP_LEVEL_STATEMENT_LIST_RULE: EXECUTABLE_STATEMENT_RULE  */
#line 324 "scanner_parser/parser.yy"
                                { (yyval.block_statement)=located(new AST_STATEMENTS_BLOCK(), (yyloc)); (yyval.block_statement)->push_back((yyvsp[0].statement)); }
#line 2175 "parser.tab.cc"
    break;

  case 36: /* STATEMENT_BLOCK_RULE: '{' STATEMENT_LIST_RULE '}'  */
#line 325 "scanner_parser/parser.yy"
                                                  { (yyval.block_statement)=(yyvsp[-1].block_statement); (yyval.block_statement)->setLexicalScope(true); located((yyval.block_statement), (yyloc)); }
#line 2181 "parser.tab.cc"
    break;

  case 37: /* STATEMENT_LIST_RULE: STATEMENT_LIST_RULE STATEMENT_RULE  */
#line 327 "scanner_parser/parser.yy"
                                         { (yyval.block_statement)=(yyvsp[-1].block_statement); (yyval.block_statement)->push_back((yyvsp[0].statement)); located((yyval.block_statement), (yyloc)); }
#line 2187 "parser.tab.cc"
    break;

  case 38: /* STATEMENT_LIST_RULE: STATEMENT_RULE  */
#line 328 "scanner_parser/parser.yy"
                     { (yyval.block_statement)=located(new AST_STATEMENTS_BLOCK(), (yyloc)); (yyval.block_statement)->push_back((yyvsp[0].statement)); }
#line 2193 "parser.tab.cc"
    break;

  case 39: /* STATEMENT_RULE: EXECUTABLE_STATEMENT_RULE  */
#line 331 "scanner_parser/parser.yy"
                                { (yyval.statement)=(yyvsp[0].statement); }
#line 2199 "parser.tab.cc"
    break;

  case 40: /* STATEMENT_RULE: DECLARATION_STATEMENT_RULE ';'  */
#line 332 "scanner_parser/parser.yy"
                                     { (yyval.statement)=(yyvsp[-1].decl_block); located((yyval.statement), (yyloc)); }
#line 2205 "parser.tab.cc"
    break;

  case 41: /* EXECUTABLE_STATEMENT_RULE: MODEL_DECLARATION  */
#line 335 "scanner_parser/parser.yy"
                        { (yyval.statement)=(yyvsp[0].model_decl); }
#line 2211 "parser.tab.cc"
    break;

  case 42: /* EXECUTABLE_STATEMENT_RULE: TENSOR_DECLARATION  */
#line 336 "scanner_parser/parser.yy"
                         { (yyval.statement)=(yyvsp[0].tensor_decl); }
#line 2217 "parser.tab.cc"
    break;

  case 43: /* EXECUTABLE_STATEMENT_RULE: GREENAI_CONTRACT  */
#line 337 "scanner_parser/parser.yy"
                       { (yyval.statement)=(yyvsp[0].greenai_contract); }
#line 2223 "parser.tab.cc"
    break;

  case 44: /* EXECUTABLE_STATEMENT_RULE: GREENAI_MEASUREMENT  */
#line 338 "scanner_parser/parser.yy"
                          { (yyval.statement)=(yyvsp[0].greenai_measure); }
#line 2229 "parser.tab.cc"
    break;

  case 45: /* EXECUTABLE_STATEMENT_RULE: C3ECO_DECLARATION  */
#line 339 "scanner_parser/parser.yy"
                        { (yyval.statement)=(yyvsp[0].c3eco_decl); }
#line 2235 "parser.tab.cc"
    break;

  case 46: /* EXECUTABLE_STATEMENT_RULE: INFER_STATEMENT  */
#line 340 "scanner_parser/parser.yy"
                      { (yyval.statement)=(yyvsp[0].infer_statement); }
#line 2241 "parser.tab.cc"
    break;

  case 47: /* EXECUTABLE_STATEMENT_RULE: RETURN_STATEMENT  */
#line 341 "scanner_parser/parser.yy"
                       { (yyval.statement)=(yyvsp[0].return_statement); }
#line 2247 "parser.tab.cc"
    break;

  case 48: /* EXECUTABLE_STATEMENT_RULE: CONTINUE ';'  */
#line 342 "scanner_parser/parser.yy"
                   { (yyval.statement)=located(new AST_CONTINUE(), (yyloc)); }
#line 2253 "parser.tab.cc"
    break;

  case 49: /* EXECUTABLE_STATEMENT_RULE: EXPRESSION_RULE ';'  */
#line 343 "scanner_parser/parser.yy"
                          { (yyval.statement)=located(new AST_EXPRESSION_STATEMENT_RULE((yyvsp[-1].expression)), (yyloc)); }
#line 2259 "parser.tab.cc"
    break;

  case 50: /* EXECUTABLE_STATEMENT_RULE: VARIABLE_RULE '=' EXPRESSION_RULE ';'  */
#line 344 "scanner_parser/parser.yy"
                                            { (yyval.statement)=located(new AST_ASSIGNMENT_RULE((yyvsp[-3].variable),(yyvsp[-1].expression)), (yyloc)); }
#line 2265 "parser.tab.cc"
    break;

  case 51: /* EXECUTABLE_STATEMENT_RULE: STATEMENT_BLOCK_RULE  */
#line 345 "scanner_parser/parser.yy"
                           { (yyval.statement)=(yyvsp[0].block_statement); }
#line 2271 "parser.tab.cc"
    break;

  case 52: /* EXECUTABLE_STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 346 "scanner_parser/parser.yy"
                                              { (yyval.statement)=located(new AST_IF_STATEMENT((yyvsp[-1].expression),(yyvsp[0].block_statement)), (yyloc)); }
#line 2277 "parser.tab.cc"
    break;

  case 53: /* EXECUTABLE_STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE  */
#line 347 "scanner_parser/parser.yy"
                                                                        { (yyval.statement)=located(new AST_IF_ELSE_STATEMENT((yyvsp[-3].expression),(yyvsp[-2].block_statement),(yyvsp[0].block_statement)), (yyloc)); }
#line 2283 "parser.tab.cc"
    break;

  case 54: /* EXECUTABLE_STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 348 "scanner_parser/parser.yy"
                                                                                      { (yyval.statement)=located(new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-5].variable),(yyvsp[-3].expression),(yyvsp[-1].expression),(yyvsp[0].block_statement)), (yyloc)); }
#line 2289 "parser.tab.cc"
    break;

  case 55: /* EXECUTABLE_STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 349 "scanner_parser/parser.yy"
                                                                                                          { (yyval.statement)=located(new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-7].variable),(yyvsp[-5].expression),(yyvsp[-3].expression),(yyvsp[-1].expression),(yyvsp[0].block_statement)), (yyloc)); }
#line 2295 "parser.tab.cc"
    break;

  case 56: /* EXECUTABLE_STATEMENT_RULE: LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 350 "scanner_parser/parser.yy"
                                                { (yyval.statement)=located(new AST_WHILE_LOOP_STATEMENT_RULE((yyvsp[-1].expression),(yyvsp[0].block_statement)), (yyloc)); }
#line 2301 "parser.tab.cc"
    break;

  case 57: /* EXECUTABLE_STATEMENT_RULE: GOTO IDENTIFIER ';'  */
#line 351 "scanner_parser/parser.yy"
                          { (yyval.statement)=located(new AST_GOTO_STATEMENT_RULE(string((yyvsp[-1].string_val))), (yyloc)); }
#line 2307 "parser.tab.cc"
    break;

  case 58: /* EXECUTABLE_STATEMENT_RULE: GOTO IDENTIFIER IF EXPRESSION_RULE ';'  */
#line 352 "scanner_parser/parser.yy"
                                             { (yyval.statement)=located(new AST_GOTO_STATEMENT_RULE((yyvsp[-1].expression),string((yyvsp[-3].string_val))), (yyloc)); }
#line 2313 "parser.tab.cc"
    break;

  case 59: /* EXECUTABLE_STATEMENT_RULE: READ READ_VARIABLE_LIST_RULE ';'  */
#line 353 "scanner_parser/parser.yy"
                                       { (yyval.statement)=(yyvsp[-1].read_statement); located((yyval.statement), (yyloc)); }
#line 2319 "parser.tab.cc"
    break;

  case 60: /* EXECUTABLE_STATEMENT_RULE: BREAK ';'  */
#line 354 "scanner_parser/parser.yy"
                { (yyval.statement)=located(new AST_BREAK(), (yyloc)); }
#line 2325 "parser.tab.cc"
    break;

  case 61: /* EXECUTABLE_STATEMENT_RULE: PRINT PRINT_VARIABLE_LIST_RULE ';'  */
#line 355 "scanner_parser/parser.yy"
                                         { (yyval.statement)=(yyvsp[-1].print_statement); located((yyval.statement), (yyloc)); }
#line 2331 "parser.tab.cc"
    break;

  case 62: /* EXECUTABLE_STATEMENT_RULE: GREENAI_REPORT_RULE ';'  */
#line 356 "scanner_parser/parser.yy"
                              { (yyval.statement)=(yyvsp[-1].greenai_report); located((yyval.statement), (yyloc)); }
#line 2337 "parser.tab.cc"
    break;

  case 63: /* EXECUTABLE_STATEMENT_RULE: AI_INFER_RULE ';'  */
#line 357 "scanner_parser/parser.yy"
                        { (yyval.statement)=(yyvsp[-1].ai_infer); located((yyval.statement), (yyloc)); }
#line 2343 "parser.tab.cc"
    break;

  case 64: /* EXECUTABLE_STATEMENT_RULE: IDENTIFIER ':'  */
#line 358 "scanner_parser/parser.yy"
                     { (yyval.statement)=located(new AST_LABEL_RULE(string((yyvsp[-1].string_val))), (yyloc)); }
#line 2349 "parser.tab.cc"
    break;

  case 65: /* EXECUTABLE_STATEMENT_RULE: ';'  */
#line 359 "scanner_parser/parser.yy"
          { (yyval.statement)=located(new AST_EXPRESSION_STATEMENT_RULE(located(new AST_LITERAL(1), (yyloc))), (yyloc)); }
#line 2355 "parser.tab.cc"
    break;

  case 66: /* RETURN_STATEMENT: RETURN EXPRESSION_RULE ';'  */
#line 362 "scanner_parser/parser.yy"
                                 { (yyval.return_statement)=located(new AST_RETURN_STATEMENT((yyvsp[-1].expression)), (yyloc)); }
#line 2361 "parser.tab.cc"
    break;

  case 67: /* RETURN_STATEMENT: RETURN ';'  */
#line 363 "scanner_parser/parser.yy"
                 { (yyval.return_statement)=located(new AST_RETURN_STATEMENT(), (yyloc)); }
#line 2367 "parser.tab.cc"
    break;

  case 68: /* INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' ARROW IDENTIFIER ';'  */
#line 366 "scanner_parser/parser.yy"
                                                               { (yyval.infer_statement)=located(new AST_INFER_STATEMENT(string((yyvsp[-6].string_val)),string((yyvsp[-4].string_val)),string((yyvsp[-1].string_val))), (yyloc)); }
#line 2373 "parser.tab.cc"
    break;

  case 69: /* INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' GREATER IDENTIFIER ';'  */
#line 367 "scanner_parser/parser.yy"
                                                                 { (yyval.infer_statement)=located(new AST_INFER_STATEMENT(string((yyvsp[-6].string_val)),string((yyvsp[-4].string_val)),string((yyvsp[-1].string_val))), (yyloc)); }
#line 2379 "parser.tab.cc"
    break;

  case 70: /* TENSOR_DECLARATION: TENSOR IDENTIFIER PRECISION_NAME STRING_LITERAL ';'  */
#line 369 "scanner_parser/parser.yy"
                                                                        {
    TensorDeclarationData d; d.name=(yyvsp[-3].string_val); d.element_type=(yyvsp[-2].string_val); d.shape_csv=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); d.dynamic=(d.shape_csv=="dynamic"); d.rank=d.dynamic?0:1;
    long long total=1; if(!d.dynamic){ d.rank=0; size_t start=0; while(start<d.shape_csv.size()){ size_t pos=d.shape_csv.find(',',start); string part=d.shape_csv.substr(start,pos==string::npos?string::npos:pos-start); total*=atoll(part.c_str()); d.rank++; if(pos==string::npos) break; start=pos+1; }} d.total_elements=total;
    (yyval.tensor_decl)=located(new AST_TENSOR_DECLARATION(d), (yyloc));
}
#line 2389 "parser.tab.cc"
    break;

  case 71: /* $@1: %empty  */
#line 375 "scanner_parser/parser.yy"
                                        { current_model=ModelDeclarationData(); current_model.name=(yyvsp[-1].string_val); }
#line 2395 "parser.tab.cc"
    break;

  case 72: /* MODEL_DECLARATION: MODEL IDENTIFIER '{' $@1 MODEL_FIELD_LIST '}' ';'  */
#line 375 "scanner_parser/parser.yy"
                                                                                                                                  { (yyval.model_decl)=located(new AST_MODEL_DECLARATION(current_model), (yyloc)); }
#line 2401 "parser.tab.cc"
    break;

  case 75: /* MODEL_FIELD: FORMAT FORMAT_NAME ';'  */
#line 377 "scanner_parser/parser.yy"
                                    { current_model.format=(yyvsp[-1].string_val); }
#line 2407 "parser.tab.cc"
    break;

  case 76: /* MODEL_FIELD: PATH STRING_LITERAL ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                           { current_model.path=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2413 "parser.tab.cc"
    break;

  case 77: /* MODEL_FIELD: TASK STRING_LITERAL ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                      { current_model.task=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2419 "parser.tab.cc"
    break;

  case 78: /* MODEL_FIELD: PRECISION PRECISION_NAME ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                      { current_model.precision=(yyvsp[-1].string_val); }
#line 2425 "parser.tab.cc"
    break;

  case 79: /* MODEL_FIELD: INPUT_SHAPE STRING_LITERAL ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                       { current_model.input_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2431 "parser.tab.cc"
    break;

  case 80: /* MODEL_FIELD: OUTPUT_SHAPE STRING_LITERAL ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                 { current_model.output_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2437 "parser.tab.cc"
    break;

  case 82: /* MODEL_FIELD: COMPACT TRUE ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   { current_model.compact=true; }
#line 2443 "parser.tab.cc"
    break;

  case 83: /* MODEL_FIELD: COMPACT FALSE ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       { current_model.compact=false; }
#line 2449 "parser.tab.cc"
    break;

  case 84: /* MODEL_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 377 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_model.has_quality_guardrail=true; current_model.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 2455 "parser.tab.cc"
    break;

  case 85: /* BACKEND_LIST: BACKEND_LIST ',' BACKEND_NAME  */
#line 378 "scanner_parser/parser.yy"
                                            { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 2461 "parser.tab.cc"
    break;

  case 86: /* BACKEND_LIST: BACKEND_NAME  */
#line 378 "scanner_parser/parser.yy"
                                                                                                               { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 2467 "parser.tab.cc"
    break;

  case 87: /* $@2: %empty  */
#line 380 "scanner_parser/parser.yy"
                                                    { current_contract=GreenAIContractData(); current_contract.name=(yyvsp[-1].string_val); }
#line 2473 "parser.tab.cc"
    break;

  case 88: /* GREENAI_CONTRACT: GREENAI_CONTRACT_T IDENTIFIER '{' $@2 CONTRACT_FIELD_LIST '}' ';'  */
#line 380 "scanner_parser/parser.yy"
                                                                                                                                                      { (yyval.greenai_contract)=located(new AST_GREENAI_CONTRACT(current_contract), (yyloc)); }
#line 2479 "parser.tab.cc"
    break;

  case 91: /* CONTRACT_FIELD: FUNCTIONAL_UNIT STRING_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                   { current_contract.functional_unit=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_functional_unit=true; }
#line 2485 "parser.tab.cc"
    break;

  case 92: /* CONTRACT_FIELD: SUCCESS_CRITERIA STRING_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                   { current_contract.success_criteria=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_success_criteria=true; }
#line 2491 "parser.tab.cc"
    break;

  case 93: /* CONTRACT_FIELD: BOUNDARY BOUNDARY_LIST ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                            { current_contract.has_boundary=true; }
#line 2497 "parser.tab.cc"
    break;

  case 94: /* CONTRACT_FIELD: MEASUREMENT_QUALITY MQ_NAME ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                      { current_contract.measurement_quality=(yyvsp[-1].string_val); current_contract.has_mq=true; }
#line 2503 "parser.tab.cc"
    break;

  case 95: /* CONTRACT_FIELD: DATA_QUALITY DQ_NAME ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.data_quality=(yyvsp[-1].string_val); current_contract.has_dq=true; }
#line 2509 "parser.tab.cc"
    break;

  case 96: /* CONTRACT_FIELD: CARBON_FACTOR LOCATION INT_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         { current_contract.carbon_factor_scope="location"; current_contract.carbon_factor=(yyvsp[-1].int_val); current_contract.has_carbon_factor=true; }
#line 2515 "parser.tab.cc"
    break;

  case 97: /* CONTRACT_FIELD: ENERGY_BUDGET_J INT_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.energy_budget_j=(yyvsp[-1].int_val); }
#line 2521 "parser.tab.cc"
    break;

  case 98: /* CONTRACT_FIELD: CARBON_BUDGET_GCO2E INT_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           { current_contract.carbon_budget_gco2e=(yyvsp[-1].int_val); }
#line 2527 "parser.tab.cc"
    break;

  case 99: /* CONTRACT_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_contract.has_quality_guardrail=true; current_contract.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 2533 "parser.tab.cc"
    break;

  case 100: /* CONTRACT_FIELD: EVIDENCE_RETENTION STRING_LITERAL ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  { current_contract.evidence_retention=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2539 "parser.tab.cc"
    break;

  case 101: /* CONTRACT_FIELD: CLAIMS_MODE EVIDENCE_ONLY ';'  */
#line 382 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    { current_contract.claims_mode="evidence_only"; }
#line 2545 "parser.tab.cc"
    break;

  case 102: /* BOUNDARY_LIST: BOUNDARY_LIST ',' BOUNDARY_NAME  */
#line 383 "scanner_parser/parser.yy"
                                               { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2551 "parser.tab.cc"
    break;

  case 103: /* BOUNDARY_LIST: BOUNDARY_NAME  */
#line 383 "scanner_parser/parser.yy"
                                                                                                            { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2557 "parser.tab.cc"
    break;

  case 104: /* $@3: %empty  */
#line 385 "scanner_parser/parser.yy"
                                                    { current_measure=GreenAIMeasurementData(); current_measure.workload=(yyvsp[-1].string_val); }
#line 2563 "parser.tab.cc"
    break;

  case 105: /* GREENAI_MEASUREMENT: GREENAI_MEASURE IDENTIFIER '{' $@3 MEASURE_FIELD_LIST '}' ';'  */
#line 385 "scanner_parser/parser.yy"
                                                                                                                                                          { (yyval.greenai_measure)=located(new AST_GREENAI_MEASUREMENT(current_measure), (yyloc)); }
#line 2569 "parser.tab.cc"
    break;

  case 108: /* MEASURE_FIELD: IDENTIFIER INT_LITERAL ';'  */
#line 387 "scanner_parser/parser.yy"
                                          { string n=(yyvsp[-2].string_val); if(n=="inferences") current_measure.inferences=(yyvsp[-1].int_val); else if(n=="watts") current_measure.watts=(yyvsp[-1].int_val); else if(n=="seconds") current_measure.seconds=(yyvsp[-1].int_val); }
#line 2575 "parser.tab.cc"
    break;

  case 109: /* MEASURE_FIELD: IDENTIFIER IDENTIFIER ';'  */
#line 387 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                          { if(string((yyvsp[-2].string_val))=="backend") current_measure.backend=(yyvsp[-1].string_val); }
#line 2581 "parser.tab.cc"
    break;

  case 110: /* $@4: %empty  */
#line 390 "scanner_parser/parser.yy"
                                           { shorthand_begin_c3eco(C3EcoDeclarationKind::CertificationProfile, (yyvsp[-1].string_val)); }
#line 2587 "parser.tab.cc"
    break;

  case 111: /* C3ECO_DECLARATION: CERTIFICATION_PROFILE IDENTIFIER '{' $@4 C3ECO_FIELD_LIST '}' ';'  */
#line 390 "scanner_parser/parser.yy"
                                                                                                                                               { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2593 "parser.tab.cc"
    break;

  case 112: /* $@5: %empty  */
#line 391 "scanner_parser/parser.yy"
                                   { shorthand_begin_c3eco(C3EcoDeclarationKind::Certification, (yyvsp[-1].string_val)); }
#line 2599 "parser.tab.cc"
    break;

  case 113: /* C3ECO_DECLARATION: CERTIFICATION IDENTIFIER '{' $@5 C3ECO_FIELD_LIST '}' ';'  */
#line 391 "scanner_parser/parser.yy"
                                                                                                                                { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2605 "parser.tab.cc"
    break;

  case 114: /* $@6: %empty  */
#line 392 "scanner_parser/parser.yy"
                                     { shorthand_begin_c3eco(C3EcoDeclarationKind::FunctionalUnit, (yyvsp[-1].string_val)); }
#line 2611 "parser.tab.cc"
    break;

  case 115: /* C3ECO_DECLARATION: FUNCTIONAL_UNIT IDENTIFIER '{' $@6 C3ECO_FIELD_LIST '}' ';'  */
#line 392 "scanner_parser/parser.yy"
                                                                                                                                   { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2617 "parser.tab.cc"
    break;

  case 116: /* $@7: %empty  */
#line 393 "scanner_parser/parser.yy"
                              { shorthand_begin_c3eco(C3EcoDeclarationKind::Workload, (yyvsp[-1].string_val)); }
#line 2623 "parser.tab.cc"
    break;

  case 117: /* C3ECO_DECLARATION: WORKLOAD IDENTIFIER '{' $@7 C3ECO_FIELD_LIST '}' ';'  */
#line 393 "scanner_parser/parser.yy"
                                                                                                                      { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2629 "parser.tab.cc"
    break;

  case 118: /* $@8: %empty  */
#line 394 "scanner_parser/parser.yy"
                              { shorthand_begin_c3eco(C3EcoDeclarationKind::Boundary, (yyvsp[-1].string_val)); }
#line 2635 "parser.tab.cc"
    break;

  case 119: /* C3ECO_DECLARATION: BOUNDARY IDENTIFIER '{' $@8 C3ECO_FIELD_LIST '}' ';'  */
#line 394 "scanner_parser/parser.yy"
                                                                                                                      { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2641 "parser.tab.cc"
    break;

  case 120: /* $@9: %empty  */
#line 395 "scanner_parser/parser.yy"
                                      { shorthand_begin_c3eco(C3EcoDeclarationKind::MeasurementPlan, (yyvsp[-1].string_val)); }
#line 2647 "parser.tab.cc"
    break;

  case 121: /* C3ECO_DECLARATION: MEASUREMENT_PLAN IDENTIFIER '{' $@9 C3ECO_FIELD_LIST '}' ';'  */
#line 395 "scanner_parser/parser.yy"
                                                                                                                                     { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2653 "parser.tab.cc"
    break;

  case 122: /* $@10: %empty  */
#line 396 "scanner_parser/parser.yy"
                                  { shorthand_begin_c3eco(C3EcoDeclarationKind::AILifecycle, (yyvsp[-1].string_val)); }
#line 2659 "parser.tab.cc"
    break;

  case 123: /* C3ECO_DECLARATION: AI_LIFECYCLE IDENTIFIER '{' $@10 C3ECO_FIELD_LIST '}' ';'  */
#line 396 "scanner_parser/parser.yy"
                                                                                                                             { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2665 "parser.tab.cc"
    break;

  case 124: /* $@11: %empty  */
#line 397 "scanner_parser/parser.yy"
                                  { shorthand_begin_c3eco(C3EcoDeclarationKind::RAGPipeline, (yyvsp[-1].string_val)); }
#line 2671 "parser.tab.cc"
    break;

  case 125: /* C3ECO_DECLARATION: RAG_PIPELINE IDENTIFIER '{' $@11 C3ECO_FIELD_LIST '}' ';'  */
#line 397 "scanner_parser/parser.yy"
                                                                                                                             { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2677 "parser.tab.cc"
    break;

  case 126: /* $@12: %empty  */
#line 398 "scanner_parser/parser.yy"
                                  { shorthand_begin_c3eco(C3EcoDeclarationKind::TokenBudget, (yyvsp[-1].string_val)); }
#line 2683 "parser.tab.cc"
    break;

  case 127: /* C3ECO_DECLARATION: TOKEN_BUDGET IDENTIFIER '{' $@12 C3ECO_FIELD_LIST '}' ';'  */
#line 398 "scanner_parser/parser.yy"
                                                                                                                             { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2689 "parser.tab.cc"
    break;

  case 128: /* $@13: %empty  */
#line 399 "scanner_parser/parser.yy"
                                   { shorthand_begin_c3eco(C3EcoDeclarationKind::ModelRouting, (yyvsp[-1].string_val)); }
#line 2695 "parser.tab.cc"
    break;

  case 129: /* C3ECO_DECLARATION: MODEL_ROUTING IDENTIFIER '{' $@13 C3ECO_FIELD_LIST '}' ';'  */
#line 399 "scanner_parser/parser.yy"
                                                                                                                               { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2701 "parser.tab.cc"
    break;

  case 130: /* $@14: %empty  */
#line 400 "scanner_parser/parser.yy"
                                { shorthand_begin_c3eco(C3EcoDeclarationKind::Guardrails, (yyvsp[-1].string_val)); }
#line 2707 "parser.tab.cc"
    break;

  case 131: /* C3ECO_DECLARATION: GUARDRAILS IDENTIFIER '{' $@14 C3ECO_FIELD_LIST '}' ';'  */
#line 400 "scanner_parser/parser.yy"
                                                                                                                          { (yyval.c3eco_decl)=located(new AST_C3ECO_DECLARATION(current_c3eco), (yyloc)); }
#line 2713 "parser.tab.cc"
    break;

  case 134: /* C3ECO_FIELD: C3ECO_FIELD_NAME STRING_LITERAL ';'  */
#line 403 "scanner_parser/parser.yy"
                                          { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::String, shorthand_unquote((yyvsp[-1].string_val))); }
#line 2719 "parser.tab.cc"
    break;

  case 135: /* C3ECO_FIELD: C3ECO_FIELD_NAME C3ECO_IDENTIFIER_VALUE ';'  */
#line 404 "scanner_parser/parser.yy"
                                                  { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::Identifier, (yyvsp[-1].string_val) == nullptr ? "" : std::string((yyvsp[-1].string_val))); }
#line 2725 "parser.tab.cc"
    break;

  case 136: /* C3ECO_FIELD: C3ECO_FIELD_NAME INT_LITERAL ';'  */
#line 405 "scanner_parser/parser.yy"
                                       { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::Integer, std::to_string((yyvsp[-1].int_val))); }
#line 2731 "parser.tab.cc"
    break;

  case 137: /* C3ECO_FIELD: C3ECO_FIELD_NAME FLOAT_LITERAL ';'  */
#line 406 "scanner_parser/parser.yy"
                                         { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::Decimal, shorthand_decimal_text((yyvsp[-1].float_val))); }
#line 2737 "parser.tab.cc"
    break;

  case 138: /* C3ECO_FIELD: C3ECO_FIELD_NAME TRUE ';'  */
#line 407 "scanner_parser/parser.yy"
                                { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::Boolean, "true"); }
#line 2743 "parser.tab.cc"
    break;

  case 139: /* C3ECO_FIELD: C3ECO_FIELD_NAME FALSE ';'  */
#line 408 "scanner_parser/parser.yy"
                                 { shorthand_add_c3eco_field((yyvsp[-2].string_val), C3EcoValueKind::Boolean, "false"); }
#line 2749 "parser.tab.cc"
    break;

  case 140: /* C3ECO_FIELD_NAME: IDENTIFIER  */
#line 410 "scanner_parser/parser.yy"
                 { (yyval.string_val)=(yyvsp[0].string_val); }
#line 2755 "parser.tab.cc"
    break;

  case 141: /* C3ECO_FIELD_NAME: CARBON_FACTOR  */
#line 411 "scanner_parser/parser.yy"
                    { (yyval.string_val)=(char*)"carbon_factor"; }
#line 2761 "parser.tab.cc"
    break;

  case 142: /* C3ECO_FIELD_NAME: FALLBACK  */
#line 412 "scanner_parser/parser.yy"
               { (yyval.string_val)=(char*)"fallback"; }
#line 2767 "parser.tab.cc"
    break;

  case 143: /* C3ECO_FIELD_NAME: QUALITY_GUARDRAIL  */
#line 413 "scanner_parser/parser.yy"
                        { (yyval.string_val)=(char*)"quality_guardrail"; }
#line 2773 "parser.tab.cc"
    break;

  case 144: /* C3ECO_FIELD_NAME: FUNCTIONAL_UNIT  */
#line 414 "scanner_parser/parser.yy"
                      { (yyval.string_val)=(char*)"functional_unit"; }
#line 2779 "parser.tab.cc"
    break;

  case 145: /* C3ECO_FIELD_NAME: BOUNDARY  */
#line 415 "scanner_parser/parser.yy"
               { (yyval.string_val)=(char*)"boundary"; }
#line 2785 "parser.tab.cc"
    break;

  case 146: /* C3ECO_IDENTIFIER_VALUE: IDENTIFIER  */
#line 418 "scanner_parser/parser.yy"
                 { (yyval.string_val)=(yyvsp[0].string_val); }
#line 2791 "parser.tab.cc"
    break;

  case 147: /* C3ECO_IDENTIFIER_VALUE: BOUNDARY_NAME  */
#line 419 "scanner_parser/parser.yy"
                    { (yyval.string_val)=(yyvsp[0].string_val); }
#line 2797 "parser.tab.cc"
    break;

  case 148: /* C3ECO_IDENTIFIER_VALUE: MQ_NAME  */
#line 420 "scanner_parser/parser.yy"
              { (yyval.string_val)=(yyvsp[0].string_val); }
#line 2803 "parser.tab.cc"
    break;

  case 149: /* C3ECO_IDENTIFIER_VALUE: DQ_NAME  */
#line 421 "scanner_parser/parser.yy"
              { (yyval.string_val)=(yyvsp[0].string_val); }
#line 2809 "parser.tab.cc"
    break;

  case 150: /* C3ECO_IDENTIFIER_VALUE: LOCATION  */
#line 422 "scanner_parser/parser.yy"
               { (yyval.string_val)=(char*)"location"; }
#line 2815 "parser.tab.cc"
    break;

  case 151: /* C3ECO_IDENTIFIER_VALUE: EVIDENCE_ONLY  */
#line 423 "scanner_parser/parser.yy"
                    { (yyval.string_val)=(char*)"evidence_only"; }
#line 2821 "parser.tab.cc"
    break;

  case 152: /* C3ECO_IDENTIFIER_VALUE: FALLBACK  */
#line 424 "scanner_parser/parser.yy"
               { (yyval.string_val)=(char*)"fallback"; }
#line 2827 "parser.tab.cc"
    break;

  case 153: /* FORMAT_NAME: ONNX  */
#line 426 "scanner_parser/parser.yy"
                  {(yyval.string_val)=(char*)"onnx";}
#line 2833 "parser.tab.cc"
    break;

  case 154: /* FORMAT_NAME: ENGINE  */
#line 426 "scanner_parser/parser.yy"
                                               {(yyval.string_val)=(char*)"engine";}
#line 2839 "parser.tab.cc"
    break;

  case 155: /* FORMAT_NAME: TORCHSCRIPT  */
#line 426 "scanner_parser/parser.yy"
                                                                                   {(yyval.string_val)=(char*)"torchscript";}
#line 2845 "parser.tab.cc"
    break;

  case 156: /* FORMAT_NAME: OPENVINO_IR  */
#line 426 "scanner_parser/parser.yy"
                                                                                                                            {(yyval.string_val)=(char*)"openvino_ir";}
#line 2851 "parser.tab.cc"
    break;

  case 157: /* FORMAT_NAME: GGUF  */
#line 426 "scanner_parser/parser.yy"
                                                                                                                                                              {(yyval.string_val)=(char*)"gguf";}
#line 2857 "parser.tab.cc"
    break;

  case 158: /* PRECISION_NAME: INT8  */
#line 427 "scanner_parser/parser.yy"
                     {(yyval.string_val)=(char*)"int8";}
#line 2863 "parser.tab.cc"
    break;

  case 159: /* PRECISION_NAME: INT4  */
#line 427 "scanner_parser/parser.yy"
                                                {(yyval.string_val)=(char*)"int4";}
#line 2869 "parser.tab.cc"
    break;

  case 160: /* PRECISION_NAME: FP16  */
#line 427 "scanner_parser/parser.yy"
                                                                           {(yyval.string_val)=(char*)"fp16";}
#line 2875 "parser.tab.cc"
    break;

  case 161: /* PRECISION_NAME: FP32  */
#line 427 "scanner_parser/parser.yy"
                                                                                                      {(yyval.string_val)=(char*)"fp32";}
#line 2881 "parser.tab.cc"
    break;

  case 162: /* PRECISION_NAME: BF16  */
#line 427 "scanner_parser/parser.yy"
                                                                                                                                 {(yyval.string_val)=(char*)"bf16";}
#line 2887 "parser.tab.cc"
    break;

  case 163: /* PRECISION_NAME: FP64  */
#line 427 "scanner_parser/parser.yy"
                                                                                                                                                            {(yyval.string_val)=(char*)"fp64";}
#line 2893 "parser.tab.cc"
    break;

  case 164: /* PRECISION_NAME: FLOAT  */
#line 427 "scanner_parser/parser.yy"
                                                                                                                                                                                        {(yyval.string_val)=(char*)"float";}
#line 2899 "parser.tab.cc"
    break;

  case 165: /* BACKEND_NAME: TENSORRT  */
#line 428 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"tensorrt";}
#line 2905 "parser.tab.cc"
    break;

  case 166: /* BACKEND_NAME: ONNXRUNTIME_TENSORRT  */
#line 428 "scanner_parser/parser.yy"
                                                                      {(yyval.string_val)=(char*)"onnxruntime_tensorrt";}
#line 2911 "parser.tab.cc"
    break;

  case 167: /* BACKEND_NAME: ONNXRUNTIME_CUDA  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                             {(yyval.string_val)=(char*)"onnxruntime_cuda";}
#line 2917 "parser.tab.cc"
    break;

  case 168: /* BACKEND_NAME: ONNXRUNTIME_CPU  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                                                                               {(yyval.string_val)=(char*)"onnxruntime_cpu";}
#line 2923 "parser.tab.cc"
    break;

  case 169: /* BACKEND_NAME: OPENVINO  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                                                                                                                         {(yyval.string_val)=(char*)"openvino";}
#line 2929 "parser.tab.cc"
    break;

  case 170: /* BACKEND_NAME: LIBTORCH  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                            {(yyval.string_val)=(char*)"libtorch";}
#line 2935 "parser.tab.cc"
    break;

  case 171: /* BACKEND_NAME: LLAMACPP  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                               {(yyval.string_val)=(char*)"llamacpp";}
#line 2941 "parser.tab.cc"
    break;

  case 172: /* BACKEND_NAME: FALLBACK  */
#line 428 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                  {(yyval.string_val)=(char*)"fallback";}
#line 2947 "parser.tab.cc"
    break;

  case 173: /* MQ_NAME: MQ1  */
#line 429 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"MQ1";}
#line 2953 "parser.tab.cc"
    break;

  case 174: /* MQ_NAME: MQ2  */
#line 429 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"MQ2";}
#line 2959 "parser.tab.cc"
    break;

  case 175: /* MQ_NAME: MQ3  */
#line 429 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"MQ3";}
#line 2965 "parser.tab.cc"
    break;

  case 176: /* MQ_NAME: MQ4  */
#line 429 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"MQ4";}
#line 2971 "parser.tab.cc"
    break;

  case 177: /* DQ_NAME: DQ1  */
#line 430 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"DQ1";}
#line 2977 "parser.tab.cc"
    break;

  case 178: /* DQ_NAME: DQ2  */
#line 430 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"DQ2";}
#line 2983 "parser.tab.cc"
    break;

  case 179: /* DQ_NAME: DQ3  */
#line 430 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"DQ3";}
#line 2989 "parser.tab.cc"
    break;

  case 180: /* DQ_NAME: DQ4  */
#line 430 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"DQ4";}
#line 2995 "parser.tab.cc"
    break;

  case 181: /* BOUNDARY_NAME: COMPUTE  */
#line 431 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"compute";}
#line 3001 "parser.tab.cc"
    break;

  case 182: /* BOUNDARY_NAME: ACCELERATOR  */
#line 431 "scanner_parser/parser.yy"
                                                            {(yyval.string_val)=(char*)"accelerator";}
#line 3007 "parser.tab.cc"
    break;

  case 183: /* BOUNDARY_NAME: STORAGE  */
#line 431 "scanner_parser/parser.yy"
                                                                                                 {(yyval.string_val)=(char*)"storage";}
#line 3013 "parser.tab.cc"
    break;

  case 184: /* BOUNDARY_NAME: NETWORK  */
#line 431 "scanner_parser/parser.yy"
                                                                                                                                  {(yyval.string_val)=(char*)"network";}
#line 3019 "parser.tab.cc"
    break;

  case 185: /* BOUNDARY_NAME: CI_CD  */
#line 431 "scanner_parser/parser.yy"
                                                                                                                                                                 {(yyval.string_val)=(char*)"ci_cd";}
#line 3025 "parser.tab.cc"
    break;

  case 186: /* BOUNDARY_NAME: THIRDPARTY  */
#line 431 "scanner_parser/parser.yy"
                                                                                                                                                                                                   {(yyval.string_val)=(char*)"thirdparty";}
#line 3031 "parser.tab.cc"
    break;

  case 187: /* AI_INFER_RULE: AI_INFER_BUILTIN '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')'  */
#line 433 "scanner_parser/parser.yy"
                                                                                             {
    if ((string((yyvsp[-7].string_val))!="ai_infer" && string((yyvsp[-7].string_val))!="aiinfer")) {
        shorthand_parser_diagnostic(shorthand::diagnostics::ParserExpectedAIInferBuiltin,
                                    "expected ai_infer builtin", (yyloc));
        YYERROR;
    }
    (yyval.ai_infer)=located(new AST_AI_INFER_RULE(string((yyvsp[-5].string_val)),string((yyvsp[-3].string_val)),string((yyvsp[-1].string_val))), (yyloc));
}
#line 3044 "parser.tab.cc"
    break;

  case 188: /* GREENAI_REPORT_RULE: GREENAI_REPORT_BUILTIN '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')'  */
#line 442 "scanner_parser/parser.yy"
                                                                                                                               {
    if (string((yyvsp[-9].string_val))!="greenai") {
        shorthand_parser_diagnostic(shorthand::diagnostics::ParserExpectedGreenAIReportBuiltin,
                                    "expected greenai report builtin", (yyloc));
        YYERROR;
    }
    (yyval.greenai_report)=located(new AST_GREENAI_REPORT_RULE(string((yyvsp[-7].string_val)),(yyvsp[-5].expression),(yyvsp[-3].expression),(yyvsp[-1].expression)), (yyloc));
}
#line 3057 "parser.tab.cc"
    break;

  case 189: /* EXPRESSION_RULE: EXPRESSION_RULE '+' EXPRESSION_RULE  */
#line 452 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"+"), (yyloc)); }
#line 3063 "parser.tab.cc"
    break;

  case 190: /* EXPRESSION_RULE: EXPRESSION_RULE '-' EXPRESSION_RULE  */
#line 453 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"-"), (yyloc)); }
#line 3069 "parser.tab.cc"
    break;

  case 191: /* EXPRESSION_RULE: EXPRESSION_RULE '*' EXPRESSION_RULE  */
#line 454 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"*"), (yyloc)); }
#line 3075 "parser.tab.cc"
    break;

  case 192: /* EXPRESSION_RULE: EXPRESSION_RULE '/' EXPRESSION_RULE  */
#line 455 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"/"), (yyloc)); }
#line 3081 "parser.tab.cc"
    break;

  case 193: /* EXPRESSION_RULE: EXPRESSION_RULE '%' EXPRESSION_RULE  */
#line 456 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"%"), (yyloc)); }
#line 3087 "parser.tab.cc"
    break;

  case 194: /* EXPRESSION_RULE: EXPRESSION_RULE LESS EXPRESSION_RULE  */
#line 457 "scanner_parser/parser.yy"
                                           { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"<"), (yyloc)); }
#line 3093 "parser.tab.cc"
    break;

  case 195: /* EXPRESSION_RULE: EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE  */
#line 458 "scanner_parser/parser.yy"
                                                    { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"<="), (yyloc)); }
#line 3099 "parser.tab.cc"
    break;

  case 196: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER EXPRESSION_RULE  */
#line 459 "scanner_parser/parser.yy"
                                              { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),">"), (yyloc)); }
#line 3105 "parser.tab.cc"
    break;

  case 197: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE  */
#line 460 "scanner_parser/parser.yy"
                                                       { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),">="), (yyloc)); }
#line 3111 "parser.tab.cc"
    break;

  case 198: /* EXPRESSION_RULE: EXPRESSION_RULE EQUAL EXPRESSION_RULE  */
#line 461 "scanner_parser/parser.yy"
                                            { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"=="), (yyloc)); }
#line 3117 "parser.tab.cc"
    break;

  case 199: /* EXPRESSION_RULE: EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE  */
#line 462 "scanner_parser/parser.yy"
                                                { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"!="), (yyloc)); }
#line 3123 "parser.tab.cc"
    break;

  case 200: /* EXPRESSION_RULE: EXPRESSION_RULE OR EXPRESSION_RULE  */
#line 463 "scanner_parser/parser.yy"
                                         { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"||"), (yyloc)); }
#line 3129 "parser.tab.cc"
    break;

  case 201: /* EXPRESSION_RULE: EXPRESSION_RULE AND EXPRESSION_RULE  */
#line 464 "scanner_parser/parser.yy"
                                          { (yyval.expression)=located(new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression),(yyvsp[0].expression),"&&"), (yyloc)); }
#line 3135 "parser.tab.cc"
    break;

  case 202: /* EXPRESSION_RULE: '-' EXPRESSION_RULE  */
#line 465 "scanner_parser/parser.yy"
                                       { (yyval.expression)=located(new AST_UNARY_EXPRESSION_RULE((yyvsp[0].expression),"-"), (yyloc)); }
#line 3141 "parser.tab.cc"
    break;

  case 203: /* EXPRESSION_RULE: '(' EXPRESSION_RULE ')'  */
#line 466 "scanner_parser/parser.yy"
                              { (yyval.expression)=(yyvsp[-1].expression); located((yyval.expression), (yyloc)); }
#line 3147 "parser.tab.cc"
    break;

  case 204: /* EXPRESSION_RULE: IDENTIFIER '(' EXPRESSION_LIST_OPT ')'  */
#line 467 "scanner_parser/parser.yy"
                                             { (yyval.expression)=located(new AST_FUNCTION_CALL_EXPRESSION(string((yyvsp[-3].string_val)),*(yyvsp[-1].expression_list)), (yyloc)); }
#line 3153 "parser.tab.cc"
    break;

  case 205: /* EXPRESSION_RULE: VARIABLE_RULE  */
#line 468 "scanner_parser/parser.yy"
                    { (yyval.expression)=(yyvsp[0].variable); }
#line 3159 "parser.tab.cc"
    break;

  case 206: /* EXPRESSION_RULE: INT_LITERAL  */
#line 469 "scanner_parser/parser.yy"
                  { (yyval.expression)=located(new AST_LITERAL((yyvsp[0].int_val)), (yyloc)); }
#line 3165 "parser.tab.cc"
    break;

  case 207: /* EXPRESSION_RULE: FLOAT_LITERAL  */
#line 470 "scanner_parser/parser.yy"
                    { (yyval.expression)=located(new AST_FLOAT_LITERAL((yyvsp[0].float_val)), (yyloc)); }
#line 3171 "parser.tab.cc"
    break;

  case 208: /* EXPRESSION_RULE: STRING_LITERAL  */
#line 471 "scanner_parser/parser.yy"
                     { (yyval.expression)=located(new AST_STRING_LITERAL(string((yyvsp[0].string_val))), (yyloc)); }
#line 3177 "parser.tab.cc"
    break;

  case 209: /* EXPRESSION_RULE: TRUE  */
#line 472 "scanner_parser/parser.yy"
           { (yyval.expression)=located(new AST_BOOL_LITERAL(true), (yyloc)); }
#line 3183 "parser.tab.cc"
    break;

  case 210: /* EXPRESSION_RULE: FALSE  */
#line 473 "scanner_parser/parser.yy"
            { (yyval.expression)=located(new AST_BOOL_LITERAL(false), (yyloc)); }
#line 3189 "parser.tab.cc"
    break;

  case 211: /* EXPRESSION_LIST_OPT: EXPRESSION_LIST_RULE  */
#line 476 "scanner_parser/parser.yy"
                           { (yyval.expression_list)=(yyvsp[0].expression_list); }
#line 3195 "parser.tab.cc"
    break;

  case 212: /* EXPRESSION_LIST_OPT: %empty  */
#line 477 "scanner_parser/parser.yy"
             { (yyval.expression_list)=shorthand_track_parser_node(new vector<AST_EXPRESSION_RULE*>()); }
#line 3201 "parser.tab.cc"
    break;

  case 213: /* EXPRESSION_LIST_RULE: EXPRESSION_LIST_RULE ',' EXPRESSION_RULE  */
#line 480 "scanner_parser/parser.yy"
                                               { (yyval.expression_list)=(yyvsp[-2].expression_list); (yyval.expression_list)->push_back((yyvsp[0].expression)); }
#line 3207 "parser.tab.cc"
    break;

  case 214: /* EXPRESSION_LIST_RULE: EXPRESSION_RULE  */
#line 481 "scanner_parser/parser.yy"
                      { (yyval.expression_list)=shorthand_track_parser_node(new vector<AST_EXPRESSION_RULE*>()); (yyval.expression_list)->push_back((yyvsp[0].expression)); }
#line 3213 "parser.tab.cc"
    break;

  case 215: /* VARIABLE_RULE: IDENTIFIER  */
#line 484 "scanner_parser/parser.yy"
                 { (yyval.variable)=located(new AST_SIMPLE_VARIABLE(string((yyvsp[0].string_val))), (yyloc)); }
#line 3219 "parser.tab.cc"
    break;

  case 216: /* VARIABLE_RULE: IDENTIFIER '[' EXPRESSION_RULE ']'  */
#line 485 "scanner_parser/parser.yy"
                                         { (yyval.variable)=located(new AST_ARRAY_VARIABLE(string((yyvsp[-3].string_val)),(yyvsp[-1].expression)), (yyloc)); }
#line 3225 "parser.tab.cc"
    break;

  case 217: /* READ_VARIABLE_LIST_RULE: READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE  */
#line 488 "scanner_parser/parser.yy"
                                                { (yyval.read_statement)=(yyvsp[-2].read_statement); (yyval.read_statement)->push_back((yyvsp[0].variable)); located((yyval.read_statement), (yyloc)); }
#line 3231 "parser.tab.cc"
    break;

  case 218: /* READ_VARIABLE_LIST_RULE: VARIABLE_RULE  */
#line 489 "scanner_parser/parser.yy"
                    { (yyval.read_statement)=located(new AST_READ_RULE(), (yyloc)); (yyval.read_statement)->push_back((yyvsp[0].variable)); }
#line 3237 "parser.tab.cc"
    break;

  case 219: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE  */
#line 492 "scanner_parser/parser.yy"
                                                   { (yyval.print_statement)=(yyvsp[-2].print_statement); (yyval.print_statement)->push_back((yyvsp[0].expression)); located((yyval.print_statement), (yyloc)); }
#line 3243 "parser.tab.cc"
    break;

  case 220: /* PRINT_VARIABLE_LIST_RULE: EXPRESSION_RULE  */
#line 493 "scanner_parser/parser.yy"
                      { (yyval.print_statement)=located(new AST_PRINT_RULE(), (yyloc)); (yyval.print_statement)->push_back((yyvsp[0].expression)); }
#line 3249 "parser.tab.cc"
    break;


#line 3253 "parser.tab.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 495 "scanner_parser/parser.yy"


void yyerror(char const *s) {
    shorthand_parser_diagnostic(shorthand::diagnostics::ParserSyntaxError, s, yylloc);
}
