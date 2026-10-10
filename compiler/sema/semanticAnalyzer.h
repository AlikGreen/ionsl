#pragma once

#include "constantEvaluator.h"
#include "genericInstantiator.h"
#include "globalScope.h"
#include "semaContext.h"
#include "../ast/declarations.h"
#include "../ast/module.h"
#include "../ast/statements.h"
#include "../ast/type.h"


namespace ionsl
{

class SemanticAnalyzer
{
public:
    SemanticAnalyzer(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator, const std::unordered_map<DeclId, std::vector<TypeId>>& specializations = {})
        :   m_module(module), m_symbols(symbolTable), m_typeSystem(typeSystem), m_scopeTable(scopeTable), m_declAllocator(declAllocator),
            m_declTable(module), m_evaluator(m_declTable, m_typeSystem),
            m_globalScope(module), m_genericInstantiator(m_typeSystem, m_declAllocator, m_module), m_specializations(specializations)
    {
    }

    void analyze();
    static void analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator, const std::unordered_map<DeclId, std::vector<TypeId>>& specializations);
private:
    friend class Module;

    Module& m_module;
    const SymbolTable& m_symbols;

    TypeSystem& m_typeSystem;
    ScopeTable& m_scopeTable;
    DeclAllocator& m_declAllocator;
    DeclTable m_declTable;
    ConstantEvaluator m_evaluator;
    GlobalScope m_globalScope;
    GenericInstantiator m_genericInstantiator;
    std::unordered_map<DeclId, std::vector<TypeId>> m_specializations{};
    bool m_speculative = false;

    TypeId checkExpression(Expression*& expression, const SemaContext& ctx);
    TypeId checkBinaryExpr(BinaryExpr& expression, const SemaContext& ctx);
    TypeId checkUnaryExpr(UnaryExpr& expression, const SemaContext& ctx);
    TypeId checkCallExpr(Expression*& slot, CallExpr& expression, const SemaContext& ctx);
    TypeId checkIdentifierCall(CallExpr& expression, const IdentifierExpr& identifier, const SemaContext& ctx);
    TypeId checkIdentifierExpr(IdentifierExpr& expression, const SemaContext& ctx) const;
    TypeId checkIndexExpr(IndexExpr& expression, const SemaContext& ctx);
    TypeId checkLiteralExpr(LiteralExpr& expression) const;
    TypeId checkFieldAccessExpr(Expression*&, FieldAccessExpr& expression, const SemaContext& ctx);

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
    void checkEnumDecl(EnumDecl& declaration, const SemaContext& ctx);
    void checkAttributeDecl(const AttributeDecl& declaration, const SemaContext& ctx);

    void checkDeclarationSignature(Declaration& declaration, const SemaContext& ctx);
    void checkFunctionDeclSignature(const FunctionDecl& declaration, const SemaContext& ctx);
    void checkStructDeclSignature(const StructDecl& declaration, const SemaContext& ctx);
    void checkInterfaceDeclSignature(const InterfaceDecl& declaration, const SemaContext& ctx);

    void checkAttribute(Attribute& attribute, const SemaContext &ctx);

    TypeId resolveType(TypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveTypeArg(TypeArgument& arg, const SemaContext& ctx);

    TypeId resolveVectorType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveMatrixType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveNamedType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, const SemaContext& ctx);
    TypeId resolveArrayType(ArrayTypeSyntax& syntax, const SemaContext& ctx);

    std::vector<DeclId> find(QualifiedName name, const SemaContext& ctx);

    static PrimitiveKind toPrimitiveKind(const std::string &name);

    Expression* makeConversion(Expression* operand, TypeId type) const;

    uint8_t componentIndex(char c);

    template<typename... Args>
    void error(SourceSpan span, std::format_string<Args...> fmt, Args&&... args)
    {
        if (!m_speculative)
            m_module.diagnostics().error(span, fmt, std::forward<Args>(args)...);
    }
};
}
