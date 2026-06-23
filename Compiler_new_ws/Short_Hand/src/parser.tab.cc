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
#include <vector>
#include <string>
static ModelDeclarationData current_model;
static GreenAIContractData current_contract;
static GreenAIMeasurementData current_measure;

  using namespace std;

  extern "C" int yylex();
  extern "C" int yyparse();
  extern "C" void yyerror(char const *s);
  extern "C" int yywrap(void){return 1;}
  extern "C" int yylineno;
  extern "C" int yydebug;
  extern union _NODE_ yylval;
  extern class AST_PROGRAM * main_program;



#line 97 "parser.tab.cc"

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
  YYSYMBOL_13_ = 13,                       /* '+'  */
  YYSYMBOL_14_ = 14,                       /* '-'  */
  YYSYMBOL_15_ = 15,                       /* '*'  */
  YYSYMBOL_16_ = 16,                       /* '/'  */
  YYSYMBOL_17_ = 17,                       /* '%'  */
  YYSYMBOL_UMINUS = 18,                    /* UMINUS  */
  YYSYMBOL_STRING_LITERAL = 19,            /* STRING_LITERAL  */
  YYSYMBOL_IDENTIFIER = 20,                /* IDENTIFIER  */
  YYSYMBOL_INT_LITERAL = 21,               /* INT_LITERAL  */
  YYSYMBOL_FLOAT_LITERAL = 22,             /* FLOAT_LITERAL  */
  YYSYMBOL_READ = 23,                      /* READ  */
  YYSYMBOL_PRINT = 24,                     /* PRINT  */
  YYSYMBOL_GOTO = 25,                      /* GOTO  */
  YYSYMBOL_BREAK = 26,                     /* BREAK  */
  YYSYMBOL_WHILE = 27,                     /* WHILE  */
  YYSYMBOL_LOOP = 28,                      /* LOOP  */
  YYSYMBOL_ELSE = 29,                      /* ELSE  */
  YYSYMBOL_IF = 30,                        /* IF  */
  YYSYMBOL_DEF = 31,                       /* DEF  */
  YYSYMBOL_INT = 32,                       /* INT  */
  YYSYMBOL_FLOAT = 33,                     /* FLOAT  */
  YYSYMBOL_STRING = 34,                    /* STRING  */
  YYSYMBOL_VOID = 35,                      /* VOID  */
  YYSYMBOL_BOOL = 36,                      /* BOOL  */
  YYSYMBOL_DOUBLE = 37,                    /* DOUBLE  */
  YYSYMBOL_RETURN = 38,                    /* RETURN  */
  YYSYMBOL_CONTINUE = 39,                  /* CONTINUE  */
  YYSYMBOL_TRUE = 40,                      /* TRUE  */
  YYSYMBOL_FALSE = 41,                     /* FALSE  */
  YYSYMBOL_MODEL = 42,                     /* MODEL  */
  YYSYMBOL_FORMAT = 43,                    /* FORMAT  */
  YYSYMBOL_PATH = 44,                      /* PATH  */
  YYSYMBOL_TASK = 45,                      /* TASK  */
  YYSYMBOL_PRECISION = 46,                 /* PRECISION  */
  YYSYMBOL_INPUT_SHAPE = 47,               /* INPUT_SHAPE  */
  YYSYMBOL_OUTPUT_SHAPE = 48,              /* OUTPUT_SHAPE  */
  YYSYMBOL_BACKEND_PREFERENCE = 49,        /* BACKEND_PREFERENCE  */
  YYSYMBOL_COMPACT = 50,                   /* COMPACT  */
  YYSYMBOL_QUALITY_GUARDRAIL = 51,         /* QUALITY_GUARDRAIL  */
  YYSYMBOL_GREENAI_CONTRACT_T = 52,        /* GREENAI_CONTRACT_T  */
  YYSYMBOL_FUNCTIONAL_UNIT = 53,           /* FUNCTIONAL_UNIT  */
  YYSYMBOL_SUCCESS_CRITERIA = 54,          /* SUCCESS_CRITERIA  */
  YYSYMBOL_BOUNDARY = 55,                  /* BOUNDARY  */
  YYSYMBOL_MEASUREMENT_QUALITY = 56,       /* MEASUREMENT_QUALITY  */
  YYSYMBOL_DATA_QUALITY = 57,              /* DATA_QUALITY  */
  YYSYMBOL_CARBON_FACTOR = 58,             /* CARBON_FACTOR  */
  YYSYMBOL_ENERGY_BUDGET_J = 59,           /* ENERGY_BUDGET_J  */
  YYSYMBOL_CARBON_BUDGET_GCO2E = 60,       /* CARBON_BUDGET_GCO2E  */
  YYSYMBOL_EVIDENCE_RETENTION = 61,        /* EVIDENCE_RETENTION  */
  YYSYMBOL_CLAIMS_MODE = 62,               /* CLAIMS_MODE  */
  YYSYMBOL_EVIDENCE_ONLY = 63,             /* EVIDENCE_ONLY  */
  YYSYMBOL_GREENAI_MEASURE = 64,           /* GREENAI_MEASURE  */
  YYSYMBOL_INFER = 65,                     /* INFER  */
  YYSYMBOL_TENSOR = 66,                    /* TENSOR  */
  YYSYMBOL_INT8 = 67,                      /* INT8  */
  YYSYMBOL_FP16 = 68,                      /* FP16  */
  YYSYMBOL_FP32 = 69,                      /* FP32  */
  YYSYMBOL_BF16 = 70,                      /* BF16  */
  YYSYMBOL_INT4 = 71,                      /* INT4  */
  YYSYMBOL_FP64 = 72,                      /* FP64  */
  YYSYMBOL_ONNX = 73,                      /* ONNX  */
  YYSYMBOL_ENGINE = 74,                    /* ENGINE  */
  YYSYMBOL_TORCHSCRIPT = 75,               /* TORCHSCRIPT  */
  YYSYMBOL_OPENVINO_IR = 76,               /* OPENVINO_IR  */
  YYSYMBOL_GGUF = 77,                      /* GGUF  */
  YYSYMBOL_TENSORRT = 78,                  /* TENSORRT  */
  YYSYMBOL_ONNXRUNTIME_TENSORRT = 79,      /* ONNXRUNTIME_TENSORRT  */
  YYSYMBOL_ONNXRUNTIME_CUDA = 80,          /* ONNXRUNTIME_CUDA  */
  YYSYMBOL_ONNXRUNTIME_CPU = 81,           /* ONNXRUNTIME_CPU  */
  YYSYMBOL_OPENVINO = 82,                  /* OPENVINO  */
  YYSYMBOL_LIBTORCH = 83,                  /* LIBTORCH  */
  YYSYMBOL_LLAMACPP = 84,                  /* LLAMACPP  */
  YYSYMBOL_FALLBACK = 85,                  /* FALLBACK  */
  YYSYMBOL_MQ1 = 86,                       /* MQ1  */
  YYSYMBOL_MQ2 = 87,                       /* MQ2  */
  YYSYMBOL_MQ3 = 88,                       /* MQ3  */
  YYSYMBOL_MQ4 = 89,                       /* MQ4  */
  YYSYMBOL_DQ1 = 90,                       /* DQ1  */
  YYSYMBOL_DQ2 = 91,                       /* DQ2  */
  YYSYMBOL_DQ3 = 92,                       /* DQ3  */
  YYSYMBOL_DQ4 = 93,                       /* DQ4  */
  YYSYMBOL_LOCATION = 94,                  /* LOCATION  */
  YYSYMBOL_CI_CD = 95,                     /* CI_CD  */
  YYSYMBOL_THIRDPARTY = 96,                /* THIRDPARTY  */
  YYSYMBOL_ACCELERATOR = 97,               /* ACCELERATOR  */
  YYSYMBOL_COMPUTE = 98,                   /* COMPUTE  */
  YYSYMBOL_STORAGE = 99,                   /* STORAGE  */
  YYSYMBOL_NETWORK = 100,                  /* NETWORK  */
  YYSYMBOL_101_ = 101,                     /* ';'  */
  YYSYMBOL_102_ = 102,                     /* '('  */
  YYSYMBOL_103_ = 103,                     /* ')'  */
  YYSYMBOL_104_ = 104,                     /* ','  */
  YYSYMBOL_105_ = 105,                     /* '['  */
  YYSYMBOL_106_ = 106,                     /* ']'  */
  YYSYMBOL_107_ = 107,                     /* '{'  */
  YYSYMBOL_108_ = 108,                     /* '}'  */
  YYSYMBOL_109_ = 109,                     /* ':'  */
  YYSYMBOL_YYACCEPT = 110,                 /* $accept  */
  YYSYMBOL_PROGRAMME_RULE = 111,           /* PROGRAMME_RULE  */
  YYSYMBOL_FUNCTION_LIST_RULE = 112,       /* FUNCTION_LIST_RULE  */
  YYSYMBOL_FUNCTION_RULE = 113,            /* FUNCTION_RULE  */
  YYSYMBOL_DECLARATION_STATEMENT_LIST_RULE = 114, /* DECLARATION_STATEMENT_LIST_RULE  */
  YYSYMBOL_ShortType = 115,                /* ShortType  */
  YYSYMBOL_DECLARATION_STATEMENT_RULE = 116, /* DECLARATION_STATEMENT_RULE  */
  YYSYMBOL_DECLARATION_VARIABLE_LIST_RULE = 117, /* DECLARATION_VARIABLE_LIST_RULE  */
  YYSYMBOL_LOGIC_BLOCK = 118,              /* LOGIC_BLOCK  */
  YYSYMBOL_STATEMENT_BLOCK_RULE = 119,     /* STATEMENT_BLOCK_RULE  */
  YYSYMBOL_STATEMENT_LIST_RULE = 120,      /* STATEMENT_LIST_RULE  */
  YYSYMBOL_STATEMENT_RULE = 121,           /* STATEMENT_RULE  */
  YYSYMBOL_RETURN_STATEMENT = 122,         /* RETURN_STATEMENT  */
  YYSYMBOL_INFER_STATEMENT = 123,          /* INFER_STATEMENT  */
  YYSYMBOL_TENSOR_DECLARATION = 124,       /* TENSOR_DECLARATION  */
  YYSYMBOL_MODEL_DECLARATION = 125,        /* MODEL_DECLARATION  */
  YYSYMBOL_126_1 = 126,                    /* $@1  */
  YYSYMBOL_MODEL_FIELD_LIST = 127,         /* MODEL_FIELD_LIST  */
  YYSYMBOL_MODEL_FIELD = 128,              /* MODEL_FIELD  */
  YYSYMBOL_BACKEND_LIST = 129,             /* BACKEND_LIST  */
  YYSYMBOL_GREENAI_CONTRACT = 130,         /* GREENAI_CONTRACT  */
  YYSYMBOL_131_2 = 131,                    /* $@2  */
  YYSYMBOL_CONTRACT_FIELD_LIST = 132,      /* CONTRACT_FIELD_LIST  */
  YYSYMBOL_CONTRACT_FIELD = 133,           /* CONTRACT_FIELD  */
  YYSYMBOL_BOUNDARY_LIST = 134,            /* BOUNDARY_LIST  */
  YYSYMBOL_GREENAI_MEASUREMENT = 135,      /* GREENAI_MEASUREMENT  */
  YYSYMBOL_136_3 = 136,                    /* $@3  */
  YYSYMBOL_MEASURE_FIELD_LIST = 137,       /* MEASURE_FIELD_LIST  */
  YYSYMBOL_MEASURE_FIELD = 138,            /* MEASURE_FIELD  */
  YYSYMBOL_FORMAT_NAME = 139,              /* FORMAT_NAME  */
  YYSYMBOL_PRECISION_NAME = 140,           /* PRECISION_NAME  */
  YYSYMBOL_BACKEND_NAME = 141,             /* BACKEND_NAME  */
  YYSYMBOL_MQ_NAME = 142,                  /* MQ_NAME  */
  YYSYMBOL_DQ_NAME = 143,                  /* DQ_NAME  */
  YYSYMBOL_BOUNDARY_NAME = 144,            /* BOUNDARY_NAME  */
  YYSYMBOL_AI_INFER_RULE = 145,            /* AI_INFER_RULE  */
  YYSYMBOL_GREENAI_REPORT_RULE = 146,      /* GREENAI_REPORT_RULE  */
  YYSYMBOL_EXPRESSION_RULE = 147,          /* EXPRESSION_RULE  */
  YYSYMBOL_VARIABLE_RULE = 148,            /* VARIABLE_RULE  */
  YYSYMBOL_READ_VARIABLE_LIST_RULE = 149,  /* READ_VARIABLE_LIST_RULE  */
  YYSYMBOL_PRINT_VARIABLE_LIST_RULE = 150  /* PRINT_VARIABLE_LIST_RULE  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  11
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   605

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  110
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  156
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  316

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   349


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
       2,     2,     2,     2,     2,     2,     2,    17,     2,     2,
     102,   103,    15,    13,   104,    14,     2,    16,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   109,   101,
       2,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   105,     2,   106,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   107,     2,   108,     2,     2,     2,     2,
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
       6,     7,     8,     9,    10,    11,    12,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    85,    85,    94,    99,   105,   110,   116,   124,   125,
     125,   126,   127,   128,   131,   139,   144,   149,   154,   159,
     163,   171,   179,   183,   190,   191,   192,   193,   194,   195,
     196,   197,   201,   206,   210,   214,   218,   222,   226,   230,
     234,   238,   242,   246,   247,   251,   255,   259,   263,   270,
     270,   272,   273,   275,   277,   277,   278,   278,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   280,   280,
     282,   282,   283,   283,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   285,   285,   287,   287,   288,
     288,   289,   289,   291,   291,   291,   291,   291,   292,   292,
     292,   292,   292,   292,   292,   293,   293,   293,   293,   293,
     293,   293,   293,   294,   294,   294,   294,   295,   295,   295,
     295,   296,   296,   296,   296,   296,   296,   298,   308,   319,
     323,   327,   331,   335,   339,   343,   347,   351,   355,   359,
     363,   367,   371,   375,   379,   383,   384,   385,   386,   390,
     394,   400,   404,   412,   416,   420,   425
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
  "GREATER_OR_EQUAL", "'+'", "'-'", "'*'", "'/'", "'%'", "UMINUS",
  "STRING_LITERAL", "IDENTIFIER", "INT_LITERAL", "FLOAT_LITERAL", "READ",
  "PRINT", "GOTO", "BREAK", "WHILE", "LOOP", "ELSE", "IF", "DEF", "INT",
  "FLOAT", "STRING", "VOID", "BOOL", "DOUBLE", "RETURN", "CONTINUE",
  "TRUE", "FALSE", "MODEL", "FORMAT", "PATH", "TASK", "PRECISION",
  "INPUT_SHAPE", "OUTPUT_SHAPE", "BACKEND_PREFERENCE", "COMPACT",
  "QUALITY_GUARDRAIL", "GREENAI_CONTRACT_T", "FUNCTIONAL_UNIT",
  "SUCCESS_CRITERIA", "BOUNDARY", "MEASUREMENT_QUALITY", "DATA_QUALITY",
  "CARBON_FACTOR", "ENERGY_BUDGET_J", "CARBON_BUDGET_GCO2E",
  "EVIDENCE_RETENTION", "CLAIMS_MODE", "EVIDENCE_ONLY", "GREENAI_MEASURE",
  "INFER", "TENSOR", "INT8", "FP16", "FP32", "BF16", "INT4", "FP64",
  "ONNX", "ENGINE", "TORCHSCRIPT", "OPENVINO_IR", "GGUF", "TENSORRT",
  "ONNXRUNTIME_TENSORRT", "ONNXRUNTIME_CUDA", "ONNXRUNTIME_CPU",
  "OPENVINO", "LIBTORCH", "LLAMACPP", "FALLBACK", "MQ1", "MQ2", "MQ3",
  "MQ4", "DQ1", "DQ2", "DQ3", "DQ4", "LOCATION", "CI_CD", "THIRDPARTY",
  "ACCELERATOR", "COMPUTE", "STORAGE", "NETWORK", "';'", "'('", "')'",
  "','", "'['", "']'", "'{'", "'}'", "':'", "$accept", "PROGRAMME_RULE",
  "FUNCTION_LIST_RULE", "FUNCTION_RULE", "DECLARATION_STATEMENT_LIST_RULE",
  "ShortType", "DECLARATION_STATEMENT_RULE",
  "DECLARATION_VARIABLE_LIST_RULE", "LOGIC_BLOCK", "STATEMENT_BLOCK_RULE",
  "STATEMENT_LIST_RULE", "STATEMENT_RULE", "RETURN_STATEMENT",
  "INFER_STATEMENT", "TENSOR_DECLARATION", "MODEL_DECLARATION", "$@1",
  "MODEL_FIELD_LIST", "MODEL_FIELD", "BACKEND_LIST", "GREENAI_CONTRACT",
  "$@2", "CONTRACT_FIELD_LIST", "CONTRACT_FIELD", "BOUNDARY_LIST",
  "GREENAI_MEASUREMENT", "$@3", "MEASURE_FIELD_LIST", "MEASURE_FIELD",
  "FORMAT_NAME", "PRECISION_NAME", "BACKEND_NAME", "MQ_NAME", "DQ_NAME",
  "BOUNDARY_NAME", "AI_INFER_RULE", "GREENAI_REPORT_RULE",
  "EXPRESSION_RULE", "VARIABLE_RULE", "READ_VARIABLE_LIST_RULE",
  "PRINT_VARIABLE_LIST_RULE", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-71)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     218,   -71,   -71,   -71,   -71,   -71,   -71,    20,   218,    67,
     -66,   -71,   426,   -12,     5,    11,   -71,   503,     9,   -71,
     -71,    73,    -4,   127,   120,   503,   503,   218,   208,   126,
     -71,   -71,   211,   212,   237,   238,   249,   -71,   503,   474,
     169,   -71,   -71,   474,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   171,   172,   276,   271,   -71,   255,   257,   174,   -71,
     -71,   -13,   503,   -71,   -71,    -7,   -71,   540,    15,   -11,
     -71,    41,   290,    41,   275,   -71,   341,   -71,   201,   204,
     209,   215,    58,   203,   379,   -71,   -71,   -71,   -71,   503,
     503,   503,   503,   503,   503,   503,   503,   503,   503,   503,
     503,   503,   -71,   503,   224,   226,   231,    19,    54,   -71,
      73,   -71,   293,   503,   -71,   -71,   503,   307,   236,   -71,
     -71,   -71,   -71,   317,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   320,   -71,   -71,   552,   585,   288,   288,   136,   136,
     136,   136,    -3,    -3,   -71,   -71,   -71,   354,   -71,   319,
     489,   240,   -71,   -71,   -71,   540,   373,   129,   265,   218,
     -71,   -71,   -71,   239,   242,   -71,   268,   269,   163,   -71,
     -71,   503,   -71,     6,    57,   267,   -16,   106,   -71,   -71,
     325,   503,    17,   265,    82,   357,   372,    58,   375,   377,
     121,    91,   378,   291,   -71,   386,   389,   391,   166,   108,
     133,   303,   390,   392,   393,   351,   314,   -71,   141,   315,
     -71,   402,   412,   321,   176,   503,   -71,   -71,   -71,   -71,
     -71,   -71,   -71,   324,   326,   327,   328,   329,   331,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,   -71,    62,   -71,   332,
     333,   414,   -71,   423,   335,   336,   -71,   -71,   -71,   -71,
     -71,   -71,   167,   -71,   -71,   -71,   -71,   -71,   337,   -71,
     -71,   -71,   -71,   338,   420,   352,   358,   359,   360,   -71,
     361,   362,   -71,   368,   438,   -71,   503,    41,   -71,   -71,
     -71,   -71,   -71,   -71,   -71,   121,   -71,   -71,   449,   450,
     -71,   -71,   -71,   166,   -71,   -71,   371,   -71,   -71,   -71,
     -71,   -71,   -71,   -71,   374,   229,   -71,   -71,   376,   381,
     -71,   -71,   -71,   -71,   -71,   -71
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     8,     9,    11,    12,    13,    10,     0,     4,    19,
       0,     1,     0,     0,    17,    14,     7,     0,   149,   145,
     146,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     147,   148,     0,     0,     0,     0,     0,    48,     0,     0,
       0,     2,    34,    20,    23,    29,    28,    25,    24,    26,
      27,     0,     0,     0,   144,     6,     0,     0,   149,   142,
     144,     0,     0,    47,   152,     0,   155,   156,     0,     0,
      43,     0,   144,     0,     0,    50,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     3,    22,    46,    45,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    31,     0,     0,    15,     0,     0,     0,    42,
       0,    44,     0,     0,    40,    39,     0,    35,     0,    49,
      54,    70,    87,     0,   104,    98,   100,   101,   102,    99,
     103,     0,   143,    21,   140,   141,   138,   139,   134,   135,
     136,   137,   129,   130,   131,   132,   133,     0,    18,     0,
       0,     0,   150,   151,   153,   154,     0,     0,     0,     0,
      57,    73,    90,     0,     0,    32,     0,     0,     0,    33,
      41,     0,    36,     0,     0,     0,     0,     0,    53,    16,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    56,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    72,     0,     0,
      89,     0,     0,     0,     0,     0,    37,     5,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,   105,
     106,   107,   108,   109,   110,   111,   112,     0,    69,     0,
       0,     0,    55,     0,     0,     0,   125,   126,   122,   121,
     123,   124,     0,    86,   113,   114,   115,   116,     0,   117,
     118,   119,   120,     0,     0,     0,     0,     0,     0,    71,
       0,     0,    88,     0,     0,   127,     0,     0,    58,    59,
      60,    61,    62,    63,    64,     0,    65,    66,     0,     0,
      74,    75,    76,     0,    77,    78,     0,    80,    81,    83,
      84,    92,    91,    51,     0,     0,    38,    68,     0,     0,
      85,    79,    52,   128,    67,    82
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -71,   -71,   -71,   -71,   330,   446,    -6,   -71,   -71,   -70,
     437,     1,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,
     292,   198,   -71,   -71,   191,   -71,   -71,   -17,   235,   424,
     -71
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     7,    12,    40,     8,     9,    10,    15,    41,    42,
      43,    44,    45,    46,    47,    48,   160,   174,   194,   237,
      49,   161,   175,   207,   252,    50,   162,   176,   210,   223,
     131,   238,   258,   263,   253,    51,    52,    53,    60,    65,
      68
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      59,   115,    13,   117,   208,    67,   106,    58,    71,    73,
      17,    76,    99,   100,   101,    66,    58,    19,    20,   113,
      11,    83,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    16,    30,    31,     1,     2,
       3,     4,     5,     6,    86,   108,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,    86,   147,    14,   172,    55,
     114,   124,   209,    58,   109,   155,   156,   110,    38,   157,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   183,
      56,    61,   216,   217,    62,    57,   111,   211,    63,   112,
     212,   215,   151,   110,    39,   125,   126,   127,   128,   129,
     130,   239,   240,   168,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    69,    39,    97,
      98,    99,   100,   101,   182,   218,   219,   220,   221,   222,
     152,   270,   271,   284,   214,   193,   285,    13,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   254,   255,   256,   257,   277,   229,
     230,   231,   232,   233,   234,   235,   236,   306,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,    70,    17,   259,   260,   261,   262,    77,    58,    19,
      20,    78,    79,   171,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    54,    30,    31,
       1,     2,     3,     4,     5,     6,    64,    80,    81,   305,
      72,   246,   247,   248,   249,   250,   251,   181,   292,    82,
      85,   293,    87,    88,    54,   103,   104,   105,    54,    62,
     276,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   116,   118,    64,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   132,    17,   120,    75,
      38,   121,   154,    58,    19,    20,   122,   123,   195,    54,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     148,   149,   313,    30,    31,   150,   158,   163,   159,   164,
     166,   169,   177,   178,   213,   153,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,    39,   180,   179,   206,   224,   102,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   225,   242,    17,   227,    38,   228,   264,   241,    18,
      19,    20,    21,    22,    23,    24,   243,    25,   244,    26,
     245,   265,   267,   266,   268,   269,   272,    28,    29,    30,
      31,    32,   273,   274,   275,   278,   288,   279,   280,   281,
     282,    33,   283,   286,   287,   289,   290,   291,   294,   295,
      17,   296,   119,    34,    35,    36,    18,    19,    20,    21,
      22,    23,    24,   297,    25,   165,    26,    27,   304,   298,
     299,   300,   301,   302,    28,    29,    30,    31,    32,   303,
     308,   309,   311,    74,   170,   312,    84,   314,    33,   226,
      37,    38,   315,   307,   310,   107,    39,   133,    17,   173,
      34,    35,    36,     0,    18,    19,    20,    21,    22,    23,
      24,     0,    25,    17,    26,     0,     0,     0,   167,    58,
      19,    20,    28,    29,    30,    31,    32,    17,     0,     0,
       0,     0,     0,    58,    19,    20,    33,    37,    38,    30,
      31,     0,     0,    39,     0,     0,     0,     0,    34,    35,
      36,     0,     0,    30,    31,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
       0,     0,     0,     0,     0,    37,    38,     0,     0,     0,
       0,    39,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,     0,     0,    38
};

static const yytype_int16 yycheck[] =
{
      17,    71,     8,    73,    20,    22,    19,    20,    25,    26,
      14,    28,    15,    16,    17,    19,    20,    21,    22,    30,
       0,    38,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,   101,    40,    41,    32,    33,
      34,    35,    36,    37,    43,    62,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    84,   103,    20,   158,   101,
     101,    33,   108,    20,   101,   112,   113,   104,   102,   116,
      43,    44,    45,    46,    47,    48,    49,    50,    51,   103,
     105,   102,   182,   183,   105,   104,   101,    11,   109,   104,
      14,   104,   103,   104,   107,    67,    68,    69,    70,    71,
      72,    40,    41,   150,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    20,   107,    13,
      14,    15,    16,    17,   171,    73,    74,    75,    76,    77,
     106,    20,    21,   101,   181,   108,   104,   173,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    86,    87,    88,    89,   215,    78,
      79,    80,    81,    82,    83,    84,    85,   277,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,   101,    14,    90,    91,    92,    93,   101,    20,    21,
      22,    20,    20,   104,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    12,    40,    41,
      32,    33,    34,    35,    36,    37,    21,    20,    20,   276,
      25,    95,    96,    97,    98,    99,   100,   104,   101,    20,
     101,   104,   101,   101,    39,     4,    21,    20,    43,   105,
     104,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,     4,    20,    61,     9,    10,    11,
      12,    13,    14,    15,    16,    17,   103,    14,   107,   101,
     102,   107,    19,    20,    21,    22,   107,   102,    51,    84,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
     106,   105,   103,    40,    41,   104,    29,    20,   102,    19,
      21,   101,   103,   101,    19,   110,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,   107,   104,   106,   108,    19,   101,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    19,   101,    14,    19,   102,    19,    94,    20,    20,
      21,    22,    23,    24,    25,    26,    20,    28,    19,    30,
      19,    21,    19,    21,    63,   101,   101,    38,    39,    40,
      41,    42,    20,    11,   103,   101,    12,   101,   101,   101,
     101,    52,   101,   101,   101,    12,   101,   101,   101,   101,
      14,    21,   101,    64,    65,    66,    20,    21,    22,    23,
      24,    25,    26,   101,    28,   101,    30,    31,    20,   101,
     101,   101,   101,   101,    38,    39,    40,    41,    42,   101,
      21,    21,   101,    27,   101,   101,    39,   101,    52,   187,
     101,   102,   101,   285,   293,    61,   107,   108,    14,   159,
      64,    65,    66,    -1,    20,    21,    22,    23,    24,    25,
      26,    -1,    28,    14,    30,    -1,    -1,    -1,    19,    20,
      21,    22,    38,    39,    40,    41,    42,    14,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    52,   101,   102,    40,
      41,    -1,    -1,   107,    -1,    -1,    -1,    -1,    64,    65,
      66,    -1,    -1,    40,    41,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    -1,    -1,   101,   102,    -1,    -1,    -1,
      -1,   107,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   102,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    -1,   102
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    32,    33,    34,    35,    36,    37,   111,   114,   115,
     116,     0,   112,   116,    20,   117,   101,    14,    20,    21,
      22,    23,    24,    25,    26,    28,    30,    31,    38,    39,
      40,    41,    42,    52,    64,    65,    66,   101,   102,   107,
     113,   118,   119,   120,   121,   122,   123,   124,   125,   130,
     135,   145,   146,   147,   148,   101,   105,   104,    20,   147,
     148,   102,   105,   109,   148,   149,    19,   147,   150,    20,
     101,   147,   148,   147,   115,   101,   147,   101,    20,    20,
      20,    20,    20,   147,   120,   101,   121,   101,   101,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,   101,     4,    21,    20,    19,   149,   147,   101,
     104,   101,   104,    30,   101,   119,     4,   119,    20,   101,
     107,   107,   107,   102,    33,    67,    68,    69,    70,    71,
      72,   140,   103,   108,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   106,   105,
     104,   103,   106,   148,    19,   147,   147,   147,    29,   102,
     126,   131,   136,    20,    19,   101,    21,    19,   147,   101,
     101,   104,   119,   114,   127,   132,   137,   103,   101,   106,
     104,   104,   147,   103,    43,    44,    45,    46,    47,    48,
      49,    50,    51,   108,   128,    51,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,   108,   133,    20,   108,
     138,    11,    14,    19,   147,   104,   119,   119,    73,    74,
      75,    76,    77,   139,    19,    19,   140,    19,    19,    78,
      79,    80,    81,    82,    83,    84,    85,   129,   141,    40,
      41,    20,   101,    20,    19,    19,    95,    96,    97,    98,
      99,   100,   134,   144,    86,    87,    88,    89,   142,    90,
      91,    92,    93,   143,    94,    21,    21,    19,    63,   101,
      20,    21,   101,    20,    11,   103,   104,   147,   101,   101,
     101,   101,   101,   101,   101,   104,   101,   101,    12,    12,
     101,   101,   101,   104,   101,   101,    21,   101,   101,   101,
     101,   101,   101,   101,    20,   147,   119,   141,    21,    21,
     144,   101,   101,   103,   101,   101
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   110,   111,   112,   112,   113,   114,   114,   115,   115,
     115,   115,   115,   115,   116,   117,   117,   117,   117,   117,
     118,   119,   120,   120,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   122,
     122,   123,   123,   124,   126,   125,   127,   127,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   129,   129,
     131,   130,   132,   132,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   134,   134,   136,   135,   137,
     137,   138,   138,   139,   139,   139,   139,   139,   140,   140,
     140,   140,   140,   140,   140,   141,   141,   141,   141,   141,
     141,   141,   141,   142,   142,   142,   142,   143,   143,   143,
     143,   144,   144,   144,   144,   144,   144,   145,   146,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   148,
     148,   149,   149,   150,   150,   150,   150
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     3,     0,     7,     3,     2,     1,     1,
       1,     1,     1,     1,     2,     3,     6,     1,     4,     0,
       1,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     4,     5,     1,     3,     5,     7,     9,     3,
       3,     5,     3,     2,     3,     2,     2,     2,     1,     3,
       2,     8,     9,     5,     0,     7,     2,     0,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     5,     3,     1,
       0,     7,     2,     0,     3,     3,     3,     3,     3,     4,
       3,     3,     5,     3,     3,     3,     1,     0,     7,     2,
       0,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     8,    10,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     1,     1,     1,     1,     1,     1,
       4,     3,     1,     3,     3,     1,     1
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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
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
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
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

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

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

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
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
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* PROGRAMME_RULE: DECLARATION_STATEMENT_LIST_RULE FUNCTION_LIST_RULE LOGIC_BLOCK  */
#line 86 "scanner_parser/parser.yy"
            {
                //fprintf(bison_output, "program\n");
                (yyval.program) = new AST_PROGRAM((yyvsp[-2].decl_block),(yyvsp[-1].functions),(yyvsp[0].code_block));
                main_program = (yyval.program);
            }
#line 1528 "parser.tab.cc"
    break;

  case 3: /* FUNCTION_LIST_RULE: FUNCTION_LIST_RULE FUNCTION_RULE ';'  */
#line 95 "scanner_parser/parser.yy"
                    {
                        (yyval.functions) = (yyvsp[-2].functions);
                        (yyval.functions)->push_back((yyvsp[-1].function));
                    }
#line 1537 "parser.tab.cc"
    break;

  case 4: /* FUNCTION_LIST_RULE: %empty  */
#line 100 "scanner_parser/parser.yy"
                    {
                        (yyval.functions) = new AST_FUNCTION_LIST_RULE();
                    }
#line 1545 "parser.tab.cc"
    break;

  case 5: /* FUNCTION_RULE: DEF ShortType IDENTIFIER '(' DECLARATION_STATEMENT_LIST_RULE ')' STATEMENT_BLOCK_RULE  */
#line 106 "scanner_parser/parser.yy"
                        {
	        	(yyval.function) = new AST_FUNCTION_RULE((yyvsp[-5].type),(yyvsp[-4].string_val),(yyvsp[-2].decl_block),(yyvsp[0].block_statement));
	      		}
#line 1553 "parser.tab.cc"
    break;

  case 6: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';'  */
#line 111 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back((yyvsp[-1].decl_block));
                       }
#line 1562 "parser.tab.cc"
    break;

  case 7: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_RULE ';'  */
#line 117 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-1].decl_block);
                       }
#line 1570 "parser.tab.cc"
    break;

  case 8: /* ShortType: INT  */
#line 124 "scanner_parser/parser.yy"
               {(yyval.type)=ShortType::Int;}
#line 1576 "parser.tab.cc"
    break;

  case 9: /* ShortType: FLOAT  */
#line 125 "scanner_parser/parser.yy"
              {(yyval.type)=ShortType::Float;}
#line 1582 "parser.tab.cc"
    break;

  case 10: /* ShortType: DOUBLE  */
#line 125 "scanner_parser/parser.yy"
                                              {(yyval.type)=ShortType::Float;}
#line 1588 "parser.tab.cc"
    break;

  case 11: /* ShortType: STRING  */
#line 126 "scanner_parser/parser.yy"
               {(yyval.type)=ShortType::String;}
#line 1594 "parser.tab.cc"
    break;

  case 12: /* ShortType: VOID  */
#line 127 "scanner_parser/parser.yy"
             {(yyval.type)=ShortType::Void;}
#line 1600 "parser.tab.cc"
    break;

  case 13: /* ShortType: BOOL  */
#line 128 "scanner_parser/parser.yy"
            {(yyval.type)=ShortType::Boolean;}
#line 1606 "parser.tab.cc"
    break;

  case 14: /* DECLARATION_STATEMENT_RULE: ShortType DECLARATION_VARIABLE_LIST_RULE  */
#line 132 "scanner_parser/parser.yy"
                   {
                       //fprintf(bison_output, "DECLARATION_STATEMENT_RULE\n");
                       (yyval.decl_block) = (yyvsp[0].decl_block);
                   }
#line 1615 "parser.tab.cc"
    break;

  case 15: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER  */
#line 140 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1624 "parser.tab.cc"
    break;

  case 16: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']'  */
#line 145 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-5].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1633 "parser.tab.cc"
    break;

  case 17: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER  */
#line 150 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1642 "parser.tab.cc"
    break;

  case 18: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER '[' INT_LITERAL ']'  */
#line 155 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1651 "parser.tab.cc"
    break;

  case 19: /* DECLARATION_VARIABLE_LIST_RULE: %empty  */
#line 159 "scanner_parser/parser.yy"
                           {(yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();}
#line 1657 "parser.tab.cc"
    break;

  case 20: /* LOGIC_BLOCK: STATEMENT_LIST_RULE  */
#line 164 "scanner_parser/parser.yy"
               {
                   //fprintf(bison_output, "LOGIC_BLOCK\n");
                   (yyval.code_block) = new AST_LOGIC_BLOCK((yyvsp[0].block_statement));
               }
#line 1666 "parser.tab.cc"
    break;

  case 21: /* STATEMENT_BLOCK_RULE: '{' STATEMENT_LIST_RULE '}'  */
#line 172 "scanner_parser/parser.yy"
                    {
                        //fprintf(bison_output, "STATEMENT_BLOCK_RULE\n");
                        (yyval.block_statement) = (yyvsp[-1].block_statement);
                    }
#line 1675 "parser.tab.cc"
    break;

  case 22: /* STATEMENT_LIST_RULE: STATEMENT_LIST_RULE STATEMENT_RULE  */
#line 180 "scanner_parser/parser.yy"
                   {
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1683 "parser.tab.cc"
    break;

  case 23: /* STATEMENT_LIST_RULE: STATEMENT_RULE  */
#line 184 "scanner_parser/parser.yy"
                   {
                       (yyval.block_statement) = new AST_STATEMENTS_BLOCK();
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1692 "parser.tab.cc"
    break;

  case 24: /* STATEMENT_RULE: MODEL_DECLARATION  */
#line 190 "scanner_parser/parser.yy"
                                     { (yyval.statement) = (yyvsp[0].model_decl); }
#line 1698 "parser.tab.cc"
    break;

  case 25: /* STATEMENT_RULE: TENSOR_DECLARATION  */
#line 191 "scanner_parser/parser.yy"
                              { (yyval.statement) = (yyvsp[0].tensor_decl); }
#line 1704 "parser.tab.cc"
    break;

  case 26: /* STATEMENT_RULE: GREENAI_CONTRACT  */
#line 192 "scanner_parser/parser.yy"
                            { (yyval.statement) = (yyvsp[0].greenai_contract); }
#line 1710 "parser.tab.cc"
    break;

  case 27: /* STATEMENT_RULE: GREENAI_MEASUREMENT  */
#line 193 "scanner_parser/parser.yy"
                               { (yyval.statement) = (yyvsp[0].greenai_measure); }
#line 1716 "parser.tab.cc"
    break;

  case 28: /* STATEMENT_RULE: INFER_STATEMENT  */
#line 194 "scanner_parser/parser.yy"
                           { (yyval.statement) = (yyvsp[0].infer_statement); }
#line 1722 "parser.tab.cc"
    break;

  case 29: /* STATEMENT_RULE: RETURN_STATEMENT  */
#line 195 "scanner_parser/parser.yy"
                            { (yyval.statement) = (yyvsp[0].return_statement); }
#line 1728 "parser.tab.cc"
    break;

  case 30: /* STATEMENT_RULE: CONTINUE ';'  */
#line 196 "scanner_parser/parser.yy"
                        { (yyval.statement) = new AST_CONTINUE(); }
#line 1734 "parser.tab.cc"
    break;

  case 31: /* STATEMENT_RULE: EXPRESSION_RULE ';'  */
#line 198 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE((yyvsp[-1].expression));
              }
#line 1742 "parser.tab.cc"
    break;

  case 32: /* STATEMENT_RULE: VARIABLE_RULE '=' EXPRESSION_RULE ';'  */
#line 202 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_ASSIGNMENT_RULE((yyvsp[-3].variable), (yyvsp[-1].expression));
              }
#line 1750 "parser.tab.cc"
    break;

  case 33: /* STATEMENT_RULE: IDENTIFIER '(' READ_VARIABLE_LIST_RULE ')' ';'  */
#line 207 "scanner_parser/parser.yy"
              {
	       (yyval.statement) = new AST_FUNCTION_CALL_RULE((yyvsp[-4].string_val),(yyvsp[-2].read_statement));
	      }
#line 1758 "parser.tab.cc"
    break;

  case 34: /* STATEMENT_RULE: STATEMENT_BLOCK_RULE  */
#line 211 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[0].block_statement);
              }
#line 1766 "parser.tab.cc"
    break;

  case 35: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 215 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_STATEMENT((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1774 "parser.tab.cc"
    break;

  case 36: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE  */
#line 219 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_ELSE_STATEMENT((yyvsp[-3].expression), (yyvsp[-2].block_statement), (yyvsp[0].block_statement));
              }
#line 1782 "parser.tab.cc"
    break;

  case 37: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 223 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-5].variable), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1790 "parser.tab.cc"
    break;

  case 38: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 227 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-7].variable), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1798 "parser.tab.cc"
    break;

  case 39: /* STATEMENT_RULE: LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 231 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_WHILE_LOOP_STATEMENT_RULE((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1806 "parser.tab.cc"
    break;

  case 40: /* STATEMENT_RULE: GOTO IDENTIFIER ';'  */
#line 235 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE(string((yyvsp[-1].string_val)));
              }
#line 1814 "parser.tab.cc"
    break;

  case 41: /* STATEMENT_RULE: GOTO IDENTIFIER IF EXPRESSION_RULE ';'  */
#line 239 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE((yyvsp[-1].expression), string((yyvsp[-3].string_val)));
              }
#line 1822 "parser.tab.cc"
    break;

  case 42: /* STATEMENT_RULE: READ READ_VARIABLE_LIST_RULE ';'  */
#line 243 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].read_statement);
              }
#line 1830 "parser.tab.cc"
    break;

  case 43: /* STATEMENT_RULE: BREAK ';'  */
#line 246 "scanner_parser/parser.yy"
                     {(yyval.statement) = new AST_BREAK();}
#line 1836 "parser.tab.cc"
    break;

  case 44: /* STATEMENT_RULE: PRINT PRINT_VARIABLE_LIST_RULE ';'  */
#line 248 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].print_statement);
              }
#line 1844 "parser.tab.cc"
    break;

  case 45: /* STATEMENT_RULE: GREENAI_REPORT_RULE ';'  */
#line 252 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].greenai_report);
              }
#line 1852 "parser.tab.cc"
    break;

  case 46: /* STATEMENT_RULE: AI_INFER_RULE ';'  */
#line 256 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].ai_infer);
              }
#line 1860 "parser.tab.cc"
    break;

  case 47: /* STATEMENT_RULE: IDENTIFIER ':'  */
#line 260 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_LABEL_RULE(string((yyvsp[-1].string_val)));
              }
#line 1868 "parser.tab.cc"
    break;

  case 48: /* STATEMENT_RULE: ';'  */
#line 264 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE(new AST_LITERAL(1));
              }
#line 1876 "parser.tab.cc"
    break;

  case 49: /* RETURN_STATEMENT: RETURN EXPRESSION_RULE ';'  */
#line 270 "scanner_parser/parser.yy"
                                             { (yyval.return_statement) = new AST_RETURN_STATEMENT((yyvsp[-1].expression)); }
#line 1882 "parser.tab.cc"
    break;

  case 50: /* RETURN_STATEMENT: RETURN ';'  */
#line 270 "scanner_parser/parser.yy"
                                                                                                 { (yyval.return_statement) = new AST_RETURN_STATEMENT(); }
#line 1888 "parser.tab.cc"
    break;

  case 51: /* INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' GREATER IDENTIFIER ';'  */
#line 272 "scanner_parser/parser.yy"
                                                                            { (yyval.infer_statement) = new AST_INFER_STATEMENT(string((yyvsp[-6].string_val)), string((yyvsp[-4].string_val)), string((yyvsp[-1].string_val))); }
#line 1894 "parser.tab.cc"
    break;

  case 52: /* INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' '-' GREATER IDENTIFIER ';'  */
#line 273 "scanner_parser/parser.yy"
                                                                                { (yyval.infer_statement) = new AST_INFER_STATEMENT(string((yyvsp[-7].string_val)), string((yyvsp[-5].string_val)), string((yyvsp[-1].string_val))); }
#line 1900 "parser.tab.cc"
    break;

  case 53: /* TENSOR_DECLARATION: TENSOR IDENTIFIER PRECISION_NAME STRING_LITERAL ';'  */
#line 275 "scanner_parser/parser.yy"
                                                                        { TensorDeclarationData d; d.name=(yyvsp[-3].string_val); d.element_type=(yyvsp[-2].string_val); d.shape_csv=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); d.dynamic=(d.shape_csv=="dynamic"); d.rank= d.dynamic?0:1; long long total=1; if(!d.dynamic){ d.rank=0; size_t start=0; while(start<d.shape_csv.size()){ size_t pos=d.shape_csv.find(',',start); string part=d.shape_csv.substr(start,pos==string::npos?string::npos:pos-start); total*= atoll(part.c_str()); d.rank++; if(pos==string::npos) break; start=pos+1; }} d.total_elements=total; (yyval.tensor_decl)=new AST_TENSOR_DECLARATION(d); }
#line 1906 "parser.tab.cc"
    break;

  case 54: /* $@1: %empty  */
#line 277 "scanner_parser/parser.yy"
                                        { current_model=ModelDeclarationData(); current_model.name=(yyvsp[-1].string_val); }
#line 1912 "parser.tab.cc"
    break;

  case 55: /* MODEL_DECLARATION: MODEL IDENTIFIER '{' $@1 MODEL_FIELD_LIST '}' ';'  */
#line 277 "scanner_parser/parser.yy"
                                                                                                                                  { (yyval.model_decl) = new AST_MODEL_DECLARATION(current_model); }
#line 1918 "parser.tab.cc"
    break;

  case 58: /* MODEL_FIELD: FORMAT FORMAT_NAME ';'  */
#line 279 "scanner_parser/parser.yy"
                                    { current_model.format=(yyvsp[-1].string_val); }
#line 1924 "parser.tab.cc"
    break;

  case 59: /* MODEL_FIELD: PATH STRING_LITERAL ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                           { current_model.path=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1930 "parser.tab.cc"
    break;

  case 60: /* MODEL_FIELD: TASK STRING_LITERAL ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                      { current_model.task=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1936 "parser.tab.cc"
    break;

  case 61: /* MODEL_FIELD: PRECISION PRECISION_NAME ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                      { current_model.precision=(yyvsp[-1].string_val); }
#line 1942 "parser.tab.cc"
    break;

  case 62: /* MODEL_FIELD: INPUT_SHAPE STRING_LITERAL ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                       { current_model.input_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1948 "parser.tab.cc"
    break;

  case 63: /* MODEL_FIELD: OUTPUT_SHAPE STRING_LITERAL ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                 { current_model.output_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1954 "parser.tab.cc"
    break;

  case 65: /* MODEL_FIELD: COMPACT TRUE ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   { current_model.compact=true; }
#line 1960 "parser.tab.cc"
    break;

  case 66: /* MODEL_FIELD: COMPACT FALSE ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       { current_model.compact=false; }
#line 1966 "parser.tab.cc"
    break;

  case 67: /* MODEL_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 279 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_model.has_quality_guardrail=true; current_model.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 1972 "parser.tab.cc"
    break;

  case 68: /* BACKEND_LIST: BACKEND_LIST ',' BACKEND_NAME  */
#line 280 "scanner_parser/parser.yy"
                                            { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 1978 "parser.tab.cc"
    break;

  case 69: /* BACKEND_LIST: BACKEND_NAME  */
#line 280 "scanner_parser/parser.yy"
                                                                                                               { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 1984 "parser.tab.cc"
    break;

  case 70: /* $@2: %empty  */
#line 282 "scanner_parser/parser.yy"
                                                    { current_contract=GreenAIContractData(); current_contract.name=(yyvsp[-1].string_val); }
#line 1990 "parser.tab.cc"
    break;

  case 71: /* GREENAI_CONTRACT: GREENAI_CONTRACT_T IDENTIFIER '{' $@2 CONTRACT_FIELD_LIST '}' ';'  */
#line 282 "scanner_parser/parser.yy"
                                                                                                                                                      { (yyval.greenai_contract) = new AST_GREENAI_CONTRACT(current_contract); }
#line 1996 "parser.tab.cc"
    break;

  case 74: /* CONTRACT_FIELD: FUNCTIONAL_UNIT STRING_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                   { current_contract.functional_unit=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_functional_unit=true; }
#line 2002 "parser.tab.cc"
    break;

  case 75: /* CONTRACT_FIELD: SUCCESS_CRITERIA STRING_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                   { current_contract.success_criteria=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_success_criteria=true; }
#line 2008 "parser.tab.cc"
    break;

  case 76: /* CONTRACT_FIELD: BOUNDARY BOUNDARY_LIST ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                            { current_contract.has_boundary=true; }
#line 2014 "parser.tab.cc"
    break;

  case 77: /* CONTRACT_FIELD: MEASUREMENT_QUALITY MQ_NAME ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                      { current_contract.measurement_quality=(yyvsp[-1].string_val); current_contract.has_mq=true; }
#line 2020 "parser.tab.cc"
    break;

  case 78: /* CONTRACT_FIELD: DATA_QUALITY DQ_NAME ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.data_quality=(yyvsp[-1].string_val); current_contract.has_dq=true; }
#line 2026 "parser.tab.cc"
    break;

  case 79: /* CONTRACT_FIELD: CARBON_FACTOR LOCATION INT_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         { current_contract.carbon_factor_scope="location"; current_contract.carbon_factor=(yyvsp[-1].int_val); current_contract.has_carbon_factor=true; }
#line 2032 "parser.tab.cc"
    break;

  case 80: /* CONTRACT_FIELD: ENERGY_BUDGET_J INT_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.energy_budget_j=(yyvsp[-1].int_val); }
#line 2038 "parser.tab.cc"
    break;

  case 81: /* CONTRACT_FIELD: CARBON_BUDGET_GCO2E INT_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           { current_contract.carbon_budget_gco2e=(yyvsp[-1].int_val); }
#line 2044 "parser.tab.cc"
    break;

  case 82: /* CONTRACT_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_contract.has_quality_guardrail=true; current_contract.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 2050 "parser.tab.cc"
    break;

  case 83: /* CONTRACT_FIELD: EVIDENCE_RETENTION STRING_LITERAL ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  { current_contract.evidence_retention=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2056 "parser.tab.cc"
    break;

  case 84: /* CONTRACT_FIELD: CLAIMS_MODE EVIDENCE_ONLY ';'  */
#line 284 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    { current_contract.claims_mode="evidence_only"; }
#line 2062 "parser.tab.cc"
    break;

  case 85: /* BOUNDARY_LIST: BOUNDARY_LIST ',' BOUNDARY_NAME  */
#line 285 "scanner_parser/parser.yy"
                                               { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2068 "parser.tab.cc"
    break;

  case 86: /* BOUNDARY_LIST: BOUNDARY_NAME  */
#line 285 "scanner_parser/parser.yy"
                                                                                                            { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2074 "parser.tab.cc"
    break;

  case 87: /* $@3: %empty  */
#line 287 "scanner_parser/parser.yy"
                                                    { current_measure=GreenAIMeasurementData(); current_measure.workload=(yyvsp[-1].string_val); }
#line 2080 "parser.tab.cc"
    break;

  case 88: /* GREENAI_MEASUREMENT: GREENAI_MEASURE IDENTIFIER '{' $@3 MEASURE_FIELD_LIST '}' ';'  */
#line 287 "scanner_parser/parser.yy"
                                                                                                                                                          { (yyval.greenai_measure) = new AST_GREENAI_MEASUREMENT(current_measure); }
#line 2086 "parser.tab.cc"
    break;

  case 91: /* MEASURE_FIELD: IDENTIFIER INT_LITERAL ';'  */
#line 289 "scanner_parser/parser.yy"
                                          { string n=(yyvsp[-2].string_val); if(n=="inferences") current_measure.inferences=(yyvsp[-1].int_val); else if(n=="watts") current_measure.watts=(yyvsp[-1].int_val); else if(n=="seconds") current_measure.seconds=(yyvsp[-1].int_val); }
#line 2092 "parser.tab.cc"
    break;

  case 92: /* MEASURE_FIELD: IDENTIFIER IDENTIFIER ';'  */
#line 289 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                          { if(string((yyvsp[-2].string_val))=="backend") current_measure.backend=(yyvsp[-1].string_val); }
#line 2098 "parser.tab.cc"
    break;

  case 93: /* FORMAT_NAME: ONNX  */
#line 291 "scanner_parser/parser.yy"
                  {(yyval.string_val)=(char*)"onnx";}
#line 2104 "parser.tab.cc"
    break;

  case 94: /* FORMAT_NAME: ENGINE  */
#line 291 "scanner_parser/parser.yy"
                                               {(yyval.string_val)=(char*)"engine";}
#line 2110 "parser.tab.cc"
    break;

  case 95: /* FORMAT_NAME: TORCHSCRIPT  */
#line 291 "scanner_parser/parser.yy"
                                                                                   {(yyval.string_val)=(char*)"torchscript";}
#line 2116 "parser.tab.cc"
    break;

  case 96: /* FORMAT_NAME: OPENVINO_IR  */
#line 291 "scanner_parser/parser.yy"
                                                                                                                            {(yyval.string_val)=(char*)"openvino_ir";}
#line 2122 "parser.tab.cc"
    break;

  case 97: /* FORMAT_NAME: GGUF  */
#line 291 "scanner_parser/parser.yy"
                                                                                                                                                              {(yyval.string_val)=(char*)"gguf";}
#line 2128 "parser.tab.cc"
    break;

  case 98: /* PRECISION_NAME: INT8  */
#line 292 "scanner_parser/parser.yy"
                     {(yyval.string_val)=(char*)"int8";}
#line 2134 "parser.tab.cc"
    break;

  case 99: /* PRECISION_NAME: INT4  */
#line 292 "scanner_parser/parser.yy"
                                                {(yyval.string_val)=(char*)"int4";}
#line 2140 "parser.tab.cc"
    break;

  case 100: /* PRECISION_NAME: FP16  */
#line 292 "scanner_parser/parser.yy"
                                                                           {(yyval.string_val)=(char*)"fp16";}
#line 2146 "parser.tab.cc"
    break;

  case 101: /* PRECISION_NAME: FP32  */
#line 292 "scanner_parser/parser.yy"
                                                                                                      {(yyval.string_val)=(char*)"fp32";}
#line 2152 "parser.tab.cc"
    break;

  case 102: /* PRECISION_NAME: BF16  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                 {(yyval.string_val)=(char*)"bf16";}
#line 2158 "parser.tab.cc"
    break;

  case 103: /* PRECISION_NAME: FP64  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                            {(yyval.string_val)=(char*)"fp64";}
#line 2164 "parser.tab.cc"
    break;

  case 104: /* PRECISION_NAME: FLOAT  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                                        {(yyval.string_val)=(char*)"float";}
#line 2170 "parser.tab.cc"
    break;

  case 105: /* BACKEND_NAME: TENSORRT  */
#line 293 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"tensorrt";}
#line 2176 "parser.tab.cc"
    break;

  case 106: /* BACKEND_NAME: ONNXRUNTIME_TENSORRT  */
#line 293 "scanner_parser/parser.yy"
                                                                      {(yyval.string_val)=(char*)"onnxruntime_tensorrt";}
#line 2182 "parser.tab.cc"
    break;

  case 107: /* BACKEND_NAME: ONNXRUNTIME_CUDA  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                             {(yyval.string_val)=(char*)"onnxruntime_cuda";}
#line 2188 "parser.tab.cc"
    break;

  case 108: /* BACKEND_NAME: ONNXRUNTIME_CPU  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                                                                               {(yyval.string_val)=(char*)"onnxruntime_cpu";}
#line 2194 "parser.tab.cc"
    break;

  case 109: /* BACKEND_NAME: OPENVINO  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                                                                                                                         {(yyval.string_val)=(char*)"openvino";}
#line 2200 "parser.tab.cc"
    break;

  case 110: /* BACKEND_NAME: LIBTORCH  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                            {(yyval.string_val)=(char*)"libtorch";}
#line 2206 "parser.tab.cc"
    break;

  case 111: /* BACKEND_NAME: LLAMACPP  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                               {(yyval.string_val)=(char*)"llamacpp";}
#line 2212 "parser.tab.cc"
    break;

  case 112: /* BACKEND_NAME: FALLBACK  */
#line 293 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                  {(yyval.string_val)=(char*)"fallback";}
#line 2218 "parser.tab.cc"
    break;

  case 113: /* MQ_NAME: MQ1  */
#line 294 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"MQ1";}
#line 2224 "parser.tab.cc"
    break;

  case 114: /* MQ_NAME: MQ2  */
#line 294 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"MQ2";}
#line 2230 "parser.tab.cc"
    break;

  case 115: /* MQ_NAME: MQ3  */
#line 294 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"MQ3";}
#line 2236 "parser.tab.cc"
    break;

  case 116: /* MQ_NAME: MQ4  */
#line 294 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"MQ4";}
#line 2242 "parser.tab.cc"
    break;

  case 117: /* DQ_NAME: DQ1  */
#line 295 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"DQ1";}
#line 2248 "parser.tab.cc"
    break;

  case 118: /* DQ_NAME: DQ2  */
#line 295 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"DQ2";}
#line 2254 "parser.tab.cc"
    break;

  case 119: /* DQ_NAME: DQ3  */
#line 295 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"DQ3";}
#line 2260 "parser.tab.cc"
    break;

  case 120: /* DQ_NAME: DQ4  */
#line 295 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"DQ4";}
#line 2266 "parser.tab.cc"
    break;

  case 121: /* BOUNDARY_NAME: COMPUTE  */
#line 296 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"compute";}
#line 2272 "parser.tab.cc"
    break;

  case 122: /* BOUNDARY_NAME: ACCELERATOR  */
#line 296 "scanner_parser/parser.yy"
                                                            {(yyval.string_val)=(char*)"accelerator";}
#line 2278 "parser.tab.cc"
    break;

  case 123: /* BOUNDARY_NAME: STORAGE  */
#line 296 "scanner_parser/parser.yy"
                                                                                                 {(yyval.string_val)=(char*)"storage";}
#line 2284 "parser.tab.cc"
    break;

  case 124: /* BOUNDARY_NAME: NETWORK  */
#line 296 "scanner_parser/parser.yy"
                                                                                                                                  {(yyval.string_val)=(char*)"network";}
#line 2290 "parser.tab.cc"
    break;

  case 125: /* BOUNDARY_NAME: CI_CD  */
#line 296 "scanner_parser/parser.yy"
                                                                                                                                                                 {(yyval.string_val)=(char*)"ci_cd";}
#line 2296 "parser.tab.cc"
    break;

  case 126: /* BOUNDARY_NAME: THIRDPARTY  */
#line 296 "scanner_parser/parser.yy"
                                                                                                                                                                                                   {(yyval.string_val)=(char*)"thirdparty";}
#line 2302 "parser.tab.cc"
    break;

  case 127: /* AI_INFER_RULE: IDENTIFIER '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')'  */
#line 299 "scanner_parser/parser.yy"
              {
                  if ((string((yyvsp[-7].string_val)) != "ai_infer" && string((yyvsp[-7].string_val)) != "aiinfer")) {
                      yyerror((char *)"expected ai_infer builtin");
                      YYERROR;
                  }
                  (yyval.ai_infer) = new AST_AI_INFER_RULE(string((yyvsp[-5].string_val)), string((yyvsp[-3].string_val)), string((yyvsp[-1].string_val)));
              }
#line 2314 "parser.tab.cc"
    break;

  case 128: /* GREENAI_REPORT_RULE: IDENTIFIER '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')'  */
#line 309 "scanner_parser/parser.yy"
              {
                  if (string((yyvsp[-9].string_val)) != "greenai") {
                      yyerror((char *)"expected greenai report builtin");
                      YYERROR;
                  }
                  (yyval.greenai_report) = new AST_GREENAI_REPORT_RULE(string((yyvsp[-7].string_val)), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression));
              }
#line 2326 "parser.tab.cc"
    break;

  case 129: /* EXPRESSION_RULE: EXPRESSION_RULE '+' EXPRESSION_RULE  */
#line 320 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "+");
               }
#line 2334 "parser.tab.cc"
    break;

  case 130: /* EXPRESSION_RULE: EXPRESSION_RULE '-' EXPRESSION_RULE  */
#line 324 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "-");
               }
#line 2342 "parser.tab.cc"
    break;

  case 131: /* EXPRESSION_RULE: EXPRESSION_RULE '*' EXPRESSION_RULE  */
#line 328 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "*");
               }
#line 2350 "parser.tab.cc"
    break;

  case 132: /* EXPRESSION_RULE: EXPRESSION_RULE '/' EXPRESSION_RULE  */
#line 332 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "/");
               }
#line 2358 "parser.tab.cc"
    break;

  case 133: /* EXPRESSION_RULE: EXPRESSION_RULE '%' EXPRESSION_RULE  */
#line 336 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "%");
               }
#line 2366 "parser.tab.cc"
    break;

  case 134: /* EXPRESSION_RULE: EXPRESSION_RULE LESS EXPRESSION_RULE  */
#line 340 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<");
               }
#line 2374 "parser.tab.cc"
    break;

  case 135: /* EXPRESSION_RULE: EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE  */
#line 344 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<=");
               }
#line 2382 "parser.tab.cc"
    break;

  case 136: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER EXPRESSION_RULE  */
#line 348 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">");
               }
#line 2390 "parser.tab.cc"
    break;

  case 137: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE  */
#line 352 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">=");
               }
#line 2398 "parser.tab.cc"
    break;

  case 138: /* EXPRESSION_RULE: EXPRESSION_RULE EQUAL EXPRESSION_RULE  */
#line 356 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "==");
               }
#line 2406 "parser.tab.cc"
    break;

  case 139: /* EXPRESSION_RULE: EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE  */
#line 360 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "!=");
               }
#line 2414 "parser.tab.cc"
    break;

  case 140: /* EXPRESSION_RULE: EXPRESSION_RULE OR EXPRESSION_RULE  */
#line 364 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "||");
               }
#line 2422 "parser.tab.cc"
    break;

  case 141: /* EXPRESSION_RULE: EXPRESSION_RULE AND EXPRESSION_RULE  */
#line 368 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "&&");
               }
#line 2430 "parser.tab.cc"
    break;

  case 142: /* EXPRESSION_RULE: '-' EXPRESSION_RULE  */
#line 372 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_UNARY_EXPRESSION_RULE((yyvsp[0].expression), "-");
               }
#line 2438 "parser.tab.cc"
    break;

  case 143: /* EXPRESSION_RULE: '(' EXPRESSION_RULE ')'  */
#line 376 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[-1].expression);
               }
#line 2446 "parser.tab.cc"
    break;

  case 144: /* EXPRESSION_RULE: VARIABLE_RULE  */
#line 380 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[0].variable);
               }
#line 2454 "parser.tab.cc"
    break;

  case 145: /* EXPRESSION_RULE: INT_LITERAL  */
#line 383 "scanner_parser/parser.yy"
                           { (yyval.expression) = new AST_LITERAL((yyvsp[0].int_val)); }
#line 2460 "parser.tab.cc"
    break;

  case 146: /* EXPRESSION_RULE: FLOAT_LITERAL  */
#line 384 "scanner_parser/parser.yy"
                             { (yyval.expression) = new AST_FLOAT_LITERAL((yyvsp[0].float_val)); }
#line 2466 "parser.tab.cc"
    break;

  case 147: /* EXPRESSION_RULE: TRUE  */
#line 385 "scanner_parser/parser.yy"
                    { (yyval.expression) = new AST_BOOL_LITERAL(true); }
#line 2472 "parser.tab.cc"
    break;

  case 148: /* EXPRESSION_RULE: FALSE  */
#line 386 "scanner_parser/parser.yy"
                     { (yyval.expression) = new AST_BOOL_LITERAL(false); }
#line 2478 "parser.tab.cc"
    break;

  case 149: /* VARIABLE_RULE: IDENTIFIER  */
#line 391 "scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_SIMPLE_VARIABLE(string((yyvsp[0].string_val)));
             }
#line 2486 "parser.tab.cc"
    break;

  case 150: /* VARIABLE_RULE: IDENTIFIER '[' EXPRESSION_RULE ']'  */
#line 395 "scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_ARRAY_VARIABLE(string((yyvsp[-3].string_val)), (yyvsp[-1].expression));
             }
#line 2494 "parser.tab.cc"
    break;

  case 151: /* READ_VARIABLE_LIST_RULE: READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE  */
#line 401 "scanner_parser/parser.yy"
                       {
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 2502 "parser.tab.cc"
    break;

  case 152: /* READ_VARIABLE_LIST_RULE: VARIABLE_RULE  */
#line 405 "scanner_parser/parser.yy"
                       {
                           (yyval.read_statement) = new AST_READ_RULE();
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 2511 "parser.tab.cc"
    break;

  case 153: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' STRING_LITERAL  */
#line 413 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 2519 "parser.tab.cc"
    break;

  case 154: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE  */
#line 417 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 2527 "parser.tab.cc"
    break;

  case 155: /* PRINT_VARIABLE_LIST_RULE: STRING_LITERAL  */
#line 421 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 2536 "parser.tab.cc"
    break;

  case 156: /* PRINT_VARIABLE_LIST_RULE: EXPRESSION_RULE  */
#line 426 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 2545 "parser.tab.cc"
    break;


#line 2549 "parser.tab.cc"

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
                      yytoken, &yylval);
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


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 433 "scanner_parser/parser.yy"



extern "C" void yyerror (char const *s)
{
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf (stderr, "%s\n", s);
}
