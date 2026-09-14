#include "typeResolver.h"

#include "semaContext.h"
#include "../ast/declarations.h"
#include "../ast/typeSyntax.h"

namespace ionsl
{
    TypeResolver::TypeResolver(TypeSystem &typeSystem, ConstantEvaluator &evaluator, SymbolTable &symbols,
        ScopeTable &scopeTable, GlobalScope& globalScope, DeclTable &decls)
            : m_typeSystem(typeSystem), m_evaluator(evaluator), m_symbols(symbols), m_scopeTable(scopeTable), m_globalScope(globalScope), m_decls(decls)
    { }

    TypeId TypeResolver::resolveType(TypeSyntax& syntax, const SemaContext& ctx)
    {
        if(auto* namedSyntax = syntax.as<NamedTypeSyntax>())
        {
            if(namedSyntax->name.string(m_symbols) == "vector")
                return resolveVectorType(*namedSyntax, ctx);
            if(namedSyntax->name.string(m_symbols) == "matrix")
                return resolveMatrixType(*namedSyntax, ctx);

            const PrimitiveKind primitiveKind = toPrimitiveKind(namedSyntax->name.string(m_symbols));
            if(primitiveKind != PrimitiveKind::Unknown)
                return syntax.resolvedType = m_typeSystem.types().getPrimitiveType(primitiveKind);


            return syntax.resolvedType = resolveNamedType(*namedSyntax, ctx);
        }

        if(auto* arraySyntax = syntax.as<ArrayTypeSyntax>())
            return syntax.resolvedType = resolveArrayType(*arraySyntax, ctx);

        return TypeId::Error; // TODO diagnostics
    }

    TypeId TypeResolver::resolveTypeArg(TypeArgument &arg, const SemaContext &ctx)
    {
        if(auto* typeArg = arg.as<TypeArgumentType>())
            return typeArg->resolvedType = resolveType(*typeArg->type, ctx);

        return TypeId::Error;
    }

    TypeId TypeResolver::resolveVectorType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        if(syntax.arguments.size() != 2)
        {
            // TODO diagnostics
            return TypeId::Error;
        }


        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        resolveType(*elementTypeSyntax, ctx);

        const auto res = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!res) return TypeId::Error; // TODO diagnostics

        const uint32_t dimension = std::get<uint64_t>(res->value);

        return syntax.resolvedType = m_typeSystem.types().getVectorType(elementTypeSyntax->resolvedType, dimension);
    }

    TypeId TypeResolver::resolveMatrixType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        if(syntax.arguments.size() != 3)
        {
            // TODO diagnostics
            return TypeId::Error;
        }

        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        resolveType(*elementTypeSyntax, ctx);

        const auto rowsRes = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!rowsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t rows = std::get<uint64_t>(rowsRes->value);

        const auto columnsRes = m_evaluator.evaluate(*syntax.arguments.at(2)->as<TypeArgumentValue>()->expression);
        if(!columnsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t columns = std::get<uint64_t>(columnsRes->value);

        return syntax.resolvedType = m_typeSystem.types().getMatrixType(elementTypeSyntax->resolvedType, rows, columns);
    }

    TypeId TypeResolver::resolveNamedType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        for(const auto* param : ctx.visibleGenericParams)
        {
            if(syntax.name.parts.size() != 1 || param->name != syntax.name.parts.back()) continue;

            if(ctx.substitutions)
                if(const auto it = ctx.substitutions->find(param->id); it != ctx.substitutions->end())
                    return it->second;

            return TypeId::Error;
        }

        // FIXME make sure to match the whole type
        auto decls = m_scopeTable.find(ctx.scope, syntax.name.parts[0]);
        if(decls.empty())
            decls = m_globalScope.find(syntax.name.parts[0]);

        for(const DeclId id : decls)
        {
            // TODO make sure type matches full signature when introducing generics
            Declaration* decl = m_decls.get(id);
            if(decl->is<StructDecl>())
            {
                return syntax.resolvedType = m_typeSystem.types().getStructType(decl->id);
            }
            if(decl->is<InterfaceDecl>())
            {
                // TODO this probably needs to resolve it to a concrete type
                return syntax.resolvedType = m_typeSystem.types().getInterfaceType(decl->id);
            }
            if(decl->is<TypeGenericParam>())
            {
                return syntax.resolvedType = m_typeSystem.types().getGenericType(decl->id);
            }
            if(const AliasDecl* alias = decl->as<AliasDecl>())
            {
                return resolveAliasType(syntax, *alias, ctx);
            }
        }

        return TypeId::Error; // TODO diagnostics
    }

    TypeId TypeResolver::resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, const SemaContext& ctx)
    {

        if(alias.genericParams.size() != syntax.arguments.size())
            return TypeId::Error; // TODO diagnostics

        std::unordered_map<DeclId, TypeId> typeSubstitutions{};


        for(size_t i = 0; i < alias.genericParams.size(); ++i)
        {
            GenericParam* param = alias.genericParams[i];
            TypeArgument* arg = syntax.arguments[i];

            if(const auto* typeParam = param->as<TypeGenericParam>())
            {
                const auto* typeArg = arg->as<TypeArgumentType>();

                if(!typeArg)
                {
                    // TODO diagnostics
                    continue;
                }

                resolveType(*typeArg->type, ctx);
                typeSubstitutions[typeParam->id] = typeArg->type->resolvedType;
            }
        }

        resolveType(*alias.targetType, ctx.forGenericDecl(alias.genericParams, typeSubstitutions));
        return syntax.resolvedType = alias.targetType->resolvedType;
    }

    TypeId TypeResolver::resolveArrayType(ArrayTypeSyntax &syntax, const SemaContext& ctx)
    {
        resolveType(*syntax.elementType, ctx);

        std::optional<uint32_t> size = std::nullopt;

        if(syntax.size)
        {
            const auto sizeRes = m_evaluator.evaluate(*syntax.size);
            if(!sizeRes) return TypeId::Error; // TODO diagnostics
            size = std::get<uint64_t>(sizeRes->value);
        }

        return syntax.resolvedType = m_typeSystem.types().getArrayType(syntax.elementType->resolvedType, size);
    }

    PrimitiveKind TypeResolver::toPrimitiveKind(const std::string &name)
    {
        const std::unordered_map<std::string, PrimitiveKind> primitiveKinds = {
            {"void", PrimitiveKind::Void},
            {"bool", PrimitiveKind::Bool},

            {"i8", PrimitiveKind::Int8},
            {"i16", PrimitiveKind::Int16},
            {"i32", PrimitiveKind::Int32},
            {"i64", PrimitiveKind::Int64},

            {"u8", PrimitiveKind::UInt8},
            {"u16", PrimitiveKind::UInt16},
            {"u32", PrimitiveKind::UInt32},
            {"u64", PrimitiveKind::UInt64},

            {"f16", PrimitiveKind::Float16},
            {"f32", PrimitiveKind::Float32},
            {"f64", PrimitiveKind::Float64},

            {"string", PrimitiveKind::String}
        };

        if(const auto it = primitiveKinds.find(name); it != primitiveKinds.end())
            return it->second;

        return PrimitiveKind::Unknown;
    }
}
