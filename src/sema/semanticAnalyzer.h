#pragma once
#include "constantEvaluator.h"
#include "../ast/declarations.h"
#include "../ast/module.h"
#include "../ast/statements.h"
#include "../ast/type.h"
#include "../ast/typeSyntax.h"

namespace ionsl
{
class SemanticAnalyzer
{
public:
    SemanticAnalyzer(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, DeclTable& declTable, ScopeTable& scopeTable)
        : m_module(module), m_symbols(symbolTable), m_typeSystem(typeSystem), m_declTable(declTable), m_scopeTable(scopeTable),
          m_constEval(declTable, m_typeSystem)
    {
    }

    void analyze();
    static void analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, DeclTable& declTable, ScopeTable& scopeTable);
private:
    Module& m_module;
    const SymbolTable& m_symbols;

    TypeSystem& m_typeSystem;
    DeclTable& m_declTable;
    ScopeTable& m_scopeTable;
    ConstantEvaluator m_constEval;

    ScopeId m_currentScope = 0;

    TypeId resolveType(TypeSyntax& syntax);
    TypeId resolveVectorType(const NamedTypeSyntax& syntax);
    TypeId resolveMatrixType(const NamedTypeSyntax& syntax);
    TypeId resolveNamedType(const NamedTypeSyntax& syntax);
    TypeId resolveArrayType(const ArrayTypeSyntax& syntax);

    TypeId checkExpression(Expression& expression);
    TypeId checkBinaryExpr(BinaryExpr& expression);
    TypeId checkUnaryExpr(UnaryExpr& expression);
    TypeId checkCallExpr(CallExpr& expression);
    TypeId checkIdentifierCall(CallExpr& expression, const IdentifierExpr& identifier);
    TypeId checkIdentifierExpr(IdentifierExpr& expression) const;
    TypeId checkIndexExpr(IndexExpr& expression);
    TypeId checkLiteralExpr(LiteralExpr& expression) const;
    TypeId checkFieldAccessExpr(FieldAccessExpr& expression);

    void checkStatement(Statement& statement);
    void checkBlockStmt(const BlockStmt& statement);
    void checkIfStmt(const IfStmt& statement);
    void checkForStmt(const ForStmt& statement);
    void checkWhileStmt(const WhileStmt& statement);
    void checkReturnStmt(const ReturnStmt& statement);
    void checkBreakContinueStmt();

    void checkDeclaration(Declaration& declaration);
    void checkDeclSignature(Declaration& declaration);

    void checkFunctionSignature(FunctionDecl& declaration);
    void checkStructSignature(const StructDecl& declaration);
    void checkInterfaceSignature(const InterfaceDecl& declaration);

    void checkFunctionDecl(const FunctionDecl& declaration);
    void checkStructDecl(const StructDecl& declaration);
    void checkInterfaceDecl(const InterfaceDecl& declaration);
    void checkValueDecl(ValueDecl& declaration);

    static PrimitiveKind toPrimitiveKind(const std::string &name);
    Expression* makeConversion(Expression* operand, TypeId type) const;
};
}
