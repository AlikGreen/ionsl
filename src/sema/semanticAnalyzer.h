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
    SemanticAnalyzer(Module& module, const SymbolTable& symbolTable, const TypeSystem& typeSystem)
        : m_module(module), m_symbols(symbolTable), m_typeSystem(typeSystem), m_constEval(m_module.declTable, m_typeSystem) { }

    void analyze();
private:
    Module& m_module;
    const SymbolTable& m_symbols;

    TypeSystem m_typeSystem;
    ConstantEvaluator m_constEval;

    ScopeId m_currentScope;

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
