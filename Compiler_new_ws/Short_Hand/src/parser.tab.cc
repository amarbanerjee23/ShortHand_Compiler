/* A Bison parser, made by GNU Bison 3.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4"

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



#line 92 "parser.tab.cc"

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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_PARSER_TAB_HH_INCLUDED
# define YY_YY_PARSER_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ETOK = 258,
    OR = 259,
    AND = 260,
    EQUAL = 261,
    NOT_EQUAL = 262,
    LESS = 263,
    LESS_OR_EQUAL = 264,
    GREATER = 265,
    GREATER_OR_EQUAL = 266,
    UMINUS = 267,
    STRING_LITERAL = 268,
    IDENTIFIER = 269,
    INT_LITERAL = 270,
    READ = 271,
    PRINT = 272,
    GOTO = 273,
    BREAK = 274,
    WHILE = 275,
    LOOP = 276,
    ELSE = 277,
    IF = 278,
    DEF = 279,
    INT = 280,
    FLOAT = 281,
    STRING = 282,
    VOID = 283,
    BOOL = 284
  };
#endif

/* Value type.  */


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_TAB_HH_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
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
#define YYLAST   324

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  45
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  64
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  132

#define YYUNDEFTOK  2
#define YYMAXUTOK   284

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
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
static const yytype_uint16 yyrline[] =
{
       0,    71,    71,    80,    86,    91,    93,    98,   104,   112,
     113,   114,   115,   116,   119,   127,   132,   137,   142,   147,
     151,   159,   166,   170,   177,   181,   186,   190,   194,   198,
     202,   206,   210,   214,   218,   222,   226,   227,   231,   235,
     243,   247,   251,   255,   259,   263,   267,   271,   275,   279,
     283,   287,   291,   295,   299,   303,   307,   314,   318,   324,
     328,   336,   340,   344,   349
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ETOK", "'='", "OR", "AND", "EQUAL",
  "NOT_EQUAL", "LESS", "LESS_OR_EQUAL", "GREATER", "GREATER_OR_EQUAL",
  "'+'", "'-'", "'*'", "'/'", "'%'", "UMINUS", "STRING_LITERAL",
  "IDENTIFIER", "INT_LITERAL", "READ", "PRINT", "GOTO", "BREAK", "WHILE",
  "LOOP", "ELSE", "IF", "DEF", "INT", "FLOAT", "STRING", "VOID", "BOOL",
  "';'", "'('", "')'", "','", "'['", "']'", "'{'", "'}'", "':'", "$accept",
  "PROGRAMME_RULE", "FUNCTION_LIST_RULE", "FUNCTION_RULE",
  "DECLARATION_STATEMENT_LIST_RULE", "ShortType",
  "DECLARATION_STATEMENT_RULE", "DECLARATION_VARIABLE_LIST_RULE",
  "LOGIC_BLOCK", "STATEMENT_BLOCK_RULE", "STATEMENT_LIST_RULE",
  "STATEMENT_RULE", "EXPRESSION_RULE", "VARIABLE_RULE",
  "READ_VARIABLE_LIST_RULE", "PRINT_VARIABLE_LIST_RULE", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,    61,   259,   260,   261,   262,   263,
     264,   265,   266,    43,    45,    42,    47,    37,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,    59,    40,    41,    44,
      91,    93,   123,   125,    58
};
# endif

#define YYPACT_NINF -53

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-53)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     130,   -53,   -53,   -53,   -53,   -53,    56,    93,    40,    26,
     -53,   130,   203,    36,    38,    44,    41,   -53,    71,    62,
     218,   -53,    77,   223,    78,   -53,    62,    62,   -53,    62,
     227,    64,   -53,   -53,   227,   -53,     6,   101,   -53,   -53,
     108,   125,   109,   127,   -53,   -53,    77,    62,   -53,   -53,
     -30,   -53,   265,    22,    35,   126,   162,   126,   164,    65,
     -53,   -53,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    62,    62,    62,    62,   -53,    62,   141,   158,   130,
      57,   143,   -53,    77,   -53,   247,    62,   -53,   -53,    62,
     155,   -53,   -53,   279,   290,   299,   299,   205,   205,   205,
     205,     9,     9,   -53,   -53,   -53,   180,   -53,   178,   286,
     165,   -53,   -53,   -53,   265,   198,    24,   187,   -53,   159,
     187,   -53,   -53,    62,   -53,   -53,   -53,   105,    62,   -53,
     126,   -53
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     9,    10,    11,    12,    13,     0,     5,    19,     0,
       1,     0,     0,     0,     0,    17,    14,     8,     0,     0,
      57,    56,     0,     0,     0,    36,     0,     0,    39,     0,
       0,     0,     2,    27,    20,    23,     0,    55,     4,     7,
       0,     0,     0,    57,    53,    55,     0,     0,    38,    60,
       0,    63,    64,     0,     0,     0,    55,     0,     0,     0,
       3,    22,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    24,     0,     0,    15,     0,
       0,     0,    35,     0,    37,     0,     0,    33,    32,     0,
      28,    54,    21,    51,    52,    49,    50,    45,    46,    47,
      48,    40,    41,    42,    43,    44,     0,    18,     0,     0,
       0,    58,    59,    61,    62,     0,     0,     0,    25,     0,
       0,    26,    34,     0,    29,    16,     6,     0,     0,    30,
       0,    31
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -53,   -53,   -53,   219,   156,   225,    -6,   -53,   -53,   -52,
     208,   -32,   -19,    47,   200,   -53
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     6,    12,    13,     7,     8,     9,    16,    32,    33,
      34,    35,    36,    45,    50,    53
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      44,    14,    61,    88,    52,    90,    82,    55,    57,    83,
      58,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    72,    73,    74,    61,    81,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,    10,   106,    84,    37,
      15,    85,    17,   123,    86,   124,   114,   115,   126,    49,
     116,    87,    38,    56,    39,   129,    19,    37,   131,    19,
      41,    37,    43,    21,    40,    20,    21,    22,    23,    24,
      25,    42,    26,    49,    27,   110,    83,    43,    54,    29,
      60,    28,    29,    14,   127,    76,    37,    30,    92,   130,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    11,     1,     2,     3,     4,     5,    77,
     112,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,   128,    78,    79,    30,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,     1,     2,     3,     4,     5,    89,    47,    30,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,   107,   117,   111,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,   108,   119,
     125,   121,    91,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,   118,    19,    70,    71,
      72,    73,    74,    20,    21,    22,    23,    24,    25,    30,
      26,    31,    27,    11,   122,   109,    18,    19,    59,    28,
      29,    19,    51,    43,    21,    30,    80,    20,    21,    22,
      23,    24,    25,     0,    26,    46,    27,     0,    47,     0,
      29,    19,    48,    28,    29,     0,   113,    43,    21,    30,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,     0,    29,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    66,    67,
      68,    69,    70,    71,    72,    73,    74,     1,     2,     3,
       4,     5,     0,     0,   120
};

static const yytype_int16 yycheck[] =
{
      19,     7,    34,    55,    23,    57,    36,    26,    27,    39,
      29,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    15,    16,    17,    59,    47,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    36,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,    76,    36,    12,
      20,    39,    36,    39,    29,   117,    85,    86,   120,    22,
      89,    36,    36,    26,    36,   127,    14,    30,   130,    14,
      39,    34,    20,    21,    40,    20,    21,    22,    23,    24,
      25,    20,    27,    46,    29,    38,    39,    20,    20,    37,
      36,    36,    37,   109,   123,     4,    59,    42,    43,   128,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    30,    31,    32,    33,    34,    35,    21,
      83,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    39,    20,    37,    42,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    31,    32,    33,    34,    35,     4,    40,    42,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    41,    28,    41,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    40,    21,
      41,    36,    38,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    36,    14,    13,    14,
      15,    16,    17,    20,    21,    22,    23,    24,    25,    42,
      27,    12,    29,    30,    36,    79,    11,    14,    30,    36,
      37,    14,    19,    20,    21,    42,    46,    20,    21,    22,
      23,    24,    25,    -1,    27,    37,    29,    -1,    40,    -1,
      37,    14,    44,    36,    37,    -1,    19,    20,    21,    42,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    37,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    31,    32,    33,
      34,    35,    -1,    -1,    38
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    31,    32,    33,    34,    35,    46,    49,    50,    51,
       0,    30,    47,    48,    51,    20,    52,    36,    50,    14,
      20,    21,    22,    23,    24,    25,    27,    29,    36,    37,
      42,    48,    53,    54,    55,    56,    57,    58,    36,    36,
      40,    39,    20,    20,    57,    58,    37,    40,    44,    58,
      59,    19,    57,    60,    20,    57,    58,    57,    57,    55,
      36,    56,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    36,     4,    21,    20,    37,
      59,    57,    36,    39,    36,    39,    29,    36,    54,     4,
      54,    38,    43,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    41,    40,    49,
      38,    41,    58,    19,    57,    57,    57,    28,    36,    21,
      38,    36,    36,    39,    54,    41,    54,    57,    39,    54,
      57,    54
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    45,    46,    47,    47,    47,    48,    49,    49,    50,
      50,    50,    50,    50,    51,    52,    52,    52,    52,    52,
      53,    54,    55,    55,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    58,    58,    59,
      59,    60,    60,    60,    60
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     3,     2,     0,     7,     3,     2,     1,
       1,     1,     1,     1,     2,     3,     6,     1,     4,     0,
       1,     3,     2,     1,     2,     4,     5,     1,     3,     5,
       7,     9,     3,     3,     5,     3,     1,     3,     2,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     3,     1,     1,     1,     4,     3,
       1,     3,     3,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
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
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
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
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
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
  case 2:
#line 72 "./scanner_parser/parser.yy"
    {
                //fprintf(bison_output, "program\n");
                (yyval.program) = new AST_PROGRAM((yyvsp[-2].decl_block),(yyvsp[-1].functions),(yyvsp[0].code_block));
                main_program = (yyval.program);
            }
#line 1380 "parser.tab.cc"
    break;

  case 3:
#line 81 "./scanner_parser/parser.yy"
    {
									(yyval.functions) = (yyvsp[-2].functions);
									(yyval.functions)->push_back((yyvsp[-1].function));
									}
#line 1389 "parser.tab.cc"
    break;

  case 4:
#line 87 "./scanner_parser/parser.yy"
    {
                    (yyval.functions) = new AST_FUNCTION_LIST_RULE();
                    (yyval.functions)->push_back((yyvsp[-1].function));
                    }
#line 1398 "parser.tab.cc"
    break;

  case 5:
#line 91 "./scanner_parser/parser.yy"
    { (yyval.functions) = new AST_FUNCTION_LIST_RULE();}
#line 1404 "parser.tab.cc"
    break;

  case 6:
#line 94 "./scanner_parser/parser.yy"
    {
	        	(yyval.function) = new AST_FUNCTION_RULE((yyvsp[-5].type),(yyvsp[-4].string_val),(yyvsp[-2].decl_block),(yyvsp[0].block_statement));
	      		}
#line 1412 "parser.tab.cc"
    break;

  case 7:
#line 99 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back((yyvsp[-1].decl_block));
                       }
#line 1421 "parser.tab.cc"
    break;

  case 8:
#line 105 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = (yyvsp[-1].decl_block);
                       }
#line 1429 "parser.tab.cc"
    break;

  case 9:
#line 112 "./scanner_parser/parser.yy"
    {(yyval.type)=ShortType::Int;}
#line 1435 "parser.tab.cc"
    break;

  case 10:
#line 113 "./scanner_parser/parser.yy"
    {(yyval.type)=ShortType::Float;}
#line 1441 "parser.tab.cc"
    break;

  case 11:
#line 114 "./scanner_parser/parser.yy"
    {(yyval.type)=ShortType::String;}
#line 1447 "parser.tab.cc"
    break;

  case 12:
#line 115 "./scanner_parser/parser.yy"
    {(yyval.type)=ShortType::Void;}
#line 1453 "parser.tab.cc"
    break;

  case 13:
#line 116 "./scanner_parser/parser.yy"
    {(yyval.type)=ShortType::Boolean;}
#line 1459 "parser.tab.cc"
    break;

  case 14:
#line 120 "./scanner_parser/parser.yy"
    {
                       //fprintf(bison_output, "DECLARATION_STATEMENT_RULE\n");
                       (yyval.decl_block) = (yyvsp[0].decl_block);
                   }
#line 1468 "parser.tab.cc"
    break;

  case 15:
#line 128 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = (yyvsp[-2].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1477 "parser.tab.cc"
    break;

  case 16:
#line 133 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = (yyvsp[-5].decl_block);
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1486 "parser.tab.cc"
    break;

  case 17:
#line 138 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[0].string_val)));
                       }
#line 1495 "parser.tab.cc"
    break;

  case 18:
#line 143 "./scanner_parser/parser.yy"
    {
                           (yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();
                           (yyval.decl_block)->push_back(string((yyvsp[-3].string_val)), (yyvsp[-1].int_val));
                       }
#line 1504 "parser.tab.cc"
    break;

  case 19:
#line 147 "./scanner_parser/parser.yy"
    {(yyval.decl_block) = new AST_DATA_DECLARATION_BLOCK();}
#line 1510 "parser.tab.cc"
    break;

  case 20:
#line 152 "./scanner_parser/parser.yy"
    {
                   //fprintf(bison_output, "LOGIC_BLOCK\n");
                   (yyval.code_block) = new AST_LOGIC_BLOCK((yyvsp[0].block_statement));
               }
#line 1519 "parser.tab.cc"
    break;

  case 21:
#line 160 "./scanner_parser/parser.yy"
    {
                        //fprintf(bison_output, "STATEMENT_BLOCK_RULE\n");
                        (yyval.block_statement) = (yyvsp[-1].block_statement);
                    }
#line 1528 "parser.tab.cc"
    break;

  case 22:
#line 167 "./scanner_parser/parser.yy"
    {
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1536 "parser.tab.cc"
    break;

  case 23:
#line 171 "./scanner_parser/parser.yy"
    {
                       (yyval.block_statement) = new AST_STATEMENTS_BLOCK();
                       (yyval.block_statement)->push_back((yyvsp[0].statement));
                   }
#line 1545 "parser.tab.cc"
    break;

  case 24:
#line 178 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE((yyvsp[-1].expression));
              }
#line 1553 "parser.tab.cc"
    break;

  case 25:
#line 182 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_ASSIGNMENT_RULE((yyvsp[-3].variable), (yyvsp[-1].expression));
              }
#line 1561 "parser.tab.cc"
    break;

  case 26:
#line 187 "./scanner_parser/parser.yy"
    {
	       (yyval.statement) = new AST_FUNCTION_CALL_RULE((yyvsp[-4].string_val),(yyvsp[-2].read_statement));
	      }
#line 1569 "parser.tab.cc"
    break;

  case 27:
#line 191 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = (yyvsp[0].block_statement);
              }
#line 1577 "parser.tab.cc"
    break;

  case 28:
#line 195 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_IF_STATEMENT((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1585 "parser.tab.cc"
    break;

  case 29:
#line 199 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_IF_ELSE_STATEMENT((yyvsp[-3].expression), (yyvsp[-2].block_statement), (yyvsp[0].block_statement));
              }
#line 1593 "parser.tab.cc"
    break;

  case 30:
#line 203 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-5].variable), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1601 "parser.tab.cc"
    break;

  case 31:
#line 207 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_FOR_LOOP_STATEMENT_RULE((yyvsp[-7].variable), (yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1609 "parser.tab.cc"
    break;

  case 32:
#line 211 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_WHILE_LOOP_STATEMENT_RULE((yyvsp[-1].expression), (yyvsp[0].block_statement));
              }
#line 1617 "parser.tab.cc"
    break;

  case 33:
#line 215 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE(string((yyvsp[-1].string_val)));
              }
#line 1625 "parser.tab.cc"
    break;

  case 34:
#line 219 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_GOTO_STATEMENT_RULE((yyvsp[-1].expression), string((yyvsp[-3].string_val)));
              }
#line 1633 "parser.tab.cc"
    break;

  case 35:
#line 223 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = (yyvsp[-1].read_statement);
              }
#line 1641 "parser.tab.cc"
    break;

  case 36:
#line 226 "./scanner_parser/parser.yy"
    {(yyval.statement) = new AST_BREAK();}
#line 1647 "parser.tab.cc"
    break;

  case 37:
#line 228 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = (yyvsp[-1].print_statement);
              }
#line 1655 "parser.tab.cc"
    break;

  case 38:
#line 232 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_LABEL_RULE(string((yyvsp[-1].string_val)));
              }
#line 1663 "parser.tab.cc"
    break;

  case 39:
#line 236 "./scanner_parser/parser.yy"
    {
                  (yyval.statement) = new AST_EXPRESSION_STATEMENT_RULE(new AST_LITERAL(1));
              }
#line 1671 "parser.tab.cc"
    break;

  case 40:
#line 244 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "+");
               }
#line 1679 "parser.tab.cc"
    break;

  case 41:
#line 248 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "-");
               }
#line 1687 "parser.tab.cc"
    break;

  case 42:
#line 252 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "*");
               }
#line 1695 "parser.tab.cc"
    break;

  case 43:
#line 256 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "/");
               }
#line 1703 "parser.tab.cc"
    break;

  case 44:
#line 260 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "%");
               }
#line 1711 "parser.tab.cc"
    break;

  case 45:
#line 264 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<");
               }
#line 1719 "parser.tab.cc"
    break;

  case 46:
#line 268 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "<=");
               }
#line 1727 "parser.tab.cc"
    break;

  case 47:
#line 272 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">");
               }
#line 1735 "parser.tab.cc"
    break;

  case 48:
#line 276 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), ">=");
               }
#line 1743 "parser.tab.cc"
    break;

  case 49:
#line 280 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "==");
               }
#line 1751 "parser.tab.cc"
    break;

  case 50:
#line 284 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "!=");
               }
#line 1759 "parser.tab.cc"
    break;

  case 51:
#line 288 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "||");
               }
#line 1767 "parser.tab.cc"
    break;

  case 52:
#line 292 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_BINARY_EXPRESSION_RULE((yyvsp[-2].expression), (yyvsp[0].expression), "&&");
               }
#line 1775 "parser.tab.cc"
    break;

  case 53:
#line 296 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_UNARY_EXPRESSION_RULE((yyvsp[0].expression), "-");
               }
#line 1783 "parser.tab.cc"
    break;

  case 54:
#line 300 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = (yyvsp[-1].expression);
               }
#line 1791 "parser.tab.cc"
    break;

  case 55:
#line 304 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = (yyvsp[0].variable);
               }
#line 1799 "parser.tab.cc"
    break;

  case 56:
#line 308 "./scanner_parser/parser.yy"
    {
                   (yyval.expression) = new AST_LITERAL((yyvsp[0].int_val));
               }
#line 1807 "parser.tab.cc"
    break;

  case 57:
#line 315 "./scanner_parser/parser.yy"
    {
                 (yyval.variable) = new AST_SIMPLE_VARIABLE(string((yyvsp[0].string_val)));
             }
#line 1815 "parser.tab.cc"
    break;

  case 58:
#line 319 "./scanner_parser/parser.yy"
    {
                 (yyval.variable) = new AST_ARRAY_VARIABLE(string((yyvsp[-3].string_val)), (yyvsp[-1].expression));
             }
#line 1823 "parser.tab.cc"
    break;

  case 59:
#line 325 "./scanner_parser/parser.yy"
    {
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 1831 "parser.tab.cc"
    break;

  case 60:
#line 329 "./scanner_parser/parser.yy"
    {
                           (yyval.read_statement) = new AST_READ_RULE();
                           (yyval.read_statement)->push_back((yyvsp[0].variable));
                       }
#line 1840 "parser.tab.cc"
    break;

  case 61:
#line 337 "./scanner_parser/parser.yy"
    {
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 1848 "parser.tab.cc"
    break;

  case 62:
#line 341 "./scanner_parser/parser.yy"
    {
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 1856 "parser.tab.cc"
    break;

  case 63:
#line 345 "./scanner_parser/parser.yy"
    {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back(new AST_STRING_LITERAL(string((yyvsp[0].string_val))));
                   }
#line 1865 "parser.tab.cc"
    break;

  case 64:
#line 350 "./scanner_parser/parser.yy"
    {
                       (yyval.print_statement) = new AST_PRINT_RULE();
                       (yyval.print_statement)->push_back((yyvsp[0].expression));
                   }
#line 1874 "parser.tab.cc"
    break;


#line 1878 "parser.tab.cc"

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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

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
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
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
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 357 "./scanner_parser/parser.yy"




void yyerror (char const *s)
{
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf (stderr, "%s\n", s);
}


