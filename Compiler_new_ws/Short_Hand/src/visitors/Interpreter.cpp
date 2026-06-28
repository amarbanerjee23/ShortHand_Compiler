#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include "../ai_runtime/AI_Runtime.h"
#include <map>

static string unquoteShortString(const string &value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

static vector<float> parseShortFloatCsv(const string &csv)
{
    vector<float> values;
    stringstream ss(csv);
    string token;
    while (getline(ss, token, ',')) {
        if (!token.empty()) values.push_back(stof(token));
    }
    return values;
}

static vector<int64_t> parseShortShapeCsv(const string &csv)
{
    return shorthand::ai::parseShapeCsv(csv);
}

static std::map<std::string, ModelDeclarationData> aiModels;
static std::map<std::string, TensorDeclarationData> aiTensors;

static shorthand::ai::ModelSpec toModelSpec(const ModelDeclarationData &d)
{
    shorthand::ai::ModelSpec spec;
    spec.name = d.name;
    spec.path = d.path;
    spec.format = shorthand::ai::parseModelFormat(d.format);
    spec.task = d.task;
    spec.precision = d.precision;
    spec.input = {"input", shorthand::ai::parseElementType(d.precision), shorthand::ai::parseShapeCsv(d.input_shape)};
    spec.input.element_count = shorthand::ai::productOfShape(spec.input.shape);
    spec.output = {"output", shorthand::ai::parseElementType(d.precision), shorthand::ai::parseShapeCsv(d.output_shape)};
    spec.output.element_count = shorthand::ai::productOfShape(spec.output.shape);
    for (const auto &backend : d.backend_preference) spec.backend_preference.push_back(shorthand::ai::parseBackendKind(backend));
    spec.compact = d.compact;
    spec.allow_fallback = true;
    if (d.has_quality_guardrail) { spec.quality_metric=d.quality_guardrail.metric; spec.quality_op=d.quality_guardrail.op; spec.quality_threshold=d.quality_guardrail.threshold; }
    return spec;
}

// program
int Interpreter::visit(AST_PROGRAM * program)
{
    //cout << "program" << endl;
    program->decl_block->accept(*this);
    program->code_block->accept(*this);
    return 0;
}

// decl_block
int Interpreter::visit(AST_DATA_DECLARATION_BLOCK * decl_block)
{
    //cout << "decl_block" << endl;

    //cout << "single ints : ";
    for(int i = 0; i < (int)decl_block->single_ints.size(); i++)
        ST_single_int[decl_block->single_ints[i]] = 0;
    //cout << endl;

    //cout << "array ints : ";
    for(int i = 0; i < (int)decl_block->array_ints.size(); i++)
        ST_array_int[decl_block->array_ints[i].first] = vector<int>(decl_block->array_ints[i].second, 0);
    //cout << endl;
    //cout << ST_single_int.size() << endl;
    //cout << ST_array_int.size() << endl;
    return 0;
}

int Interpreter::visit(AST_FUNCTION_LIST_RULE *functions) {
    // Define function's signature
	for(auto function : functions->functions)
		{
		function->accept(*this);
		}
	return 0;
}


// code_block
int Interpreter::visit(AST_LOGIC_BLOCK * code_block)
{
    //cout << "code_block" << endl;
    code_block->block_statement->accept(*this);
    return 0;
}

//
// all statements
//
int Interpreter::visit(AST_EXPRESSION_STATEMENT_RULE * expression_statement)
{
    //cout << "expression_statement" << endl;
    expression_statement->expression->accept(*this);
    return 0;
}

int Interpreter::visit(AST_ASSIGNMENT_RULE * assignment_statement)
{
    //cout << "assignment_statement" << endl;
    int value = assignment_statement->expression->accept(*this);
    assignment_statement->variable->accept(*this);
    string type = assignment_statement->variable->type;

    if(type == "array")
        ST_array_int[str][num] = value;
    else
        ST_single_int[str] = value;
    return 0;
}

int Interpreter::visit(AST_STATEMENTS_BLOCK * block_statement)
{
    //cout << "block_statement" << endl;
    for(int i = 0; i < (int)block_statement->statements.size(); i++)
        block_statement->statements[i]->accept(*this);
    return 0;
}

int Interpreter::visit(AST_FUNCTION_RULE * function)
{
   function->block_statement->accept(*this);
   return 0;
}

int Interpreter::visit(AST_FUNCTION_CALL_RULE * function)
{
   function->parameters->accept(*this);
   return 0;
}

int Interpreter::visit(AST_IF_STATEMENT * if_statement)
{
    //cout << "if_statement" << endl;
    //cout << "if condition" << endl;
    int value = if_statement->condition->accept(*this);
    if(value)
    {
        //cout << "if_block" << endl;
        if_statement->if_block->accept(*this);
    }
    return 0;
}

int Interpreter::visit(AST_IF_ELSE_STATEMENT * ifelse_statement)
{
    //cout << "ifelse_statement" << endl;
    //cout << "if condition" << endl;
    int value = ifelse_statement->condition->accept(*this);
    if(value)
    {
        //cout << "if_block" << endl;
        ifelse_statement->if_block->accept(*this);
    }
    else
    {
        //cout << "else_block" << endl;
        ifelse_statement->else_block->accept(*this);
    }
    return 0;
}

int Interpreter::visit(AST_FOR_LOOP_STATEMENT_RULE * for_statement)
{
    //cout << "for_statement" << endl;

    //cout << "from" << endl;
    int from = for_statement->from->accept(*this);

    for_statement->variable->accept(*this);
    string type = for_statement->variable->type;
    string name = str;
    int idx = num;

    if(type == "array") ST_array_int[name][idx] = from;
    else ST_single_int[name] = from;

    while(1)
    {
        for_statement->variable->accept(*this);
        string type = for_statement->variable->type;
        string name = str;
        int idx = num;
        int val;
        if(type == "array") val = ST_array_int[name][idx];
        else val = ST_single_int[name];
        //cout << name << endl;

        int to = for_statement->to->accept(*this);

        if(val >= to) break;
        for_statement->for_block->accept(*this);

        //cout << "step" << endl;
        int step = for_statement->step->accept(*this);

        for_statement->variable->accept(*this);
        type = for_statement->variable->type;
        name = str;
        idx = num;
        //cout << type << " " << name << " " << ST_single_int[name];
        if(type == "array") ST_array_int[name][idx] += step;
        else ST_single_int[name] += step;
    }

    return 0;
}

int Interpreter::visit(AST_WHILE_LOOP_STATEMENT_RULE * while_statement)
{
    //cout << "while_statement" << endl;
    while(1)
    {
        //cout << "while condition" << endl;
        int value = while_statement->condition->accept(*this);
        if(!value) break;
        //cout << "while_block" << endl;
        while_statement->while_block->accept(*this);
    }
    return 0;
}

int Interpreter::visit(AST_GOTO_STATEMENT_RULE * goto_statement)
{
    //cout << "goto_statement : " << goto_statement->label << endl;
    if(goto_statement->condition) goto_statement->condition->accept(*this);
    return 0;
}

int Interpreter::visit(AST_BREAK * break_statement) {
    // Define function's signature
	return 0;
}

int Interpreter::visit(AST_READ_RULE * read_statement)
{
    //cout << "read_statement" << endl;
    for(int i = 0; i < (int)read_statement->variables.size(); i++)
    {
        int x;
        cin >> x;
        read_statement->variables[i]->accept(*this);
        string type = read_statement->variables[i]->type;
        //cout << str << " " << num << endl;
        if(type == "array") ST_array_int[str][num] = x;
        else ST_single_int[str] = x;
    }
    return 0;
}

int Interpreter::visit(AST_PRINT_RULE * print_statement)
{
    //cout << "print_statement" << endl;
    for(int i = 0; i < (int)print_statement->printables.size(); i++)
    {
        if(print_statement->printables[i].expression)
        {
            //cout << "expression in print" << endl;
            int value = print_statement->printables[i].expression->accept(*this);
            cout << value << " ";
        }
        else
        {
            print_statement->printables[i].string_literal->accept(*this);
            cout << str.substr(1, str.length()-2) << " ";
        }
    }
    cout << endl;
    return 0;
}

int Interpreter::visit(AST_LABEL_RULE * label_statement)
{
    //cout << "label_statement : " << label_statement->label << endl;
    return 0;
}

int Interpreter::visit(AST_GREENAI_REPORT_RULE * greenai_report)
{
    int inferences = greenai_report->inferences->accept(*this);
    int watts = greenai_report->watts->accept(*this);
    int seconds = greenai_report->seconds->accept(*this);
    int energy_joules = watts * seconds;
    int inferences_per_joule = energy_joules == 0 ? 0 : inferences / energy_joules;

    cout << "GreenAI workload " << greenai_report->workload_name.substr(1, greenai_report->workload_name.length() - 2)
         << ": inferences=" << inferences
         << " energy_j=" << energy_joules
         << " inf_per_j=" << inferences_per_joule
         << endl;
    return 0;
}

int Interpreter::visit(AST_AI_INFER_RULE * ai_infer)
{
    string model_path = unquoteShortString(ai_infer->model_path);
    string shape_csv = unquoteShortString(ai_infer->shape_csv);
    string input_csv = unquoteShortString(ai_infer->input_csv);

    TensorData tensor;
    tensor.shape = parseShortShapeCsv(shape_csv);
    tensor.data = parseShortFloatCsv(input_csv);

    AI_Runtime runtime;
    if (!runtime.loadModel(model_path)) {
        cout << "AI inference error: " << runtime.getLastError() << endl;
        return 1;
    }

    vector<float> output;
    if (!runtime.run(tensor, output)) {
        cout << "AI inference error: " << runtime.getLastError() << endl;
        return 1;
    }

    cout << "AI inference output:";
    for (float value : output) {
        cout << " " << value;
    }
    cout << endl;
    return 0;
}


//
// all expressions
//
int Interpreter::visit(AST_BINARY_EXPRESSION_RULE * binary_operator_expression)
{
    int op = binary_operator_expression->op;
    //cout << "binary_operator_expression : " << op << endl;
    //cout << "left" << endl;
    int l = binary_operator_expression->left->accept(*this);
    //cout << "right" << endl;
    int r = binary_operator_expression->right->accept(*this);

    //cout << "binary expression " << l << " " << r << " " << " op " << op << endl;
    int ans;
    if(op == 1) ans = l + r;
    else if(op == 2) ans = l - r;
    else if(op == 3) ans = l * r;
    else if(op == 4) ans = l / r;
    else if(op == 5) ans = l % r;
    else if(op == 6) ans = l < r;
    else if(op == 7) ans = l > r;
    else if(op == 8) ans = l <= r;
    else if(op == 9) ans = l >= r;
    else if(op == 10) ans = l == r;
    else if(op == 11) ans = l != r;
    else if(op == 12) ans = l || r;
    else if(op == 13) ans = l && r;
    else cout << "ERROR" << endl;

    return ans;
}

int Interpreter::visit(AST_UNARY_EXPRESSION_RULE * unary_operator_expression)
{
    int op = unary_operator_expression->op;
    //cout << "unary_operator_expression : " << op << endl;
    int v = unary_operator_expression->expression->accept(*this);

    int ans;
    if(op == 1) ans = -v;
    else cout << "ERROR" << endl;

    return ans;
}


//
// variables and literals
//
int Interpreter::visit(AST_SIMPLE_VARIABLE * variable_single_int)
{
    //cout << "variable_single_int : " << variable_single_int->variable_name << endl;
    str = variable_single_int->variable_name;
    return ST_single_int[str];
}

int Interpreter::visit(AST_ARRAY_VARIABLE * variable_array_int)
{
    //cout << "variable_array_int : " << variable_array_int->array_name << endl;
    //cout << "index expression" << endl;
    num = variable_array_int->index->accept(*this);
    str = variable_array_int->array_name;
    return ST_array_int[str][num];
}

int Interpreter::visit(AST_LITERAL * int_literal)
{
    //cout << "int_literal : " << int_literal->int_literal << endl;
    num = int_literal->int_literal;
    return num;
}

int Interpreter::visit(AST_STRING_LITERAL * string_literal)
{
    //cout << "string_literal : " << string_literal->string_literal << endl;
    str = string_literal->string_literal;
    return 0;
}
int Interpreter::visit(AST_MODEL_DECLARATION * n){ aiModels[n->data.name]=n->data; cout << "Registered model " << n->data.name << " (fallback-capable)" << endl; return 0; }
int Interpreter::visit(AST_TENSOR_DECLARATION * n){ aiTensors[n->data.name]=n->data; return 0; }
int Interpreter::visit(AST_GREENAI_CONTRACT * n){ cout << "GreenAI contract " << n->data.name << " evidence_only" << endl; return 0; }
int Interpreter::visit(AST_GREENAI_MEASUREMENT * n){ cout << "GreenAI workload " << n->data.workload << " measurement_status=declared_budget_only" << endl; return 0; }
int Interpreter::visit(AST_INFER_STATEMENT * n){
    auto modelIt = aiModels.find(n->model_name);
    auto tensorIt = aiTensors.find(n->input_name);
    if (modelIt == aiModels.end()) { cout << "AI inference error: model=" << n->model_name << " runtime_backend=fallback reason=unknown_model" << endl; return 1; }
    if (tensorIt == aiTensors.end()) { cout << "AI inference error: model=" << n->model_name << " runtime_backend=fallback reason=unknown_tensor" << endl; return 1; }
    auto model = toModelSpec(modelIt->second);
    shorthand::ai::TensorBuffer input;
    input.spec.name = tensorIt->second.name;
    input.spec.element_type = shorthand::ai::parseElementType(tensorIt->second.element_type);
    input.spec.shape = shorthand::ai::parseShapeCsv(tensorIt->second.shape_csv);
    input.spec.element_count = shorthand::ai::productOfShape(input.spec.shape);
    input.f32_data.assign(input.spec.element_count, 0.0f);
    shorthand::ai::AIRuntime runtime;
    auto result = runtime.infer(model, input);
    if (result.status == shorthand::ai::InferenceStatus::Success) {
        cout << "AI inference success: model=" << n->model_name << " runtime_backend=" << result.backend_name << " provider=" << result.provider_name << " outputs=" << result.output_f32.size() << endl;
    } else if (result.backend == shorthand::ai::BackendKind::Fallback || result.status == shorthand::ai::InferenceStatus::NotExecuted) {
        cout << "AI inference fallback: model=" << n->model_name << " runtime_backend=fallback inference_status=" << shorthand::ai::inferenceStatusToString(result.status) << " reason=" << result.reason << endl;
    } else {
        cout << "AI inference error: model=" << n->model_name << " runtime_backend=" << result.backend_name << " reason=" << result.reason << endl;
    }
    return 0;
}
int Interpreter::visit(AST_CONTINUE *){ return 0; }
int Interpreter::visit(AST_RETURN_STATEMENT * n){ if(n->expression) n->expression->accept(*this); return 0; }
int Interpreter::visit(AST_BOOL_LITERAL * n){ num = n->value ? 1 : 0; return num; }
int Interpreter::visit(AST_FLOAT_LITERAL * n){ num = (int)n->value; return num; }
int Interpreter::visit(AST_FUNCTION_CALL_EXPRESSION *){ return 0; }
