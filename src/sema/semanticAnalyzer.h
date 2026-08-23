#pragma once
#include "../ast/module.h"
#include "../ast/statements.h"
#include "../ast/type.h"
#include "../ast/typeSyntax.h"

namespace ionsl
{
class SemanticAnalyzer
{
public:
    SemanticAnalyzer(Module& module, const SymbolTable& symbolTable)
        : m_module(module), m_symbols(symbolTable) { }

    void analyze();
private:
    Module& m_module;
    const SymbolTable& m_symbols;

    std::unordered_map<SymbolId, std::vector<DeclId>> m_declTable;

    void buildDeclTable();

    TypeId resolveType(TypeSyntax& syntax);
    TypeId resolveVectorType(const NamedTypeSyntax& syntax);
    TypeId resolveMatrixType(NamedTypeSyntax& syntax);

    TypeId checkExpression(Expression& expression);
    void checkStatement(Statement& statement);
    void checkDeclaration(Declaration& declaration);
};
}
