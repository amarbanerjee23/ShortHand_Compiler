/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_HH_INCLUDED
# define YY_YY_PARSER_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ETOK = 258,                    /* ETOK  */
    OR = 259,                      /* OR  */
    AND = 260,                     /* AND  */
    EQUAL = 261,                   /* EQUAL  */
    NOT_EQUAL = 262,               /* NOT_EQUAL  */
    LESS = 263,                    /* LESS  */
    LESS_OR_EQUAL = 264,           /* LESS_OR_EQUAL  */
    GREATER = 265,                 /* GREATER  */
    GREATER_OR_EQUAL = 266,        /* GREATER_OR_EQUAL  */
    ARROW = 267,                   /* ARROW  */
    UMINUS = 268,                  /* UMINUS  */
    STRING_LITERAL = 269,          /* STRING_LITERAL  */
    IDENTIFIER = 270,              /* IDENTIFIER  */
    AI_INFER_BUILTIN = 271,        /* AI_INFER_BUILTIN  */
    GREENAI_REPORT_BUILTIN = 272,  /* GREENAI_REPORT_BUILTIN  */
    INT_LITERAL = 273,             /* INT_LITERAL  */
    FLOAT_LITERAL = 274,           /* FLOAT_LITERAL  */
    READ = 275,                    /* READ  */
    PRINT = 276,                   /* PRINT  */
    GOTO = 277,                    /* GOTO  */
    BREAK = 278,                   /* BREAK  */
    WHILE = 279,                   /* WHILE  */
    LOOP = 280,                    /* LOOP  */
    ELSE = 281,                    /* ELSE  */
    IF = 282,                      /* IF  */
    DEF = 283,                     /* DEF  */
    INT = 284,                     /* INT  */
    FLOAT = 285,                   /* FLOAT  */
    STRING = 286,                  /* STRING  */
    VOID = 287,                    /* VOID  */
    BOOL = 288,                    /* BOOL  */
    DOUBLE = 289,                  /* DOUBLE  */
    RETURN = 290,                  /* RETURN  */
    CONTINUE = 291,                /* CONTINUE  */
    TRUE = 292,                    /* TRUE  */
    FALSE = 293,                   /* FALSE  */
    PACKAGE = 294,                 /* PACKAGE  */
    MODULE = 295,                  /* MODULE  */
    IMPORT = 296,                  /* IMPORT  */
    AS = 297,                      /* AS  */
    MODEL = 298,                   /* MODEL  */
    FORMAT = 299,                  /* FORMAT  */
    PATH = 300,                    /* PATH  */
    TASK = 301,                    /* TASK  */
    PRECISION = 302,               /* PRECISION  */
    INPUT_SHAPE = 303,             /* INPUT_SHAPE  */
    OUTPUT_SHAPE = 304,            /* OUTPUT_SHAPE  */
    BACKEND_PREFERENCE = 305,      /* BACKEND_PREFERENCE  */
    COMPACT = 306,                 /* COMPACT  */
    QUALITY_GUARDRAIL = 307,       /* QUALITY_GUARDRAIL  */
    GREENAI_CONTRACT_T = 308,      /* GREENAI_CONTRACT_T  */
    FUNCTIONAL_UNIT = 309,         /* FUNCTIONAL_UNIT  */
    SUCCESS_CRITERIA = 310,        /* SUCCESS_CRITERIA  */
    BOUNDARY = 311,                /* BOUNDARY  */
    MEASUREMENT_QUALITY = 312,     /* MEASUREMENT_QUALITY  */
    DATA_QUALITY = 313,            /* DATA_QUALITY  */
    CARBON_FACTOR = 314,           /* CARBON_FACTOR  */
    ENERGY_BUDGET_J = 315,         /* ENERGY_BUDGET_J  */
    CARBON_BUDGET_GCO2E = 316,     /* CARBON_BUDGET_GCO2E  */
    EVIDENCE_RETENTION = 317,      /* EVIDENCE_RETENTION  */
    CLAIMS_MODE = 318,             /* CLAIMS_MODE  */
    EVIDENCE_ONLY = 319,           /* EVIDENCE_ONLY  */
    GREENAI_MEASURE = 320,         /* GREENAI_MEASURE  */
    INFER = 321,                   /* INFER  */
    TENSOR = 322,                  /* TENSOR  */
    CERTIFICATION_PROFILE = 323,   /* CERTIFICATION_PROFILE  */
    CERTIFICATION = 324,           /* CERTIFICATION  */
    WORKLOAD = 325,                /* WORKLOAD  */
    MEASUREMENT_PLAN = 326,        /* MEASUREMENT_PLAN  */
    AI_LIFECYCLE = 327,            /* AI_LIFECYCLE  */
    RAG_PIPELINE = 328,            /* RAG_PIPELINE  */
    TOKEN_BUDGET = 329,            /* TOKEN_BUDGET  */
    MODEL_ROUTING = 330,           /* MODEL_ROUTING  */
    GUARDRAILS = 331,              /* GUARDRAILS  */
    INT8 = 332,                    /* INT8  */
    FP16 = 333,                    /* FP16  */
    FP32 = 334,                    /* FP32  */
    BF16 = 335,                    /* BF16  */
    INT4 = 336,                    /* INT4  */
    FP64 = 337,                    /* FP64  */
    ONNX = 338,                    /* ONNX  */
    ENGINE = 339,                  /* ENGINE  */
    TORCHSCRIPT = 340,             /* TORCHSCRIPT  */
    OPENVINO_IR = 341,             /* OPENVINO_IR  */
    GGUF = 342,                    /* GGUF  */
    TENSORRT = 343,                /* TENSORRT  */
    ONNXRUNTIME_TENSORRT = 344,    /* ONNXRUNTIME_TENSORRT  */
    ONNXRUNTIME_CUDA = 345,        /* ONNXRUNTIME_CUDA  */
    ONNXRUNTIME_CPU = 346,         /* ONNXRUNTIME_CPU  */
    OPENVINO = 347,                /* OPENVINO  */
    LIBTORCH = 348,                /* LIBTORCH  */
    LLAMACPP = 349,                /* LLAMACPP  */
    FALLBACK = 350,                /* FALLBACK  */
    MQ1 = 351,                     /* MQ1  */
    MQ2 = 352,                     /* MQ2  */
    MQ3 = 353,                     /* MQ3  */
    MQ4 = 354,                     /* MQ4  */
    DQ1 = 355,                     /* DQ1  */
    DQ2 = 356,                     /* DQ2  */
    DQ3 = 357,                     /* DQ3  */
    DQ4 = 358,                     /* DQ4  */
    LOCATION = 359,                /* LOCATION  */
    CI_CD = 360,                   /* CI_CD  */
    THIRDPARTY = 361,              /* THIRDPARTY  */
    ACCELERATOR = 362,             /* ACCELERATOR  */
    COMPUTE = 363,                 /* COMPUTE  */
    STORAGE = 364,                 /* STORAGE  */
    NETWORK = 365                  /* NETWORK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_HH_INCLUDED  */
