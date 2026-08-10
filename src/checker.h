#pragma once
#include <unordered_set>

#include "declTable.h"
#include "module.h"

namespace ionsl
{
class Checker
{
public:
    explicit Checker(Module &module);
    std::vector<Diagnostic> check();

    static std::vector<Diagnostic> check(Module& module);
private:
    struct TopLevel{};
    using ScopeDeclType = std::variant<const FunctionDecl*, const BlockStmt*, const IfStmt*, const ForStmt*, const WhileStmt*, TopLevel>;
    struct Scope;

    DeclTable m_declTable;
    const std::vector<DeclNode>& m_declNodes;
    std::vector<Diagnostic> m_diagnostics;
    std::vector<Scope> m_scopes;

    std::unordered_set<const FunctionDecl*> m_functions;
    std::unordered_set<const StructDecl*> m_structs;

    void checkDecl(const DeclNode& decl);
    void checkFunctionDecl(const FunctionDecl& decl);
    void checkVarDecl(const VarDecl& decl, SourceSpan span);
    void checkStructDecl(const StructDecl& decl);

    void checkExpr(const ExprNode& expr);
    void checkBinaryExpr(const BinaryExpr& expr);
    void checkIdentifierExpr(const IdentifierExpr& expr, SourceSpan span);
    void checkIndexExpr(const IndexExpr& expr);
    void checkUnaryExpr(const UnaryExpr& expr);
    void checkFieldAccessExpr(const FieldAccessExpr& expr);
    void checkFunctionCallExpr(const FunctionCallExpr& expr);
    void checkTypeExpr(const TypeExpr& expr);

    void checkStmt(const StmtNode& stmt);
    void checkBlockStmt(const BlockStmt& block, ScopeDeclType type = static_cast<BlockStmt*>(nullptr));
    void checkForStmt(const ForStmt& stmt);
    void checkWhileStmt(const WhileStmt& stmt);
    void checkIfStmt(const IfStmt& stmt);
    void checkReturnStmt(const ReturnStmt& stmt);
    void checkBreakStmt(const BreakStmt& stmt, SourceSpan span);
    void checkContinueStmt(const ContinueStmt& stmt, SourceSpan span);
    void checkExprStmt(const ExprStmt& stmt);

    void checkType(const Type& t);

    bool isDeclared(const std::string &name) const;

    template<typename T>
    [[nodiscard]] const Scope* getFirstScopeOf() const
    {
        for(const auto& scope : m_scopes)
        {
            if(std::holds_alternative<T>(scope.scopeDecl))
                return &scope;
        }

        return nullptr;
    }

    struct Scope
    {
        ScopeDeclType scopeDecl;
        std::unordered_set<std::string> identifiers{};
    };
};
}
