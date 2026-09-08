#include "typeResolver.h"

#include "../ast/declarations.h"
#include "../ast/typeSyntax.h"

namespace ionsl
{
    TypeResolver::TypeResolver(TypeSystem &typeSystem, ConstantEvaluator &evaluator, SymbolTable &symbols,
        ScopeTable &scopeTable, DeclTable &decls)
            : m_typeSystem(typeSystem), m_evaluator(evaluator), m_symbols(symbols), m_scopeTable(scopeTable), m_decls(decls)
    { }

    TypeId TypeResolver::resolveType(TypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {
        if(auto* namedSyntax = syntax.as<NamedTypeSyntax>())
        {
            if(namedSyntax->name.string(m_symbols) == "vector")
                return resolveVectorType(*namedSyntax, scope, typeSubstitutions);
            if(namedSyntax->name.string(m_symbols) == "matrix")
                return resolveMatrixType(*namedSyntax, scope, typeSubstitutions);

            PrimitiveKind primitiveKind = toPrimitiveKind(namedSyntax->name.string(m_symbols));
            if(primitiveKind != PrimitiveKind::Unknown)
                return m_typeSystem.types().getPrimitiveType(primitiveKind);


            return resolveNamedType(*namedSyntax, scope, typeSubstitutions);
        }

        if(auto* arraySyntax = syntax.as<ArrayTypeSyntax>())
            return resolveArrayType(*arraySyntax, scope, typeSubstitutions);

        return TypeId::Error; // TODO diagnostics
    }

    TypeId TypeResolver::resolveVectorType(NamedTypeSyntax &syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {
        if(syntax.arguments.size() != 2)
        {
            // TODO diagnostics
            return TypeId::Error;
        }


        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        resolveType(*elementTypeSyntax, scope, typeSubstitutions);

        const auto res = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!res) return TypeId::Error; // TODO diagnostics

        const uint32_t dimension = std::get<uint64_t>(res->value);

        return syntax.resolvedType = m_typeSystem.types().getVectorType(elementTypeSyntax->resolvedType, dimension);
    }

    TypeId TypeResolver::resolveMatrixType(NamedTypeSyntax &syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {
        if(syntax.arguments.size() != 3)
        {
            // TODO diagnostics
            return TypeId::Error;
        }

        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        resolveType(*elementTypeSyntax, scope, typeSubstitutions);

        const auto rowsRes = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!rowsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t rows = std::get<uint64_t>(rowsRes->value);

        const auto columnsRes = m_evaluator.evaluate(*syntax.arguments.at(2)->as<TypeArgumentValue>()->expression);
        if(!columnsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t columns = std::get<uint64_t>(columnsRes->value);

        return syntax.resolvedType = m_typeSystem.types().getMatrixType(elementTypeSyntax->resolvedType, rows, columns);
    }

    TypeId TypeResolver::resolveNamedType(NamedTypeSyntax &syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {
        const auto decls = m_scopeTable.findDecls(scope, syntax.name);

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
            if(const AliasDecl* alias = decl->as<AliasDecl>())
            {
                return resolveAliasType(syntax, *alias, typeSubstitutions);
            }
            if(const TypeGenericParam* param = decl->as<TypeGenericParam>())
            {
                if(auto it = typeSubstitutions.find(id); it != typeSubstitutions.end())
                {
                    return syntax.resolvedType = it->second;
                }
                return TypeId::Error;
            }
        }

        return TypeId::Error; // TODO diagnostics
    }

    TypeId TypeResolver::resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {

        if(alias.genericParams.size() != syntax.arguments.size())
            return TypeId::Error; // TODO diagnostics


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

                resolveType(*typeArg->type, alias.scope, typeSubstitutions);
                typeSubstitutions[typeParam->id] = typeArg->type->resolvedType;
            }
        }

        resolveType(*alias.targetType, alias.scope, typeSubstitutions);
        return syntax.resolvedType = alias.targetType->resolvedType;
    }

    TypeId TypeResolver::resolveArrayType(ArrayTypeSyntax &syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions)
    {
        resolveType(*syntax.elementType, scope, typeSubstitutions);

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
