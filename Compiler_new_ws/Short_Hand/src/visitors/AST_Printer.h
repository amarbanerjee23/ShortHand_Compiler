#ifndef _astPrintVisitor_H
#define _astPrintVisitor_H
#include "../ast/AST.h"

class AST_Printer;

class AST_Printer : public Visitor
{
public:

    int visit(AST_PROGRAM * program);
    int visit(AST_DATA_DECLARATION_BLOCK * decl_block);
    int visit(AST_FUNCTION_LIST_RULE * functions);
    int visit(AST_LOGIC_BLOCK * code_block);

    int visit(AST_EXPRESSION_STATEMENT_RULE * expression_statement);
    int visit(AST_ASSIGNMENT_RULE * assignment_statement);
    int visit(AST_FUNCTION_RULE *);
    int visit(AST_FUNCTION_CALL_RULE *);
    int visit(AST_BREAK *);
    int visit(AST_STATEMENTS_BLOCK * block_statement);
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
    int visit(AST_MODEL_DECLARATION *); int visit(AST_TENSOR_DECLARATION *); int visit(AST_GREENAI_CONTRACT *); int visit(AST_GREENAI_MEASUREMENT *); int visit(AST_INFER_STATEMENT *); int visit(AST_CONTINUE *); int visit(AST_RETURN_STATEMENT *);

    int visit(AST_BINARY_EXPRESSION_RULE * binary_operator_expression);
    int visit(AST_UNARY_EXPRESSION_RULE * unary_operator_expression);

    int visit(AST_SIMPLE_VARIABLE * variable_single_int);
    int visit(AST_ARRAY_VARIABLE * variable_array_int);
    int visit(AST_LITERAL * int_literal);
    int visit(AST_STRING_LITERAL * string_literal);
    int visit(AST_BOOL_LITERAL *); int visit(AST_FLOAT_LITERAL *); int visit(AST_FUNCTION_CALL_EXPRESSION *);
};
#endif
