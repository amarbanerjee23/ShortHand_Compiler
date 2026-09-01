#ifndef SHORTHAND_SEMANTIC_ANALYZER_H
#define SHORTHAND_SEMANTIC_ANALYZER_H

#include "../ast/AST.h"
#include "../type_system/ProductionTypeSystem.h"
#include "Diagnostics.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

class SemanticAnalyzer : public Visitor {
public:
    struct FunctionSignature {
        ShortType return_type = ShortType::Void;
        std::vector<ShortType> parameter_types;
    };

    Diagnostics diagnostics;

    static std::set<std::string> functionNames(AST_PROGRAM *program);
    static std::map<std::string, std::size_t> functionArities(AST_PROGRAM *program);
    static std::map<std::string, FunctionSignature> functionSignatures(AST_PROGRAM *program);
    static std::set<std::string> globalNames(AST_PROGRAM *program);
    void setImportedFunctions(const std::map<std::string, std::size_t> &arities,
                              bool allow_external_calls);
    void setImportedFunctions(const std::map<std::string, FunctionSignature> &signatures,
                              bool allow_external_calls);

    int visit(AST_PROGRAM*) override;
    int visit(AST_DATA_DECLARATION_BLOCK*) override;
    int visit(AST_FUNCTION_LIST_RULE*) override;
    int visit(AST_LOGIC_BLOCK*) override;
    int visit(AST_EXPRESSION_STATEMENT_RULE*) override;
    int visit(AST_FUNCTION_RULE*) override;
    int visit(AST_FUNCTION_CALL_RULE*) override;
    int visit(AST_ASSIGNMENT_RULE*) override;
    int visit(AST_STATEMENTS_BLOCK*) override;
    int visit(AST_IF_STATEMENT*) override;
    int visit(AST_BREAK*) override;
    int visit(AST_IF_ELSE_STATEMENT*) override;
    int visit(AST_FOR_LOOP_STATEMENT_RULE*) override;
    int visit(AST_WHILE_LOOP_STATEMENT_RULE*) override;
    int visit(AST_GOTO_STATEMENT_RULE*) override;
    int visit(AST_READ_RULE*) override;
    int visit(AST_PRINT_RULE*) override;
    int visit(AST_LABEL_RULE*) override;
    int visit(AST_GREENAI_REPORT_RULE*) override;
    int visit(AST_AI_INFER_RULE*) override;
    int visit(AST_MODEL_DECLARATION*) override;
    int visit(AST_TENSOR_DECLARATION*) override;
    int visit(AST_GREENAI_CONTRACT*) override;
    int visit(AST_GREENAI_MEASUREMENT*) override;
    int visit(AST_C3ECO_DECLARATION*) override;
    int visit(AST_INFER_STATEMENT*) override;
    int visit(AST_CONTINUE*) override;
    int visit(AST_RETURN_STATEMENT*) override;
    int visit(AST_BINARY_EXPRESSION_RULE*) override;
    int visit(AST_UNARY_EXPRESSION_RULE*) override;
    int visit(AST_SIMPLE_VARIABLE*) override;
    int visit(AST_ARRAY_VARIABLE*) override;
    int visit(AST_LITERAL*) override;
    int visit(AST_STRING_LITERAL*) override;
    int visit(AST_BOOL_LITERAL*) override;
    int visit(AST_FLOAT_LITERAL*) override;
    int visit(AST_FUNCTION_CALL_EXPRESSION*) override;

private:
    struct VariableInfo {
        ShortType type = ShortType::Void;
        bool is_array = false;
    };

    std::map<std::string, ModelDeclarationData> models;
    std::map<std::string, TensorDeclarationData> tensors;
    std::map<std::string, GreenAIContractData> contracts;
    std::set<std::string> c3eco_declarations;
    std::vector<AST_C3ECO_DECLARATION *> c3eco_profile_nodes;
    std::set<std::string> functions;
    std::map<std::string, std::size_t> function_arity;
    std::set<std::string> imported_functions;
    std::set<std::string> globals;
    std::map<std::string, ShortType> global_types;
    std::map<std::string, ShortType> global_array_types;
    std::vector<std::map<std::string, VariableInfo>> local_scopes;
    std::vector<std::set<std::string>> block_label_scopes;
    std::map<std::string, ShortType> function_return_types;
    std::map<std::string, std::vector<ShortType>> function_parameter_types;
    std::vector<ShortType> return_types;
    bool allow_external_function_calls = true;
    int loopDepth = 0;

    bool isDeclared(const std::string &name) const;
    static bool isExecutableScalarType(ShortType type);
    bool lookupType(const std::string &name, ShortType &type, bool &is_array) const;
    ShortType expressionType(AST_EXPRESSION_RULE *expression);
    bool requireConditionType(const void *node, AST_EXPRESSION_RULE *expression);
    bool definitelyReturns(AST_STATEMENT_RULE *statement) const;
    void declareInCurrentScope(AST_DATA_DECLARATION_BLOCK *block);
    void validateCall(const void *node, const std::string &name, std::size_t arity);
    void validateCallTypes(const void *node,
                           const std::string &name,
                           const std::vector<AST_EXPRESSION_RULE *> &arguments);
    void validateC3EcoProfiles();
    void validateC3EcoTypedDeclaration(AST_C3ECO_DECLARATION *node);
};

#endif
