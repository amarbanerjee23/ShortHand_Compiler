#include <vector>
#include <string>
#include <iostream>
#include "AST.h"
using namespace std;


void SymbolTableInsert(AST_DATA_DECLARATION_VARIABLE * var)
{

}

/*
===========================================
=============== methods ====================
===========================================
*/

/*
============== main program ==================
*/
AST_PROGRAM::AST_PROGRAM(AST_DATA_DECLARATION_BLOCK * decl_block, AST_FUNCTION_LIST_RULE * functions, AST_LOGIC_BLOCK * code_block)
{
    this->decl_block = decl_block;
    this->code_block = code_block;
    this->functions = functions;
}
int AST_PROGRAM::accept(Visitor & v)
{
    return v.visit(this);
}

void AST_PROGRAM::setProgramName(char * progName){
  this->program_name = progName;
};


/*
============== declaration block ==================
*/
void AST_DATA_DECLARATION_BLOCK::push_back(string name)
{
    single_ints.push_back(name);
}
void AST_DATA_DECLARATION_BLOCK::push_back(string name, int size)
{
    array_ints.push_back(make_pair(name, size));
}
void AST_DATA_DECLARATION_BLOCK::push_back(AST_DATA_DECLARATION_BLOCK * decl_block)
{
    single_ints.insert(single_ints.end(), (decl_block->single_ints).begin(), (decl_block->single_ints).end());
    array_ints.insert(array_ints.end(), (decl_block->array_ints).begin(), (decl_block->array_ints).end());
}
int AST_DATA_DECLARATION_BLOCK::accept(Visitor & v)
{
    return v.visit(this);
}


//------------------------------

void AST_FUNCTION_LIST_RULE::push_back(AST_FUNCTION_RULE * function)
{
	functions.push_back(function);

}

int AST_FUNCTION_LIST_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

AST_FUNCTION_RULE::AST_FUNCTION_RULE(ShortType type,char * functionName, AST_DATA_DECLARATION_BLOCK * parameters, AST_STATEMENTS_BLOCK * block_statement)
{
    this->type = type;
    this->function_name = functionName;
    this->block_statement = block_statement;
    this->parameters = parameters;
}
int AST_FUNCTION_RULE::accept(Visitor & v)
{
    return v.visit(this);
}


/*
============== code block ==================
*/
//------------------------------
AST_LOGIC_BLOCK::AST_LOGIC_BLOCK(AST_STATEMENTS_BLOCK * block_statement)
{
    this->block_statement = block_statement;
}
int AST_LOGIC_BLOCK::accept(Visitor & v)
{
    return v.visit(this);
}

/*
============== statement ==================
*/
//------------------------------
AST_EXPRESSION_STATEMENT_RULE::AST_EXPRESSION_STATEMENT_RULE(AST_EXPRESSION_RULE * expression)
{
    this->expression = expression;
}
int AST_EXPRESSION_STATEMENT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}



//------------------------------
AST_FUNCTION_CALL_RULE::AST_FUNCTION_CALL_RULE(char * functionName,  AST_READ_RULE * param)
{
    this->function_name = functionName;
    this->parameters = param;
}
int AST_FUNCTION_CALL_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_ASSIGNMENT_RULE::AST_ASSIGNMENT_RULE(AST_VARIABLE_RULE * variable, AST_EXPRESSION_RULE * expression)
{
    this->variable = variable;
    this->expression = expression;
}
int AST_ASSIGNMENT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
void AST_STATEMENTS_BLOCK::push_back(AST_STATEMENT_RULE * statement)
{
    statements.push_back(statement);
}
int AST_STATEMENTS_BLOCK::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_IF_STATEMENT::AST_IF_STATEMENT(AST_EXPRESSION_RULE * condition, AST_STATEMENTS_BLOCK * if_block)
{
    this->condition = condition;
    this->if_block = if_block;
}
int AST_IF_STATEMENT::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_IF_ELSE_STATEMENT::AST_IF_ELSE_STATEMENT(AST_EXPRESSION_RULE * condition, AST_STATEMENTS_BLOCK * if_block, AST_STATEMENTS_BLOCK * else_block)
{
    this->condition = condition;
    this->if_block = if_block;
    this->else_block = else_block;
}
int AST_IF_ELSE_STATEMENT::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_FOR_LOOP_STATEMENT_RULE::AST_FOR_LOOP_STATEMENT_RULE(AST_VARIABLE_RULE * variable, AST_EXPRESSION_RULE * from, AST_EXPRESSION_RULE * step, AST_EXPRESSION_RULE * to, AST_STATEMENTS_BLOCK * for_block)
{
    this->variable = variable;
    this->from = from;
    this->step = step;
    this->to = to;
    this->for_block = for_block;
}
AST_FOR_LOOP_STATEMENT_RULE::AST_FOR_LOOP_STATEMENT_RULE(AST_VARIABLE_RULE * variable, AST_EXPRESSION_RULE * from, AST_EXPRESSION_RULE * to, AST_STATEMENTS_BLOCK * for_block)
{
    this->variable = variable;
    this->from = from;
    this->step = new AST_LITERAL(1);
    this->to = to;
    this->for_block = for_block;
}
int AST_FOR_LOOP_STATEMENT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}
//------------------------------

int AST_BREAK::accept(Visitor & v)
{
    return v.visit(this);
}


//------------------------------
AST_WHILE_LOOP_STATEMENT_RULE::AST_WHILE_LOOP_STATEMENT_RULE(AST_EXPRESSION_RULE * condition, AST_STATEMENTS_BLOCK * while_block)
{
    this->condition = condition;
    this->while_block = while_block;
}
int AST_WHILE_LOOP_STATEMENT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_GOTO_STATEMENT_RULE::AST_GOTO_STATEMENT_RULE(AST_EXPRESSION_RULE * condition, string label)
{
    this->condition = condition;
    this->label = label;
}
AST_GOTO_STATEMENT_RULE::AST_GOTO_STATEMENT_RULE(string label)
{
    //this->condition = new AST_int_literal(1);
    this->condition = NULL;
    this->label = label;
}
int AST_GOTO_STATEMENT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
void AST_READ_RULE::push_back(AST_VARIABLE_RULE * variable)
{
    variables.push_back(variable);
}
int AST_READ_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
void AST_PRINT_RULE::push_back(AST_STRING_LITERAL * string_literal)
{
    struct AST_PRINTABLE_ITEM x;
    x.string_literal = string_literal;
    x.expression = NULL;
    printables.push_back(x);
}
void AST_PRINT_RULE::push_back(AST_EXPRESSION_RULE * expression)
{
    struct AST_PRINTABLE_ITEM x;
    x.string_literal = NULL;
    x.expression = expression;
    printables.push_back(x);
}
int AST_PRINT_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_LABEL_RULE::AST_LABEL_RULE(string label)
{
    this->label = label;
}
int AST_LABEL_RULE::accept(Visitor & v)
{
    return v.visit(this);
}


/*
============== expression ==================
*/

AST_BINARY_EXPRESSION_RULE::AST_BINARY_EXPRESSION_RULE(AST_EXPRESSION_RULE * left, AST_EXPRESSION_RULE * right, string op)
{
    this->left = left;
    this->right = right;
    if     (op == "+") this->op = PLUS;
    else if(op == "-") this->op = MINUS;
    else if(op == "/") this->op = DIVIDE;
    else if(op == "*") this->op = MULTIPLY;
    else if(op == "%") this->op = MODULO;
    else if(op == "<") this->op = LESS;
    else if(op == ">") this->op = GREATER;
    else if(op == ">=") this->op = GREATER_OR_EQUAL;
    else if(op == "<=") this->op = LESS_OR_EQUAL;
    else if(op == "==") this->op = EQUAL;
    else if(op == "!=") this->op = NOT_EQUAL;
    else if(op == "||") this->op = OR;
    else if(op == "&&") this->op = AND;
    else                this->op = ERROR;
}
int AST_BINARY_EXPRESSION_RULE::accept(Visitor & v)
{
    return v.visit(this);
}

AST_UNARY_EXPRESSION_RULE::AST_UNARY_EXPRESSION_RULE(AST_EXPRESSION_RULE * expression, string op)
{
    this->expression = expression;
    if(op == "-") this->op = UMINUS;
    else          this->op = ERROR;
}
int AST_UNARY_EXPRESSION_RULE::accept(Visitor & v)
{
    return v.visit(this);
}



/*
============== variables and literals ==================
*/
//------------------------------
AST_SIMPLE_VARIABLE::AST_SIMPLE_VARIABLE(string variable_name)
{
    this->type = "single";
    this->variable_name = variable_name;
}
int AST_SIMPLE_VARIABLE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_ARRAY_VARIABLE::AST_ARRAY_VARIABLE(string array_name, AST_EXPRESSION_RULE * index)
{
    this->type = "array";
    this->array_name = array_name;
    this->index = index;
}
int AST_ARRAY_VARIABLE::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_LITERAL::AST_LITERAL(int int_literal)
{
    this->int_literal = int_literal;
}
int AST_LITERAL::accept(Visitor & v)
{
    return v.visit(this);
}

//------------------------------
AST_STRING_LITERAL::AST_STRING_LITERAL(string string_literal)
{
    this->string_literal = string_literal;
}
int AST_STRING_LITERAL::accept(Visitor & v)
{
    return v.visit(this);
}
