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
#line 1 "./scanner_parser/parser.yy"


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "./ast/AST.h"
#include "./ast/AST.cpp"

  using namespace std;

  extern "C" int yylex();
  extern "C" int yyparse();
  extern "C" void yyerror(char *s);
  extern "C" int yywrap(void){return 1;}
  extern "C" int yylineno;
  extern "C" int yydebug;
  extern union _NODE_ yylval;
  extern class AST_PROGRAM * main_program;



#line 93 "parser.tab.cc"

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
  YYSYMBOL_READ = 22,                      /* READ  */
  YYSYMBOL_PRINT = 23,                     /* PRINT  */
  YYSYMBOL_GOTO = 24,                      /* GOTO  */
  YYSYMBOL_BREAK = 25,                     /* BREAK  */
  YYSYMBOL_WHILE = 26,                     /* WHILE  */
  YYSYMBOL_LOOP = 27,                      /* LOOP  */
  YYSYMBOL_ELSE = 28,                      /* ELSE  */
  YYSYMBOL_IF = 29,                        /* IF  */
  YYSYMBOL_DEF = 30,                       /* DEF  */
  YYSYMBOL_INT = 31,                       /* INT  */
  YYSYMBOL_FLOAT = 32,                     /* FLOAT  */
  YYSYMBOL_STRING = 33,                    /* STRING  */
  YYSYMBOL_VOID = 34,                      /* VOID  */
  YYSYMBOL_BOOL = 35,                      /* BOOL  */
  YYSYMBOL_36_ = 36,                       /* ';'  */
  YYSYMBOL_37_ = 37,                       /* '('  */
  YYSYMBOL_38_ = 38,                       /* ')'  */
  YYSYMBOL_39_ = 39,                       /* ','  */
  YYSYMBOL_40_ = 40,                       /* '['  */
  YYSYMBOL_41_ = 41,                       /* ']'  */
  YYSYMBOL_42_ = 42,                       /* '{'  */
  YYSYMBOL_43_ = 43,                       /* '}'  */
  YYSYMBOL_44_ = 44,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 45,                  /* $accept  */
  YYSYMBOL_PROGRAMME_RULE = 46,            /* PROGRAMME_RULE  */
  YYSYMBOL_FUNCTION_LIST_RULE = 47,        /* FUNCTION_LIST_RULE  */
  YYSYMBOL_FUNCTION_RULE = 48,             /* FUNCTION_RULE  */
  YYSYMBOL_DECLARATION_STATEMENT_LIST_RULE = 49, /* DECLARATION_STATEMENT_LIST_RULE  */
  YYSYMBOL_ShortType = 50,                 /* ShortType  */
  YYSYMBOL_DECLARATION_STATEMENT_RULE = 51, /* DECLARATION_STATEMENT_RULE  */
  YYSYMBOL_DECLARATION_VARIABLE_LIST_RULE = 52, /* DECLARATION_VARIABLE_LIST_RULE  */
  YYSYMBOL_LOGIC_BLOCK = 53,               /* LOGIC_BLOCK  */
  YYSYMBOL_STATEMENT_BLOCK_RULE = 54,      /* STATEMENT_BLOCK_RULE  */
  YYSYMBOL_STATEMENT_LIST_RULE = 55,       /* STATEMENT_LIST_RULE  */
  YYSYMBOL_STATEMENT_RULE = 56,            /* STATEMENT_RULE  */
  YYSYMBOL_AI_INFER_RULE = 57,             /* AI_INFER_RULE  */
  YYSYMBOL_GREENAI_REPORT_RULE = 58,       /* GREENAI_REPORT_RULE  */
  YYSYMBOL_EXPRESSION_RULE = 59,           /* EXPRESSION_RULE  */
  YYSYMBOL_VARIABLE_RULE = 60,             /* VARIABLE_RULE  */
  YYSYMBOL_READ_VARIABLE_LIST_RULE = 61,   /* READ_VARIABLE_LIST_RULE  */
  YYSYMBOL_PRINT_VARIABLE_LIST_RULE = 62   /* PRINT_VARIABLE_LIST_RULE  */
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
typedef yytype_uint8 yy_state_t;

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
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   442

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  45
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  68
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  148

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   284


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
      37,    38,    15,    13,    39,    14,     2,    16,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    44,    36,
       2,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    40,     2,    41,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    42,     2,    43,     2,     2,     2,     2,
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
      31,    32,    33,    34,    35
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    73,    73,    82,    88,    93,    95,   100,   106,   114,
     115,   116,   117,   118,   121,   129,   134,   139,   144,   149,
     153,   161,   168,   172,   179,   183,   188,   192,   196,   200,
     204,   208,   212,   216,   220,   224,   228,   229,   233,   237,
     241,   245,   252,   262,   273,   277,   281,   285,   289,   293,
     297,   301,   305,   309,   313,   317,   321,   325,   329,   333,
     337,   344,   348,   354,   358,   366,   370,   374,   379
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
  "STRING_LITERAL", "IDENTIFIER", "INT_LITERAL", "READ", "PRINT", "GOTO",
  "BREAK", "WHILE", "LOOP", "ELSE", "IF", "DEF", "INT", "FLOAT", "STRING",
  "VOID", "BOOL", "';'", "'('", "')'", "','", "'['", "']'", "'{'", "'}'",
  "':'", "$accept", "PROGRAMME_RULE", "FUNCTION_LIST_RULE",
  "FUNCTION_RULE", "DECLARATION_STATEMENT_LIST_RULE", "ShortType",
  "DECLARATION_STATEMENT_RULE", "DECLARATION_VARIABLE_LIST_RULE",
  "LOGIC_BLOCK", "STATEMENT_BLOCK_RULE", "STATEMENT_LIST_RULE",
  "STATEMENT_RULE", "AI_INFER_RULE", "GREENAI_REPORT_RULE",
  "EXPRESSION_RULE", "VARIABLE_RULE", "READ_VARIABLE_LIST_RULE",
  "PRINT_VARIABLE_LIST_RULE", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-47)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     182,   -47,   -47,   -47,   -47,   -47,    31,   172,   -15,     1,
     -47,   182,    65,     9,    38,    -1,    39,   -47,    61,    62,
      -4,   -47,    64,    -5,    73,   -47,    62,    62,   -47,    62,
     387,    70,   -47,   -47,   387,   -47,    75,    77,   314,    93,
     -47,   -47,    94,    96,    81,    74,   -47,   -47,     3,    62,
     -47,   -47,   -33,   -47,   425,     5,    -8,   133,   115,   133,
     267,   363,   -47,   -47,   -47,   -47,    62,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    62,    62,   -47,
      62,    79,    82,   182,    84,    71,   171,   -47,    64,   -47,
      -2,    62,   -47,   -47,    62,   123,   -47,   -47,   157,   252,
      53,    53,   205,   205,   205,   205,    88,    88,   -47,   -47,
     -47,   327,   -47,   131,   122,     6,   125,   -47,   -47,   -47,
     425,   359,   184,   116,   -47,   167,   116,   170,   219,   -47,
     -47,    62,   -47,   -47,   -47,   191,    62,   120,   173,   232,
      62,   -47,   -47,    62,   133,   280,   -47,   -47
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     9,    10,    11,    12,    13,     0,     5,    19,     0,
       1,     0,     0,     0,     0,    17,    14,     8,     0,     0,
      61,    60,     0,     0,     0,    36,     0,     0,    41,     0,
       0,     0,     2,    27,    20,    23,     0,     0,     0,    59,
       4,     7,     0,     0,     0,    61,    57,    59,     0,     0,
      40,    64,     0,    67,    68,     0,     0,     0,    59,     0,
       0,     0,     3,    22,    39,    38,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    24,
       0,     0,    15,     0,     0,     0,     0,    35,     0,    37,
       0,     0,    33,    32,     0,    28,    58,    21,    55,    56,
      53,    54,    49,    50,    51,    52,    44,    45,    46,    47,
      48,     0,    18,     0,     0,     0,     0,    62,    63,    65,
      66,     0,     0,     0,    25,     0,     0,     0,     0,    26,
      34,     0,    29,    16,     6,     0,     0,     0,     0,     0,
       0,    30,    42,     0,     0,     0,    31,    43
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -47,   -47,   -47,   238,   168,   241,    -6,   -47,   -47,   -46,
     223,   -32,   -47,   -47,   -19,    12,   206,   -47
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     6,    12,    13,     7,     8,     9,    16,    32,    33,
      34,    35,    36,    37,    38,    47,    52,    55
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      46,    14,    63,    87,    54,    15,    88,    57,    59,    19,
      60,    93,    19,    95,    53,    45,    21,   119,    45,    21,
      19,    91,    84,    45,    39,   127,    45,    21,    92,    63,
      86,    10,    29,    48,    51,    29,    49,    17,    58,    42,
      50,    89,    39,    29,    90,    40,    39,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
      51,   111,    70,    71,    72,    73,    74,    75,    76,    77,
      78,   120,   121,    39,    41,   122,    19,   132,    43,    19,
     134,    44,    45,    21,    45,    20,    21,    22,    23,    24,
      25,   141,    26,    56,    27,    11,   128,    80,   146,    29,
     118,    28,    29,    76,    77,    78,    62,    30,    14,   116,
      88,    64,   137,    65,    49,    81,    82,   139,    83,    94,
     112,   144,   113,   115,   145,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,   123,   125,     1,     2,     3,     4,     5,    30,   140,
     126,   129,    30,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    30,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    11,     1,     2,     3,     4,     5,   133,   135,
     138,   142,   117,     1,     2,     3,     4,     5,    74,    75,
      76,    77,    78,   131,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      31,   114,    18,    61,    85,     0,     0,     0,   136,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
       0,   143,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,     0,     0,
       0,     0,     0,     0,     0,    96,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   147,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,     0,     0,     0,     0,     0,
      79,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   124,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    19,     0,     0,
       0,     0,     0,    20,    21,    22,    23,    24,    25,     0,
      26,     0,    27,     0,     0,   130,     0,     0,     0,    28,
      29,    19,     0,     0,     0,    30,    97,    20,    21,    22,
      23,    24,    25,     0,    26,     0,    27,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78
};

static const yytype_int16 yycheck[] =
{
      19,     7,    34,    36,    23,    20,    39,    26,    27,    14,
      29,    57,    14,    59,    19,    20,    21,    19,    20,    21,
      14,    29,    19,    20,    12,    19,    20,    21,    36,    61,
      49,     0,    37,    37,    22,    37,    40,    36,    26,    40,
      44,    36,    30,    37,    39,    36,    34,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      48,    80,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    90,    91,    61,    36,    94,    14,   123,    39,    14,
     126,    20,    20,    21,    20,    20,    21,    22,    23,    24,
      25,   137,    27,    20,    29,    30,   115,     4,   144,    37,
      88,    36,    37,    15,    16,    17,    36,    42,   114,    38,
      39,    36,   131,    36,    40,    21,    20,   136,    37,     4,
      41,   140,    40,    39,   143,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    28,    21,    31,    32,    33,    34,    35,    42,    39,
      38,    36,    42,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    42,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    30,    31,    32,    33,    34,    35,    41,    39,
      19,    38,    41,    31,    32,    33,    34,    35,    13,    14,
      15,    16,    17,    39,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      12,    83,    11,    30,    48,    -1,    -1,    -1,    39,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    39,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    14,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    -1,
      27,    -1,    29,    -1,    -1,    36,    -1,    -1,    -1,    36,
      37,    14,    -1,    -1,    -1,    42,    43,    20,    21,    22,
      23,    24,    25,    -1,    27,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    -1,    -1,    -1,    42,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    31,    32,    33,    34,    35,    46,    49,    50,    51,
       0,    30,    47,    48,    51,    20,    52,    36,    50,    14,
      20,    21,    22,    23,    24,    25,    27,    29,    36,    37,
      42,    48,    53,    54,    55,    56,    57,    58,    59,    60,
      36,    36,    40,    39,    20,    20,    59,    60,    37,    40,
      44,    60,    61,    19,    59,    62,    20,    59,    60,    59,
      59,    55,    36,    56,    36,    36,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    36,
       4,    21,    20,    37,    19,    61,    59,    36,    39,    36,
      39,    29,    36,    54,     4,    54,    38,    43,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    41,    40,    49,    39,    38,    41,    60,    19,
      59,    59,    59,    28,    36,    21,    38,    19,    59,    36,
      36,    39,    54,    41,    54,    39,    39,    59,    19,    59,
      39,    54,    38,    39,    59,    59,    54,    38
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    45,    46,    47,    47,    47,    48,    49,    49,    50,
      50,    50,    50,    50,    51,    52,    52,    52,    52,    52,
      53,    54,    55,    55,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    57,    58,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    60,    60,    61,    61,    62,    62,    62,    62
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     3,     2,     0,     7,     3,     2,     1,
       1,     1,     1,     1,     2,     3,     6,     1,     4,     0,
       1,     3,     2,     1,     2,     4,     5,     1,     3,     5,
       7,     9,     3,     3,     5,     3,     1,     3,     2,     2,
       2,     1,     8,    10,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     3,     1,
       1,     1,     4,     3,     1,     3,     3,     1,     1
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
#line 74 "./scanner_parser/parser.yy"
            {
                //fprintf(bison_output, "program\n");
                (yyval.program) = new AST_PROGRAM((yyvsp[-2].decl_block),(yyvsp[-1].functions),(yyvsp[0].code_block));
                main_program = (yyval.program);
            }
#line 1296 "parser.tab.cc"
    break;

  case 3: /* FUNCTION_LIST_RULE: FUNCTION_LIST_RULE FUNCTION_RULE ';'  */
#line 83 "./scanner_parser/parser.yy"
                                                                        {
									(yyval.functions) = (yyvsp[-2].functions);
									(yyval.functions)->push_back((yyvsp[-1].function));
									}
#line 1305 "parser.tab.cc"
    break;

  case 4: /* FUNCTION_LIST_RULE: FUNCTION_RULE ';'  */
#line 89 "./scanner_parser/parser.yy"
                    {
                    (yyval.functions) = new AST_FUNCTION_LIST_RULE();
                    (yyval.functions)->push_back((yyvsp[-1].function));
                    }
#line 1314 "parser.tab.cc"
    break;

  case 5: /* FUNCTION_LIST_RULE: %empty  */
#line 93 "./scanner_parser/parser.yy"
                             { (yyval.functions) = new AST_FUNCTION_LIST_RULE();}
#line 1320 "parser.tab.cc"
    break;

  case 6: /* FUNCTION_RULE: DEF ShortType IDENTIFIER '(' DECLARATION_STATEMENT_LIST_RULE ')' STATEMENT_BLOCK_RULE  */
#line 96 "./scanner_parser/parser.yy"
                        {
	        	(yyval.function) = new AST_FUNCTION_RULE((yyvsp[-5].type),(yyvsp[-4].string_val),(yyvsp[-2].decl_block),(yyvsp[0].block_statement));
	      		}
#line 1328 "parser.tab.cc"
    break;

  case 7: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';'  */
#line 101 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back((yyvsp[-1].decl_block));
                       }
#line 1337 "parser.tab.cc"
    break;

  case 8: /* DECLARATION_STATEMENT_LIST_RULE: DECLARATION_STATEMENT_RULE ';'  */
#line 107 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-1].decl_block);
                       }
#line 1345 "parser.tab.cc"
    break;

  case 9: /* ShortType: INT  */
#line 114 "./scanner_parser/parser.yy"
               {(yyval.type)=ShortType::Int;}
#line 1351 "parser.tab.cc"
    break;

  case 10: /* ShortType: FLOAT  */
#line 115 "./scanner_parser/parser.yy"
              {(yyval.type)=ShortType::Float;}
#line 1357 "parser.tab.cc"
    break;

  case 11: /* ShortType: STRING  */
#line 116 "./scanner_parser/parser.yy"
               {(yyval.type)=ShortType::String;}
#line 1363 "parser.tab.cc"
    break;

  case 12: /* ShortType: VOID  */
#line 117 "./scanner_parser/parser.yy"
             {(yyval.type)=ShortType::Void;}
#line 1369 "parser.tab.cc"
    break;

  case 13: /* ShortType: BOOL  */
#line 118 "./scanner_parser/parser.yy"
            {(yyval.type)=ShortType::Boolean;}
#line 1375 "parser.tab.cc"
    break;

  case 14: /* DECLARATION_STATEMENT_RULE: ShortType DECLARATION_VARIABLE_LIST_RULE  */
#line 122 "./scanner_parser/parser.yy"
                   {
                       //fprintf(bison_output, "DECLARATION_STATEMENT_RULE\n");
                       (yyval.decl_block) = (yyvsp[0].decl_block);
                   }
#line 1384 "parser.tab.cc"
    break;

  case 15: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER  */
#line 130 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1393 "parser.tab.cc"
    break;

  case 16: /* DECLARATION_VARIABLE_LIST_RULE: DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']'  */
#line 135 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = (yyvsp[-5].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1402 "parser.tab.cc"
    break;

  case 17: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER  */
#line 140 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1411 "parser.tab.cc"
    break;

  case 18: /* DECLARATION_VARIABLE_LIST_RULE: IDENTIFIER '[' INT_LITERAL ']'  */
#line 145 "./scanner_parser/parser.yy"
                       {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1420 "parser.tab.cc"
    break;

  case 19: /* DECLARATION_VARIABLE_LIST_RULE: %empty  */
#line 149 "./scanner_parser/parser.yy"
                           {(yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();}
#line 1426 "parser.tab.cc"
    break;

  case 20: /* LOGIC_BLOCK: STATEMENT_LIST_RULE  */
#line 154 "./scanner_parser/parser.yy"
               {
                   //fprintf(bison_output, "LOGIC_BLOCK\n");
                   (yyval.code_block) = new AST_LOGIC_BLOCK((yyvsp[0].block_statement));
               }
#line 1435 "parser.tab.cc"
    break;

  case 21: /* STATEMENT_BLOCK_RULE: '{' STATEMENT_LIST_RULE '}'  */
#line 162 "./scanner_parser/parser.yy"
                    {
                        //fprintf(bison_output, "STATEMENT_BLOCK_RULE\n");
                        (yyval.block_statement) = (yyvsp[-1].block_statement);
                    }
#line 1444 "parser.tab.cc"
    break;

  case 22: /* STATEMENT_LIST_RULE: STATEMENT_LIST_RULE STATEMENT_RULE  */
#line 169 "./scanner_parser/parser.yy"
                   {
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1452 "parser.tab.cc"
    break;

  case 23: /* STATEMENT_LIST_RULE: STATEMENT_RULE  */
#line 173 "./scanner_parser/parser.yy"
                   {
                       (yyval.block_statement) = new AST_STATEMENTS_BLOCK();
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1461 "parser.tab.cc"
    break;

  case 24: /* STATEMENT_RULE: EXPRESSION_RULE ';'  */
#line 180 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE((yyvsp[-1].expression));
              }
#line 1469 "parser.tab.cc"
    break;

  case 25: /* STATEMENT_RULE: VARIABLE_RULE '=' EXPRESSION_RULE ';'  */
#line 184 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_ASSIGNMENT_RULE((yyvsp[-3].variable), (yyvsp[-1].expression));
              }
#line 1477 "parser.tab.cc"
    break;

  case 26: /* STATEMENT_RULE: IDENTIFIER '(' READ_VARIABLE_LIST_RULE ')' ';'  */
#line 189 "./scanner_parser/parser.yy"
              {
	       (yyval.statement) = new AST_FUNCTION_CALL_RULE((yyvsp[-4].string_val),(yyvsp[-2].read_statement));
	      }
#line 1485 "parser.tab.cc"
    break;

  case 27: /* STATEMENT_RULE: STATEMENT_BLOCK_RULE  */
#line 193 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[0].block_statement);
              }
#line 1493 "parser.tab.cc"
    break;

  case 28: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 197 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_STATEMENT((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1501 "parser.tab.cc"
    break;

  case 29: /* STATEMENT_RULE: IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE  */
#line 201 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_IF_ELSE_STATEMENT((yyvsp[-3].expression), (yyvsp[-2].block_statement), (yyvsp[0].block_statement));
              }
#line 1509 "parser.tab.cc"
    break;

  case 30: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 205 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-5].variable), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1517 "parser.tab.cc"
    break;

  case 31: /* STATEMENT_RULE: LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 209 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-7].variable), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1525 "parser.tab.cc"
    break;

  case 32: /* STATEMENT_RULE: LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE  */
#line 213 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_WHILE_LOOP_STATEMENT_RULE((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1533 "parser.tab.cc"
    break;

  case 33: /* STATEMENT_RULE: GOTO IDENTIFIER ';'  */
#line 217 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE(string((yyvsp[-1].string_val)));
              }
#line 1541 "parser.tab.cc"
    break;

  case 34: /* STATEMENT_RULE: GOTO IDENTIFIER IF EXPRESSION_RULE ';'  */
#line 221 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE((yyvsp[-1].expression), string((yyvsp[-3].string_val)));
              }
#line 1549 "parser.tab.cc"
    break;

  case 35: /* STATEMENT_RULE: READ READ_VARIABLE_LIST_RULE ';'  */
#line 225 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].read_statement);
              }
#line 1557 "parser.tab.cc"
    break;

  case 36: /* STATEMENT_RULE: BREAK  */
#line 228 "./scanner_parser/parser.yy"
                 {(yyval.statement) = new AST_BREAK();}
#line 1563 "parser.tab.cc"
    break;

  case 37: /* STATEMENT_RULE: PRINT PRINT_VARIABLE_LIST_RULE ';'  */
#line 230 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].print_statement);
              }
#line 1571 "parser.tab.cc"
    break;

  case 38: /* STATEMENT_RULE: GREENAI_REPORT_RULE ';'  */
#line 234 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].greenai_report);
              }
#line 1579 "parser.tab.cc"
    break;

  case 39: /* STATEMENT_RULE: AI_INFER_RULE ';'  */
#line 238 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = (yyvsp[-1].ai_infer);
              }
#line 1587 "parser.tab.cc"
    break;

  case 40: /* STATEMENT_RULE: IDENTIFIER ':'  */
#line 242 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_LABEL_RULE(string((yyvsp[-1].string_val)));
              }
#line 1595 "parser.tab.cc"
    break;

  case 41: /* STATEMENT_RULE: ';'  */
#line 246 "./scanner_parser/parser.yy"
              {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE(new AST_LITERAL(1));
              }
#line 1603 "parser.tab.cc"
    break;

  case 42: /* AI_INFER_RULE: IDENTIFIER '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')'  */
#line 253 "./scanner_parser/parser.yy"
              {
                  if (string((yyvsp[-7].string_val)) != "ai_infer") {
                      yyerror((char *)"expected ai_infer builtin");
                      YYERROR;
                  }
                  (yyval.ai_infer) = new AST_AI_INFER_RULE(string((yyvsp[-5].string_val)), string((yyvsp[-3].string_val)), string((yyvsp[-1].string_val)));
              }
#line 1615 "parser.tab.cc"
    break;

  case 43: /* GREENAI_REPORT_RULE: IDENTIFIER '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')'  */
#line 263 "./scanner_parser/parser.yy"
              {
                  if (string((yyvsp[-9].string_val)) != "greenai") {
                      yyerror((char *)"expected greenai report builtin");
                      YYERROR;
                  }
                  (yyval.greenai_report) = new AST_GREENAI_REPORT_RULE(string((yyvsp[-7].string_val)), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression));
              }
#line 1627 "parser.tab.cc"
    break;

  case 44: /* EXPRESSION_RULE: EXPRESSION_RULE '+' EXPRESSION_RULE  */
#line 274 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "+");
               }
#line 1635 "parser.tab.cc"
    break;

  case 45: /* EXPRESSION_RULE: EXPRESSION_RULE '-' EXPRESSION_RULE  */
#line 278 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "-");
               }
#line 1643 "parser.tab.cc"
    break;

  case 46: /* EXPRESSION_RULE: EXPRESSION_RULE '*' EXPRESSION_RULE  */
#line 282 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "*");
               }
#line 1651 "parser.tab.cc"
    break;

  case 47: /* EXPRESSION_RULE: EXPRESSION_RULE '/' EXPRESSION_RULE  */
#line 286 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "/");
               }
#line 1659 "parser.tab.cc"
    break;

  case 48: /* EXPRESSION_RULE: EXPRESSION_RULE '%' EXPRESSION_RULE  */
#line 290 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "%");
               }
#line 1667 "parser.tab.cc"
    break;

  case 49: /* EXPRESSION_RULE: EXPRESSION_RULE LESS EXPRESSION_RULE  */
#line 294 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<");
               }
#line 1675 "parser.tab.cc"
    break;

  case 50: /* EXPRESSION_RULE: EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE  */
#line 298 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<=");
               }
#line 1683 "parser.tab.cc"
    break;

  case 51: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER EXPRESSION_RULE  */
#line 302 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">");
               }
#line 1691 "parser.tab.cc"
    break;

  case 52: /* EXPRESSION_RULE: EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE  */
#line 306 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">=");
               }
#line 1699 "parser.tab.cc"
    break;

  case 53: /* EXPRESSION_RULE: EXPRESSION_RULE EQUAL EXPRESSION_RULE  */
#line 310 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "==");
               }
#line 1707 "parser.tab.cc"
    break;

  case 54: /* EXPRESSION_RULE: EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE  */
#line 314 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "!=");
               }
#line 1715 "parser.tab.cc"
    break;

  case 55: /* EXPRESSION_RULE: EXPRESSION_RULE OR EXPRESSION_RULE  */
#line 318 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "||");
               }
#line 1723 "parser.tab.cc"
    break;

  case 56: /* EXPRESSION_RULE: EXPRESSION_RULE AND EXPRESSION_RULE  */
#line 322 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "&&");
               }
#line 1731 "parser.tab.cc"
    break;

  case 57: /* EXPRESSION_RULE: '-' EXPRESSION_RULE  */
#line 326 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_UNARY_EXPRESSION_RULE((yyvsp[0].expression), "-");
               }
#line 1739 "parser.tab.cc"
    break;

  case 58: /* EXPRESSION_RULE: '(' EXPRESSION_RULE ')'  */
#line 330 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[-1].expression);
               }
#line 1747 "parser.tab.cc"
    break;

  case 59: /* EXPRESSION_RULE: VARIABLE_RULE  */
#line 334 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = (yyvsp[0].variable);
               }
#line 1755 "parser.tab.cc"
    break;

  case 60: /* EXPRESSION_RULE: INT_LITERAL  */
#line 338 "./scanner_parser/parser.yy"
               {
                   (yyval.expression) = new AST_LITERAL((yyvsp[0].int_val));
               }
#line 1763 "parser.tab.cc"
    break;

  case 61: /* VARIABLE_RULE: IDENTIFIER  */
#line 345 "./scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_SIMPLE_VARIABLE(string((yyvsp[0].string_val)));
             }
#line 1771 "parser.tab.cc"
    break;

  case 62: /* VARIABLE_RULE: IDENTIFIER '[' EXPRESSION_RULE ']'  */
#line 349 "./scanner_parser/parser.yy"
             {
                 (yyval.variable) = new AST_ARRAY_VARIABLE(string((yyvsp[-3].string_val)), (yyvsp[-1].expression));
             }
#line 1779 "parser.tab.cc"
    break;

  case 63: /* READ_VARIABLE_LIST_RULE: READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE  */
#line 355 "./scanner_parser/parser.yy"
                       {
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 1787 "parser.tab.cc"
    break;

  case 64: /* READ_VARIABLE_LIST_RULE: VARIABLE_RULE  */
#line 359 "./scanner_parser/parser.yy"
                       {
                           (yyval.read_statement) = new AST_READ_RULE();
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 1796 "parser.tab.cc"
    break;

  case 65: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' STRING_LITERAL  */
#line 367 "./scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 1804 "parser.tab.cc"
    break;

  case 66: /* PRINT_VARIABLE_LIST_RULE: PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE  */
#line 371 "./scanner_parser/parser.yy"
                   {
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 1812 "parser.tab.cc"
    break;

  case 67: /* PRINT_VARIABLE_LIST_RULE: STRING_LITERAL  */
#line 375 "./scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 1821 "parser.tab.cc"
    break;

  case 68: /* PRINT_VARIABLE_LIST_RULE: EXPRESSION_RULE  */
#line 380 "./scanner_parser/parser.yy"
                   {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 1830 "parser.tab.cc"
    break;


#line 1834 "parser.tab.cc"

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

#line 387 "./scanner_parser/parser.yy"




void yyerror (char const *s)
{
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf (stderr, "%s\n", s);
}
