#include "astWalker.h"

namespace ionsl
{
    void AstWalker::walk(const std::vector<DeclNode>& decls)
    {
        for (const auto& decl : decls) walk(decl);
    }

    void AstWalker::walk(const DeclNode& decl)
    {
        if (onDecl) onDecl(decl);

        std::visit([this]<typename T0>(const T0& d)
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, StructDecl>)    walkStruct(d);
            else if constexpr (std::is_same_v<T, FunctionDecl>) walkFunction(d);
            else if constexpr (std::is_same_v<T, VarDecl>)   walkVar(d);
            else if constexpr (std::is_same_v<T, InterfaceDecl>) walkInterface(d);
        }, decl.decl);
    }

    void AstWalker::walkStruct(const StructDecl& decl)
    {
        for (const auto& field : decl.fields)
        {
            walk(field.type);
            if (field.initializer) walk(*field.initializer);
        }
        for (const auto& method : decl.methods)
            walkFunction(method.decl);
    }

    void AstWalker::walkFunction(const FunctionDecl& decl)
    {
        for (const auto& param : decl.params)
        {
            walk(param.type);
            if (param.defaultValue) walk(*param.defaultValue);
        }
        walk(decl.returnType);
        if (decl.body) walkBlock(*decl.body);
    }

    void AstWalker::walkVar(const VarDecl& decl)
    {
        walk(decl.type);
        if (decl.initializer) walk(*decl.initializer);
    }

    void AstWalker::walkInterface(const InterfaceDecl& decl)
    {
        for (const auto& method : decl.methods)
            walkFunction(method.decl);
    }

    void AstWalker::walk(const StmtNode& stmt)
    {
        if (onStmt) onStmt(stmt);

        std::visit([this]<typename T0>(const T0& s)
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, DeclStmt>)  walk(s.decl);
            else if constexpr (std::is_same_v<T, ExprStmt>)  walk(s.expr);
            else if constexpr (std::is_same_v<T, BlockStmt>) walkBlock(s);
            else if constexpr (std::is_same_v<T, ForStmt>)   walkFor(s);
            else if constexpr (std::is_same_v<T, WhileStmt>) walkWhile(s);
            else if constexpr (std::is_same_v<T, IfStmt>)    walkIf(s);
            else if constexpr (std::is_same_v<T, ReturnStmt>) walkReturn(s);
        }, stmt.stmt);
    }

    void AstWalker::walkBlock(const BlockStmt& stmt)
    {
        for (const auto& s : stmt.statements) walk(s);
    }

    void AstWalker::walkIf(const IfStmt& stmt)
    {
        walk(stmt.condition);
        walkBlock(stmt.thenBranch);
        if (stmt.elseBranch) walkBlock(*stmt.elseBranch);
    }

    void AstWalker::walkWhile(const WhileStmt& stmt)
    {
        walk(stmt.condition); walkBlock(stmt.body);
    }

    void AstWalker::walkFor(const ForStmt& stmt)
    {
        walk(*stmt.init);
        walk(stmt.condition);
        walk(stmt.increment);
        walkBlock(stmt.body);
    }

    void AstWalker::walkReturn(const ReturnStmt& stmt)
    {
        if (stmt.expr) walk(*stmt.expr);
    }

    void AstWalker::walk(const ExprNode& expr)
    {
        if (onExpr) onExpr(expr);

        std::visit([this]<typename T0>(const T0& e)
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, BinaryExpr>)      walkBinary(e);
            else if constexpr (std::is_same_v<T, UnaryExpr>)  walkUnary(e);
            else if constexpr (std::is_same_v<T, FieldAccessExpr>) walkFieldAccess(e);
            else if constexpr (std::is_same_v<T, FunctionCallExpr>) walkFunctionCall(e);
            else if constexpr (std::is_same_v<T, IndexExpr>)  walkIndex(e);
            else if constexpr (std::is_same_v<T, TypeExpr>)   walk(e.type);
        }, expr.expr);
    }

    void AstWalker::walkBinary(const BinaryExpr& expr)
    {
        walk(*expr.left); walk(*expr.right);
    }

    void AstWalker::walkUnary(const UnaryExpr& expr)
    {
        walk(*expr.operand);
    }

    void AstWalker::walkFieldAccess(const FieldAccessExpr& expr)
    {
        walk(*expr.object);
    }

    void AstWalker::walkFunctionCall(const FunctionCallExpr& expr)
    {
        walk(*expr.callee);
        for (const auto& arg : expr.args) walk(arg);
    }

    void AstWalker::walkIndex(const IndexExpr& expr)
    {
        walk(*expr.array); walk(*expr.index);
    }

    void AstWalker::walk(const Type& type)
    {
        if (onType) onType(type);

        std::visit([this]<typename T0>(const T0& t)
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, ArrayType>)
            {
                walk(*t.elementType);
                if (t.size) walk(**t.size);
            }
            else if constexpr (std::is_same_v<T, ResourceBindingType>)
            {
                if (t.elementType) walk(*t.elementType);
            }
        }, type.kind);
    }
}
