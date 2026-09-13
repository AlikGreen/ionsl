#pragma once
#include <span>

#include "constantEvaluator.h"
#include "globalScope.h"
#include "typeResolver.h"
#include "semaContext.h"
#include "../ast/declarations.h"
#include "../ast/statements.h"
#include "../ast/type.h"


namespace ionsl
{

class SemanticAnalyzer
{
public:
    SemanticAnalyzer(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator)
        :   m_module(module), m_symbols(symbolTable), m_typeSystem(typeSystem), m_scopeTable(scopeTable), m_declAllocator(declAllocator),
            m_declTable(module), m_constEval(m_declTable, m_typeSystem),
            m_globalScope(module), m_typeResolver(typeSystem, m_constEval, symbolTable, scopeTable, m_globalScope, m_declTable)
    {
    }

    void analyze();
    static void analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator);
private:
    Module& m_module;
    const SymbolTable& m_symbols;

    TypeSystem& m_typeSystem;
    ScopeTable& m_scopeTable;
    DeclAllocator& m_declAllocator;
    DeclTable m_declTable;
    ConstantEvaluator m_constEval;
    GlobalScope m_globalScope;
    TypeResolver m_typeResolver;

    TypeId checkExpression(Expression*& expression, const SemaContext& ctx);
    TypeId checkBinaryExpr(BinaryExpr& expression, const SemaContext& ctx);
    TypeId checkUnaryExpr(UnaryExpr& expression, const SemaContext& ctx);
    TypeId checkCallExpr(Expression*& slot, CallExpr& expression, const SemaContext& ctx);
    TypeId checkIdentifierCall(CallExpr& expression, const IdentifierExpr& identifier, const SemaContext& ctx);
    TypeId checkIdentifierExpr(IdentifierExpr& expression, const SemaContext& ctx) const;
    TypeId checkIndexExpr(IndexExpr& expression, const SemaContext& ctx);
    TypeId checkLiteralExpr(LiteralExpr& expression) const;
    TypeId checkFieldAccessExpr(FieldAccessExpr& expression, const SemaContext& ctx);

    void checkStatement(Statement& statement, const SemaContext& ctx);
    void checkBlockStmt(const BlockStmt& statement, const SemaContext& ctx);
    void checkIfStmt(IfStmt& statement, const SemaContext& ctx);
    void checkForStmt(ForStmt& statement, const SemaContext& ctx);
    void checkWhileStmt(WhileStmt& statement, const SemaContext& ctx);
    void checkReturnStmt(ReturnStmt& statement, const SemaContext& ctx);
    void checkBreakContinueStmt();

    void checkDeclaration(Declaration& declaration, const SemaContext& ctx);
    void checkFunctionDecl(const FunctionDecl& declaration, const SemaContext& ctx);
    void checkStructDecl(const StructDecl& declaration, const SemaContext& ctx);
    void checkInterfaceDecl(const InterfaceDecl& declaration, const SemaContext& ctx);
    void checkValueDecl(ValueDecl& declaration, const SemaContext& ctx);

    Expression* makeConversion(Expression* operand, TypeId type) const;
};
}
