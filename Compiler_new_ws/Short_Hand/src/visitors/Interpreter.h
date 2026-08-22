#ifndef _astInterpretVisitor_H
#define _astInterpretVisitor_H

#include "../ast/AST.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Interpreter : public Visitor
{
public:
    using IntValue = std::int32_t;

private:
    struct RuntimeValue {
        ShortType type = ShortType::Int;
        IntValue int_value = 0;
        double float_value = 0.0;
        std::string string_value;
    };

    struct Frame {
        std::map<std::string, RuntimeValue> scalars;
        std::map<std::string, std::vector<RuntimeValue>> arrays;
    };

    enum class FlowSignal {
        Normal,
        Return,
        Break,
        Continue
    };

    std::vector<Frame> frames;
    std::map<std::string, AST_FUNCTION_RULE *> functions;
    std::map<AST_FUNCTION_RULE *, std::string> function_sources;
    std::map<std::string, ModelDeclarationData> aiModels;
    std::map<std::string, TensorDeclarationData> aiTensors;

    std::string entry_source;
    std::string active_source;
    std::string str;
    int num = 0;
    RuntimeValue current_value;
    FlowSignal flow = FlowSignal::Normal;
    RuntimeValue return_value;
    bool runtime_failed = false;
    std::size_t call_depth = 0;
    static constexpr std::size_t kMaxCallDepth = 256;

    Frame &globalFrame();
    void initializeDeclarations(AST_DATA_DECLARATION_BLOCK *decl_block);
    void registerFunctions(AST_FUNCTION_LIST_RULE *functions, const std::string &source_path);
    static RuntimeValue defaultValue(ShortType type);
    static bool truthy(const RuntimeValue &value);
    RuntimeValue evaluate(AST_EXPRESSION_RULE *expression);
    bool readScalar(const std::string &name, RuntimeValue &value) const;
    bool writeScalar(const std::string &name, const RuntimeValue &value);
    bool readArray(const std::string &name, int index, RuntimeValue &value) const;
    bool writeArray(const std::string &name, int index, const RuntimeValue &value);
    void runtimeError(const void *node, const char *code, const std::string &message);
    int invokeFunction(const void *call_node,
                       const std::string &name,
                       const std::vector<RuntimeValue> &arguments,
                       RuntimeValue &result);
    static IntValue wrapAdd(IntValue left, IntValue right);
    static IntValue wrapSub(IntValue left, IntValue right);
    static IntValue wrapMul(IntValue left, IntValue right);

public:
    explicit Interpreter(const std::string &source_path = std::string());
    bool addLibraryProgram(AST_PROGRAM *program, const std::string &source_path);
    bool ok() const { return !runtime_failed; }

    int visit(AST_PROGRAM * program);
    int visit(AST_DATA_DECLARATION_BLOCK * decl_block);
    int visit(AST_FUNCTION_LIST_RULE *functions);
    int visit(AST_LOGIC_BLOCK * code_block);

    int visit(AST_EXPRESSION_STATEMENT_RULE * expression_statement);
    int visit(AST_ASSIGNMENT_RULE * assignment_statement);
    int visit(AST_STATEMENTS_BLOCK * block_statement);
    int visit(AST_FUNCTION_RULE *);
    int visit(AST_BREAK *);
    int visit(AST_FUNCTION_CALL_RULE * function);
    int visit(AST_IF_STATEMENT * if_statement);
    int visit(AST_IF_ELSE_STATEMENT * ifelse_statement);
    int visit(AST_FOR_LOOP_STATEMENT_RULE * for_statement);
    int visit(AST_WHILE_LOOP_STATEMENT_RULE * while_statement);
    int visit(AST_GOTO_STATEMENT_RULE * goto_statement);
    int visit(AST_READ_RULE * read_statement);
    int visit(AST_PRINT_RULE * print_statement);
    int visit(AST_LABEL_RULE * label_statement);
    int visit(AST_GREENAI_REPORT_RULE * greenai_report);
    int visit(AST_AI_INFER_RULE * ai_infer);
    int visit(AST_MODEL_DECLARATION *);
    int visit(AST_TENSOR_DECLARATION *);
    int visit(AST_GREENAI_CONTRACT *);
    int visit(AST_GREENAI_MEASUREMENT *);
    int visit(AST_C3ECO_DECLARATION *);
    int visit(AST_INFER_STATEMENT *);
    int visit(AST_CONTINUE *);
    int visit(AST_RETURN_STATEMENT *);

    int visit(AST_BINARY_EXPRESSION_RULE * binary_operator_expression);
    int visit(AST_UNARY_EXPRESSION_RULE * unary_operator_expression);

    int visit(AST_SIMPLE_VARIABLE * variable_single_int);
    int visit(AST_ARRAY_VARIABLE * variable_array_int);
    int visit(AST_LITERAL * int_literal);
    int visit(AST_STRING_LITERAL * string_literal);
    int visit(AST_BOOL_LITERAL *);
    int visit(AST_FLOAT_LITERAL *);
    int visit(AST_FUNCTION_CALL_EXPRESSION *);
};

#endif
