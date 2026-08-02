#include "resolver.h"

#include <utility>

namespace ionsl
{
    Resolver::Resolver(ResolveDesc desc)
        : m_desc(std::move(desc))
    {
    }

    Module Resolver::resolve()
    {
        m_ast = m_desc.modules.at(0).ast();

        for(size_t i = 1; i < m_desc.modules.size(); i++)
        {
            for(const auto& decl : m_desc.modules.at(i).ast())
            {
                m_ast.push_back(decl);
            }
        }

        for(auto& decl : m_ast)
        {
            if(const auto struc = std::get_if<StructDecl>(&decl.decl))
                m_structs[struc->name] = StructType { struc };
            if(const auto interface = std::get_if<InterfaceDecl>(&decl.decl))
                m_interfaces[interface->name] = InterfaceType { interface };
        }

        for(auto& decl : m_ast)
        {
            resolveDecl(decl);
        }


        return {m_desc.modules.at(0).path(), std::move(m_ast), {}};
    }

    Module Resolver::resolve(ResolveDesc desc)
    {
        Resolver r{std::move(desc)};
        return r.resolve();
    }

    void Resolver::resolveDecl(DeclNode &decl)
    {
        std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                    resolveFunctionDecl(d);
                else if constexpr (std::is_same_v<T, StructDecl>)
                    resolveStructDecl(d);
                else if constexpr (std::is_same_v<T, VarDecl>)
                    resolveVarDecl(d);
                else if constexpr (std::is_same_v<T, InterfaceDecl>)
                    resolveInterfaceDecl(d);
            },
            decl.decl
        );
    }

    void Resolver::resolveFunctionDecl(FunctionDecl &decl)
    {

        for(const auto& spec : m_desc.specializations)
        {
            // TODO match whole signature
            if(spec.genericFunction->name == decl.name)
                m_currentTypeBindings = spec.bindings;
        }

        decl.returnType = resolveType(std::move(decl.returnType));

        for(auto& param : decl.params)
        {
            param.type = resolveType(std::move(param.type));
        }

        if(decl.body.has_value())
            resolveBlockStmt(decl.body.value());
    }

    void Resolver::resolveVarDecl(VarDecl &decl)
    {
        decl.type = resolveType(std::move(decl.type));

        if(decl.initializer)
            resolveExpr(*decl.initializer);
    }

    void Resolver::resolveStructDecl(StructDecl& decl)
    {
        for(auto& field : decl.fields)
        {
            field.type = resolveType(std::move(field.type));
        }

        for(auto& method : decl.methods)
        {
            resolveFunctionDecl(method.decl);
        }
    }

    void Resolver::resolveInterfaceDecl(InterfaceDecl &decl)
    {
        for(auto& method : decl.methods)
        {
            resolveFunctionDecl(method.decl);
        }
    }

    void Resolver::resolveExpr(ExprNode &expr)
    {
        std::visit(
            [this, &expr]<typename T0>(T0&& e)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, BinaryExpr>)
                    resolveBinaryExpr(e);
                else if constexpr (std::is_same_v<T, IdentifierExpr>)
                    expr.expr = resolveIdentifierExpr(std::move(e));
                else if constexpr (std::is_same_v<T, IndexExpr>)
                    resolveIndexExpr(e);
                else if constexpr (std::is_same_v<T, UnaryExpr>)
                    resolveUnaryExpr(e);
                else if constexpr (std::is_same_v<T, FieldAccessExpr>)
                    resolveFieldAccessExpr(e);
                else if constexpr (std::is_same_v<T, FunctionCallExpr>)
                    resolveFunctionCallExpr(e);
            },
            expr.expr
        );
    }

    void Resolver::resolveTypeExpr(TypeExpr &expr)
    {
        expr.type = resolveType(std::move(expr.type));
    }

    void Resolver::resolveBinaryExpr(BinaryExpr &expr)
    {
        resolveExpr(*expr.left);
        resolveExpr(*expr.right);
    }

    void Resolver::resolveUnaryExpr(UnaryExpr &expr)
    {
        resolveExpr(*expr.operand);
    }

    void Resolver::resolveIndexExpr(IndexExpr &expr)
    {
        resolveExpr(*expr.array);
        resolveExpr(*expr.index);
    }

    void Resolver::resolveFunctionCallExpr(FunctionCallExpr &expr)
    {
        resolveExpr(*expr.callee);

        for(auto& arg : expr.args)
            resolveExpr(arg);
    }

    void Resolver::resolveFieldAccessExpr(FieldAccessExpr &expr)
    {
        resolveExpr(*expr.object);
    }

    Expr Resolver::resolveIdentifierExpr(IdentifierExpr expr)
    {
        if(const auto kind = nameToPrimitiveKind(expr.name))
        {
            return TypeExpr { .type = Type {  .kind = PrimitiveType { *kind } } };
        }
        if(isVectorTypeName(expr.name) && !expr.genericArgs.empty())
        {
            const int dimensions = expr.name[3] - '0';

            VectorType type{};
            type.dimension = dimensions;
            type.scalarType = std::get<PrimitiveType>(expr.genericArgs[0].kind).kind;

            return TypeExpr { .type = Type {  .kind = type } };
        }
        if(isMatrixTypeName(expr.name) && !expr.genericArgs.empty())
        {
            const int rows = expr.name[3] - '0';
            const int columns = expr.name[5] - '0';

            MatrixType type{};
            type.rows = rows;
            type.columns = columns;
            type.scalarType = std::get<PrimitiveType>(expr.genericArgs[0].kind).kind;

            return TypeExpr { .type = Type {  .kind = type } };
        }
        if(m_structs.contains(expr.name))
        {
            return TypeExpr { .type = Type {  .kind = m_structs.at(expr.name) } };
        }
        if(m_currentTypeBindings.contains(expr.name))
        {
            return TypeExpr { .type = Type {  .kind = m_currentTypeBindings.at(expr.name) } };
        }
        if(m_interfaces.contains(expr.name))
        {
            return TypeExpr { .type = Type {  .kind = m_interfaces.at(expr.name) } };
        }
        if(kResourceTypeNames.contains(expr.name) && !expr.genericArgs.empty())
        {
            ResourceBindingType type{};
            type.kind = kResourceTypeNames.at(expr.name);
            type.elementType = Box<Type>::make(std::move(expr.genericArgs[0]));

            return TypeExpr { .type = Type {  .kind = std::move(type) } };
        }

        return std::move(expr);
    }

    void Resolver::resolveStmt(StmtNode &stmt)
    {
        std::visit(
            [this]<typename T0>(T0&& e)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, BlockStmt>)
                    resolveBlockStmt(e);
                if constexpr (std::is_same_v<T, IfStmt>)
                    resolveIfStmt(e);
                if constexpr (std::is_same_v<T, ForStmt>)
                    resolveForStmt(e);
                if constexpr (std::is_same_v<T, WhileStmt>)
                    resolveWhileStmt(e);
                if constexpr (std::is_same_v<T, ReturnStmt>)
                    resolveReturnStmt(e);
                if constexpr (std::is_same_v<T, DeclStmt>)
                    resolveDecl(e.decl);
                if constexpr (std::is_same_v<T, ExprStmt>)
                    resolveExpr(e.expr);
            },
            stmt.stmt
        );
    }

    void Resolver::resolveBlockStmt(BlockStmt &block)
    {
        for(auto& stmt : block.statements)
        {
            resolveStmt(stmt);
        }
    }

    void Resolver::resolveIfStmt(IfStmt &stmt)
    {
        resolveExpr(stmt.condition);
        resolveBlockStmt(stmt.thenBranch);
        if(stmt.elseBranch.has_value())
            resolveBlockStmt(stmt.elseBranch.value());
    }

    void Resolver::resolveForStmt(ForStmt &stmt)
    {
        resolveStmt(*stmt.init);
        resolveExpr(stmt.condition);
        resolveExpr(stmt.increment);
        resolveBlockStmt(stmt.body);
    }

    void Resolver::resolveWhileStmt(WhileStmt &stmt)
    {
        resolveExpr(stmt.condition);
        resolveBlockStmt(stmt.body);
    }

    void Resolver::resolveReturnStmt(ReturnStmt& stmt)
    {
        resolveExpr(*stmt.expr);
    }

    Type Resolver::resolveType(Type original)
    {
        return std::visit(
            [this, original]<typename T0>(T0&& type) -> Type
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, CustomType>)
                {
                    if(const auto it = m_structs.find(type.name.name()); it != m_structs.end())
                    {
                        return Type{
                            .trivia = original.trivia,
                            .span = original.span,
                            .kind = it->second,
                        };
                    }

                    if(const auto it = m_currentTypeBindings.find(type.name.name()); it != m_currentTypeBindings.end())
                    {
                        return Type{
                            .trivia = original.trivia,
                            .span = original.span,
                            .kind = it->second,
                        };
                    }

                    if(const auto it = m_interfaces.find(type.name.name()); it != m_interfaces.end())
                    {
                        return Type{
                            .trivia = original.trivia,
                            .span = original.span,
                            .kind = it->second,
                        };
                    }
                }
                else if constexpr (std::is_same_v<T, ArrayType>)
                {
                    if(type.size.has_value())
                        resolveExpr(**type.size);

                    type.elementType = Box<Type>::make(std::move(resolveType(*type.elementType)));

                    return Type{
                        .trivia = original.trivia,
                        .span = original.span,
                        .kind = type,
                    };
                }
                else if constexpr (std::is_same_v<T, StructType>)
                {
                    for(auto& field : const_cast<StructDecl*>(type.decl)->fields) // TODO probably shouldn't do this
                    {
                        field.type = std::move(resolveType(field.type));

                        if(field.initializer)
                            resolveExpr(*field.initializer);
                    }

                    return Type {
                        .trivia = original.trivia,
                        .span = original.span,
                        .kind = type,
                    };
                }
                else if constexpr (std::is_same_v<T, ResourceBindingType>)
                {
                    type.elementType = Box<Type>::make(std::move(resolveType(*type.elementType)));

                    return Type {
                        .trivia = original.trivia,
                        .span = original.span,
                        .kind = type,
                    };
                }

                return original;
            },
            original.kind
        );
    }
}
