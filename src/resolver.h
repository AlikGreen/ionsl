#pragma once
#include <set>

#include "ast.h"
#include "module.h"

namespace ionsl
{
struct SpecializationRequest
{
    const FunctionDecl* genericFunction;
    std::unordered_map<std::string, StructType> bindings;
};

struct ResolveDesc
{
    std::vector<Module> modules;
    std::vector<SpecializationRequest> specializations;
};

enum class StructureRoles
{
    VertexInput, StageOutput, ResourceContent
};

class Resolver
{
public:
    explicit Resolver(ResolveDesc desc);
    Module resolve();

    static Module resolve(ResolveDesc desc);
private:
    ResolveDesc m_desc;

    std::vector<DeclNode> m_ast;

    std::unordered_map<std::string, StructType> m_structs;
    std::unordered_map<std::string, InterfaceType> m_interfaces;
    std::unordered_map<std::string, StructType> m_currentTypeBindings;

    void resolveDecl(DeclNode& decl);
    void resolveFunctionDecl(FunctionDecl& decl);
    void resolveVarDecl(VarDecl& decl);
    void resolveStructDecl(StructDecl& decl);
    void resolveInterfaceDecl(InterfaceDecl& decl);

    void resolveExpr(ExprNode& expr);
    void resolveTypeExpr(TypeExpr& expr);
    void resolveBinaryExpr(BinaryExpr& expr);
    void resolveUnaryExpr(UnaryExpr& expr);
    void resolveIndexExpr(IndexExpr& expr);
    void resolveFunctionCallExpr(FunctionCallExpr& expr);
    void resolveFieldAccessExpr(FieldAccessExpr& expr);
    Expr resolveIdentifierExpr(IdentifierExpr expr);

    void resolveStmt(StmtNode& stmt);
    void resolveBlockStmt(BlockStmt& block);
    void resolveIfStmt(IfStmt& stmt);
    void resolveForStmt(ForStmt& stmt);
    void resolveWhileStmt(WhileStmt& stmt);
    void resolveReturnStmt(ReturnStmt& stmt);

    Type resolveType(Type original);
};
}
