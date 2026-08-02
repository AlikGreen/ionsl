#include "checker.h"

#include <format>

namespace ionsl
{
    Checker::Checker(const Module &module)
        : m_declNodes(module.ast())
    {

    }

    std::vector<Diagnostic> Checker::check()
    {
        m_scopes.push_back(Scope{ TopLevel{}, {} });
        for(const auto& decl : m_declNodes)
        {
            std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                {
                    m_functions.emplace(&d);
                    m_scopes.back().identifiers.emplace(d.name);
                }
                else if constexpr (std::is_same_v<T, StructDecl>)
                    m_structs.emplace(&d);
            },
            decl.decl
            );
        }

        for(const auto& decl : m_declNodes)
        {
            checkDecl(decl);
        }

        return m_diagnostics;
    }

    std::vector<Diagnostic> Checker::check(const Module &module)
    {
        Checker checker{module};
        return checker.check();
    }

    void Checker::checkDecl(const DeclNode &decl)
    {
        std::visit(
            [this, decl]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                    checkFunctionDecl(d);
                else if constexpr (std::is_same_v<T, StructDecl>)
                    checkStructDecl(d);
                else if constexpr (std::is_same_v<T, VarDecl>)
                    checkVarDecl(d, decl.span);
            },
        decl.decl
        );
    }

    void Checker::checkFunctionDecl(const FunctionDecl &decl)
    {
        for(const auto& param : decl.params)
        {
            checkType(param.type);

            if(param.defaultValue)
                checkExpr(*param.defaultValue);

            m_scopes.back().identifiers.emplace(param.name);
        }

        if(decl.body)
            checkBlockStmt(*decl.body, &decl);
    }

    void Checker::checkVarDecl(const VarDecl &decl, SourceSpan span)
    {
        if(isDeclared(decl.name))
            m_diagnostics.emplace_back(std::format("redefinition: identifier '{}' has already been declared", decl.name), span, Severity::Error);

        checkType(decl.type);

        if(decl.initializer)
            checkExpr(*decl.initializer);

        m_scopes.back().identifiers.emplace(decl.name);
    }

    void Checker::checkStructDecl(const StructDecl &decl)
    {
        for(const auto& field : decl.fields)
        {
            checkType(field.type);

            if(field.initializer)
                checkExpr(*field.initializer);
        }
    }

    void Checker::checkExpr(const ExprNode &expr)
    {
        std::visit(
            [this, expr]<typename T0>(T0&& e)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, BinaryExpr>)
                    checkBinaryExpr(e);
                else if constexpr (std::is_same_v<T, IdentifierExpr>)
                    checkIdentifierExpr(e, expr.span);
                else if constexpr (std::is_same_v<T, IndexExpr>)
                    checkIndexExpr(e);
                else if constexpr (std::is_same_v<T, UnaryExpr>)
                    checkUnaryExpr(e);
                else if constexpr (std::is_same_v<T, FieldAccessExpr>)
                    checkFieldAccessExpr(e);
                else if constexpr (std::is_same_v<T, FunctionCallExpr>)
                    checkFunctionCallExpr(e);
                else if constexpr (std::is_same_v<T, TypeExpr>)
                    checkTypeExpr(e);
            },
            expr.expr
        );
    }

    void Checker::checkBinaryExpr(const BinaryExpr &expr)
    {
        checkExpr(*expr.left);
        checkExpr(*expr.right);
    }

    void Checker::checkIdentifierExpr(const IdentifierExpr &expr, SourceSpan span)
    {
        if(!isDeclared(expr.name))
            m_diagnostics.emplace_back(std::format("undeclared identifier '{}'", expr.name), span, Severity::Error);

        for(const auto& type : expr.genericArgs)
            checkType(type);
    }

    void Checker::checkIndexExpr(const IndexExpr &expr)
    {
        checkExpr(*expr.array);
        checkExpr(*expr.index);
    }

    void Checker::checkUnaryExpr(const UnaryExpr &expr)
    {
        checkExpr(*expr.operand);
    }

    void Checker::checkFieldAccessExpr(const FieldAccessExpr &expr)
    {
        checkExpr(*expr.object);
        // TODO check fields of object
    }

    void Checker::checkFunctionCallExpr(const FunctionCallExpr &expr)
    {
        checkExpr(*expr.callee);

        for(const auto& arg : expr.args)
            checkExpr(arg);
    }

    void Checker::checkTypeExpr(const TypeExpr &expr)
    {
        checkType(expr.type);
    }

    void Checker::checkStmt(const StmtNode &stmt)
    {
        std::visit(
            [this, stmt]<typename T0>(T0&& s)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, DeclStmt>)
                    checkDecl(s.decl);
                else if constexpr (std::is_same_v<T, ExprStmt>)
                    checkExprStmt(s);
                else if constexpr (std::is_same_v<T, BlockStmt>)
                    checkBlockStmt(s);
                else if constexpr (std::is_same_v<T, ForStmt>)
                    checkForStmt(s);
                else if constexpr (std::is_same_v<T, WhileStmt>)
                    checkWhileStmt(s);
                else if constexpr (std::is_same_v<T, IfStmt>)
                    checkIfStmt(s);
                else if constexpr (std::is_same_v<T, ReturnStmt>)
                    checkReturnStmt(s);
                else if constexpr (std::is_same_v<T, BreakStmt>)
                    checkBreakStmt(s, stmt.span);
                else if constexpr (std::is_same_v<T, ContinueStmt>)
                    checkContinueStmt(s, stmt.span);
            },
            stmt.stmt
        );
    }

    void Checker::checkBlockStmt(const BlockStmt &block, ScopeDeclType type)
    {
        if(std::holds_alternative<const BlockStmt*>(type))
            type = &block;

        m_scopes.emplace_back(type);

        for(const auto& stmt : block.statements)
            checkStmt(stmt);

        m_scopes.pop_back();
    }

    void Checker::checkForStmt(const ForStmt &stmt)
    {
        checkExpr(stmt.condition);
        checkExpr(stmt.increment);
        checkStmt(*stmt.init);

        checkBlockStmt(stmt.body, &stmt);
    }

    void Checker::checkWhileStmt(const WhileStmt &stmt)
    {
        checkExpr(stmt.condition);
        checkBlockStmt(stmt.body, &stmt);
    }

    void Checker::checkIfStmt(const IfStmt &stmt)
    {
        checkExpr(stmt.condition);
        checkBlockStmt(stmt.thenBranch, &stmt);
        if(stmt.elseBranch.has_value())
            checkBlockStmt(*stmt.elseBranch, &stmt);
    }

    void Checker::checkReturnStmt(const ReturnStmt &stmt)
    {
        const auto scope = getFirstScopeOf<const FunctionDecl*>();
        auto funcDecl = std::get<const FunctionDecl*>(scope->scopeDecl);

        // TODO add some function like evaluateType(expr) and use that to check return type against return value

        if(stmt.expr)
            checkExpr(*stmt.expr);
    }

    void Checker::checkBreakStmt(const BreakStmt &stmt, SourceSpan span)
    {
        if(!getFirstScopeOf<const ForStmt*>() && !getFirstScopeOf<const WhileStmt*>())
            m_diagnostics.emplace_back("'break' statement may only be used within a loop", span, Severity::Error);
    }

    void Checker::checkContinueStmt(const ContinueStmt &stmt, SourceSpan span)
    {
        if(!getFirstScopeOf<const ForStmt*>() && !getFirstScopeOf<const WhileStmt*>())
            m_diagnostics.emplace_back("'continue' statement may only be used within a loop", span, Severity::Error);
    }

    void Checker::checkExprStmt(const ExprStmt &stmt)
    {
        checkExpr(stmt.expr);
    }

    void Checker::checkType(const Type &t)
    {
        std::visit(
            [this, t]<typename T0>(T0&& type)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, CustomType>)
                    m_diagnostics.emplace_back(std::format("undeclared identifier '{}'", type.name.name()), t.span, Severity::Error);
                else if constexpr (std::is_same_v<T, ArrayType>)
                {
                    if(type.size.has_value())
                        checkExpr(**type.size);

                    checkType(*type.elementType);
                }
                else if constexpr (std::is_same_v<T, StructType>)
                {
                    for(const auto& field : type.decl->fields)
                    {
                        checkType(field.type);

                        if(field.initializer)
                            checkExpr(*field.initializer);
                    }
                }
                else if constexpr (std::is_same_v<T, ResourceBindingType>)
                    checkType(*type.elementType);
            },
            t.kind
        );
    }

    bool Checker::isDeclared(const std::string &name) const
    {
        for(const auto& scope : m_scopes)
        {
            if(auto it = scope.identifiers.find(name); it != scope.identifiers.end())
                return true;
        }

        return false;
    }

}
