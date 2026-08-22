#include "Interpreter.h"

#include "../ai_runtime/AI_Runtime.h"
#include "../ast/SourceRange.h"
#include "DiagnosticCodes.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace diag = shorthand::diagnostics;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

static string unquoteShortString(const string &value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        return value.substr(1, value.size() - 2);
    return value;
}

static vector<float> parseShortFloatCsv(const string &csv)
{
    vector<float> values;
    std::stringstream ss(csv);
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
    for (const auto &backend : d.backend_preference)
        spec.backend_preference.push_back(shorthand::ai::parseBackendKind(backend));
    spec.compact = d.compact;
    spec.allow_fallback = true;
    if (d.has_quality_guardrail) {
        spec.quality_metric = d.quality_guardrail.metric;
        spec.quality_op = d.quality_guardrail.op;
        spec.quality_threshold = d.quality_guardrail.threshold;
    }
    return spec;
}

Interpreter::Interpreter(const std::string &source_path)
    : entry_source(source_path), active_source(source_path)
{
    frames.push_back(Frame{});
}

Interpreter::Frame &Interpreter::globalFrame()
{
    if (frames.empty()) frames.push_back(Frame{});
    return frames.front();
}

Interpreter::RuntimeValue Interpreter::defaultValue(ShortType type)
{
    RuntimeValue value;
    value.type = type;
    return value;
}

bool Interpreter::truthy(const RuntimeValue &value)
{
    if (value.type == ShortType::Float) return value.float_value != 0.0;
    if (value.type == ShortType::String) return !value.string_value.empty();
    return value.int_value != 0;
}

Interpreter::RuntimeValue Interpreter::evaluate(AST_EXPRESSION_RULE *expression)
{
    current_value = defaultValue(ShortType::Int);
    if (expression != nullptr) expression->accept(*this);
    return current_value;
}

void Interpreter::initializeDeclarationsInto(AST_DATA_DECLARATION_BLOCK *decl_block, Frame &frame)
{
    if (decl_block == nullptr) return;
    for (const auto &entry : decl_block->typed_scalars) {
        if (!frame.scalars.emplace(entry.name, defaultValue(entry.type)).second) {
            runtimeError(decl_block, diag::RuntimeInvalidState,
                         "duplicate scalar declaration reached runtime: `" + entry.name + "`");
            return;
        }
    }
    for (const auto &entry : decl_block->typed_arrays) {
        if (!frame.arrays.emplace(entry.name, vector<RuntimeValue>(
                static_cast<std::size_t>(entry.size), defaultValue(entry.type))).second) {
            runtimeError(decl_block, diag::RuntimeInvalidState,
                         "duplicate array declaration reached runtime: `" + entry.name + "`");
            return;
        }
    }
}

void Interpreter::initializeDeclarations(AST_DATA_DECLARATION_BLOCK *decl_block)
{
    if (frames.empty()) frames.push_back(Frame{});
    initializeDeclarationsInto(decl_block, frames.back());
}

void Interpreter::registerFunctions(AST_FUNCTION_LIST_RULE *list, const std::string &source_path)
{
    if (list == nullptr) return;
    for (AST_FUNCTION_RULE *function : list->functions) {
        if (function == nullptr || function->function_name == nullptr) continue;
        functions[function->function_name] = function;
        function_sources[function] = source_path;
    }
}

bool Interpreter::addLibraryProgram(AST_PROGRAM *program, const std::string &source_path)
{
    if (program == nullptr) return false;
    initializeDeclarationsInto(program->decl_block, globalFrame());
    registerFunctions(program->functions, source_path);
    return true;
}

bool Interpreter::readScalar(const std::string &name, RuntimeValue &value) const
{
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        auto found = it->scalars.find(name);
        if (found != it->scalars.end()) {
            value = found->second;
            return true;
        }
    }
    return false;
}

bool Interpreter::writeScalar(const std::string &name, const RuntimeValue &value)
{
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        auto found = it->scalars.find(name);
        if (found != it->scalars.end()) {
            if (found->second.type != value.type) return false;
            found->second = value;
            return true;
        }
    }
    return false;
}

bool Interpreter::readArray(const std::string &name, int index, RuntimeValue &value) const
{
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        auto found = it->arrays.find(name);
        if (found == it->arrays.end()) continue;
        if (index < 0 || static_cast<std::size_t>(index) >= found->second.size()) return false;
        value = found->second[static_cast<std::size_t>(index)];
        return true;
    }
    return false;
}

bool Interpreter::writeArray(const std::string &name, int index, const RuntimeValue &value)
{
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        auto found = it->arrays.find(name);
        if (found == it->arrays.end()) continue;
        if (index < 0 || static_cast<std::size_t>(index) >= found->second.size()) return false;
        if (found->second[static_cast<std::size_t>(index)].type != value.type) return false;
        found->second[static_cast<std::size_t>(index)] = value;
        return true;
    }
    return false;
}

void Interpreter::runtimeError(const void *node, const char *code, const std::string &message)
{
    runtime_failed = true;
    SourceRange range = shorthand_get_ast_source_range(node);
    if (!range.valid()) {
        range.begin.line = range.end.line = 1;
        range.begin.column = range.end.column = 1;
    }
    const string source = active_source.empty() ? (entry_source.empty() ? "<runtime>" : entry_source) : active_source;
    cerr << source << ':' << range.begin.line << ':' << range.begin.column
         << ": error: [" << code << "] " << message
         << " [range " << range.begin.line << ':' << range.begin.column
         << '-' << range.end.line << ':' << range.end.column << "]\n";
}

static Interpreter::IntValue fromUnsigned32(std::uint32_t value)
{
    if (value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        return static_cast<std::int32_t>(value);
    const std::uint32_t distance = std::numeric_limits<std::uint32_t>::max() - value;
    return static_cast<std::int32_t>(-1 - static_cast<std::int64_t>(distance));
}

Interpreter::IntValue Interpreter::wrapAdd(IntValue left, IntValue right)
{
    return fromUnsigned32(static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

Interpreter::IntValue Interpreter::wrapSub(IntValue left, IntValue right)
{
    return fromUnsigned32(static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

Interpreter::IntValue Interpreter::wrapMul(IntValue left, IntValue right)
{
    return fromUnsigned32(static_cast<std::uint32_t>(left) * static_cast<std::uint32_t>(right));
}

int Interpreter::invokeFunction(const void *call_node,
                                const std::string &name,
                                const std::vector<RuntimeValue> &arguments,
                                RuntimeValue &result)
{
    auto found = functions.find(name);
    if (found == functions.end() || found->second == nullptr) {
        runtimeError(call_node, diag::RuntimeInvalidState, "function registry missing `" + name + "`");
        return 1;
    }
    AST_FUNCTION_RULE *function = found->second;
    if (function->parameters == nullptr || !function->parameters->array_ints.empty()) {
        runtimeError(call_node, diag::RuntimeInvalidState,
                     "unsupported function parameter layout for `" + name + "`");
        return 1;
    }
    if (function->parameters->single_ints.size() != arguments.size()) {
        runtimeError(call_node, diag::SemanticFunctionArityMismatch,
                     "function " + name + " arity changed after semantic validation");
        return 1;
    }
    if (call_depth >= kMaxCallDepth) {
        runtimeError(call_node, diag::RuntimeInvalidState,
                     "maximum deterministic function call depth exceeded");
        return 1;
    }

    Frame frame;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (i >= function->parameters->typed_scalars.size() ||
            function->parameters->typed_scalars[i].type != arguments[i].type) {
            runtimeError(call_node, diag::RuntimeInvalidState,
                         "function argument type mismatch reached runtime after semantic validation");
            return 1;
        }
        frame.scalars[function->parameters->typed_scalars[i].name] = arguments[i];
    }

    const FlowSignal caller_flow = flow;
    const string caller_goto_target = goto_target;
    const RuntimeValue caller_return = return_value;
    const string caller_source = active_source;
    frames.push_back(std::move(frame));
    ++call_depth;
    flow = FlowSignal::Normal;
    goto_target.clear();
    return_value = defaultValue(function->type);
    auto source = function_sources.find(function);
    if (source != function_sources.end()) active_source = source->second;

    if (function->block_statement) function->block_statement->accept(*this);

    if (flow == FlowSignal::Break || flow == FlowSignal::Continue || flow == FlowSignal::Goto) {
        runtimeError(function, diag::RuntimeInvalidState,
                     "control-flow signal escaped function `" + name + "`");
    }
    if (!runtime_failed && function->type != ShortType::Void && flow != FlowSignal::Return) {
        runtimeError(function, diag::RuntimeInvalidState,
                     "non-void function completed without a return value: `" + name + "`");
    }
    result = flow == FlowSignal::Return ? return_value : defaultValue(function->type);

    --call_depth;
    frames.pop_back();
    active_source = caller_source;
    flow = caller_flow;
    goto_target = caller_goto_target;
    return_value = caller_return;
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_PROGRAM *program)
{
    initializeDeclarations(program->decl_block);
    registerFunctions(program->functions, entry_source);
    if (program->code_block) program->code_block->accept(*this);
    if (!runtime_failed && flow != FlowSignal::Normal)
        runtimeError(program, diag::RuntimeInvalidState, "control-flow signal escaped the top-level program");
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_DATA_DECLARATION_BLOCK *decl_block)
{
    initializeDeclarations(decl_block);
    return 0;
}

int Interpreter::visit(AST_FUNCTION_LIST_RULE *list)
{
    registerFunctions(list, active_source.empty() ? entry_source : active_source);
    return 0;
}

int Interpreter::visit(AST_LOGIC_BLOCK *code_block)
{
    if (code_block->block_statement) code_block->block_statement->accept(*this);
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_FUNCTION_RULE *) { return 0; }

int Interpreter::visit(AST_FUNCTION_CALL_RULE *function)
{
    vector<RuntimeValue> arguments;
    if (function->parameters) {
        for (AST_VARIABLE_RULE *parameter : function->parameters->variables) {
            arguments.push_back(evaluate(parameter));
            if (runtime_failed) return 1;
        }
    }
    RuntimeValue ignored = defaultValue(ShortType::Void);
    return invokeFunction(function,
                          function->function_name == nullptr ? string() : string(function->function_name),
                          arguments, ignored);
}

int Interpreter::visit(AST_EXPRESSION_STATEMENT_RULE *expression_statement)
{
    if (expression_statement->expression) evaluate(expression_statement->expression);
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_ASSIGNMENT_RULE *assignment_statement)
{
    const RuntimeValue value = evaluate(assignment_statement->expression);
    if (runtime_failed) return 1;
    assignment_statement->variable->accept(*this);
    if (runtime_failed) return 1;
    if (assignment_statement->variable->type == "array") {
        if (!writeArray(str, num, value)) {
            runtimeError(assignment_statement, diag::RuntimeArrayBounds,
                         "array write is outside declared bounds for `" + str + "`");
            return 1;
        }
    } else if (!writeScalar(str, value)) {
        runtimeError(assignment_statement, diag::RuntimeInvalidState,
                     "assignment target is not declared: `" + str + "`");
        return 1;
    }
    return 0;
}

int Interpreter::visit(AST_STATEMENTS_BLOCK *block_statement)
{
    if (block_statement == nullptr) return 0;
    if (block_statement->lexical_scope) frames.push_back(Frame{});

    std::map<std::string, std::size_t> labels;
    for (std::size_t i = 0; i < block_statement->statements.size(); ++i) {
        auto *label = dynamic_cast<AST_LABEL_RULE *>(block_statement->statements[i]);
        if (label != nullptr) labels.emplace(label->label, i);
    }

    std::size_t pc = 0;
    while (pc < block_statement->statements.size() && !runtime_failed) {
        if (flow != FlowSignal::Normal) break;
        AST_STATEMENT_RULE *statement = block_statement->statements[pc];
        if (statement) statement->accept(*this);
        if (runtime_failed) break;
        if (flow == FlowSignal::Goto) {
            const auto target = labels.find(goto_target);
            if (target == labels.end()) {
                runtimeError(statement, diag::RuntimeInvalidState,
                             "goto target escaped its validated lexical block: `" + goto_target + "`");
                break;
            }
            if (++goto_transfers > kMaxGotoTransfers) {
                runtimeError(statement, diag::RuntimeInvalidState,
                             "maximum deterministic goto transfer budget exceeded");
                break;
            }
            flow = FlowSignal::Normal;
            goto_target.clear();
            pc = target->second + 1;
            continue;
        }
        if (flow != FlowSignal::Normal) break;
        ++pc;
    }
    if (block_statement->lexical_scope) frames.pop_back();
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_IF_STATEMENT *if_statement)
{
    const RuntimeValue value = evaluate(if_statement->condition);
    if (!runtime_failed && truthy(value) && if_statement->if_block) if_statement->if_block->accept(*this);
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_IF_ELSE_STATEMENT *ifelse_statement)
{
    const RuntimeValue value = evaluate(ifelse_statement->condition);
    if (runtime_failed) return 1;
    if (truthy(value)) {
        if (ifelse_statement->if_block) ifelse_statement->if_block->accept(*this);
    } else if (ifelse_statement->else_block) {
        ifelse_statement->else_block->accept(*this);
    }
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_FOR_LOOP_STATEMENT_RULE *for_statement)
{
    const IntValue initial = evaluate(for_statement->from).int_value;
    if (runtime_failed) return 1;
    for_statement->variable->accept(*this);
    if (runtime_failed) return 1;
    if (for_statement->variable->type == "array") {
        if (!writeArray(str, num, RuntimeValue{ShortType::Int, initial, 0.0, {}})) {
            runtimeError(for_statement, diag::RuntimeArrayBounds, "loop variable array index out of bounds");
            return 1;
        }
    } else if (!writeScalar(str, RuntimeValue{ShortType::Int, initial, 0.0, {}})) {
        runtimeError(for_statement, diag::RuntimeInvalidState, "loop variable is not declared");
        return 1;
    }

    while (!runtime_failed) {
        const IntValue current = evaluate(for_statement->variable).int_value;
        if (runtime_failed) break;
        const IntValue step = evaluate(for_statement->step).int_value;
        if (runtime_failed) break;
        if (step == 0) {
            runtimeError(for_statement->step, diag::RuntimeLoopStepZero,
                         "loop step must be non-zero");
            break;
        }
        const IntValue limit = evaluate(for_statement->to).int_value;
        if (runtime_failed) break;
        const bool in_range = step > 0 ? current < limit : current > limit;
        if (!in_range) break;

        if (for_statement->for_block) for_statement->for_block->accept(*this);
        if (runtime_failed || flow == FlowSignal::Return) break;
        if (flow == FlowSignal::Break) {
            flow = FlowSignal::Normal;
            break;
        }
        if (flow == FlowSignal::Continue) flow = FlowSignal::Normal;

        const IntValue after_body = evaluate(for_statement->variable).int_value;
        if (runtime_failed) break;
        const IntValue next = wrapAdd(after_body, step);
        if (for_statement->variable->type == "array") {
            if (!writeArray(str, num, RuntimeValue{ShortType::Int, next, 0.0, {}})) {
                runtimeError(for_statement, diag::RuntimeArrayBounds, "loop variable array index out of bounds");
                break;
            }
        } else if (!writeScalar(str, RuntimeValue{ShortType::Int, next, 0.0, {}})) {
            runtimeError(for_statement, diag::RuntimeInvalidState, "loop variable is not declared");
            break;
        }
    }
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_WHILE_LOOP_STATEMENT_RULE *while_statement)
{
    while (!runtime_failed) {
        const RuntimeValue value = evaluate(while_statement->condition);
        if (runtime_failed || !truthy(value)) break;
        if (while_statement->while_block) while_statement->while_block->accept(*this);
        if (runtime_failed || flow == FlowSignal::Return) break;
        if (flow == FlowSignal::Break) {
            flow = FlowSignal::Normal;
            break;
        }
        if (flow == FlowSignal::Continue) flow = FlowSignal::Normal;
    }
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_GOTO_STATEMENT_RULE *goto_statement)
{
    if (goto_statement->condition != nullptr) {
        const RuntimeValue condition = evaluate(goto_statement->condition);
        if (runtime_failed || !truthy(condition)) return runtime_failed ? 1 : 0;
    }
    goto_target = goto_statement->label;
    flow = FlowSignal::Goto;
    return 0;
}

int Interpreter::visit(AST_BREAK *)
{
    flow = FlowSignal::Break;
    return 0;
}

int Interpreter::visit(AST_CONTINUE *)
{
    flow = FlowSignal::Continue;
    return 0;
}

int Interpreter::visit(AST_RETURN_STATEMENT *statement)
{
    if (call_depth == 0) {
        runtimeError(statement, diag::RuntimeInvalidState, "return reached top-level runtime");
        return 1;
    }
    if (statement->expression != nullptr) return_value = evaluate(statement->expression);
    if (!runtime_failed) flow = FlowSignal::Return;
    return runtime_failed ? 1 : 0;
}

int Interpreter::visit(AST_READ_RULE *read_statement)
{
    for (AST_VARIABLE_RULE *variable : read_statement->variables) {
        variable->accept(*this);
        if (runtime_failed) return 1;
        RuntimeValue value = current_value;
        if (value.type == ShortType::Float) cin >> value.float_value;
        else cin >> value.int_value;
        if (!cin) {
            runtimeError(read_statement, diag::RuntimeInvalidState, "read failed to parse typed input");
            return 1;
        }
        if (variable->type == "array") {
            if (!writeArray(str, num, value)) {
                runtimeError(read_statement, diag::RuntimeArrayBounds, "read target array index out of bounds");
                return 1;
            }
        } else if (!writeScalar(str, value)) {
            runtimeError(read_statement, diag::RuntimeInvalidState, "read target is not declared");
            return 1;
        }
    }
    return 0;
}

int Interpreter::visit(AST_PRINT_RULE *print_statement)
{
    for (std::size_t i = 0; i < print_statement->printables.size(); ++i) {
        if (i != 0) cout << ' ';
        AST_PRINTABLE_ITEM &item = print_statement->printables[i];
        if (item.expression) {
            const RuntimeValue value = evaluate(item.expression);
            if (runtime_failed) return 1;
            if (value.type == ShortType::Float) cout << std::setprecision(17) << value.float_value;
            else if (value.type == ShortType::String) cout << value.string_value;
            else cout << value.int_value;
        } else if (item.string_literal) {
            const RuntimeValue value = evaluate(item.string_literal);
            cout << value.string_value;
        }
    }
    cout << endl;
    return 0;
}

int Interpreter::visit(AST_LABEL_RULE *) { return 0; }

int Interpreter::visit(AST_GREENAI_REPORT_RULE *greenai_report)
{
    const IntValue inferences = evaluate(greenai_report->inferences).int_value;
    const IntValue watts = evaluate(greenai_report->watts).int_value;
    const IntValue seconds = evaluate(greenai_report->seconds).int_value;
    if (runtime_failed) return 1;
    const IntValue energy_joules = wrapMul(watts, seconds);
    const IntValue inferences_per_joule = energy_joules == 0 ? 0 : inferences / energy_joules;
    cout << "GreenAI workload " << unquoteShortString(greenai_report->workload_name)
         << ": inferences=" << inferences
         << " energy_j=" << energy_joules
         << " inf_per_j=" << inferences_per_joule << endl;
    return 0;
}

int Interpreter::visit(AST_AI_INFER_RULE *ai_infer)
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
    for (float value : output) cout << ' ' << value;
    cout << endl;
    return 0;
}

int Interpreter::visit(AST_BINARY_EXPRESSION_RULE *expression)
{
    const RuntimeValue left_value = evaluate(expression->left);
    if (runtime_failed) return 0;
    const RuntimeValue right_value = evaluate(expression->right);
    if (runtime_failed) return 0;

    current_value = defaultValue(ShortType::Boolean);
    if (left_value.type == ShortType::Float) {
        const double left = left_value.float_value;
        const double right = right_value.float_value;
        if (expression->op == AST_BINARY_EXPRESSION_RULE::PLUS ||
            expression->op == AST_BINARY_EXPRESSION_RULE::MINUS ||
            expression->op == AST_BINARY_EXPRESSION_RULE::MULTIPLY ||
            expression->op == AST_BINARY_EXPRESSION_RULE::DIVIDE) {
            if (expression->op == AST_BINARY_EXPRESSION_RULE::DIVIDE && right == 0.0) {
                runtimeError(expression, diag::RuntimeArithmeticDomainError,
                             "float division by zero is forbidden by shorthand.type_memory.v1");
                return 0;
            }
            current_value = defaultValue(ShortType::Float);
            if (expression->op == AST_BINARY_EXPRESSION_RULE::PLUS) current_value.float_value = left + right;
            else if (expression->op == AST_BINARY_EXPRESSION_RULE::MINUS) current_value.float_value = left - right;
            else if (expression->op == AST_BINARY_EXPRESSION_RULE::MULTIPLY) current_value.float_value = left * right;
            else current_value.float_value = left / right;
            return 0;
        }
        if (expression->op == AST_BINARY_EXPRESSION_RULE::LESS) current_value.int_value = left < right;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::GREATER) current_value.int_value = left > right;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::LESS_OR_EQUAL) current_value.int_value = left <= right;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::GREATER_OR_EQUAL) current_value.int_value = left >= right;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::EQUAL) current_value.int_value = left == right;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::NOT_EQUAL) current_value.int_value = left != right;
        return current_value.int_value;
    }
    if (left_value.type == ShortType::String) {
        if (expression->op == AST_BINARY_EXPRESSION_RULE::EQUAL)
            current_value.int_value = left_value.string_value == right_value.string_value;
        else if (expression->op == AST_BINARY_EXPRESSION_RULE::NOT_EQUAL)
            current_value.int_value = left_value.string_value != right_value.string_value;
        return current_value.int_value;
    }

    const IntValue left = left_value.int_value;
    const IntValue right = right_value.int_value;

    switch (expression->op) {
        case AST_BINARY_EXPRESSION_RULE::PLUS:
            current_value = defaultValue(ShortType::Int);
            return current_value.int_value = wrapAdd(left, right);
        case AST_BINARY_EXPRESSION_RULE::MINUS:
            current_value = defaultValue(ShortType::Int);
            return current_value.int_value = wrapSub(left, right);
        case AST_BINARY_EXPRESSION_RULE::MULTIPLY:
            current_value = defaultValue(ShortType::Int);
            return current_value.int_value = wrapMul(left, right);
        case AST_BINARY_EXPRESSION_RULE::DIVIDE:
            if (right == 0 || (left == std::numeric_limits<IntValue>::min() && right == -1)) {
                runtimeError(expression, diag::RuntimeArithmeticDomainError,
                             "integer division has an invalid divisor or overflow case");
                return 0;
            }
            current_value = defaultValue(ShortType::Int);
            return current_value.int_value = left / right;
        case AST_BINARY_EXPRESSION_RULE::MODULO:
            if (right == 0 || (left == std::numeric_limits<IntValue>::min() && right == -1)) {
                runtimeError(expression, diag::RuntimeArithmeticDomainError,
                             "integer remainder has an invalid divisor or overflow case");
                return 0;
            }
            current_value = defaultValue(ShortType::Int);
            return current_value.int_value = left % right;
        case AST_BINARY_EXPRESSION_RULE::LESS: return current_value.int_value = left < right;
        case AST_BINARY_EXPRESSION_RULE::GREATER: return current_value.int_value = left > right;
        case AST_BINARY_EXPRESSION_RULE::LESS_OR_EQUAL: return current_value.int_value = left <= right;
        case AST_BINARY_EXPRESSION_RULE::GREATER_OR_EQUAL: return current_value.int_value = left >= right;
        case AST_BINARY_EXPRESSION_RULE::EQUAL: return current_value.int_value = left == right;
        case AST_BINARY_EXPRESSION_RULE::NOT_EQUAL: return current_value.int_value = left != right;
        case AST_BINARY_EXPRESSION_RULE::OR: return current_value.int_value = (left != 0) || (right != 0);
        case AST_BINARY_EXPRESSION_RULE::AND: return current_value.int_value = (left != 0) && (right != 0);
        default:
            runtimeError(expression, diag::RuntimeInvalidState, "unsupported binary operator reached runtime");
            return 0;
    }
}

int Interpreter::visit(AST_UNARY_EXPRESSION_RULE *expression)
{
    const RuntimeValue value = evaluate(expression->expression);
    if (runtime_failed) return 0;
    if (expression->op == AST_UNARY_EXPRESSION_RULE::UMINUS) {
        current_value = value;
        if (value.type == ShortType::Float) {
            current_value.float_value = -value.float_value;
            return 0;
        }
        return current_value.int_value = wrapSub(0, value.int_value);
    }
    runtimeError(expression, diag::RuntimeInvalidState, "unsupported unary operator reached runtime");
    return 0;
}

int Interpreter::visit(AST_SIMPLE_VARIABLE *variable)
{
    str = variable->variable_name;
    RuntimeValue value;
    if (!readScalar(str, value)) {
        runtimeError(variable, diag::RuntimeInvalidState, "scalar is not declared: `" + str + "`");
        return 0;
    }
    current_value = value;
    return value.int_value;
}

int Interpreter::visit(AST_ARRAY_VARIABLE *variable)
{
    num = evaluate(variable->index).int_value;
    str = variable->array_name;
    if (runtime_failed) return 0;
    RuntimeValue value;
    if (!readArray(str, num, value)) {
        runtimeError(variable, diag::RuntimeArrayBounds,
                     "array index is outside declared bounds for `" + str + "`");
        return 0;
    }
    current_value = value;
    return value.int_value;
}

int Interpreter::visit(AST_LITERAL *literal) {
    current_value = defaultValue(ShortType::Int);
    current_value.int_value = literal->int_literal;
    return current_value.int_value;
}

int Interpreter::visit(AST_STRING_LITERAL *literal)
{
    str = literal->string_literal;
    current_value = defaultValue(ShortType::String);
    current_value.string_value = unquoteShortString(str);
    return 0;
}

int Interpreter::visit(AST_BOOL_LITERAL *literal) {
    current_value = defaultValue(ShortType::Boolean);
    current_value.int_value = literal->value ? 1 : 0;
    return current_value.int_value;
}

int Interpreter::visit(AST_FLOAT_LITERAL *literal)
{
    current_value = defaultValue(ShortType::Float);
    current_value.float_value = literal->value;
    return 0;
}

int Interpreter::visit(AST_FUNCTION_CALL_EXPRESSION *call)
{
    vector<RuntimeValue> arguments;
    for (AST_EXPRESSION_RULE *argument : call->arguments) {
        arguments.push_back(evaluate(argument));
        if (runtime_failed) return 0;
    }
    RuntimeValue result = defaultValue(ShortType::Void);
    if (invokeFunction(call, call->function_name, arguments, result) != 0) return 0;
    current_value = result;
    return result.int_value;
}

int Interpreter::visit(AST_MODEL_DECLARATION *n)
{
    aiModels[n->data.name] = n->data;
    cout << "Registered model " << n->data.name << " (fallback-capable)" << endl;
    return 0;
}

int Interpreter::visit(AST_TENSOR_DECLARATION *n)
{
    aiTensors[n->data.name] = n->data;
    return 0;
}

int Interpreter::visit(AST_GREENAI_CONTRACT *n)
{
    cout << "GreenAI contract " << n->data.name << " evidence_only" << endl;
    return 0;
}

int Interpreter::visit(AST_GREENAI_MEASUREMENT *n)
{
    cout << "GreenAI workload " << n->data.workload << " measurement_status=declared_budget_only" << endl;
    return 0;
}

int Interpreter::visit(AST_C3ECO_DECLARATION *)
{
    return 0;
}

int Interpreter::visit(AST_INFER_STATEMENT *n)
{
    auto modelIt = aiModels.find(n->model_name);
    auto tensorIt = aiTensors.find(n->input_name);
    if (modelIt == aiModels.end()) {
        cout << "AI inference error: model=" << n->model_name << " runtime_backend=fallback reason=unknown_model" << endl;
        return 1;
    }
    if (tensorIt == aiTensors.end()) {
        cout << "AI inference error: model=" << n->model_name << " runtime_backend=fallback reason=unknown_tensor" << endl;
        return 1;
    }
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
        cout << "AI inference success: model=" << n->model_name
             << " runtime_backend=" << result.backend_name
             << " provider=" << result.provider_name
             << " outputs=" << result.output_f32.size() << endl;
    } else if (result.backend == shorthand::ai::BackendKind::Fallback ||
               result.status == shorthand::ai::InferenceStatus::NotExecuted) {
        cout << "AI inference fallback: model=" << n->model_name
             << " runtime_backend=fallback inference_status="
             << shorthand::ai::inferenceStatusToString(result.status)
             << " reason=" << result.reason << endl;
    } else {
        cout << "AI inference error: model=" << n->model_name
             << " runtime_backend=" << result.backend_name
             << " reason=" << result.reason << endl;
    }
    return 0;
}
