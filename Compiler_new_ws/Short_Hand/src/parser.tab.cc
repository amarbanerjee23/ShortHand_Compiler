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
#define YYLAST   584

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  110
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  155
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  312

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
     270,   272,   274,   276,   276,   277,   277,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   279,   279,   281,
     281,   282,   282,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   284,   284,   286,   286,   287,   287,
     288,   288,   290,   290,   290,   290,   290,   291,   291,   291,
     291,   291,   291,   291,   292,   292,   292,   292,   292,   292,
     292,   292,   293,   293,   293,   293,   294,   294,   294,   294,
     295,   295,   295,   295,   295,   295,   297,   307,   318,   322,
     326,   330,   334,   338,   342,   346,   350,   354,   358,   362,
     366,   370,   374,   378,   382,   383,   384,   385,   389,   393,
     399,   403,   411,   415,   419,   424
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
     166,   -71,   -71,   -71,   -71,   -71,   -71,    20,   166,    67,
     -66,   -71,   434,   -12,     5,    11,   -71,   410,     9,   -71,
     -71,    73,    -4,   127,    93,   410,   410,   166,   378,    94,
     -71,   -71,   184,   212,   224,   225,   226,   -71,   410,   467,
      95,   -71,   -71,   467,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   158,   159,   279,   258,   -71,   237,   243,   161,   -71,
     -71,   -13,   410,   -71,   -71,    -7,   -71,   537,    15,   -11,
     -71,    41,   260,    41,   261,   -71,   313,   -71,   175,   196,
     253,   229,    58,   201,   345,   -71,   -71,   -71,   -71,   410,
     410,   410,   410,   410,   410,   410,   410,   410,   410,   410,
     410,   410,   -71,   410,   255,   259,   268,    19,    54,   -71,
      73,   -71,   382,   410,   -71,   -71,   410,   334,   272,   -71,
     -71,   -71,   -71,   361,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   363,   -71,   -71,   262,   548,   239,   239,   136,   136,
     136,   136,    -3,    -3,   -71,   -71,   -71,   328,   -71,   372,
     482,   293,   -71,   -71,   -71,   537,   341,   129,   288,   166,
     -71,   -71,   -71,   302,   305,   -71,   301,   304,   163,   -71,
     -71,   410,   -71,     6,    57,   254,   -16,   401,   -71,   -71,
     394,   410,    17,   288,    82,   396,   397,    58,   398,   402,
     157,    91,   400,   324,   -71,   406,   408,   409,   202,   290,
     298,   339,   413,   414,   417,   374,   337,   -71,   141,   338,
     -71,   420,   340,   176,   410,   -71,   -71,   -71,   -71,   -71,
     -71,   -71,   343,   344,   348,   360,   362,   365,   -71,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,    16,   -71,   366,   367,
     429,   -71,   457,   369,   370,   -71,   -71,   -71,   -71,   -71,
     -71,    62,   -71,   -71,   -71,   -71,   -71,   376,   -71,   -71,
     -71,   -71,   377,   461,   384,   393,   412,   415,   -71,   416,
     419,   -71,   423,   -71,   410,    41,   -71,   -71,   -71,   -71,
     -71,   -71,   -71,   157,   -71,   -71,   462,   489,   -71,   -71,
     -71,   202,   -71,   -71,   424,   -71,   -71,   -71,   -71,   -71,
     -71,   -71,   214,   -71,   -71,   425,   426,   -71,   -71,   -71,
     -71,   -71
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     8,     9,    11,    12,    13,    10,     0,     4,    19,
       0,     1,     0,     0,    17,    14,     7,     0,   148,   144,
     145,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     146,   147,     0,     0,     0,     0,     0,    48,     0,     0,
       0,     2,    34,    20,    23,    29,    28,    25,    24,    26,
      27,     0,     0,     0,   143,     6,     0,     0,   148,   141,
     143,     0,     0,    47,   151,     0,   154,   155,     0,     0,
      43,     0,   143,     0,     0,    50,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     3,    22,    46,    45,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    31,     0,     0,    15,     0,     0,     0,    42,
       0,    44,     0,     0,    40,    39,     0,    35,     0,    49,
      53,    69,    86,     0,   103,    97,    99,   100,   101,    98,
     102,     0,   142,    21,   139,   140,   137,   138,   133,   134,
     135,   136,   128,   129,   130,   131,   132,     0,    18,     0,
       0,     0,   149,   150,   152,   153,     0,     0,     0,     0,
      56,    72,    89,     0,     0,    32,     0,     0,     0,    33,
      41,     0,    36,     0,     0,     0,     0,     0,    52,    16,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    71,     0,     0,
      88,     0,     0,     0,     0,    37,     5,    92,    93,    94,
      95,    96,     0,     0,     0,     0,     0,     0,   104,   105,
     106,   107,   108,   109,   110,   111,     0,    68,     0,     0,
       0,    54,     0,     0,     0,   124,   125,   121,   120,   122,
     123,     0,    85,   112,   113,   114,   115,     0,   116,   117,
     118,   119,     0,     0,     0,     0,     0,     0,    70,     0,
       0,    87,     0,   126,     0,     0,    57,    58,    59,    60,
      61,    62,    63,     0,    64,    65,     0,     0,    73,    74,
      75,     0,    76,    77,     0,    79,    80,    82,    83,    91,
      90,    51,     0,    38,    67,     0,     0,    84,    78,   127,
      66,    81
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -71,   -71,   -71,   -71,   352,   487,    -6,   -71,   -71,   -70,
     476,     1,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,
     331,   238,   -71,   -71,   246,   -71,   -71,   -17,   222,   468,
     -71
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     7,    12,    40,     8,     9,    10,    15,    41,    42,
      43,    44,    45,    46,    47,    48,   160,   174,   194,   236,
      49,   161,   175,   207,   251,    50,   162,   176,   210,   222,
     131,   237,   257,   262,   252,    51,    52,    53,    60,    65,
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
      56,    61,   215,   216,    62,    57,   111,   282,    63,   112,
     283,   214,   151,   110,    39,   125,   126,   127,   128,   129,
     130,   238,   239,   168,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    69,    39,    97,
      98,    99,   100,   101,   182,   217,   218,   219,   220,   221,
     152,   269,   270,   290,   213,   193,   291,    13,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,    70,    77,    85,   275,     1,     2,
       3,     4,     5,     6,    78,   303,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,    79,   171,    54,   228,   229,   230,   231,   232,
     233,   234,   235,    64,    80,    81,    82,    72,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   302,   104,    87,
      88,    54,   103,   105,   116,    54,    62,   181,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     274,   118,   120,    64,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   245,   246,   247,
     248,   249,   250,   121,   132,   195,    54,   196,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   309,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   123,   153,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,    17,
     122,   148,   206,   158,   149,    18,    19,    20,    21,    22,
      23,    24,   150,    25,   159,    26,   253,   254,   255,   256,
     102,   163,   164,    28,    29,    30,    31,    32,   258,   259,
     260,   261,    17,   166,   169,    39,    17,    33,    58,    19,
      20,   154,    58,    19,    20,   177,   178,   179,   180,    34,
      35,    36,   211,   212,   119,   223,   224,   226,    30,    31,
     240,   227,    30,    31,    17,   241,   242,   243,   244,   165,
      58,    19,    20,   263,   264,   265,   266,   267,   268,   271,
     272,   286,   170,   273,   276,   277,    37,    38,    17,   278,
      30,    31,    39,   133,    18,    19,    20,    21,    22,    23,
      24,   279,    25,   280,    26,    27,   281,   284,   285,   287,
     288,   289,    28,    29,    30,    31,    32,   292,   293,    75,
      38,    17,   294,   305,    38,   295,    33,    18,    19,    20,
      21,    22,    23,    24,   296,    25,    17,    26,    34,    35,
      36,   167,    58,    19,    20,    28,    29,    30,    31,    32,
     306,   173,    38,   297,    74,    84,   298,   299,   225,    33,
     300,   304,    30,    31,   301,   308,   310,   311,     0,   107,
       0,    34,    35,    36,     0,    37,    38,   307,     0,     0,
       0,    39,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,     0,     0,    37,    38,
       0,     0,     0,     0,    39,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    38
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
     105,   102,   182,   183,   105,   104,   101,   101,   109,   104,
     104,   104,   103,   104,   107,    67,    68,    69,    70,    71,
      72,    40,    41,   150,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    20,   107,    13,
      14,    15,    16,    17,   171,    73,    74,    75,    76,    77,
     106,    20,    21,   101,   181,   108,   104,   173,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,   101,   101,   101,   214,    32,    33,
      34,    35,    36,    37,    20,   275,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    20,   104,    12,    78,    79,    80,    81,    82,
      83,    84,    85,    21,    20,    20,    20,    25,     9,    10,
      11,    12,    13,    14,    15,    16,    17,   274,    21,   101,
     101,    39,     4,    20,     4,    43,   105,   104,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
     104,    20,   107,    61,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    95,    96,    97,
      98,    99,   100,   107,   103,    51,    84,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,   103,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,   102,   110,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    14,
     107,   106,   108,    29,   105,    20,    21,    22,    23,    24,
      25,    26,   104,    28,   102,    30,    86,    87,    88,    89,
     101,    20,    19,    38,    39,    40,    41,    42,    90,    91,
      92,    93,    14,    21,   101,   107,    14,    52,    20,    21,
      22,    19,    20,    21,    22,   103,   101,   106,   104,    64,
      65,    66,    11,    19,   101,    19,    19,    19,    40,    41,
      20,    19,    40,    41,    14,   101,    20,    19,    19,   101,
      20,    21,    22,    94,    21,    21,    19,    63,   101,   101,
      20,    12,   101,   103,   101,   101,   101,   102,    14,   101,
      40,    41,   107,   108,    20,    21,    22,    23,    24,    25,
      26,   101,    28,   101,    30,    31,   101,   101,   101,    12,
     101,   101,    38,    39,    40,    41,    42,   101,   101,   101,
     102,    14,    21,    21,   102,   101,    52,    20,    21,    22,
      23,    24,    25,    26,   101,    28,    14,    30,    64,    65,
      66,    19,    20,    21,    22,    38,    39,    40,    41,    42,
      21,   159,   102,   101,    27,    39,   101,   101,   187,    52,
     101,   283,    40,    41,   101,   101,   101,   101,    -1,    61,
      -1,    64,    65,    66,    -1,   101,   102,   291,    -1,    -1,
      -1,   107,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    -1,   101,   102,
      -1,    -1,    -1,    -1,   107,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   102
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
     138,    11,    19,   147,   104,   119,   119,    73,    74,    75,
      76,    77,   139,    19,    19,   140,    19,    19,    78,    79,
      80,    81,    82,    83,    84,    85,   129,   141,    40,    41,
      20,   101,    20,    19,    19,    95,    96,    97,    98,    99,
     100,   134,   144,    86,    87,    88,    89,   142,    90,    91,
      92,    93,   143,    94,    21,    21,    19,    63,   101,    20,
      21,   101,    20,   103,   104,   147,   101,   101,   101,   101,
     101,   101,   101,   104,   101,   101,    12,    12,   101,   101,
     101,   104,   101,   101,    21,   101,   101,   101,   101,   101,
     101,   101,   147,   119,   141,    21,    21,   144,   101,   103,
     101,   101
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   110,   111,   112,   112,   113,   114,   114,   115,   115,
     115,   115,   115,   115,   116,   117,   117,   117,   117,   117,
     118,   119,   120,   120,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   122,
     122,   123,   124,   126,   125,   127,   127,   128,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   129,   129,   131,
     130,   132,   132,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   134,   134,   136,   135,   137,   137,
     138,   138,   139,   139,   139,   139,   139,   140,   140,   140,
     140,   140,   140,   140,   141,   141,   141,   141,   141,   141,
     141,   141,   142,   142,   142,   142,   143,   143,   143,   143,
     144,   144,   144,   144,   144,   144,   145,   146,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   148,   148,
     149,   149,   150,   150,   150,   150
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     3,     0,     7,     3,     2,     1,     1,
       1,     1,     1,     1,     2,     3,     6,     1,     4,     0,
       1,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     4,     5,     1,     3,     5,     7,     9,     3,
       3,     5,     3,     2,     3,     2,     2,     2,     1,     3,
       2,     8,     5,     0,     7,     2,     0,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     5,     3,     1,     0,
       7,     2,     0,     3,     3,     3,     3,     3,     4,     3,
       3,     5,     3,     3,     3,     1,     0,     7,     2,     0,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     8,    10,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     3,     1,     1,     1,     1,     1,     1,     4,
       3,     1,     3,     3,     1,     1
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
#line 1524 "parser.tab.cc"
    break;

  case 3: /* FUNCTION_LIST_RULE: FUNCTION_LIST_RULE FUNCTION_RULE ';'  */
#line 95 "scanner_parser/parser.yy"
                    {
                        (yyval.functions) = (yyvsp[-2].functions);
                        (yyval.functions)->push_back((yyvsp[-1].function));
                    }
#line 1533 "parser.tab.cc"
    break;

  case 4: /* FUNCTION_LIST_RULE: %empty  */
#line 100 "scanner_parser/parser.yy"
                    {
                        (yyval.functions) = new AST_FUNCTION_LIST_RULE();
                    }
#line 1541 "parser.tab.cc"
    break;

  case 5: /* FUNCTION_RULE: DEF ShortType IDENTIFIER '(' DECLARATION_STATEMENT_LIST_RULE ')' STATEMENT_BLOCK_RULE  */
#line 106 "scanner_parser/parser.yy"
                        {
	        	(yyval.function) = new AST_FUNCTION_RULE((yyvsp[-5].type),(yyvsp[-4].string_val),(yyvsp[-2].decl_block),(yyvsp[0].block_statement));
	      		}
#line 1549 "parser.tab.cc"
    break;

  case 6: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';'  */
#line 111 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back((yyvsp[-1].decl_block));
                       }
#line 1558 "parser.tab.cc"
    break;

  case 7: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_RULE ';'  */
#line 117 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-1].decl_block);
                       }
#line 1566 "parser.tab.cc"
    break;

  case 8: /* ShortType: INT  */
#line 124 "scanner_parser/parser.yy"
               {(yyval.type)=ShortType::Int;}
#line 1572 "parser.tab.cc"
    break;

  case 9: /* ShortType: FLOAT  */
#line 125 "scanner_parser/parser.yy"
              {(yyval.type)=ShortType::Float;}
#line 1578 "parser.tab.cc"
    break;

  case 10: /* ShortType: DOUBLE  */
#line 125 "scanner_parser/parser.yy"
                                              {(yyval.type)=ShortType::Float;}
#line 1584 "parser.tab.cc"
    break;

  case 11: /* ShortType: STRING  */
#line 126 "scanner_parser/parser.yy"
               {(yyval.type)=ShortType::String;}
#line 1590 "parser.tab.cc"
    break;

  case 12: /* ShortType: VOID  */
#line 127 "scanner_parser/parser.yy"
             {(yyval.type)=ShortType::Void;}
#line 1596 "parser.tab.cc"
    break;

  case 13: /* ShortType: BOOL  */
#line 128 "scanner_parser/parser.yy"
            {(yyval.type)=ShortType::Boolean;}
#line 1602 "parser.tab.cc"
    break;

  case 14: /* DECLARATION_STATEMENT_RULE: ShortType DECLARATION_VARIABLE_LIST_RULE  */
#line 132 "scanner_parser/parser.yy"
                   {
                       //fprintf(bison_output, "DECLARATION_STATEMENT_RULE\n");
                       (yyval.decl_block) = (yyvsp[0].decl_block);
                   }
#line 1611 "parser.tab.cc"
    break;

  case 15: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER  */
#line 140 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1620 "parser.tab.cc"
    break;

  case 16: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']'  */
#line 145 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-5].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1629 "parser.tab.cc"
    break;

  case 17: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER  */
#line 150 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1638 "parser.tab.cc"
    break;

  case 18: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER '[' INT_LITERAL ']'  */
#line 155 "scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1647 "parser.tab.cc"
    break;

  case 19: /* DECLARATION_VARIABLE_LIST_RULE: %empty  */
#line 159 "scanner_parser/parser.yy"
                           {(yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();}
#line 1653 "parser.tab.cc"
    break;

  case 20: /* LOGIC_BLOCK: STATEMENT_LIST_RULE  */
#line 164 "scanner_parser/parser.yy"
               {
                   //fprintf(bison_output, "LOGIC_BLOCK\n");
                   (yyval.code_block) = new AST_LOGIC_BLOCK((yyvsp[0].block_statement));
               }
#line 1662 "parser.tab.cc"
    break;

  case 21: /* STATEMENT_BLOCK_RULE: '{' STATEMENT_LIST_RULE '}'  */
#line 172 "scanner_parser/parser.yy"
                    {
                        //fprintf(bison_output, "STATEMENT_BLOCK_RULE\n");
                        (yyval.block_statement) = (yyvsp[-1].block_statement);
                    }
#line 1671 "parser.tab.cc"
    break;

  case 22: /* STATEMENT_LIST_RULE: STATEMENT_LIST_RULE STATEMENT_RULE  */
#line 180 "scanner_parser/parser.yy"
                   {
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1679 "parser.tab.cc"
    break;

  case 23: /* STATEMENT_LIST_RULE: STATEMENT_RULE  */
#line 184 "scanner_parser/parser.yy"
                   {
                       (yyval.block_statement) = new AST_STATEMENTS_BLOCK();
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1688 "parser.tab.cc"
    break;

  case 24: /* STATEMENT_RULE: MODEL_DECLARATION  */
#line 190 "scanner_parser/parser.yy"
                                     { (yyval.statement) = (yyvsp[0].model_decl); }
#line 1694 "parser.tab.cc"
    break;

  case 25: /* STATEMENT_RULE: TENSOR_DECLARATION  */
#line 191 "scanner_parser/parser.yy"
                              { (yyval.statement) = (yyvsp[0].tensor_decl); }
#line 1700 "parser.tab.cc"
    break;

  case 26: /* STATEMENT_RULE: GREENAI_CONTRACT  */
#line 192 "scanner_parser/parser.yy"
                            { (yyval.statement) = (yyvsp[0].greenai_contract); }
#line 1706 "parser.tab.cc"
    break;

  case 27: /* STATEMENT_RULE: GREENAI_MEASUREMENT  */
#line 193 "scanner_parser/parser.yy"
                               { (yyval.statement) = (yyvsp[0].greenai_measure); }
#line 1712 "parser.tab.cc"
    break;

  case 28: /* STATEMENT_RULE: INFER_STATEMENT  */
#line 194 "scanner_parser/parser.yy"
                           { (yyval.statement) = (yyvsp[0].infer_statement); }
#line 1718 "parser.tab.cc"
    break;

  case 29: /* STATEMENT_RULE: RETURN_STATEMENT  */
#line 195 "scanner_parser/parser.yy"
                            { (yyval.statement) = (yyvsp[0].return_statement); }
#line 1724 "parser.tab.cc"
    break;

  case 30: /* STATEMENT_RULE: CONTINUE ';'  */
#line 196 "scanner_parser/parser.yy"
                        { (yyval.statement) = new AST_CONTINUE(); }
#line 1730 "parser.tab.cc"
    break;

  case 31: /* STATEMENT_RULE: EXPRESSION_RULE ';'  */
#line 198 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE((yyvsp[-1].expression));
              }
#line 1738 "parser.tab.cc"
    break;

  case 32: /* STATEMENT_RULE: VARIABLE_RULE '=' EXPRESSION_RULE ';'  */
#line 202 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_ASSIGNMENT_RULE((yyvsp[-3].variable), (yyvsp[-1].expression));
              }
#line 1746 "parser.tab.cc"
    break;

  case 33: /* STATEMENT_RULE: IDENTIFIER '(' READ_VARIABLE_LIST_RULE ')' ';'  */
#line 207 "scanner_parser/parser.yy"
              {
	       (yyval.statement) = new AST_FUNCTION_CALL_RULE((yyvsp[-4].string_val),(yyvsp[-2].read_statement));
	      }
#line 1754 "parser.tab.cc"
    break;

  case 34: /* STATEMENT_RULE: STATEMENT_BLOCK_RULE  */
#line 211 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[0].block_statement);
              }
#line 1762 "parser.tab.cc"
    break;

  case 35: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 215 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_STATEMENT((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1770 "parser.tab.cc"
    break;

  case 36: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE  */
#line 219 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_ELSE_STATEMENT((yyvsp[-3].expression), (yyvsp[-2].block_statement), (yyvsp[0].block_statement));
              }
#line 1778 "parser.tab.cc"
    break;

  case 37: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 223 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-5].variable), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1786 "parser.tab.cc"
    break;

  case 38: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 227 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-7].variable), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1794 "parser.tab.cc"
    break;

  case 39: /* STATEMENT_RULE: LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 231 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_WHILE_LOOP_STATEMENT_RULE((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1802 "parser.tab.cc"
    break;

  case 40: /* STATEMENT_RULE: GOTO IDENTIFIER ';'  */
#line 235 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE(string((yyvsp[-1].string_val)));
              }
#line 1810 "parser.tab.cc"
    break;

  case 41: /* STATEMENT_RULE: GOTO IDENTIFIER IF EXPRESSION_RULE ';'  */
#line 239 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE((yyvsp[-1].expression), string((yyvsp[-3].string_val)));
              }
#line 1818 "parser.tab.cc"
    break;

  case 42: /* STATEMENT_RULE: READ READ_VARIABLE_LIST_RULE ';'  */
#line 243 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].read_statement);
              }
#line 1826 "parser.tab.cc"
    break;

  case 43: /* STATEMENT_RULE: BREAK ';'  */
#line 246 "scanner_parser/parser.yy"
                     {(yyval.statement) = new AST_BREAK();}
#line 1832 "parser.tab.cc"
    break;

  case 44: /* STATEMENT_RULE: PRINT PRINT_VARIABLE_LIST_RULE ';'  */
#line 248 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].print_statement);
              }
#line 1840 "parser.tab.cc"
    break;

  case 45: /* STATEMENT_RULE: GREENAI_REPORT_RULE ';'  */
#line 252 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].greenai_report);
              }
#line 1848 "parser.tab.cc"
    break;

  case 46: /* STATEMENT_RULE: AI_INFER_RULE ';'  */
#line 256 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].ai_infer);
              }
#line 1856 "parser.tab.cc"
    break;

  case 47: /* STATEMENT_RULE: IDENTIFIER ':'  */
#line 260 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_LABEL_RULE(string((yyvsp[-1].string_val)));
              }
#line 1864 "parser.tab.cc"
    break;

  case 48: /* STATEMENT_RULE: ';'  */
#line 264 "scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE(new AST_LITERAL(1));
              }
#line 1872 "parser.tab.cc"
    break;

  case 49: /* RETURN_STATEMENT: RETURN EXPRESSION_RULE ';'  */
#line 270 "scanner_parser/parser.yy"
                                             { (yyval.return_statement) = new AST_RETURN_STATEMENT((yyvsp[-1].expression)); }
#line 1878 "parser.tab.cc"
    break;

  case 50: /* RETURN_STATEMENT: RETURN ';'  */
#line 270 "scanner_parser/parser.yy"
                                                                                                 { (yyval.return_statement) = new AST_RETURN_STATEMENT(); }
#line 1884 "parser.tab.cc"
    break;

  case 51: /* INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' GREATER IDENTIFIER ';'  */
#line 272 "scanner_parser/parser.yy"
                                                                            { (yyval.infer_statement) = new AST_INFER_STATEMENT(string((yyvsp[-6].string_val)), string((yyvsp[-4].string_val)), string((yyvsp[-1].string_val))); }
#line 1890 "parser.tab.cc"
    break;

  case 52: /* TENSOR_DECLARATION: TENSOR IDENTIFIER PRECISION_NAME STRING_LITERAL ';'  */
#line 274 "scanner_parser/parser.yy"
                                                                        { TensorDeclarationData d; d.name=(yyvsp[-3].string_val); d.element_type=(yyvsp[-2].string_val); d.shape_csv=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); d.dynamic=(d.shape_csv=="dynamic"); d.rank= d.dynamic?0:1; long long total=1; if(!d.dynamic){ d.rank=0; size_t start=0; while(start<d.shape_csv.size()){ size_t pos=d.shape_csv.find(',',start); string part=d.shape_csv.substr(start,pos==string::npos?string::npos:pos-start); total*= atoll(part.c_str()); d.rank++; if(pos==string::npos) break; start=pos+1; }} d.total_elements=total; (yyval.tensor_decl)=new AST_TENSOR_DECLARATION(d); }
#line 1896 "parser.tab.cc"
    break;

  case 53: /* $@1: %empty  */
#line 276 "scanner_parser/parser.yy"
                                        { current_model=ModelDeclarationData(); current_model.name=(yyvsp[-1].string_val); }
#line 1902 "parser.tab.cc"
    break;

  case 54: /* MODEL_DECLARATION: MODEL IDENTIFIER '{' $@1 MODEL_FIELD_LIST '}' ';'  */
#line 276 "scanner_parser/parser.yy"
                                                                                                                                  { (yyval.model_decl) = new AST_MODEL_DECLARATION(current_model); }
#line 1908 "parser.tab.cc"
    break;

  case 57: /* MODEL_FIELD: FORMAT FORMAT_NAME ';'  */
#line 278 "scanner_parser/parser.yy"
                                    { current_model.format=(yyvsp[-1].string_val); }
#line 1914 "parser.tab.cc"
    break;

  case 58: /* MODEL_FIELD: PATH STRING_LITERAL ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                           { current_model.path=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1920 "parser.tab.cc"
    break;

  case 59: /* MODEL_FIELD: TASK STRING_LITERAL ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                      { current_model.task=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1926 "parser.tab.cc"
    break;

  case 60: /* MODEL_FIELD: PRECISION PRECISION_NAME ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                      { current_model.precision=(yyvsp[-1].string_val); }
#line 1932 "parser.tab.cc"
    break;

  case 61: /* MODEL_FIELD: INPUT_SHAPE STRING_LITERAL ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                       { current_model.input_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1938 "parser.tab.cc"
    break;

  case 62: /* MODEL_FIELD: OUTPUT_SHAPE STRING_LITERAL ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                 { current_model.output_shape=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 1944 "parser.tab.cc"
    break;

  case 64: /* MODEL_FIELD: COMPACT TRUE ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   { current_model.compact=true; }
#line 1950 "parser.tab.cc"
    break;

  case 65: /* MODEL_FIELD: COMPACT FALSE ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       { current_model.compact=false; }
#line 1956 "parser.tab.cc"
    break;

  case 66: /* MODEL_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 278 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_model.has_quality_guardrail=true; current_model.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 1962 "parser.tab.cc"
    break;

  case 67: /* BACKEND_LIST: BACKEND_LIST ',' BACKEND_NAME  */
#line 279 "scanner_parser/parser.yy"
                                            { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 1968 "parser.tab.cc"
    break;

  case 68: /* BACKEND_LIST: BACKEND_NAME  */
#line 279 "scanner_parser/parser.yy"
                                                                                                               { current_model.backend_preference.push_back((yyvsp[0].string_val)); }
#line 1974 "parser.tab.cc"
    break;

  case 69: /* $@2: %empty  */
#line 281 "scanner_parser/parser.yy"
                                                    { current_contract=GreenAIContractData(); current_contract.name=(yyvsp[-1].string_val); }
#line 1980 "parser.tab.cc"
    break;

  case 70: /* GREENAI_CONTRACT: GREENAI_CONTRACT_T IDENTIFIER '{' $@2 CONTRACT_FIELD_LIST '}' ';'  */
#line 281 "scanner_parser/parser.yy"
                                                                                                                                                      { (yyval.greenai_contract) = new AST_GREENAI_CONTRACT(current_contract); }
#line 1986 "parser.tab.cc"
    break;

  case 73: /* CONTRACT_FIELD: FUNCTIONAL_UNIT STRING_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                   { current_contract.functional_unit=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_functional_unit=true; }
#line 1992 "parser.tab.cc"
    break;

  case 74: /* CONTRACT_FIELD: SUCCESS_CRITERIA STRING_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                   { current_contract.success_criteria=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); current_contract.has_success_criteria=true; }
#line 1998 "parser.tab.cc"
    break;

  case 75: /* CONTRACT_FIELD: BOUNDARY BOUNDARY_LIST ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                            { current_contract.has_boundary=true; }
#line 2004 "parser.tab.cc"
    break;

  case 76: /* CONTRACT_FIELD: MEASUREMENT_QUALITY MQ_NAME ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                      { current_contract.measurement_quality=(yyvsp[-1].string_val); current_contract.has_mq=true; }
#line 2010 "parser.tab.cc"
    break;

  case 77: /* CONTRACT_FIELD: DATA_QUALITY DQ_NAME ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.data_quality=(yyvsp[-1].string_val); current_contract.has_dq=true; }
#line 2016 "parser.tab.cc"
    break;

  case 78: /* CONTRACT_FIELD: CARBON_FACTOR LOCATION INT_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         { current_contract.carbon_factor_scope="location"; current_contract.carbon_factor=(yyvsp[-1].int_val); current_contract.has_carbon_factor=true; }
#line 2022 "parser.tab.cc"
    break;

  case 79: /* CONTRACT_FIELD: ENERGY_BUDGET_J INT_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { current_contract.energy_budget_j=(yyvsp[-1].int_val); }
#line 2028 "parser.tab.cc"
    break;

  case 80: /* CONTRACT_FIELD: CARBON_BUDGET_GCO2E INT_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           { current_contract.carbon_budget_gco2e=(yyvsp[-1].int_val); }
#line 2034 "parser.tab.cc"
    break;

  case 81: /* CONTRACT_FIELD: QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { current_contract.has_quality_guardrail=true; current_contract.quality_guardrail={string((yyvsp[-3].string_val)),">=",(double)(yyvsp[-1].int_val)}; }
#line 2040 "parser.tab.cc"
    break;

  case 82: /* CONTRACT_FIELD: EVIDENCE_RETENTION STRING_LITERAL ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  { current_contract.evidence_retention=string((yyvsp[-1].string_val)).substr(1,string((yyvsp[-1].string_val)).size()-2); }
#line 2046 "parser.tab.cc"
    break;

  case 83: /* CONTRACT_FIELD: CLAIMS_MODE EVIDENCE_ONLY ';'  */
#line 283 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    { current_contract.claims_mode="evidence_only"; }
#line 2052 "parser.tab.cc"
    break;

  case 84: /* BOUNDARY_LIST: BOUNDARY_LIST ',' BOUNDARY_NAME  */
#line 284 "scanner_parser/parser.yy"
                                               { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2058 "parser.tab.cc"
    break;

  case 85: /* BOUNDARY_LIST: BOUNDARY_NAME  */
#line 284 "scanner_parser/parser.yy"
                                                                                                            { current_contract.boundary.push_back((yyvsp[0].string_val)); }
#line 2064 "parser.tab.cc"
    break;

  case 86: /* $@3: %empty  */
#line 286 "scanner_parser/parser.yy"
                                                    { current_measure=GreenAIMeasurementData(); current_measure.workload=(yyvsp[-1].string_val); }
#line 2070 "parser.tab.cc"
    break;

  case 87: /* GREENAI_MEASUREMENT: GREENAI_MEASURE IDENTIFIER '{' $@3 MEASURE_FIELD_LIST '}' ';'  */
#line 286 "scanner_parser/parser.yy"
                                                                                                                                                          { (yyval.greenai_measure) = new AST_GREENAI_MEASUREMENT(current_measure); }
#line 2076 "parser.tab.cc"
    break;

  case 90: /* MEASURE_FIELD: IDENTIFIER INT_LITERAL ';'  */
#line 288 "scanner_parser/parser.yy"
                                          { string n=(yyvsp[-2].string_val); if(n=="inferences") current_measure.inferences=(yyvsp[-1].int_val); else if(n=="watts") current_measure.watts=(yyvsp[-1].int_val); else if(n=="seconds") current_measure.seconds=(yyvsp[-1].int_val); }
#line 2082 "parser.tab.cc"
    break;

  case 91: /* MEASURE_FIELD: IDENTIFIER IDENTIFIER ';'  */
#line 288 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                          { if(string((yyvsp[-2].string_val))=="backend") current_measure.backend=(yyvsp[-1].string_val); }
#line 2088 "parser.tab.cc"
    break;

  case 92: /* FORMAT_NAME: ONNX  */
#line 290 "scanner_parser/parser.yy"
                  {(yyval.string_val)=(char*)"onnx";}
#line 2094 "parser.tab.cc"
    break;

  case 93: /* FORMAT_NAME: ENGINE  */
#line 290 "scanner_parser/parser.yy"
                                               {(yyval.string_val)=(char*)"engine";}
#line 2100 "parser.tab.cc"
    break;

  case 94: /* FORMAT_NAME: TORCHSCRIPT  */
#line 290 "scanner_parser/parser.yy"
                                                                                   {(yyval.string_val)=(char*)"torchscript";}
#line 2106 "parser.tab.cc"
    break;

  case 95: /* FORMAT_NAME: OPENVINO_IR  */
#line 290 "scanner_parser/parser.yy"
                                                                                                                            {(yyval.string_val)=(char*)"openvino_ir";}
#line 2112 "parser.tab.cc"
    break;

  case 96: /* FORMAT_NAME: GGUF  */
#line 290 "scanner_parser/parser.yy"
                                                                                                                                                              {(yyval.string_val)=(char*)"gguf";}
#line 2118 "parser.tab.cc"
    break;

  case 97: /* PRECISION_NAME: INT8  */
#line 291 "scanner_parser/parser.yy"
                     {(yyval.string_val)=(char*)"int8";}
#line 2124 "parser.tab.cc"
    break;

  case 98: /* PRECISION_NAME: INT4  */
#line 291 "scanner_parser/parser.yy"
                                                {(yyval.string_val)=(char*)"int4";}
#line 2130 "parser.tab.cc"
    break;

  case 99: /* PRECISION_NAME: FP16  */
#line 291 "scanner_parser/parser.yy"
                                                                           {(yyval.string_val)=(char*)"fp16";}
#line 2136 "parser.tab.cc"
    break;

  case 100: /* PRECISION_NAME: FP32  */
#line 291 "scanner_parser/parser.yy"
                                                                                                      {(yyval.string_val)=(char*)"fp32";}
#line 2142 "parser.tab.cc"
    break;

  case 101: /* PRECISION_NAME: BF16  */
#line 291 "scanner_parser/parser.yy"
                                                                                                                                 {(yyval.string_val)=(char*)"bf16";}
#line 2148 "parser.tab.cc"
    break;

  case 102: /* PRECISION_NAME: FP64  */
#line 291 "scanner_parser/parser.yy"
                                                                                                                                                            {(yyval.string_val)=(char*)"fp64";}
#line 2154 "parser.tab.cc"
    break;

  case 103: /* PRECISION_NAME: FLOAT  */
#line 291 "scanner_parser/parser.yy"
                                                                                                                                                                                        {(yyval.string_val)=(char*)"float";}
#line 2160 "parser.tab.cc"
    break;

  case 104: /* BACKEND_NAME: TENSORRT  */
#line 292 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"tensorrt";}
#line 2166 "parser.tab.cc"
    break;

  case 105: /* BACKEND_NAME: ONNXRUNTIME_TENSORRT  */
#line 292 "scanner_parser/parser.yy"
                                                                      {(yyval.string_val)=(char*)"onnxruntime_tensorrt";}
#line 2172 "parser.tab.cc"
    break;

  case 106: /* BACKEND_NAME: ONNXRUNTIME_CUDA  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                             {(yyval.string_val)=(char*)"onnxruntime_cuda";}
#line 2178 "parser.tab.cc"
    break;

  case 107: /* BACKEND_NAME: ONNXRUNTIME_CPU  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                               {(yyval.string_val)=(char*)"onnxruntime_cpu";}
#line 2184 "parser.tab.cc"
    break;

  case 108: /* BACKEND_NAME: OPENVINO  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                                                                         {(yyval.string_val)=(char*)"openvino";}
#line 2190 "parser.tab.cc"
    break;

  case 109: /* BACKEND_NAME: LIBTORCH  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                            {(yyval.string_val)=(char*)"libtorch";}
#line 2196 "parser.tab.cc"
    break;

  case 110: /* BACKEND_NAME: LLAMACPP  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                               {(yyval.string_val)=(char*)"llamacpp";}
#line 2202 "parser.tab.cc"
    break;

  case 111: /* BACKEND_NAME: FALLBACK  */
#line 292 "scanner_parser/parser.yy"
                                                                                                                                                                                                                                                                                                                                  {(yyval.string_val)=(char*)"fallback";}
#line 2208 "parser.tab.cc"
    break;

  case 112: /* MQ_NAME: MQ1  */
#line 293 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"MQ1";}
#line 2214 "parser.tab.cc"
    break;

  case 113: /* MQ_NAME: MQ2  */
#line 293 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"MQ2";}
#line 2220 "parser.tab.cc"
    break;

  case 114: /* MQ_NAME: MQ3  */
#line 293 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"MQ3";}
#line 2226 "parser.tab.cc"
    break;

  case 115: /* MQ_NAME: MQ4  */
#line 293 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"MQ4";}
#line 2232 "parser.tab.cc"
    break;

  case 116: /* DQ_NAME: DQ1  */
#line 294 "scanner_parser/parser.yy"
             {(yyval.string_val)=(char*)"DQ1";}
#line 2238 "parser.tab.cc"
    break;

  case 117: /* DQ_NAME: DQ2  */
#line 294 "scanner_parser/parser.yy"
                                      {(yyval.string_val)=(char*)"DQ2";}
#line 2244 "parser.tab.cc"
    break;

  case 118: /* DQ_NAME: DQ3  */
#line 294 "scanner_parser/parser.yy"
                                                               {(yyval.string_val)=(char*)"DQ3";}
#line 2250 "parser.tab.cc"
    break;

  case 119: /* DQ_NAME: DQ4  */
#line 294 "scanner_parser/parser.yy"
                                                                                        {(yyval.string_val)=(char*)"DQ4";}
#line 2256 "parser.tab.cc"
    break;

  case 120: /* BOUNDARY_NAME: COMPUTE  */
#line 295 "scanner_parser/parser.yy"
                       {(yyval.string_val)=(char*)"compute";}
#line 2262 "parser.tab.cc"
    break;

  case 121: /* BOUNDARY_NAME: ACCELERATOR  */
#line 295 "scanner_parser/parser.yy"
                                                            {(yyval.string_val)=(char*)"accelerator";}
#line 2268 "parser.tab.cc"
    break;

  case 122: /* BOUNDARY_NAME: STORAGE  */
#line 295 "scanner_parser/parser.yy"
                                                                                                 {(yyval.string_val)=(char*)"storage";}
#line 2274 "parser.tab.cc"
    break;

  case 123: /* BOUNDARY_NAME: NETWORK  */
#line 295 "scanner_parser/parser.yy"
                                                                                                                                  {(yyval.string_val)=(char*)"network";}
#line 2280 "parser.tab.cc"
    break;

  case 124: /* BOUNDARY_NAME: CI_CD  */
#line 295 "scanner_parser/parser.yy"
                                                                                                                                                                 {(yyval.string_val)=(char*)"ci_cd";}
#line 2286 "parser.tab.cc"
    break;

  case 125: /* BOUNDARY_NAME: THIRDPARTY  */
#line 295 "scanner_parser/parser.yy"
                                                                                                                                                                                                   {(yyval.string_val)=(char*)"thirdparty";}
#line 2292 "parser.tab.cc"
    break;

  case 126: /* AI_INFER_RULE: IDENTIFIER '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')'  */
#line 298 "scanner_parser/parser.yy"
              {
                  if ((string((yyvsp[-7].string_val)) != "ai_infer" && string((yyvsp[-7].string_val)) != "aiinfer")) {
                      yyerror((char *)"expected ai_infer builtin");
                      YYERROR;
                  }
                  (yyval.ai_infer) = new AST_AI_INFER_RULE(string((yyvsp[-5].string_val)), string((yyvsp[-3].string_val)), string((yyvsp[-1].string_val)));
              }
#line 2304 "parser.tab.cc"
    break;

  case 127: /* GREENAI_REPORT_RULE: IDENTIFIER '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')'  */
#line 308 "scanner_parser/parser.yy"
              {
                  if (string((yyvsp[-9].string_val)) != "greenai") {
                      yyerror((char *)"expected greenai report builtin");
                      YYERROR;
                  }
                  (yyval.greenai_report) = new AST_GREENAI_REPORT_RULE(string((yyvsp[-7].string_val)), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression));
              }
#line 2316 "parser.tab.cc"
    break;

  case 128: /* EXPRESSION_RULE: EXPRESSION_RULE '+' EXPRESSION_RULE  */
#line 319 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "+");
               }
#line 2324 "parser.tab.cc"
    break;

  case 129: /* EXPRESSION_RULE: EXPRESSION_RULE '-' EXPRESSION_RULE  */
#line 323 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "-");
               }
#line 2332 "parser.tab.cc"
    break;

  case 130: /* EXPRESSION_RULE: EXPRESSION_RULE '*' EXPRESSION_RULE  */
#line 327 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "*");
               }
#line 2340 "parser.tab.cc"
    break;

  case 131: /* EXPRESSION_RULE: EXPRESSION_RULE '/' EXPRESSION_RULE  */
#line 331 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "/");
               }
#line 2348 "parser.tab.cc"
    break;

  case 132: /* EXPRESSION_RULE: EXPRESSION_RULE '%' EXPRESSION_RULE  */
#line 335 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "%");
               }
#line 2356 "parser.tab.cc"
    break;

  case 133: /* EXPRESSION_RULE: EXPRESSION_RULE LESS EXPRESSION_RULE  */
#line 339 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<");
               }
#line 2364 "parser.tab.cc"
    break;

  case 134: /* EXPRESSION_RULE: EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE  */
#line 343 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<=");
               }
#line 2372 "parser.tab.cc"
    break;

  case 135: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER EXPRESSION_RULE  */
#line 347 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">");
               }
#line 2380 "parser.tab.cc"
    break;

  case 136: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE  */
#line 351 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">=");
               }
#line 2388 "parser.tab.cc"
    break;

  case 137: /* EXPRESSION_RULE: EXPRESSION_RULE EQUAL EXPRESSION_RULE  */
#line 355 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "==");
               }
#line 2396 "parser.tab.cc"
    break;

  case 138: /* EXPRESSION_RULE: EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE  */
#line 359 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "!=");
               }
#line 2404 "parser.tab.cc"
    break;

  case 139: /* EXPRESSION_RULE: EXPRESSION_RULE OR EXPRESSION_RULE  */
#line 363 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "||");
               }
#line 2412 "parser.tab.cc"
    break;

  case 140: /* EXPRESSION_RULE: EXPRESSION_RULE AND EXPRESSION_RULE  */
#line 367 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "&&");
               }
#line 2420 "parser.tab.cc"
    break;

  case 141: /* EXPRESSION_RULE: '-' EXPRESSION_RULE  */
#line 371 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_UNARY_EXPRESSION_RULE((yyvsp[0].expression), "-");
               }
#line 2428 "parser.tab.cc"
    break;

  case 142: /* EXPRESSION_RULE: '(' EXPRESSION_RULE ')'  */
#line 375 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[-1].expression);
               }
#line 2436 "parser.tab.cc"
    break;

  case 143: /* EXPRESSION_RULE: VARIABLE_RULE  */
#line 379 "scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[0].variable);
               }
#line 2444 "parser.tab.cc"
    break;

  case 144: /* EXPRESSION_RULE: INT_LITERAL  */
#line 382 "scanner_parser/parser.yy"
                           { (yyval.expression) = new AST_LITERAL((yyvsp[0].int_val)); }
#line 2450 "parser.tab.cc"
    break;

  case 145: /* EXPRESSION_RULE: FLOAT_LITERAL  */
#line 383 "scanner_parser/parser.yy"
                             { (yyval.expression) = new AST_FLOAT_LITERAL((yyvsp[0].float_val)); }
#line 2456 "parser.tab.cc"
    break;

  case 146: /* EXPRESSION_RULE: TRUE  */
#line 384 "scanner_parser/parser.yy"
                    { (yyval.expression) = new AST_BOOL_LITERAL(true); }
#line 2462 "parser.tab.cc"
    break;

  case 147: /* EXPRESSION_RULE: FALSE  */
#line 385 "scanner_parser/parser.yy"
                     { (yyval.expression) = new AST_BOOL_LITERAL(false); }
#line 2468 "parser.tab.cc"
    break;

  case 148: /* VARIABLE_RULE: IDENTIFIER  */
#line 390 "scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_SIMPLE_VARIABLE(string((yyvsp[0].string_val)));
             }
#line 2476 "parser.tab.cc"
    break;

  case 149: /* VARIABLE_RULE: IDENTIFIER '[' EXPRESSION_RULE ']'  */
#line 394 "scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_ARRAY_VARIABLE(string((yyvsp[-3].string_val)), (yyvsp[-1].expression));
             }
#line 2484 "parser.tab.cc"
    break;

  case 150: /* READ_VARIABLE_LIST_RULE: READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE  */
#line 400 "scanner_parser/parser.yy"
                       {
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 2492 "parser.tab.cc"
    break;

  case 151: /* READ_VARIABLE_LIST_RULE: VARIABLE_RULE  */
#line 404 "scanner_parser/parser.yy"
                       {
                           (yyval.read_statement) = new AST_READ_RULE();
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 2501 "parser.tab.cc"
    break;

  case 152: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' STRING_LITERAL  */
#line 412 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 2509 "parser.tab.cc"
    break;

  case 153: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE  */
#line 416 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 2517 "parser.tab.cc"
    break;

  case 154: /* PRINT_VARIABLE_LIST_RULE: STRING_LITERAL  */
#line 420 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 2526 "parser.tab.cc"
    break;

  case 155: /* PRINT_VARIABLE_LIST_RULE: EXPRESSION_RULE  */
#line 425 "scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 2535 "parser.tab.cc"
    break;


#line 2539 "parser.tab.cc"

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

#line 432 "scanner_parser/parser.yy"



void yyerror (char const *s)
{
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf (stderr, "%s\n", s);
}
