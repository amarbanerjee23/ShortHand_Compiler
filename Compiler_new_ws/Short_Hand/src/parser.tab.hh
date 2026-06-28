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
    INT_LITERAL = 271,             /* INT_LITERAL  */
    FLOAT_LITERAL = 272,           /* FLOAT_LITERAL  */
    READ = 273,                    /* READ  */
    PRINT = 274,                   /* PRINT  */
    GOTO = 275,                    /* GOTO  */
    BREAK = 276,                   /* BREAK  */
    WHILE = 277,                   /* WHILE  */
    LOOP = 278,                    /* LOOP  */
    ELSE = 279,                    /* ELSE  */
    IF = 280,                      /* IF  */
    DEF = 281,                     /* DEF  */
    INT = 282,                     /* INT  */
    FLOAT = 283,                   /* FLOAT  */
    STRING = 284,                  /* STRING  */
    VOID = 285,                    /* VOID  */
    BOOL = 286,                    /* BOOL  */
    DOUBLE = 287,                  /* DOUBLE  */
    RETURN = 288,                  /* RETURN  */
    CONTINUE = 289,                /* CONTINUE  */
    TRUE = 290,                    /* TRUE  */
    FALSE = 291,                   /* FALSE  */
    MODEL = 292,                   /* MODEL  */
    FORMAT = 293,                  /* FORMAT  */
    PATH = 294,                    /* PATH  */
    TASK = 295,                    /* TASK  */
    PRECISION = 296,               /* PRECISION  */
    INPUT_SHAPE = 297,             /* INPUT_SHAPE  */
    OUTPUT_SHAPE = 298,            /* OUTPUT_SHAPE  */
    BACKEND_PREFERENCE = 299,      /* BACKEND_PREFERENCE  */
    COMPACT = 300,                 /* COMPACT  */
    QUALITY_GUARDRAIL = 301,       /* QUALITY_GUARDRAIL  */
    GREENAI_CONTRACT_T = 302,      /* GREENAI_CONTRACT_T  */
    FUNCTIONAL_UNIT = 303,         /* FUNCTIONAL_UNIT  */
    SUCCESS_CRITERIA = 304,        /* SUCCESS_CRITERIA  */
    BOUNDARY = 305,                /* BOUNDARY  */
    MEASUREMENT_QUALITY = 306,     /* MEASUREMENT_QUALITY  */
    DATA_QUALITY = 307,            /* DATA_QUALITY  */
    CARBON_FACTOR = 308,           /* CARBON_FACTOR  */
    ENERGY_BUDGET_J = 309,         /* ENERGY_BUDGET_J  */
    CARBON_BUDGET_GCO2E = 310,     /* CARBON_BUDGET_GCO2E  */
    EVIDENCE_RETENTION = 311,      /* EVIDENCE_RETENTION  */
    CLAIMS_MODE = 312,             /* CLAIMS_MODE  */
    EVIDENCE_ONLY = 313,           /* EVIDENCE_ONLY  */
    GREENAI_MEASURE = 314,         /* GREENAI_MEASURE  */
    INFER = 315,                   /* INFER  */
    TENSOR = 316,                  /* TENSOR  */
    INT8 = 317,                    /* INT8  */
    FP16 = 318,                    /* FP16  */
    FP32 = 319,                    /* FP32  */
    BF16 = 320,                    /* BF16  */
    INT4 = 321,                    /* INT4  */
    FP64 = 322,                    /* FP64  */
    ONNX = 323,                    /* ONNX  */
    ENGINE = 324,                  /* ENGINE  */
    TORCHSCRIPT = 325,             /* TORCHSCRIPT  */
    OPENVINO_IR = 326,             /* OPENVINO_IR  */
    GGUF = 327,                    /* GGUF  */
    TENSORRT = 328,                /* TENSORRT  */
    ONNXRUNTIME_TENSORRT = 329,    /* ONNXRUNTIME_TENSORRT  */
    ONNXRUNTIME_CUDA = 330,        /* ONNXRUNTIME_CUDA  */
    ONNXRUNTIME_CPU = 331,         /* ONNXRUNTIME_CPU  */
    OPENVINO = 332,                /* OPENVINO  */
    LIBTORCH = 333,                /* LIBTORCH  */
    LLAMACPP = 334,                /* LLAMACPP  */
    FALLBACK = 335,                /* FALLBACK  */
    MQ1 = 336,                     /* MQ1  */
    MQ2 = 337,                     /* MQ2  */
    MQ3 = 338,                     /* MQ3  */
    MQ4 = 339,                     /* MQ4  */
    DQ1 = 340,                     /* DQ1  */
    DQ2 = 341,                     /* DQ2  */
    DQ3 = 342,                     /* DQ3  */
    DQ4 = 343,                     /* DQ4  */
    LOCATION = 344,                /* LOCATION  */
    CI_CD = 345,                   /* CI_CD  */
    THIRDPARTY = 346,              /* THIRDPARTY  */
    ACCELERATOR = 347,             /* ACCELERATOR  */
    COMPUTE = 348,                 /* COMPUTE  */
    STORAGE = 349,                 /* STORAGE  */
    NETWORK = 350                  /* NETWORK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_HH_INCLUDED  */
