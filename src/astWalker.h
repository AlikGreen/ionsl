#pragma once
#include <functional>

#include "ast.h"

namespace ionsl
{
class AstWalker
{
public:
    std::function<void(const DeclNode&)> onDecl;
    std::function<void(const StmtNode&)> onStmt;
    std::function<void(const ExprNode&)> onExpr;
    std::function<void(const Type&)> onType;

    void walk(const std::vector<DeclNode>& decls);
    void walk(const DeclNode& decl);
    void walk(const StmtNode& stmt);
    void walk(const ExprNode& expr);
    void walk(const Type& type);
private:
    void walkStruct(const StructDecl& decl);
    void walkFunction(const FunctionDecl& decl);
    void walkVar(const VarDecl& decl);
    void walkInterface(const InterfaceDecl& decl);

    void walkBlock(const BlockStmt& stmt);
    void walkIf(const IfStmt& stmt);
    void walkWhile(const WhileStmt& stmt);
    void walkFor(const ForStmt& stmt);
    void walkReturn(const ReturnStmt& stmt);

    void walkBinary(const BinaryExpr& expr);
    void walkUnary(const UnaryExpr& expr);
    void walkFieldAccess(const FieldAccessExpr& expr);
    void walkFunctionCall(const FunctionCallExpr& expr);
    void walkIndex(const IndexExpr& expr);
};
}
