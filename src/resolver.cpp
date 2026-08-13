#include "resolver.h"

#include <utility>

#include "astWalker.h"

namespace ionsl
{
    Resolver::Resolver(ResolveDesc desc)
        : m_desc(std::move(desc))
    {
    }

    Module Resolver::resolve()
    {
        m_ast.clear();

        for (const auto & module : m_desc.modules)
        {
            for (auto& decl : module.decls)
                m_ast.push_back(decl);
        }

        for (auto& decl : m_ast)
        {
            resolveDeclSignature(decl);
        }

        m_declTable = DeclTable(m_ast);

        for(auto& decl : m_ast)
        {
            resolveDecl(decl);
        }

        return {m_desc.modules.at(0).path, std::move(m_ast), {}};
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
                else if constexpr (std::is_same_v<T, TypeDefDecl>)
                    resolveTypeDefDecl(d);
            },
            decl.decl
        );
    }

    void Resolver::resolveDeclSignature(DeclNode &decl)
    {
        // maybe need to resolve more signatures
        // might need to add ones later like operator
        // and templated structs/interfaces
        std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                    resolveFunctionDeclSignature(d);
            },
            decl.decl
        );
    }

    void Resolver::resolveFunctionDecl(FunctionDecl &decl)
    {
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

    void Resolver::resolveTypeDefDecl(TypeDefDecl &decl)
    {
        resolveType(decl.type);
    }

    void Resolver::resolveFunctionDeclSignature(FunctionDecl &decl)
    {
        m_currentTypeBindings.clear();
        for(const auto& spec : m_desc.specializations)
        {
            // TODO match whole signature (maybe not actually needed for specialization)
            if(spec.genericFunction->name == decl.name)
                m_currentTypeBindings = spec.bindings;
        }

        decl.returnType = resolveType(std::move(decl.returnType));

        for(auto& param : decl.params)
        {
            param.type = resolveType(std::move(param.type));
        }
    }

    Type Resolver::resolveExpr(ExprNode &expr)
    {
        return std::visit(
            [this, &expr]<typename T0>(T0&& e) -> Type
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, BinaryExpr>)
                    return resolveBinaryExpr(e);
                else if constexpr (std::is_same_v<T, IdentifierExpr>)
                    return resolveIdentifierExpr(e, expr);
                else if constexpr (std::is_same_v<T, IndexExpr>)
                    return resolveIndexExpr(e);
                else if constexpr (std::is_same_v<T, UnaryExpr>)
                    return resolveUnaryExpr(e);
                else if constexpr (std::is_same_v<T, FieldAccessExpr>)
                    return resolveFieldAccessExpr(e);
                else if constexpr (std::is_same_v<T, FunctionCallExpr>)
                    return resolveFunctionCallExpr(e);

                return Type{ .kind = PrimitiveType { .kind = PrimitiveKind::Unknown }};
            },
            expr.expr
        );
    }

    Type Resolver::resolveTypeExpr(TypeExpr &expr)
    {
        expr.type = resolveType(std::move(expr.type));
        return expr.type;
    }

    Type Resolver::resolveBinaryExpr(BinaryExpr &expr)
    {
        auto leftType = resolveExpr(*expr.left);
        auto rightType = resolveExpr(*expr.right);

        auto ltrCost = m_typeSystem.getConversionCost(leftType, rightType);
        auto rtlCost = m_typeSystem.getConversionCost(rightType, leftType);

        if (!ltrCost.has_value() && !rtlCost.has_value())
            return Type::invalid();

        uint32_t rCost = ltrCost.has_value() ? ltrCost.value() : ~0u;
        uint32_t lCost = rtlCost.has_value() ? ltrCost.value() : ~0u;

        if (rCost < lCost)
            return rightType;

        return leftType;
    }

    Type Resolver::resolveUnaryExpr(UnaryExpr &expr)
    {
        return resolveExpr(*expr.operand);
    }

    Type Resolver::resolveIndexExpr(IndexExpr &expr)
    {
        resolveExpr(*expr.index);
        return resolveExpr(*expr.array);
    }

    Type Resolver::resolveFunctionCallExpr(FunctionCallExpr &expr)
    {
        for(auto& arg : expr.args)
            resolveExpr(arg);

        return resolveExpr(*expr.callee);
    }

    Type Resolver::resolveFieldAccessExpr(FieldAccessExpr &expr)
    {
        resolveExpr(*expr.object);
        // TODO get type from member
        return Type { .kind = PrimitiveType { .kind = PrimitiveKind::Unknown }};
    }

    Type Resolver::resolveIdentifierExpr(IdentifierExpr& expr, ExprNode& node)
    {
        if(const auto kind = nameToPrimitiveKind(expr.name))
        {
            auto type = Type {  .kind = PrimitiveType { *kind } };
            node.expr = TypeExpr { .type = type };
            return type;
        }
        if(expr.name == "vector" && expr.genericArgs.size() == 2)
        {
            VectorType vecType{};
            vecType.scalarType = Box<ExprNode>::make(std::move(expr.genericArgs[0]));
            vecType.dimension = Box<ExprNode>::make(std::move(expr.genericArgs[1]));

            auto type = Type { .kind = vecType };
            node.expr = TypeExpr { .type = type };
            return type;
        }
        if(expr.name == "matrix" && expr.genericArgs.size() == 3)
        {
            MatrixType matType{};
            matType.scalarType = Box<ExprNode>::make(std::move(expr.genericArgs[0]));
            matType.rows = Box<ExprNode>::make(std::move(expr.genericArgs[1]));
            matType.columns = Box<ExprNode>::make(std::move(expr.genericArgs[2]));

            auto type = Type {  .kind = matType };
            node.expr = TypeExpr { .type = type };
            return type;
        }
        if(m_resolvedTypes.contains(expr.name))
        {
            auto type = m_resolvedTypes.at(expr.name);
            node.expr = TypeExpr { .type = type };
            return type;
        }
        if(m_currentTypeBindings.contains(expr.name))
        {
            auto type = Type {  .kind = m_currentTypeBindings.at(expr.name) };
            node.expr = TypeExpr { .type = type };
            return type;
        }

        return Type { .kind = PrimitiveType{ PrimitiveKind::Unknown } };
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
                    if (const auto it = m_resolvedTypes.find(type.name.name()); it != m_resolvedTypes.end())
                        return it->second;

                    if(const auto it = m_currentTypeBindings.find(type.name.name()); it != m_currentTypeBindings.end())
                    {
                        return Type{
                            .trivia = original.trivia,
                            .span = original.span,
                            .kind = it->second,
                        };
                    }

                    for (const auto& d : m_ast)
                    {
                        auto newType = std::visit(
                        [&type, this, &original, d]<typename T1>(T1&& decl) -> std::optional<Type>
                        {
                            using DeclType = std::decay_t<T1>;
                            if (decl.name != type.name.name())
                                return std::nullopt;

                            if constexpr (std::is_same_v<DeclType, StructDecl>)
                            {
                                StructType structType{};
                                structType.declId = d.id;

                                auto t = Type{
                                    .trivia = original.trivia,
                                    .span = original.span,
                                    .kind = structType,
                                };
                                m_resolvedTypes[decl.name] = t;
                                return t;
                            }
                            if constexpr (std::is_same_v<DeclType, InterfaceDecl>)
                            {
                                InterfaceType interfaceDecl{};
                                interfaceDecl.declId = d.id;

                                auto t = Type{
                                    .trivia = original.trivia,
                                    .span = original.span,
                                    .kind = interfaceDecl,
                                };
                                m_resolvedTypes[decl.name] = t;
                                return t;
                            }
                            if constexpr (std::is_same_v<DeclType, TypeDefDecl>)
                            {
                                auto t = resolveType(decl.type);
                                m_resolvedTypes[decl.name] = t;
                                return t;
                            }

                            return std::nullopt;
                        }, d.decl);

                        if (newType.has_value())
                            return newType.value();
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
                    for(auto& field : std::get<StructDecl>(m_declTable.get(type.declId)->decl).fields)
                    {
                        field.type = std::move(resolveType(field.type));

                        if(field.initializer)
                        {
                            auto exprType = resolveExpr(*field.initializer);
                        }
                    }

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
