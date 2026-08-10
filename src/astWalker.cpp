#include "astWalker.h"

namespace ionsl
{
    void AstWalker::walk(std::vector<DeclNode>& decls)
    {
        for (auto& decl : decls) walk(decl);
    }

    void AstWalker::walk(DeclNode& decl)
    {
        if (onDecl) onDecl(decl);

        std::visit([this]<typename T0>(T0& d)
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, StructDecl>)    walkStruct(d);
            else if constexpr (std::is_same_v<T, FunctionDecl>) walkFunction(d);
            else if constexpr (std::is_same_v<T, VarDecl>)   walkVar(d);
            else if constexpr (std::is_same_v<T, InterfaceDecl>) walkInterface(d);
        }, decl.decl);
    }

    void AstWalker::walkStruct(StructDecl& decl)
    {
        for (auto& field : decl.fields)
        {
            walk(field.type);
            if (field.initializer) walk(*field.initializer);
        }
        for (auto& method : decl.methods)
            walkFunction(method.decl);
    }

    void AstWalker::walkFunction(FunctionDecl& decl)
    {
        for (auto& param : decl.params)
        {
            walk(param.type);
            if (param.defaultValue) walk(*param.defaultValue);
        }
        walk(decl.returnType);
        if (decl.body) walkBlock(*decl.body);
    }

    void AstWalker::walkVar(VarDecl& decl)
    {
        walk(decl.type);
        if (decl.initializer) walk(*decl.initializer);
    }

    void AstWalker::walkInterface(InterfaceDecl& decl)
    {
        for (auto& method : decl.methods)
            walkFunction(method.decl);
    }

    void AstWalker::walk(StmtNode& stmt)
    {
        if (onStmt) onStmt(stmt);

        std::visit([this]<typename T0>(T0& s)
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

    void AstWalker::walkBlock(BlockStmt& stmt)
    {
        for (auto& s : stmt.statements) walk(s);
    }

    void AstWalker::walkIf(IfStmt& stmt)
    {
        walk(stmt.condition);
        walkBlock(stmt.thenBranch);
        if (stmt.elseBranch) walkBlock(*stmt.elseBranch);
    }

    void AstWalker::walkWhile(WhileStmt& stmt)
    {
        walk(stmt.condition); walkBlock(stmt.body);
    }

    void AstWalker::walkFor(ForStmt& stmt)
    {
        walk(*stmt.init);
        walk(stmt.condition);
        walk(stmt.increment);
        walkBlock(stmt.body);
    }

    void AstWalker::walkReturn(ReturnStmt& stmt)
    {
        if (stmt.expr) walk(*stmt.expr);
    }

    void AstWalker::walk(ExprNode& expr)
    {
        if (onExpr) onExpr(expr);

        std::visit([this]<typename T0>(T0& e)
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

    void AstWalker::walkBinary(BinaryExpr& expr)
    {
        walk(*expr.left); walk(*expr.right);
    }

    void AstWalker::walkUnary(UnaryExpr& expr)
    {
        walk(*expr.operand);
    }

    void AstWalker::walkFieldAccess(FieldAccessExpr& expr)
    {
        walk(*expr.object);
    }

    void AstWalker::walkFunctionCall(FunctionCallExpr& expr)
    {
        walk(*expr.callee);
        for (auto& arg : expr.args) walk(arg);
    }

    void AstWalker::walkIndex(IndexExpr& expr)
    {
        walk(*expr.array); walk(*expr.index);
    }

    void AstWalker::walk(Type& type)
    {
        if (onType) onType(type);

        std::visit([this]<typename T0>(T0& t)
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
