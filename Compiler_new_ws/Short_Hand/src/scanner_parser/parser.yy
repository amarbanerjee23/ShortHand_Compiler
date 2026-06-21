%{

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


%}



%start PROGRAMME_RULE

%type <program> PROGRAMME_RULE
%type <decl_block> DECLARATION_STATEMENT_LIST_RULE
%type <decl_block> DECLARATION_STATEMENT_RULE
%type <decl_block> DECLARATION_VARIABLE_LIST_RULE
%type <functions>  FUNCTION_LIST_RULE
%type <function>   FUNCTION_RULE
%type <code_block> LOGIC_BLOCK
%type <break_statement> BREAK
%type <block_statement> STATEMENT_BLOCK_RULE
%type <block_statement> STATEMENT_LIST_RULE
%type <statement> STATEMENT_RULE
%type <read_statement> READ_VARIABLE_LIST_RULE
%type <variable> VARIABLE_RULE
%type <print_statement> PRINT_VARIABLE_LIST_RULE
%type <greenai_report> GREENAI_REPORT_RULE
%type <ai_infer> AI_INFER_RULE
%type <model_decl> MODEL_DECLARATION
%type <tensor_decl> TENSOR_DECLARATION
%type <greenai_contract> GREENAI_CONTRACT
%type <greenai_measure> GREENAI_MEASUREMENT
%type <infer_statement> INFER_STATEMENT
%type <return_statement> RETURN_STATEMENT
%type <string_val> BACKEND_NAME FORMAT_NAME PRECISION_NAME MQ_NAME DQ_NAME BOUNDARY_NAME
%type <expression> EXPRESSION_RULE
%type <type> ShortType

%token ETOK

%right '='

%left OR
%left AND
%left EQUAL NOT_EQUAL
%left LESS LESS_OR_EQUAL GREATER GREATER_OR_EQUAL
%left '+' '-'
%left '*' '/' '%'
%right UMINUS

%token <string_val> STRING_LITERAL
%token <string_val> IDENTIFIER
%token <int_val> INT_LITERAL
%token <float_val> FLOAT_LITERAL
%token READ
%token PRINT
%token GOTO BREAK
%token WHILE
%token LOOP
%token ELSE
%token IF DEF
%token INT FLOAT STRING VOID BOOL DOUBLE RETURN CONTINUE TRUE FALSE MODEL FORMAT PATH TASK PRECISION INPUT_SHAPE OUTPUT_SHAPE BACKEND_PREFERENCE COMPACT QUALITY_GUARDRAIL GREENAI_CONTRACT_T FUNCTIONAL_UNIT SUCCESS_CRITERIA BOUNDARY MEASUREMENT_QUALITY DATA_QUALITY CARBON_FACTOR ENERGY_BUDGET_J CARBON_BUDGET_GCO2E EVIDENCE_RETENTION CLAIMS_MODE EVIDENCE_ONLY GREENAI_MEASURE INFER TENSOR INT8 FP16 FP32 BF16 INT4 FP64 ONNX ENGINE TORCHSCRIPT OPENVINO_IR GGUF TENSORRT ONNXRUNTIME_TENSORRT ONNXRUNTIME_CUDA ONNXRUNTIME_CPU OPENVINO LIBTORCH LLAMACPP FALLBACK MQ1 MQ2 MQ3 MQ4 DQ1 DQ2 DQ3 DQ4 LOCATION CI_CD THIRDPARTY ACCELERATOR COMPUTE STORAGE NETWORK

%%


PROGRAMME_RULE:    DECLARATION_STATEMENT_LIST_RULE FUNCTION_LIST_RULE LOGIC_BLOCK
            {
                //fprintf(bison_output, "program\n");
                $$ = new AST_PROGRAM($1,$2,$3);
                main_program = $$;
            }
;


FUNCTION_LIST_RULE: FUNCTION_LIST_RULE FUNCTION_RULE ';'
                    {
                        $$ = $1;
                        $$->push_back($2);
                    }
                    | %empty
                    {
                        $$ = new AST_FUNCTION_LIST_RULE();
                    }
;

FUNCTION_RULE: DEF ShortType IDENTIFIER '(' DECLARATION_STATEMENT_LIST_RULE ')' STATEMENT_BLOCK_RULE
	      		{
	        	$$ = new AST_FUNCTION_RULE($2,$3,$5,$7);
	      		};

DECLARATION_STATEMENT_LIST_RULE:   DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';'
                       {
                           $$ = $1;
                           $$->push_back($2);
                       }

                       |   DECLARATION_STATEMENT_RULE ';'
                       {
                           $$ = $1;
                       }

;


ShortType: INT {$$=ShortType::Int;}
      | FLOAT {$$=ShortType::Float;} | DOUBLE {$$=ShortType::Float;}
      | STRING {$$=ShortType::String;}
      | VOID {$$=ShortType::Void;}
      | BOOL{$$=ShortType::Boolean;}
      ;

DECLARATION_STATEMENT_RULE:    ShortType DECLARATION_VARIABLE_LIST_RULE
                   {
                       //fprintf(bison_output, "DECLARATION_STATEMENT_RULE\n");
                       $$ = $2;
                   }
;


DECLARATION_VARIABLE_LIST_RULE:    DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER
                       {
                           $$ = $1;
                           $$->push_back(string($3));
                       }
                  |    DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']'
                       {
                           $$ = $1;
                           $$->push_back(string($3), $5);
                       }
                  |    IDENTIFIER
                       {
                           $$ = new AST_DATA_DECLARATION_BLOCK();
                           $$->push_back(string($1));
                       }
                  |    IDENTIFIER '[' INT_LITERAL ']'
                       {
                           $$ = new AST_DATA_DECLARATION_BLOCK();
                           $$->push_back(string($1), $3);
                       }
                  | %empty {$$ = new AST_DATA_DECLARATION_BLOCK();}
;


LOGIC_BLOCK:     STATEMENT_LIST_RULE
               {
                   //fprintf(bison_output, "LOGIC_BLOCK\n");
                   $$ = new AST_LOGIC_BLOCK($1);
               }
;


STATEMENT_BLOCK_RULE:    '{' STATEMENT_LIST_RULE '}'
                    {
                        //fprintf(bison_output, "STATEMENT_BLOCK_RULE\n");
                        $$ = $2;
                    }
;


STATEMENT_LIST_RULE:    STATEMENT_LIST_RULE STATEMENT_RULE
                   {
                       $$->push_back($2);
                   }
              |    STATEMENT_RULE
                   {
                       $$ = new AST_STATEMENTS_BLOCK();
                       $$->push_back($1);
                   }
;

STATEMENT_RULE:    MODEL_DECLARATION { $$ = $1; }
         | TENSOR_DECLARATION { $$ = $1; }
         | GREENAI_CONTRACT { $$ = $1; }
         | GREENAI_MEASUREMENT { $$ = $1; }
         | INFER_STATEMENT { $$ = $1; }
         | RETURN_STATEMENT { $$ = $1; }
         | CONTINUE ';' { $$ = new AST_CONTINUE(); }
         | EXPRESSION_RULE ';'
              {
                  $$ = new AST_EXPRESSION_STATEMENT_RULE($1);
              }
         |    VARIABLE_RULE '=' EXPRESSION_RULE ';'
              {
                  $$ = new AST_ASSIGNMENT_RULE($1, $3);
              }

         |    IDENTIFIER'(' READ_VARIABLE_LIST_RULE ')' ';'
	      {
	       $$ = new AST_FUNCTION_CALL_RULE($1,$3);
	      }
         |    STATEMENT_BLOCK_RULE
              {
                  $$ = $1;
              }
         |    IF EXPRESSION_RULE STATEMENT_BLOCK_RULE
              {
                  $$ = new AST_IF_STATEMENT($2, $3);
              }
         |    IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE
              {
                  $$ = new AST_IF_ELSE_STATEMENT($2, $3, $5);
              }
         |    LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE
              {
                  $$ = new AST_FOR_LOOP_STATEMENT_RULE($2, $4, $6, $7);
              }
         |    LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE
              {
                  $$ = new AST_FOR_LOOP_STATEMENT_RULE($2, $4, $6, $8, $9);
              }
         |    LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE
              {
                  $$ = new AST_WHILE_LOOP_STATEMENT_RULE($2, $3);
              }
         |    GOTO IDENTIFIER ';'
              {
                  $$ = new AST_GOTO_STATEMENT_RULE(string($2));
              }
         |    GOTO IDENTIFIER IF EXPRESSION_RULE ';'
              {
                  $$ = new AST_GOTO_STATEMENT_RULE($4, string($2));
              }
         |    READ READ_VARIABLE_LIST_RULE ';'
              {
                  $$ = $2;
              }
         | BREAK ';' {$$ = new AST_BREAK();}
         |    PRINT PRINT_VARIABLE_LIST_RULE ';'
              {
                  $$ = $2;
              }
         |    GREENAI_REPORT_RULE ';'
              {
                  $$ = $1;
              }
         |    AI_INFER_RULE ';'
              {
                  $$ = $1;
              }
         |    IDENTIFIER ':'
              {
                  $$ = new AST_LABEL_RULE(string($1));
              }
         |    ';'
              {
                  $$ = new AST_EXPRESSION_STATEMENT_RULE(new AST_LITERAL(1));
              }

;

RETURN_STATEMENT: RETURN EXPRESSION_RULE ';' { $$ = new AST_RETURN_STATEMENT($2); } | RETURN ';' { $$ = new AST_RETURN_STATEMENT(); };

INFER_STATEMENT: INFER IDENTIFIER '(' IDENTIFIER ')' GREATER IDENTIFIER ';' { $$ = new AST_INFER_STATEMENT(string($2), string($4), string($7)); };

TENSOR_DECLARATION: TENSOR IDENTIFIER PRECISION_NAME STRING_LITERAL ';' { TensorDeclarationData d; d.name=$2; d.element_type=$3; d.shape_csv=string($4).substr(1,string($4).size()-2); d.dynamic=(d.shape_csv=="dynamic"); d.rank= d.dynamic?0:1; long long total=1; if(!d.dynamic){ d.rank=0; size_t start=0; while(start<d.shape_csv.size()){ size_t pos=d.shape_csv.find(',',start); string part=d.shape_csv.substr(start,pos==string::npos?string::npos:pos-start); total*= atoll(part.c_str()); d.rank++; if(pos==string::npos) break; start=pos+1; }} d.total_elements=total; $$=new AST_TENSOR_DECLARATION(d); };

MODEL_DECLARATION: MODEL IDENTIFIER '{' { current_model=ModelDeclarationData(); current_model.name=$2; } MODEL_FIELD_LIST '}' ';' { $$ = new AST_MODEL_DECLARATION(current_model); };
MODEL_FIELD_LIST: MODEL_FIELD_LIST MODEL_FIELD | %empty;
MODEL_FIELD: FORMAT FORMAT_NAME ';' { current_model.format=$2; } | PATH STRING_LITERAL ';' { current_model.path=string($2).substr(1,string($2).size()-2); } | TASK STRING_LITERAL ';' { current_model.task=string($2).substr(1,string($2).size()-2); } | PRECISION PRECISION_NAME ';' { current_model.precision=$2; } | INPUT_SHAPE STRING_LITERAL ';' { current_model.input_shape=string($2).substr(1,string($2).size()-2); } | OUTPUT_SHAPE STRING_LITERAL ';' { current_model.output_shape=string($2).substr(1,string($2).size()-2); } | BACKEND_PREFERENCE BACKEND_LIST ';' | COMPACT TRUE ';' { current_model.compact=true; } | COMPACT FALSE ';' { current_model.compact=false; } | QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';' { current_model.has_quality_guardrail=true; current_model.quality_guardrail={string($2),">=",(double)$4}; };
BACKEND_LIST: BACKEND_LIST ',' BACKEND_NAME { current_model.backend_preference.push_back($3); } | BACKEND_NAME { current_model.backend_preference.push_back($1); };

GREENAI_CONTRACT: GREENAI_CONTRACT_T IDENTIFIER '{' { current_contract=GreenAIContractData(); current_contract.name=$2; } CONTRACT_FIELD_LIST '}' ';' { $$ = new AST_GREENAI_CONTRACT(current_contract); };
CONTRACT_FIELD_LIST: CONTRACT_FIELD_LIST CONTRACT_FIELD | %empty;
CONTRACT_FIELD: FUNCTIONAL_UNIT STRING_LITERAL ';' { current_contract.functional_unit=string($2).substr(1,string($2).size()-2); current_contract.has_functional_unit=true; } | SUCCESS_CRITERIA STRING_LITERAL ';' { current_contract.success_criteria=string($2).substr(1,string($2).size()-2); current_contract.has_success_criteria=true; } | BOUNDARY BOUNDARY_LIST ';' { current_contract.has_boundary=true; } | MEASUREMENT_QUALITY MQ_NAME ';' { current_contract.measurement_quality=$2; current_contract.has_mq=true; } | DATA_QUALITY DQ_NAME ';' { current_contract.data_quality=$2; current_contract.has_dq=true; } | CARBON_FACTOR LOCATION INT_LITERAL ';' { current_contract.carbon_factor_scope="location"; current_contract.carbon_factor=$3; current_contract.has_carbon_factor=true; } | ENERGY_BUDGET_J INT_LITERAL ';' { current_contract.energy_budget_j=$2; } | CARBON_BUDGET_GCO2E INT_LITERAL ';' { current_contract.carbon_budget_gco2e=$2; } | QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';' { current_contract.has_quality_guardrail=true; current_contract.quality_guardrail={string($2),">=",(double)$4}; } | EVIDENCE_RETENTION STRING_LITERAL ';' { current_contract.evidence_retention=string($2).substr(1,string($2).size()-2); } | CLAIMS_MODE EVIDENCE_ONLY ';' { current_contract.claims_mode="evidence_only"; };
BOUNDARY_LIST: BOUNDARY_LIST ',' BOUNDARY_NAME { current_contract.boundary.push_back($3); } | BOUNDARY_NAME { current_contract.boundary.push_back($1); };

GREENAI_MEASUREMENT: GREENAI_MEASURE IDENTIFIER '{' { current_measure=GreenAIMeasurementData(); current_measure.workload=$2; } MEASURE_FIELD_LIST '}' ';' { $$ = new AST_GREENAI_MEASUREMENT(current_measure); };
MEASURE_FIELD_LIST: MEASURE_FIELD_LIST MEASURE_FIELD | %empty;
MEASURE_FIELD: IDENTIFIER INT_LITERAL ';' { string n=$1; if(n=="inferences") current_measure.inferences=$2; else if(n=="watts") current_measure.watts=$2; else if(n=="seconds") current_measure.seconds=$2; } | IDENTIFIER IDENTIFIER ';' { if(string($1)=="backend") current_measure.backend=$2; };

FORMAT_NAME: ONNX {$$=(char*)"onnx";} | ENGINE {$$=(char*)"engine";} | TORCHSCRIPT {$$=(char*)"torchscript";} | OPENVINO_IR {$$=(char*)"openvino_ir";} | GGUF {$$=(char*)"gguf";};
PRECISION_NAME: INT8 {$$=(char*)"int8";} | INT4 {$$=(char*)"int4";} | FP16 {$$=(char*)"fp16";} | FP32 {$$=(char*)"fp32";} | BF16 {$$=(char*)"bf16";} | FP64 {$$=(char*)"fp64";} | FLOAT {$$=(char*)"float";} ;
BACKEND_NAME: TENSORRT {$$=(char*)"tensorrt";} | ONNXRUNTIME_TENSORRT {$$=(char*)"onnxruntime_tensorrt";} | ONNXRUNTIME_CUDA {$$=(char*)"onnxruntime_cuda";} | ONNXRUNTIME_CPU {$$=(char*)"onnxruntime_cpu";} | OPENVINO {$$=(char*)"openvino";} | LIBTORCH {$$=(char*)"libtorch";} | LLAMACPP {$$=(char*)"llamacpp";} | FALLBACK {$$=(char*)"fallback";};
MQ_NAME: MQ1 {$$=(char*)"MQ1";} | MQ2 {$$=(char*)"MQ2";} | MQ3 {$$=(char*)"MQ3";} | MQ4 {$$=(char*)"MQ4";};
DQ_NAME: DQ1 {$$=(char*)"DQ1";} | DQ2 {$$=(char*)"DQ2";} | DQ3 {$$=(char*)"DQ3";} | DQ4 {$$=(char*)"DQ4";};
BOUNDARY_NAME: COMPUTE {$$=(char*)"compute";} | ACCELERATOR {$$=(char*)"accelerator";} | STORAGE {$$=(char*)"storage";} | NETWORK {$$=(char*)"network";} | CI_CD {$$=(char*)"ci_cd";} | THIRDPARTY {$$=(char*)"thirdparty";};

AI_INFER_RULE: IDENTIFIER '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')'
              {
                  if ((string($1) != "ai_infer" && string($1) != "aiinfer")) {
                      yyerror((char *)"expected ai_infer builtin");
                      YYERROR;
                  }
                  $$ = new AST_AI_INFER_RULE(string($3), string($5), string($7));
              }
;

GREENAI_REPORT_RULE: IDENTIFIER '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')'
              {
                  if (string($1) != "greenai") {
                      yyerror((char *)"expected greenai report builtin");
                      YYERROR;
                  }
                  $$ = new AST_GREENAI_REPORT_RULE(string($3), $5, $7, $9);
              }
;


EXPRESSION_RULE:    EXPRESSION_RULE '+' EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "+");
               }
          |    EXPRESSION_RULE '-' EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "-");
               }
          |    EXPRESSION_RULE '*' EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "*");
               }
          |    EXPRESSION_RULE '/' EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "/");
               }
          |    EXPRESSION_RULE '%' EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "%");
               }
          |    EXPRESSION_RULE LESS EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "<");
               }
          |    EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "<=");
               }
          |    EXPRESSION_RULE GREATER EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, ">");
               }
          |    EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, ">=");
               }
          |    EXPRESSION_RULE EQUAL EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "==");
               }
          |    EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "!=");
               }
          |    EXPRESSION_RULE OR EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "||");
               }
          |    EXPRESSION_RULE AND EXPRESSION_RULE
               {
                   $$ = new AST_BINARY_EXPRESSION_RULE($1, $3, "&&");
               }
          |    '-' EXPRESSION_RULE    %prec UMINUS
               {
                   $$ = new AST_UNARY_EXPRESSION_RULE($2, "-");
               }
          |    '(' EXPRESSION_RULE ')'
               {
                   $$ = $2;
               }
          |    VARIABLE_RULE
               {
                   $$ = $1;
               }
          |    INT_LITERAL { $$ = new AST_LITERAL($1); }
          |    FLOAT_LITERAL { $$ = new AST_FLOAT_LITERAL($1); }
          |    TRUE { $$ = new AST_BOOL_LITERAL(true); }
          |    FALSE { $$ = new AST_BOOL_LITERAL(false); }
;


VARIABLE_RULE:    IDENTIFIER
             {
                 $$ = new AST_SIMPLE_VARIABLE(string($1));
             }
        |    IDENTIFIER '[' EXPRESSION_RULE ']'
             {
                 $$ = new AST_ARRAY_VARIABLE(string($1), $3);
             }
;

READ_VARIABLE_LIST_RULE:    READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE
                       {
                           $$->push_back($3);
                       }
                  |    VARIABLE_RULE
                       {
                           $$ = new AST_READ_RULE();
                           $$->push_back($1);
                       }
;


PRINT_VARIABLE_LIST_RULE:    PRINT_VARIABLE_LIST_RULE ',' STRING_LITERAL
                   {
                       $$->push_back(new AST_STRING_LITERAL(string($3)));
                   }
              |    PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE
                   {
                       $$->push_back($3);
                   }
              |    STRING_LITERAL
                   {
                       $$ = new AST_PRINT_RULE();
                       $$->push_back(new AST_STRING_LITERAL(string($1)));
                   }
              |    EXPRESSION_RULE
                   {
                       $$ = new AST_PRINT_RULE();
                       $$->push_back($1);
                   }
;


%%


void yyerror (char const *s)
{
        fprintf (stderr, "----------------ERROR----------------\n");
        fprintf (stderr, "%s\n", s);
}
