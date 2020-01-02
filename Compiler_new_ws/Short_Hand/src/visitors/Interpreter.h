#ifndef _astInterpretVisitor_H
#define _astInterpretVisitor_H
#include "../ast/AST.h"
#include<map>

class Interpreter;

class Interpreter : public Visitor
{
private:
    map<string, int> ST_single_int;
    map<string, vector<int> > ST_array_int;
    string str;
    int num;

public:
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

    int visit(AST_BINARY_EXPRESSION_RULE * binary_operator_expression);
    int visit(AST_UNARY_EXPRESSION_RULE * unary_operator_expression);

    int visit(AST_SIMPLE_VARIABLE * variable_single_int);
    int visit(AST_ARRAY_VARIABLE * variable_array_int);
    int visit(AST_LITERAL * int_literal);
    int visit(AST_STRING_LITERAL * string_literal);
};
#endif
