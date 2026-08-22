# ShortHand beta-0.2 language grammar

Language version: beta-0.2

Conformance contract: beta-0.2

grammar_conformance_status: parser_accurate_matrix_guarded

grammar_matrix: tests/conformance/grammar_matrix_beta_0_2.tsv

parser_only_validation_mode: short_hand source.short parse

production_claim: false

Historical stability marker retained for earlier gates: Language version: beta-0.1

This document describes the syntax accepted by `scanner.ll` and `parser.yy`. It deliberately does not advertise aliases that only the runtime type parser understands. Semantic acceptance is a separate contract enforced after parsing.

## Notation

```ebnf
A , B        sequence
A | B        alternative
[ A ]        optional
{ A }        zero or more
```

## Lexical grammar

```ebnf
letter          = "A"…"Z" | "a"…"z" | "_" ;
digit           = "0"…"9" ;
identifier      = letter , { letter | digit } ;
integer_literal = digit , { digit } ;
float_literal   = digit , { digit } , "." , digit , { digit } ;
string_literal  = '"' , { character - '"' } , '"' ;
line_comment    = "//" , { character - newline }
                | "#" , { character - newline } ;
block_comment   = "/*" , { character } , "*/" ;
```

Whitespace and comments are discarded by the scanner. Identifiers and keywords are case-sensitive. Backslash escape processing, including escaped quotes, is not part of the beta-0.2 string-literal contract.

## Program shape

The current parser has an ordered top-level shape. Primitive declarations come first, then function definitions, then a non-empty logic statement list.

```ebnf
program             = global_declarations , function_definitions , logic_block ;
global_declarations = declaration_statement , ";" ,
                      { declaration_statement , ";" } ;
function_definitions = { function_definition , ";" } ;
logic_block          = statement_list ;
statement_list       = statement , { statement } ;
```

An empty program is not conforming. AI and Green AI declarations are statements in the logic block, not members of the initial primitive declaration section.

## Primitive declarations and functions

```ebnf
short_type            = "int" | "float" | "double" | "string"
                      | "void" | "bool" ;
declaration_statement = short_type , [ declarator_list ] ;
declarator_list       = declarator , { "," , declarator } ;
declarator            = identifier | identifier , "[" , integer_literal , "]" ;

function_definition   = "def" , short_type , identifier , "(" ,
                        parameter_declarations , ")" , block ;
parameter_declarations = declaration_statement , ";" ,
                         { declaration_statement , ";" } ;
```

Parameter declarations are semicolon-terminated. The current parser does not accept an empty `()` parameter list. This limitation is explicit in the beta-0.2 boundary corpus and is not presented as an ideal final-language design.

## Blocks and core statements

```ebnf
block                = "{" , statement_list , "}" ;
statement            = model_declaration
                     | tensor_declaration
                     | greenai_contract
                     | greenai_measurement
                     | infer_statement
                     | return_statement
                     | continue_statement
                     | expression_statement
                     | assignment
                     | function_call
                     | block
                     | if_statement
                     | loop_statement
                     | goto_statement
                     | read_statement
                     | break_statement
                     | print_statement
                     | greenai_report
                     | legacy_ai_infer
                     | label_statement
                     | empty_statement ;

assignment           = variable , "=" , expression , ";" ;
expression_statement = expression , ";" ;
function_call        = identifier , "(" , variable_list , ")" , ";" ;
if_statement         = "if" , expression , block , [ "else" , block ] ;
loop_statement       = "loop" , variable , "=" , expression , "," , expression , block
                     | "loop" , variable , "=" , expression , "," , expression ,
                       "," , expression , block
                     | "loop" , expression , block ;
goto_statement       = "goto" , identifier , [ "if" , expression ] , ";" ;
read_statement       = "read" , variable_list , ";" ;
break_statement      = "break" , ";" ;
continue_statement   = "continue" , ";" ;
return_statement     = "return" , [ expression ] , ";" ;
print_statement      = "print" , printable , { "," , printable } , ";" ;
printable            = string_literal | expression ;
label_statement      = identifier , ":" ;
empty_statement      = ";" ;
variable_list        = variable , { "," , variable } ;
```

Blocks are non-empty in beta-0.2. `loop condition { ... }` is the accepted condition-loop spelling. The scanner reserves `while`, but the parser has no `while` production. `for` is not a keyword.

Function-call arguments are variables, not arbitrary expressions, in the current grammar.

## Expressions

Precedence, from lowest to highest, is assignment, logical OR, logical AND, equality, relational, addition/subtraction, multiplication/division/remainder, and unary minus.

```ebnf
expression       = variable
                 | integer_literal
                 | float_literal
                 | "true"
                 | "false"
                 | "-" , expression
                 | "(" , expression , ")"
                 | expression , binary_operator , expression ;
binary_operator  = "+" | "-" | "*" | "/" | "%"
                 | "<" | "<=" | ">" | ">="
                 | "==" | "!=" | "||" | "&&" ;
variable         = identifier | identifier , "[" , expression , "]" ;
```

String literals and function calls are not general expressions in the historical beta-0.2 base matrix.

### Beta-0.4 expression extension

The active beta-0.4 contract adds `string_literal` as an expression alternative. The print production consequently consumes expressions directly while preserving every beta-0.2 print fixture. Function calls remain statements with variable arguments.

```ebnf
beta_0_4_expression = expression | string_literal ;
print_statement     = "print" , beta_0_4_expression ,
                      { "," , beta_0_4_expression } , ";" ;
```

This additive extension is traced by `tests/conformance/type_matrix_beta_0_4.tsv` and does not rewrite the historical beta-0.2 matrix.

## Tensor declarations

```ebnf
precision_name     = "int8" | "int4" | "fp16" | "fp32"
                   | "bf16" | "fp64" | "float" ;
tensor_declaration = "tensor" , identifier , precision_name ,
                     string_literal , ";" ;
```

The shape is syntactically a string. Shape validity and element-type support are semantic concerns. Aliases such as `float32`, `f32`, `float16`, `f16`, `bfloat16`, and `int32` are not scanner tokens in this contract.

## Model declarations

```ebnf
model_format      = "onnx" | "engine" | "torchscript"
                  | "openvino_ir" | "gguf" ;
backend_name      = "tensorrt" | "onnxruntime_tensorrt"
                  | "onnxruntime_cuda" | "onnxruntime_cpu"
                  | "openvino" | "libtorch" | "llamacpp"
                  | "fallback" ;
model_declaration = "model" , identifier , "{" ,
                    { model_field } , "}" , ";" ;
model_field       = "format" , model_format , ";"
                  | "path" , string_literal , ";"
                  | "task" , string_literal , ";"
                  | "precision" , precision_name , ";"
                  | "input_shape" , string_literal , ";"
                  | "output_shape" , string_literal , ";"
                  | "backend_preference" , backend_name ,
                    { "," , backend_name } , ";"
                  | "compact" , ( "true" | "false" ) , ";"
                  | "quality_guardrail" , identifier , ">=" ,
                    integer_literal , ";" ;
```

The grammar permits fields in any order and permits an empty model block. Semantic validation defines required fields, valid formats, shape validity, precision support, backend compatibility and guardrail requirements. Only an integer `>=` quality guardrail is syntactically accepted in beta-0.2.

## Inference and legacy AI compatibility

```ebnf
infer_statement = "infer" , identifier , "(" , identifier , ")" ,
                  ( "->" | ">" ) , identifier , ";" ;
legacy_ai_infer = ( "ai_infer" | "aiinfer" ) , "(" ,
                  string_literal , "," , string_literal , "," ,
                  string_literal , ")" , ";" ;
```

`->` is canonical. `>` is retained as a compatibility form. The parser validates the legacy builtin name and emits a stable parser diagnostic for other identifiers using that shape.

## Green AI contracts

```ebnf
boundary_name    = "compute" | "accelerator" | "storage"
                 | "network" | "ci_cd" | "thirdparty" ;
mq_name          = "MQ1" | "MQ2" | "MQ3" | "MQ4" ;
dq_name          = "DQ1" | "DQ2" | "DQ3" | "DQ4" ;

greenai_contract = "greenai_contract" , identifier , "{" ,
                   { contract_field } , "}" , ";" ;
contract_field   = "functional_unit" , string_literal , ";"
                 | "success_criteria" , string_literal , ";"
                 | "boundary" , boundary_name ,
                   { "," , boundary_name } , ";"
                 | "measurement_quality" , mq_name , ";"
                 | "data_quality" , dq_name , ";"
                 | "carbon_factor" , "location" , integer_literal , ";"
                 | "energy_budget_j" , integer_literal , ";"
                 | "carbon_budget_gco2e" , integer_literal , ";"
                 | "quality_guardrail" , identifier , ">=" ,
                   integer_literal , ";"
                 | "evidence_retention" , string_literal , ";"
                 | "claims_mode" , "evidence_only" , ";" ;
```

Semantic validation determines which contract fields are mandatory and preserves the claim-safe `evidence_only` boundary.

## Green AI measurements and reports

```ebnf
greenai_measurement = "greenai_measure" , identifier , "{" ,
                      { measurement_field } , "}" , ";" ;
measurement_field   = identifier , integer_literal , ";"
                    | identifier , identifier , ";" ;
greenai_report      = "greenai" , "(" , string_literal , "," ,
                      expression , "," , expression , "," ,
                      expression , ")" , ";" ;
```

The parser accepts generic measurement field identifiers. The current AST recognizes `inferences`, `watts`, `seconds`, and `backend`. The Green AI report production validates that the call identifier is exactly `greenai`.

## Conformance rules

The authoritative executable mapping is `tests/conformance/grammar_matrix_beta_0_2.tsv`.

Every matrix row records:

1. a stable coverage ID,
2. language area,
3. scanner, parser or CLI source,
4. an implementation anchor,
5. a fixture,
6. accept or reject expectation,
7. rationale.

`short_hand source.short parse` validates syntax without semantic analysis. Normal run, print, compile and evidence modes continue to apply semantic validation. PR67 remains responsible for malformed-input corpus expansion, parser recovery and robustness testing.
