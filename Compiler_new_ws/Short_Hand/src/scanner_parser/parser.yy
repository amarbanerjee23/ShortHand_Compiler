%{
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "./ast/AST.h"
#include "./ast/ModuleAST.h"
#include "./ast/SourceRange.h"
#include "./visitors/DiagnosticCodes.h"
#include <vector>
#include <string>
#include <unordered_set>
static ModelDeclarationData current_model;
static GreenAIContractData current_contract;
static GreenAIMeasurementData current_measure;
static C3EcoDeclarationData current_c3eco;
static std::string current_module_path;
static std::string current_import_alias;
using namespace std;
extern "C" int yylex();
extern "C" int yyparse();
extern "C" void yyerror(char const *s);
extern "C" int yywrap(void){return 1;}
extern "C" int yydebug;
extern union _NODE_ yylval;
extern class AST_PROGRAM * main_program;
extern class AST_MODULE_PREAMBLE * main_module_preamble;
extern const char *shorthand_source_path;
%}

%locations

%code {
struct ShorthandParserAllocation {
    void *pointer;
    void (*destroy)(void *);
};

class ShorthandParserAllocationRegistry {
private:
    vector<ShorthandParserAllocation> allocations;
    unordered_set<void *> tracked;

public:
    template <typename T>
    T *track(T *node) {
        if (node == nullptr) return node;
        if (tracked.insert(static_cast<void *>(node)).second) {
            allocations.push_back(
                ShorthandParserAllocation{
                    static_cast<void *>(node),
                    [](void *pointer) { delete static_cast<T *>(pointer); }});
        }
        return node;
    }

    ~ShorthandParserAllocationRegistry() {
        for (vector<ShorthandParserAllocation>::reverse_iterator it = allocations.rbegin();
             it != allocations.rend(); ++it) {
            it->destroy(it->pointer);
        }
        main_program = nullptr;
        main_module_preamble = nullptr;
    }
};

static ShorthandParserAllocationRegistry &shorthand_parser_allocation_registry() {
    static ShorthandParserAllocationRegistry registry;
    return registry;
}

template <typename T>
static T *shorthand_track_parser_node(T *node) {
    return shorthand_parser_allocation_registry().track(node);
}

static SourceRange shorthand_range(const YYLTYPE &loc) {
    SourceRange range;
    range.begin.line = loc.first_line;
    range.begin.column = loc.first_column;
    range.end.line = loc.last_line;
    range.end.column = loc.last_column;
    return range;
}

static void shorthand_parser_diagnostic(const char *code,
                                        const char *message,
                                        const YYLTYPE &loc) {
    const char *path = shorthand_source_path == nullptr ? "<input>" : shorthand_source_path;
    fprintf(stderr, "----------------ERROR----------------\n");
    fprintf(stderr,
            "%s:%d:%d: error: [%s] %s [range %d:%d-%d:%d]\n",
            path,
            loc.first_line,
            loc.first_column,
            code,
            message,
            loc.first_line,
            loc.first_column,
            loc.last_line,
            loc.last_column);
}

template <typename T>
static T *located(T *node, const YYLTYPE &loc) {
    shorthand_track_parser_node(node);
    shorthand_set_ast_source_range(node, shorthand_range(loc));
    return node;
}

static AST_MODULE_PREAMBLE *shorthand_ensure_module_preamble(const YYLTYPE &loc) {
    if (main_module_preamble == nullptr) {
        const char *path = shorthand_source_path == nullptr ? "<input>" : shorthand_source_path;
        main_module_preamble = shorthand_track_parser_node(new AST_MODULE_PREAMBLE(path));
        shorthand_set_ast_source_range(main_module_preamble, shorthand_range(loc));
    }
    return main_module_preamble;
}

static void shorthand_begin_c3eco(C3EcoDeclarationKind kind, const char *name) {
    current_c3eco = C3EcoDeclarationData();
    current_c3eco.kind = kind;
    current_c3eco.name = name == nullptr ? "" : std::string(name);
}

static std::string shorthand_unquote(const char *value) {
    if (value == nullptr) return "";
    std::string text(value);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
        return text.substr(1, text.size() - 2);
    return text;
}

static void shorthand_add_c3eco_field(const char *name, const char *value) {
    const std::string field_name = name == nullptr ? "" : std::string(name);
    for (auto &field : current_c3eco.fields) {
        if (field.name == field_name) {
            field.values.push_back(shorthand_unquote(value));
            return;
        }
    }
    C3EcoFieldData field;
    field.name = field_name;
    field.values.push_back(shorthand_unquote(value));
    current_c3eco.fields.push_back(field);
}
}

%start PROGRAMME_RULE
%type <program> PROGRAMME_RULE
%type <decl_block> DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE DECLARATION_VARIABLE_LIST_RULE FUNCTION_PARAMETER_LIST_RULE
%type <functions> FUNCTION_LIST_RULE
%type <function> FUNCTION_RULE
%type <code_block> LOGIC_BLOCK
%type <break_statement> BREAK
%type <block_statement> STATEMENT_BLOCK_RULE STATEMENT_LIST_RULE
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
%type <c3eco_decl> C3ECO_DECLARATION
%type <infer_statement> INFER_STATEMENT
%type <return_statement> RETURN_STATEMENT
%type <string_val> BACKEND_NAME FORMAT_NAME PRECISION_NAME MQ_NAME DQ_NAME BOUNDARY_NAME C3ECO_FIELD_NAME
%type <expression> EXPRESSION_RULE
%type <expression_list> EXPRESSION_LIST_RULE EXPRESSION_LIST_OPT
%type <type> ShortType

%token ETOK
%right '='
%left OR
%left AND
%left EQUAL NOT_EQUAL
%left LESS LESS_OR_EQUAL GREATER GREATER_OR_EQUAL
%token ARROW
%left '+' '-'
%left '*' '/' '%'
%right UMINUS

%token <string_val> STRING_LITERAL IDENTIFIER AI_INFER_BUILTIN GREENAI_REPORT_BUILTIN
%token <int_val> INT_LITERAL
%token <float_val> FLOAT_LITERAL
%token READ PRINT GOTO BREAK WHILE LOOP ELSE IF DEF
%token INT FLOAT STRING VOID BOOL DOUBLE RETURN CONTINUE TRUE FALSE
%token PACKAGE MODULE IMPORT AS
%token MODEL FORMAT PATH TASK PRECISION INPUT_SHAPE OUTPUT_SHAPE BACKEND_PREFERENCE COMPACT QUALITY_GUARDRAIL
%token GREENAI_CONTRACT_T FUNCTIONAL_UNIT SUCCESS_CRITERIA BOUNDARY MEASUREMENT_QUALITY DATA_QUALITY CARBON_FACTOR ENERGY_BUDGET_J CARBON_BUDGET_GCO2E EVIDENCE_RETENTION CLAIMS_MODE EVIDENCE_ONLY GREENAI_MEASURE INFER TENSOR
%token CERTIFICATION WORKLOAD MEASUREMENT_PLAN AI_LIFECYCLE RAG_PIPELINE TOKEN_BUDGET MODEL_ROUTING GUARDRAILS
%token INT8 FP16 FP32 BF16 INT4 FP64 ONNX ENGINE TORCHSCRIPT OPENVINO_IR GGUF TENSORRT ONNXRUNTIME_TENSORRT ONNXRUNTIME_CUDA ONNXRUNTIME_CPU OPENVINO LIBTORCH LLAMACPP FALLBACK
%token MQ1 MQ2 MQ3 MQ4 DQ1 DQ2 DQ3 DQ4 LOCATION CI_CD THIRDPARTY ACCELERATOR COMPUTE STORAGE NETWORK

%%

PROGRAMME_RULE: MODULE_PREAMBLE_RULE DECLARATION_STATEMENT_LIST_RULE FUNCTION_LIST_RULE LOGIC_BLOCK
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble(@1);
        if (preamble->hasAnyDeclaration() && !preamble->hasModule()) {
            shorthand_parser_diagnostic(
                shorthand::diagnostics::ParserModuleRequired,
                "a module declaration is required when package or import declarations are present",
                @1);
            YYERROR;
        }
        $$ = located(new AST_PROGRAM($2,$3,$4), @$);
        main_program = $$;
    };

MODULE_PREAMBLE_RULE:
      %empty { shorthand_ensure_module_preamble(@$); }
    | MODULE_PREAMBLE_RULE PACKAGE_DECLARATION
    | MODULE_PREAMBLE_RULE MODULE_DECLARATION
    | MODULE_PREAMBLE_RULE IMPORT_DECLARATION;

PACKAGE_DECLARATION: PACKAGE MODULE_PATH ';'
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble(@$);
        if (preamble->hasPackage()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicatePackageDeclaration,
                                        "duplicate package declaration", @$);
            YYERROR;
        }
        if (preamble->hasModule() || preamble->hasImports()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "package declaration must precede module and import declarations", @$);
            YYERROR;
        }
        preamble->setPackage(current_module_path, shorthand_range(@$));
    };

MODULE_DECLARATION: MODULE MODULE_PATH ';'
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble(@$);
        if (preamble->hasModule()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateModuleDeclaration,
                                        "duplicate module declaration", @$);
            YYERROR;
        }
        if (preamble->hasImports()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "module declaration must precede import declarations", @$);
            YYERROR;
        }
        preamble->setModule(current_module_path, shorthand_range(@$));
    };

IMPORT_DECLARATION: IMPORT MODULE_PATH IMPORT_ALIAS_OPT ';'
    {
        AST_MODULE_PREAMBLE *preamble = shorthand_ensure_module_preamble(@$);
        if (!preamble->hasModule()) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserModuleDeclarationOrder,
                                        "import declaration requires a preceding module declaration", @$);
            YYERROR;
        }
        if (preamble->hasImportPath(current_module_path)) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateImportPath,
                                        "duplicate import path", @$);
            YYERROR;
        }
        if (preamble->hasImportAlias(current_import_alias)) {
            shorthand_parser_diagnostic(shorthand::diagnostics::ParserDuplicateImportAlias,
                                        "duplicate import alias", @$);
            YYERROR;
        }
        preamble->addImport(current_module_path, current_import_alias, shorthand_range(@$));
    };

MODULE_PATH:
      IDENTIFIER { current_module_path = string($1); }
    | MODULE_PATH '.' IDENTIFIER { current_module_path += "."; current_module_path += string($3); };

IMPORT_ALIAS_OPT:
      %empty { current_import_alias.clear(); }
    | AS IDENTIFIER { current_import_alias = string($2); };

FUNCTION_LIST_RULE:
      FUNCTION_LIST_RULE FUNCTION_RULE ';' { $$=$1; $$->push_back($2); located($$, @$); }
    | %empty { $$=located(new AST_FUNCTION_LIST_RULE(), @$); };

FUNCTION_RULE: DEF ShortType IDENTIFIER '(' FUNCTION_PARAMETER_LIST_RULE ')' STATEMENT_BLOCK_RULE
    { $7->setLexicalScope(false); $$=located(new AST_FUNCTION_RULE($2,$3,$5,$7), @$); };

FUNCTION_PARAMETER_LIST_RULE:
      DECLARATION_STATEMENT_LIST_RULE { $$=$1; }
    | %empty { $$=located(new AST_DATA_DECLARATION_BLOCK(), @$); };

DECLARATION_STATEMENT_LIST_RULE:
      DECLARATION_STATEMENT_LIST_RULE DECLARATION_STATEMENT_RULE ';' { $$=$1; $$->push_back($2); located($$, @$); }
    | DECLARATION_STATEMENT_RULE ';' { $$=$1; located($$, @$); };

ShortType: INT {$$=ShortType::Int;} | FLOAT {$$=ShortType::Float;} | DOUBLE {$$=ShortType::Float;} | STRING {$$=ShortType::String;} | VOID {$$=ShortType::Void;} | BOOL {$$=ShortType::Boolean;};

DECLARATION_STATEMENT_RULE: ShortType DECLARATION_VARIABLE_LIST_RULE { $$=$2; $$->setType($1); located($$, @$); };

DECLARATION_VARIABLE_LIST_RULE:
      DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER { $$=$1; $$->push_back(string($3)); located($$, @$); }
    | DECLARATION_VARIABLE_LIST_RULE ',' IDENTIFIER '[' INT_LITERAL ']' { $$=$1; $$->push_back(string($3),$5); located($$, @$); }
    | IDENTIFIER { $$=located(new AST_DATA_DECLARATION_BLOCK(), @$); $$->push_back(string($1)); }
    | IDENTIFIER '[' INT_LITERAL ']' { $$=located(new AST_DATA_DECLARATION_BLOCK(), @$); $$->push_back(string($1),$3); }
    | %empty { $$=located(new AST_DATA_DECLARATION_BLOCK(), @$); };

LOGIC_BLOCK: STATEMENT_LIST_RULE { $$=located(new AST_LOGIC_BLOCK($1), @$); };
STATEMENT_BLOCK_RULE: '{' STATEMENT_LIST_RULE '}' { $$=$2; $$->setLexicalScope(true); located($$, @$); };
STATEMENT_LIST_RULE:
      STATEMENT_LIST_RULE STATEMENT_RULE { $$=$1; $$->push_back($2); located($$, @$); }
    | STATEMENT_RULE { $$=located(new AST_STATEMENTS_BLOCK(), @$); $$->push_back($1); };

STATEMENT_RULE:
      MODEL_DECLARATION { $$=$1; }
    | TENSOR_DECLARATION { $$=$1; }
    | GREENAI_CONTRACT { $$=$1; }
    | GREENAI_MEASUREMENT { $$=$1; }
    | C3ECO_DECLARATION { $$=$1; }
    | INFER_STATEMENT { $$=$1; }
    | RETURN_STATEMENT { $$=$1; }
    | CONTINUE ';' { $$=located(new AST_CONTINUE(), @$); }
    | DECLARATION_STATEMENT_RULE ';' { $$=$1; located($$, @$); }
    | EXPRESSION_RULE ';' { $$=located(new AST_EXPRESSION_STATEMENT_RULE($1), @$); }
    | VARIABLE_RULE '=' EXPRESSION_RULE ';' { $$=located(new AST_ASSIGNMENT_RULE($1,$3), @$); }
    | STATEMENT_BLOCK_RULE { $$=$1; }
    | IF EXPRESSION_RULE STATEMENT_BLOCK_RULE { $$=located(new AST_IF_STATEMENT($2,$3), @$); }
    | IF EXPRESSION_RULE STATEMENT_BLOCK_RULE ELSE STATEMENT_BLOCK_RULE { $$=located(new AST_IF_ELSE_STATEMENT($2,$3,$5), @$); }
    | LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE { $$=located(new AST_FOR_LOOP_STATEMENT_RULE($2,$4,$6,$7), @$); }
    | LOOP VARIABLE_RULE '=' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE STATEMENT_BLOCK_RULE { $$=located(new AST_FOR_LOOP_STATEMENT_RULE($2,$4,$6,$8,$9), @$); }
    | LOOP EXPRESSION_RULE STATEMENT_BLOCK_RULE { $$=located(new AST_WHILE_LOOP_STATEMENT_RULE($2,$3), @$); }
    | GOTO IDENTIFIER ';' { $$=located(new AST_GOTO_STATEMENT_RULE(string($2)), @$); }
    | GOTO IDENTIFIER IF EXPRESSION_RULE ';' { $$=located(new AST_GOTO_STATEMENT_RULE($4,string($2)), @$); }
    | READ READ_VARIABLE_LIST_RULE ';' { $$=$2; located($$, @$); }
    | BREAK ';' { $$=located(new AST_BREAK(), @$); }
    | PRINT PRINT_VARIABLE_LIST_RULE ';' { $$=$2; located($$, @$); }
    | GREENAI_REPORT_RULE ';' { $$=$1; located($$, @$); }
    | AI_INFER_RULE ';' { $$=$1; located($$, @$); }
    | IDENTIFIER ':' { $$=located(new AST_LABEL_RULE(string($1)), @$); }
    | ';' { $$=located(new AST_EXPRESSION_STATEMENT_RULE(located(new AST_LITERAL(1), @$)), @$); };

RETURN_STATEMENT:
      RETURN EXPRESSION_RULE ';' { $$=located(new AST_RETURN_STATEMENT($2), @$); }
    | RETURN ';' { $$=located(new AST_RETURN_STATEMENT(), @$); };

INFER_STATEMENT:
      INFER IDENTIFIER '(' IDENTIFIER ')' ARROW IDENTIFIER ';' { $$=located(new AST_INFER_STATEMENT(string($2),string($4),string($7)), @$); }
    | INFER IDENTIFIER '(' IDENTIFIER ')' GREATER IDENTIFIER ';' { $$=located(new AST_INFER_STATEMENT(string($2),string($4),string($7)), @$); };

TENSOR_DECLARATION: TENSOR IDENTIFIER PRECISION_NAME STRING_LITERAL ';' {
    TensorDeclarationData d; d.name=$2; d.element_type=$3; d.shape_csv=string($4).substr(1,string($4).size()-2); d.dynamic=(d.shape_csv=="dynamic"); d.rank=d.dynamic?0:1;
    long long total=1; if(!d.dynamic){ d.rank=0; size_t start=0; while(start<d.shape_csv.size()){ size_t pos=d.shape_csv.find(',',start); string part=d.shape_csv.substr(start,pos==string::npos?string::npos:pos-start); total*=atoll(part.c_str()); d.rank++; if(pos==string::npos) break; start=pos+1; }} d.total_elements=total;
    $$=located(new AST_TENSOR_DECLARATION(d), @$);
};

MODEL_DECLARATION: MODEL IDENTIFIER '{' { current_model=ModelDeclarationData(); current_model.name=$2; } MODEL_FIELD_LIST '}' ';' { $$=located(new AST_MODEL_DECLARATION(current_model), @$); };
MODEL_FIELD_LIST: MODEL_FIELD_LIST MODEL_FIELD | %empty;
MODEL_FIELD: FORMAT FORMAT_NAME ';' { current_model.format=$2; } | PATH STRING_LITERAL ';' { current_model.path=string($2).substr(1,string($2).size()-2); } | TASK STRING_LITERAL ';' { current_model.task=string($2).substr(1,string($2).size()-2); } | PRECISION PRECISION_NAME ';' { current_model.precision=$2; } | INPUT_SHAPE STRING_LITERAL ';' { current_model.input_shape=string($2).substr(1,string($2).size()-2); } | OUTPUT_SHAPE STRING_LITERAL ';' { current_model.output_shape=string($2).substr(1,string($2).size()-2); } | BACKEND_PREFERENCE BACKEND_LIST ';' | COMPACT TRUE ';' { current_model.compact=true; } | COMPACT FALSE ';' { current_model.compact=false; } | QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';' { current_model.has_quality_guardrail=true; current_model.quality_guardrail={string($2),">=",(double)$4}; };
BACKEND_LIST: BACKEND_LIST ',' BACKEND_NAME { current_model.backend_preference.push_back($3); } | BACKEND_NAME { current_model.backend_preference.push_back($1); };

GREENAI_CONTRACT: GREENAI_CONTRACT_T IDENTIFIER '{' { current_contract=GreenAIContractData(); current_contract.name=$2; } CONTRACT_FIELD_LIST '}' ';' { $$=located(new AST_GREENAI_CONTRACT(current_contract), @$); };
CONTRACT_FIELD_LIST: CONTRACT_FIELD_LIST CONTRACT_FIELD | %empty;
CONTRACT_FIELD: FUNCTIONAL_UNIT STRING_LITERAL ';' { current_contract.functional_unit=string($2).substr(1,string($2).size()-2); current_contract.has_functional_unit=true; } | SUCCESS_CRITERIA STRING_LITERAL ';' { current_contract.success_criteria=string($2).substr(1,string($2).size()-2); current_contract.has_success_criteria=true; } | BOUNDARY BOUNDARY_LIST ';' { current_contract.has_boundary=true; } | MEASUREMENT_QUALITY MQ_NAME ';' { current_contract.measurement_quality=$2; current_contract.has_mq=true; } | DATA_QUALITY DQ_NAME ';' { current_contract.data_quality=$2; current_contract.has_dq=true; } | CARBON_FACTOR LOCATION INT_LITERAL ';' { current_contract.carbon_factor_scope="location"; current_contract.carbon_factor=$3; current_contract.has_carbon_factor=true; } | ENERGY_BUDGET_J INT_LITERAL ';' { current_contract.energy_budget_j=$2; } | CARBON_BUDGET_GCO2E INT_LITERAL ';' { current_contract.carbon_budget_gco2e=$2; } | QUALITY_GUARDRAIL IDENTIFIER GREATER_OR_EQUAL INT_LITERAL ';' { current_contract.has_quality_guardrail=true; current_contract.quality_guardrail={string($2),">=",(double)$4}; } | EVIDENCE_RETENTION STRING_LITERAL ';' { current_contract.evidence_retention=string($2).substr(1,string($2).size()-2); } | CLAIMS_MODE EVIDENCE_ONLY ';' { current_contract.claims_mode="evidence_only"; };
BOUNDARY_LIST: BOUNDARY_LIST ',' BOUNDARY_NAME { current_contract.boundary.push_back($3); } | BOUNDARY_NAME { current_contract.boundary.push_back($1); };

GREENAI_MEASUREMENT: GREENAI_MEASURE IDENTIFIER '{' { current_measure=GreenAIMeasurementData(); current_measure.workload=$2; } MEASURE_FIELD_LIST '}' ';' { $$=located(new AST_GREENAI_MEASUREMENT(current_measure), @$); };
MEASURE_FIELD_LIST: MEASURE_FIELD_LIST MEASURE_FIELD | %empty;
MEASURE_FIELD: IDENTIFIER INT_LITERAL ';' { string n=$1; if(n=="inferences") current_measure.inferences=$2; else if(n=="watts") current_measure.watts=$2; else if(n=="seconds") current_measure.seconds=$2; } | IDENTIFIER IDENTIFIER ';' { if(string($1)=="backend") current_measure.backend=$2; };

C3ECO_DECLARATION:
      CERTIFICATION IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::Certification, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | FUNCTIONAL_UNIT IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::FunctionalUnit, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | WORKLOAD IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::Workload, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | BOUNDARY IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::Boundary, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | MEASUREMENT_PLAN IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::MeasurementPlan, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | AI_LIFECYCLE IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::AILifecycle, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | RAG_PIPELINE IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::RAGPipeline, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | TOKEN_BUDGET IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::TokenBudget, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | MODEL_ROUTING IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::ModelRouting, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); }
    | GUARDRAILS IDENTIFIER '{' { shorthand_begin_c3eco(C3EcoDeclarationKind::Guardrails, $2); } C3ECO_FIELD_LIST '}' ';' { $$=located(new AST_C3ECO_DECLARATION(current_c3eco), @$); };
C3ECO_FIELD_LIST: C3ECO_FIELD_LIST C3ECO_FIELD | %empty;
C3ECO_FIELD: C3ECO_FIELD_NAME STRING_LITERAL ';' { shorthand_add_c3eco_field($1, $2); };
C3ECO_FIELD_NAME:
      IDENTIFIER { $$=$1; }
    | CARBON_FACTOR { $$=(char*)"carbon_factor"; }
    | FALLBACK { $$=(char*)"fallback"; }
    | QUALITY_GUARDRAIL { $$=(char*)"quality_guardrail"; }
    | FUNCTIONAL_UNIT { $$=(char*)"functional_unit"; }
    | BOUNDARY { $$=(char*)"boundary"; };

FORMAT_NAME: ONNX {$$=(char*)"onnx";} | ENGINE {$$=(char*)"engine";} | TORCHSCRIPT {$$=(char*)"torchscript";} | OPENVINO_IR {$$=(char*)"openvino_ir";} | GGUF {$$=(char*)"gguf";};
PRECISION_NAME: INT8 {$$=(char*)"int8";} | INT4 {$$=(char*)"int4";} | FP16 {$$=(char*)"fp16";} | FP32 {$$=(char*)"fp32";} | BF16 {$$=(char*)"bf16";} | FP64 {$$=(char*)"fp64";} | FLOAT {$$=(char*)"float";};
BACKEND_NAME: TENSORRT {$$=(char*)"tensorrt";} | ONNXRUNTIME_TENSORRT {$$=(char*)"onnxruntime_tensorrt";} | ONNXRUNTIME_CUDA {$$=(char*)"onnxruntime_cuda";} | ONNXRUNTIME_CPU {$$=(char*)"onnxruntime_cpu";} | OPENVINO {$$=(char*)"openvino";} | LIBTORCH {$$=(char*)"libtorch";} | LLAMACPP {$$=(char*)"llamacpp";} | FALLBACK {$$=(char*)"fallback";};
MQ_NAME: MQ1 {$$=(char*)"MQ1";} | MQ2 {$$=(char*)"MQ2";} | MQ3 {$$=(char*)"MQ3";} | MQ4 {$$=(char*)"MQ4";};
DQ_NAME: DQ1 {$$=(char*)"DQ1";} | DQ2 {$$=(char*)"DQ2";} | DQ3 {$$=(char*)"DQ3";} | DQ4 {$$=(char*)"DQ4";};
BOUNDARY_NAME: COMPUTE {$$=(char*)"compute";} | ACCELERATOR {$$=(char*)"accelerator";} | STORAGE {$$=(char*)"storage";} | NETWORK {$$=(char*)"network";} | CI_CD {$$=(char*)"ci_cd";} | THIRDPARTY {$$=(char*)"thirdparty";};

AI_INFER_RULE: AI_INFER_BUILTIN '(' STRING_LITERAL ',' STRING_LITERAL ',' STRING_LITERAL ')' {
    if ((string($1)!="ai_infer" && string($1)!="aiinfer")) {
        shorthand_parser_diagnostic(shorthand::diagnostics::ParserExpectedAIInferBuiltin,
                                    "expected ai_infer builtin", @$);
        YYERROR;
    }
    $$=located(new AST_AI_INFER_RULE(string($3),string($5),string($7)), @$);
};

GREENAI_REPORT_RULE: GREENAI_REPORT_BUILTIN '(' STRING_LITERAL ',' EXPRESSION_RULE ',' EXPRESSION_RULE ',' EXPRESSION_RULE ')' {
    if (string($1)!="greenai") {
        shorthand_parser_diagnostic(shorthand::diagnostics::ParserExpectedGreenAIReportBuiltin,
                                    "expected greenai report builtin", @$);
        YYERROR;
    }
    $$=located(new AST_GREENAI_REPORT_RULE(string($3),$5,$7,$9), @$);
};

EXPRESSION_RULE:
      EXPRESSION_RULE '+' EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"+"), @$); }
    | EXPRESSION_RULE '-' EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"-"), @$); }
    | EXPRESSION_RULE '*' EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"*"), @$); }
    | EXPRESSION_RULE '/' EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"/"), @$); }
    | EXPRESSION_RULE '%' EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"%"), @$); }
    | EXPRESSION_RULE LESS EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"<"), @$); }
    | EXPRESSION_RULE LESS_OR_EQUAL EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"<="), @$); }
    | EXPRESSION_RULE GREATER EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,">"), @$); }
    | EXPRESSION_RULE GREATER_OR_EQUAL EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,">="), @$); }
    | EXPRESSION_RULE EQUAL EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"=="), @$); }
    | EXPRESSION_RULE NOT_EQUAL EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"!="), @$); }
    | EXPRESSION_RULE OR EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"||"), @$); }
    | EXPRESSION_RULE AND EXPRESSION_RULE { $$=located(new AST_BINARY_EXPRESSION_RULE($1,$3,"&&"), @$); }
    | '-' EXPRESSION_RULE %prec UMINUS { $$=located(new AST_UNARY_EXPRESSION_RULE($2,"-"), @$); }
    | '(' EXPRESSION_RULE ')' { $$=$2; located($$, @$); }
    | IDENTIFIER '(' EXPRESSION_LIST_OPT ')' { $$=located(new AST_FUNCTION_CALL_EXPRESSION(string($1),*$3), @$); }
    | VARIABLE_RULE { $$=$1; }
    | INT_LITERAL { $$=located(new AST_LITERAL($1), @$); }
    | FLOAT_LITERAL { $$=located(new AST_FLOAT_LITERAL($1), @$); }
    | STRING_LITERAL { $$=located(new AST_STRING_LITERAL(string($1)), @$); }
    | TRUE { $$=located(new AST_BOOL_LITERAL(true), @$); }
    | FALSE { $$=located(new AST_BOOL_LITERAL(false), @$); };

EXPRESSION_LIST_OPT:
      EXPRESSION_LIST_RULE { $$=$1; }
    | %empty { $$=shorthand_track_parser_node(new vector<AST_EXPRESSION_RULE*>()); };

EXPRESSION_LIST_RULE:
      EXPRESSION_LIST_RULE ',' EXPRESSION_RULE { $$=$1; $$->push_back($3); }
    | EXPRESSION_RULE { $$=shorthand_track_parser_node(new vector<AST_EXPRESSION_RULE*>()); $$->push_back($1); };

VARIABLE_RULE:
      IDENTIFIER { $$=located(new AST_SIMPLE_VARIABLE(string($1)), @$); }
    | IDENTIFIER '[' EXPRESSION_RULE ']' { $$=located(new AST_ARRAY_VARIABLE(string($1),$3), @$); };

READ_VARIABLE_LIST_RULE:
      READ_VARIABLE_LIST_RULE ',' VARIABLE_RULE { $$=$1; $$->push_back($3); located($$, @$); }
    | VARIABLE_RULE { $$=located(new AST_READ_RULE(), @$); $$->push_back($1); };

PRINT_VARIABLE_LIST_RULE:
      PRINT_VARIABLE_LIST_RULE ',' EXPRESSION_RULE { $$=$1; $$->push_back($3); located($$, @$); }
    | EXPRESSION_RULE { $$=located(new AST_PRINT_RULE(), @$); $$->push_back($1); };

%%

void yyerror(char const *s) {
    shorthand_parser_diagnostic(shorthand::diagnostics::ParserSyntaxError, s, yylloc);
}
