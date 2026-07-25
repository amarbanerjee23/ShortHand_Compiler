# ShortHand beta language grammar, EBNF draft

Status: beta grammar draft for conformance and review.

Language version: beta-0.1

Conformance contract: beta-0.1

This document records the current accepted AI, GreenAI, and core statement surface used by the compiler tests. It is not yet a full language standard. The goal is to create a stable contract that parser tests, semantic tests, diagnostics, and future MLIR lowering can reference.

## Lexical conventions

```ebnf
identifier      = letter , { letter | digit | "_" } ;
integer         = digit , { digit } ;
number          = integer | integer , "." , integer ;
string          = '"' , { character - '"' } , '"' ;
shape_string    = string ;
boolean         = "true" | "false" ;
element_type    = "float" | "float32" | "f32" | "float16" | "fp16" | "f16" | "bfloat16" | "bf16" | "int8" | "int4" | "int32" | "int" ;
model_format    = "onnx" | "engine" | "tensorrt" | "torchscript" | "torch" | "openvino_ir" | "openvino" | "gguf" ;
backend_kind    = "fallback" | "onnxruntime" | "onnxruntime_cpu" | "onnxruntime_cuda" | "onnxruntime_tensorrt" | "tensorrt" | "openvino" | "libtorch" | "llamacpp" | "llama_cpp" | "llama.cpp" ;
quality_metric  = identifier ;
quality_op      = ">=" | ">" | "<=" | "<" | "==" ;
```

## Program structure

```ebnf
program         = { declaration | function | statement } ;
declaration     = data_declaration | tensor_declaration | model_declaration | greenai_contract | greenai_measurement ;
```

## Core statements

```ebnf
statement       = assignment
                | expression_statement
                | print_statement
                | read_statement
                | if_statement
                | loop_statement
                | infer_statement
                | greenai_report
                | break_statement
                | continue_statement
                | return_statement
                | block ;

block           = "{" , { statement } , "}" ;
assignment      = variable , "=" , expression , ";" ;
expression_statement = expression , ";" ;
break_statement = "break" , ";" ;
continue_statement = "continue" , ";" ;
return_statement = "return" , [ expression ] , ";" ;
```

## Tensor declaration

```ebnf
tensor_declaration = "tensor" , identifier , element_type , shape_string , ";" ;
```

Example:

```short
 tensor input float "1,4";
```

## Model declaration

```ebnf
model_declaration = "model" , identifier , "{" ,
                    model_field , { model_field } ,
                    "}" , ";" ;

model_field       = "format" , model_format , ";"
                  | "path" , string , ";"
                  | "task" , string , ";"
                  | "precision" , element_type , ";"
                  | "input_shape" , shape_string , ";"
                  | "output_shape" , shape_string , ";"
                  | "backend_preference" , backend_kind , { "," , backend_kind } , ";"
                  | "compact" , boolean , ";"
                  | "quality_guardrail" , quality_metric , quality_op , number , ";" ;
```

## Inference statement

```ebnf
infer_statement = "infer" , identifier , "(" , identifier , ")" , "->" , identifier , ";" ;
```

The model and tensor names are validated semantically after parsing.

## GreenAI contract

```ebnf
greenai_contract = "greenai_contract" , identifier , "{" ,
                   contract_field , { contract_field } ,
                   "}" , ";" ;

contract_field   = "functional_unit" , string , ";"
                 | "success_criteria" , string , ";"
                 | "boundary" , identifier , { "," , identifier } , ";"
                 | "measurement_quality" , identifier , ";"
                 | "data_quality" , identifier , ";"
                 | "carbon_factor" , identifier , number , ";"
                 | "energy_budget_j" , number , ";"
                 | "carbon_budget_gco2e" , number , ";"
                 | "quality_guardrail" , quality_metric , quality_op , number , ";"
                 | "evidence_retention" , string , ";"
                 | "claims_mode" , identifier , ";" ;
```

## GreenAI measurement

```ebnf
greenai_measurement = "greenai_measure" , identifier , "{" ,
                      "inferences" , integer , ";" ,
                      "watts" , number , ";" ,
                      "seconds" , number , ";" ,
                      "backend" , identifier , ";" ,
                      "}" , ";" ;
```

## Legacy AI inference statement

The legacy statement remains supported for compatibility.

```ebnf
legacy_ai_infer = "ai_infer" , string , string , string , ";" ;
```

## Conformance rule

Every new syntax addition must add at least one parser-positive fixture, one parser-negative or semantic-negative fixture when applicable, and a conformance manifest entry. Production claims must not be added merely because a grammar rule exists.

The beta-0.1 conformance rules and compatibility policy are defined in `docs/language_versioning_and_conformance.md`.
