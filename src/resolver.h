#pragma once
#include <set>

#include "ast.h"
#include "declTable.h"
#include "module.h"
#include "typeSystem.h"

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

    DeclTable m_declTable{};
    TypeSystem m_typeSystem{};

    std::unordered_map<std::string, Type> m_resolvedTypes;
    std::unordered_map<std::string, StructType> m_currentTypeBindings;

    void resolveDecl(DeclNode& decl);

    void resolveDeclSignature(DeclNode &decl);

    void resolveFunctionDecl(FunctionDecl& decl);
    void resolveVarDecl(VarDecl& decl);
    void resolveStructDecl(StructDecl& decl);
    void resolveInterfaceDecl(InterfaceDecl& decl);
    void resolveTypeDefDecl(TypeDefDecl& decl);

    void resolveFunctionDeclSignature(FunctionDecl& decl);
    void resolveTypeDefDeclSignature(TypeDefDecl& decl);

    Type resolveExpr(ExprNode& expr);
    Type resolveTypeExpr(TypeExpr& expr);
    Type resolveBinaryExpr(BinaryExpr& expr);
    Type resolveUnaryExpr(UnaryExpr& expr);
    Type resolveIndexExpr(IndexExpr& expr);
    Type resolveFunctionCallExpr(FunctionCallExpr& expr);
    Type resolveFieldAccessExpr(FieldAccessExpr& expr);
    Type resolveIdentifierExpr(IdentifierExpr& expr, ExprNode& node);

    void resolveStmt(StmtNode& stmt);
    void resolveBlockStmt(BlockStmt& block);
    void resolveIfStmt(IfStmt& stmt);
    void resolveForStmt(ForStmt& stmt);
    void resolveWhileStmt(WhileStmt& stmt);
    void resolveReturnStmt(ReturnStmt& stmt);

    Type resolveType(Type original);
};
}
