#include "AST_Printer.h"
// program
int AST_Printer::visit(AST_PROGRAM * program)
{
    cout << "program" << endl;
    program->decl_block->accept(*this);
    program->code_block->accept(*this);
    return 0;
}

// decl_block
int AST_Printer::visit(AST_DATA_DECLARATION_BLOCK * decl_block)
{
    cout << "decl_block" << endl;

    cout << "single ints : ";
    for(int i = 0; i < (int)decl_block->single_ints.size(); i++)
        cout << decl_block->single_ints[i] << ", ";
    cout << endl;

    cout << "array ints : ";
    for(int i = 0; i < (int)decl_block->array_ints.size(); i++)
        cout << decl_block->array_ints[i].first << ":" << decl_block->array_ints[i].second << ", ";
    cout << endl;
    return 0;
}

int AST_Printer::visit(AST_FUNCTION_LIST_RULE *functions) {
    // Define function's signature

	for(auto function : functions->functions)
		{
		function->accept(*this);
		}
	return 0;
}

// code_block
int AST_Printer::visit(AST_LOGIC_BLOCK * code_block)
{
    cout << "code_block" << endl;
    code_block->block_statement->accept(*this);
    return 0;
}

//
// all statements
//
int AST_Printer::visit(AST_EXPRESSION_STATEMENT_RULE * expression_statement)
{
    cout << "expression_statement" << endl;
    expression_statement->expression->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_ASSIGNMENT_RULE * assignment_statement)
{
    cout << "assignment_statement" << endl;
    assignment_statement->expression->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_FUNCTION_RULE * function)
{
    cout << "function" << endl;
    function->block_statement->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_FUNCTION_CALL_RULE * function)
{
    cout << "function called" << endl;
    function->parameters->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_STATEMENTS_BLOCK * block_statement)
{
    cout << "block_statement" << endl;
    for(int i = 0; i < (int)block_statement->statements.size(); i++)
        block_statement->statements[i]->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_IF_STATEMENT * if_statement)
{
    cout << "if_statement" << endl;
    cout << "if condition" << endl;
    if_statement->condition->accept(*this);
    cout << "if_block" << endl;
    if_statement->if_block->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_IF_ELSE_STATEMENT * ifelse_statement)
{
    cout << "ifelse_statement" << endl;
    cout << "if condition" << endl;
    ifelse_statement->condition->accept(*this);
    cout << "if_block" << endl;
    ifelse_statement->if_block->accept(*this);
    cout << "else_block" << endl;
    ifelse_statement->else_block->accept(*this);
    return 0;
}
int AST_Printer::visit(AST_BREAK * break_statement) {
    // Define function's signature
	cout << "break" << endl;
	return 0;
}

int AST_Printer::visit(AST_FOR_LOOP_STATEMENT_RULE * for_statement)
{
    cout << "for_statement" << endl;
    cout << "from" << endl;
    for_statement->from->accept(*this);
    cout << "step" << endl;
    for_statement->step->accept(*this);
    cout << "to" << endl;
    for_statement->to->accept(*this);
    cout << "for_block" << endl;
    for_statement->for_block->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_WHILE_LOOP_STATEMENT_RULE * while_statement)
{
    cout << "while_statement" << endl;
    cout << "while condition" << endl;
    while_statement->condition->accept(*this);
    cout << "while_block" << endl;
    while_statement->while_block->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_GOTO_STATEMENT_RULE * goto_statement)
{
    cout << "goto_statement : " << goto_statement->label << endl;
    if(goto_statement->condition) goto_statement->condition->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_READ_RULE * read_statement)
{
    cout << "read_statement" << endl;
    return 0;
}

int AST_Printer::visit(AST_PRINT_RULE * print_statement)
{
    cout << "print_statement" << endl;
    return 0;
}

int AST_Printer::visit(AST_LABEL_RULE * label_statement)
{
    cout << "label_statement : " << label_statement->label << endl;
    return 0;
}


//
// all expressions
//
int AST_Printer::visit(AST_BINARY_EXPRESSION_RULE * binary_operator_expression)
{
    cout << "binary_operator_expression : " << binary_operator_expression->op << endl;
    cout << "left" << endl;
    binary_operator_expression->left->accept(*this);
    cout << "right" << endl;
    binary_operator_expression->right->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_UNARY_EXPRESSION_RULE * unary_operator_expression)
{
    cout << "unary_operator_expression : " << unary_operator_expression->op << endl;
    unary_operator_expression->expression->accept(*this);
    return 0;
}


//
// variables and literals
//
int AST_Printer::visit(AST_SIMPLE_VARIABLE * variable_single_int)
{
    cout << "variable_single_int : " << variable_single_int->variable_name << endl;
    return 0;
}

int AST_Printer::visit(AST_ARRAY_VARIABLE * variable_array_int)
{
    cout << "variable_array_int : " << variable_array_int->array_name << endl;
    cout << "index expression" << endl;
    variable_array_int->index->accept(*this);
    return 0;
}

int AST_Printer::visit(AST_LITERAL * int_literal)
{
    cout << "int_literal : " << int_literal->int_literal << endl;
    return 0;
}

int AST_Printer::visit(AST_STRING_LITERAL * string_literal)
{
    cout << "string_literal : " << string_literal->string_literal << endl;
    return 0;
}
