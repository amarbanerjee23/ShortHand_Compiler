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
    UMINUS = 267,                  /* UMINUS  */
    STRING_LITERAL = 268,          /* STRING_LITERAL  */
    IDENTIFIER = 269,              /* IDENTIFIER  */
    INT_LITERAL = 270,             /* INT_LITERAL  */
    FLOAT_LITERAL = 271,           /* FLOAT_LITERAL  */
    READ = 272,                    /* READ  */
    PRINT = 273,                   /* PRINT  */
    GOTO = 274,                    /* GOTO  */
    BREAK = 275,                   /* BREAK  */
    WHILE = 276,                   /* WHILE  */
    LOOP = 277,                    /* LOOP  */
    ELSE = 278,                    /* ELSE  */
    IF = 279,                      /* IF  */
    DEF = 280,                     /* DEF  */
    INT = 281,                     /* INT  */
    FLOAT = 282,                   /* FLOAT  */
    STRING = 283,                  /* STRING  */
    VOID = 284,                    /* VOID  */
    BOOL = 285,                    /* BOOL  */
    DOUBLE = 286,                  /* DOUBLE  */
    RETURN = 287,                  /* RETURN  */
    CONTINUE = 288,                /* CONTINUE  */
    TRUE = 289,                    /* TRUE  */
    FALSE = 290,                   /* FALSE  */
    MODEL = 291,                   /* MODEL  */
    FORMAT = 292,                  /* FORMAT  */
    PATH = 293,                    /* PATH  */
    TASK = 294,                    /* TASK  */
    PRECISION = 295,               /* PRECISION  */
    INPUT_SHAPE = 296,             /* INPUT_SHAPE  */
    OUTPUT_SHAPE = 297,            /* OUTPUT_SHAPE  */
    BACKEND_PREFERENCE = 298,      /* BACKEND_PREFERENCE  */
    COMPACT = 299,                 /* COMPACT  */
    QUALITY_GUARDRAIL = 300,       /* QUALITY_GUARDRAIL  */
    GREENAI_CONTRACT_T = 301,      /* GREENAI_CONTRACT_T  */
    FUNCTIONAL_UNIT = 302,         /* FUNCTIONAL_UNIT  */
    SUCCESS_CRITERIA = 303,        /* SUCCESS_CRITERIA  */
    BOUNDARY = 304,                /* BOUNDARY  */
    MEASUREMENT_QUALITY = 305,     /* MEASUREMENT_QUALITY  */
    DATA_QUALITY = 306,            /* DATA_QUALITY  */
    CARBON_FACTOR = 307,           /* CARBON_FACTOR  */
    ENERGY_BUDGET_J = 308,         /* ENERGY_BUDGET_J  */
    CARBON_BUDGET_GCO2E = 309,     /* CARBON_BUDGET_GCO2E  */
    EVIDENCE_RETENTION = 310,      /* EVIDENCE_RETENTION  */
    CLAIMS_MODE = 311,             /* CLAIMS_MODE  */
    EVIDENCE_ONLY = 312,           /* EVIDENCE_ONLY  */
    GREENAI_MEASURE = 313,         /* GREENAI_MEASURE  */
    INFER = 314,                   /* INFER  */
    TENSOR = 315,                  /* TENSOR  */
    INT8 = 316,                    /* INT8  */
    FP16 = 317,                    /* FP16  */
    FP32 = 318,                    /* FP32  */
    BF16 = 319,                    /* BF16  */
    INT4 = 320,                    /* INT4  */
    FP64 = 321,                    /* FP64  */
    ONNX = 322,                    /* ONNX  */
    ENGINE = 323,                  /* ENGINE  */
    TORCHSCRIPT = 324,             /* TORCHSCRIPT  */
    OPENVINO_IR = 325,             /* OPENVINO_IR  */
    GGUF = 326,                    /* GGUF  */
    TENSORRT = 327,                /* TENSORRT  */
    ONNXRUNTIME_TENSORRT = 328,    /* ONNXRUNTIME_TENSORRT  */
    ONNXRUNTIME_CUDA = 329,        /* ONNXRUNTIME_CUDA  */
    ONNXRUNTIME_CPU = 330,         /* ONNXRUNTIME_CPU  */
    OPENVINO = 331,                /* OPENVINO  */
    LIBTORCH = 332,                /* LIBTORCH  */
    LLAMACPP = 333,                /* LLAMACPP  */
    FALLBACK = 334,                /* FALLBACK  */
    MQ1 = 335,                     /* MQ1  */
    MQ2 = 336,                     /* MQ2  */
    MQ3 = 337,                     /* MQ3  */
    MQ4 = 338,                     /* MQ4  */
    DQ1 = 339,                     /* DQ1  */
    DQ2 = 340,                     /* DQ2  */
    DQ3 = 341,                     /* DQ3  */
    DQ4 = 342,                     /* DQ4  */
    LOCATION = 343,                /* LOCATION  */
    CI_CD = 344,                   /* CI_CD  */
    THIRDPARTY = 345,              /* THIRDPARTY  */
    ACCELERATOR = 346,             /* ACCELERATOR  */
    COMPUTE = 347,                 /* COMPUTE  */
    STORAGE = 348,                 /* STORAGE  */
    NETWORK = 349                  /* NETWORK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_HH_INCLUDED  */
