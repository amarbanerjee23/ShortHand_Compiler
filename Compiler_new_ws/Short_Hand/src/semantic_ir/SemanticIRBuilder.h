#pragma once

#include "SemanticIR.h"
#include "../ast/AST.h"
#include <map>

// Called only after the ordinary semantic/module checks. No AST pointers escape.
class SemanticIRBuilder {
public:
    bool build(const std::vector<std::pair<AST_PROGRAM *, std::string>> &libraries,
               AST_PROGRAM *entry, const std::string &file,
               shorthand::semantic_ir::ProgramIR &result, std::string &error);
private:
    using Expr = shorthand::semantic_ir::Expression;
    using Stmt = shorthand::semantic_ir::Statement;
    using Type = shorthand::semantic_ir::ScalarType;
    using Var = shorthand::semantic_ir::Variable;
    shorthand::semantic_ir::ProgramIR result_;
    std::string file_, error_;
    std::map<std::string, Type> globals_, returns_;
    std::vector<std::map<std::string, Type>> scopes_;
    shorthand::semantic_ir::SourceRange range(const void *node) const;
    std::vector<Var> declarations(AST_DATA_DECLARATION_BLOCK *node);
    void signatures(AST_PROGRAM *node);
    void functions(AST_PROGRAM *node);
    Type variableType(const std::string &name);
    Expr expression(AST_EXPRESSION_RULE *node);
    Stmt statement(AST_STATEMENT_RULE *node);
};
