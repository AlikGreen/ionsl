#pragma once
#include <functional>

#include "ast.h"

namespace ionsl
{
class AstWalker
{
public:
    std::function<void(DeclNode&)> onDecl;
    std::function<void(StmtNode&)> onStmt;
    std::function<void(ExprNode&)> onExpr;
    std::function<void(Type&)> onType;

    void walk(std::vector<DeclNode>& decls);
    void walk(DeclNode& decl);
    void walk(StmtNode& stmt);
    void walk(ExprNode& expr);
    void walk(Type& type);
private:
    void walkStruct(StructDecl& decl);
    void walkFunction(FunctionDecl& decl);
    void walkVar(VarDecl& decl);
    void walkInterface(InterfaceDecl& decl);

    void walkBlock(BlockStmt& stmt);
    void walkIf(IfStmt& stmt);
    void walkWhile(WhileStmt& stmt);
    void walkFor(ForStmt& stmt);
    void walkReturn(ReturnStmt& stmt);

    void walkBinary(BinaryExpr& expr);
    void walkUnary(UnaryExpr& expr);
    void walkFieldAccess(FieldAccessExpr& expr);
    void walkFunctionCall(FunctionCallExpr& expr);
    void walkIndex(IndexExpr& expr);
};
}
