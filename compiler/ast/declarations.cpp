#include "declarations.h"

#include <random>

#include "expressions.h"
#include "statements.h"
#include "typeSyntax.h"

namespace ionsl
{

    ValueDecl* ValueDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<ValueDecl>();
        decl->span = span;
        decl->id = id;
        decl->attributes = attributes;

        if(initializer)
            decl->initializer = initializer->clone(arena);

        decl->name = name;

        decl->type = type->clone(arena);

        return decl;
    }

    FunctionDecl* FunctionDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<FunctionDecl>();
        decl->span = span;
        decl->id = id;
        decl->attributes = attributes;
        decl->name = name;

        decl->returnType = returnType->clone(arena);

        for(const auto* param : genericParams)
            decl->genericParams.push_back(param->clone(arena));

        for(const auto* param : params)
            decl->params.push_back(param->clone(arena));

        if(body)
            decl->body = body->clone(arena);

        return decl;
    }

    InterfaceDecl* InterfaceDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<InterfaceDecl>();
        decl->span = span;
        decl->id = id;
        decl->attributes = attributes;
        decl->name = name;

        for(const auto* method : methods)
            decl->methods.push_back(method->clone(arena));

        return decl;
    }

    ValueDecl* StructDecl::findField(SymbolId name) const
    {
        for(const auto& field : fields)
        {
            if(field->name == name)
                return field;
        }

        return nullptr;
    }

    StructDecl* StructDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<StructDecl>();
        decl->span = span;
        decl->id = id;
        decl->attributes = attributes;
        decl->name = name;

        for(const auto* method : methods)
            decl->methods.push_back(method->clone(arena));

        for(const auto* field : fields)
            decl->fields.push_back(field->clone(arena));

        for(const auto* interface : interfaces)
            decl->interfaces.push_back(interface->clone(arena));

        decl->resolvedInterfaces = resolvedInterfaces;

        return decl;
    }

    AliasDecl* AliasDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<AliasDecl>();
        decl->span = span;
        decl->name = name;
        decl->id = id;
        decl->attributes = attributes;
        decl->targetType = targetType->clone(arena);

        for(const auto param : genericParams)
            decl->genericParams.push_back(param->clone(arena));


        return decl;
    }

    EnumDecl* EnumDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<EnumDecl>();
        decl->span = span;
        decl->name = name;
        decl->id = id;
        decl->attributes = attributes;

        decl->underlyingType = underlyingType->clone(arena);
        decl->members = members;
        for (auto& member : decl->members)
        {
            member.initializer = member.initializer->clone(arena);
        }
        return decl;
    }

    AttributeDecl* AttributeDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<AttributeDecl>();
        decl->span = span;
        decl->id = id;
        decl->attributes = attributes;
        decl->name = name;

        for(const auto* field : fields)
            decl->fields.push_back(field->clone(arena));

        return decl;
    }

    ErrorDecl* ErrorDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<ErrorDecl>();
        decl->id = id;
        decl->name = name;
        decl->span = span;
        decl->attributes = attributes;
        return decl;
    }

    TypeGenericParam* TypeGenericParam::clone(Arena &arena) const
    {
        auto* decl = arena.create<TypeGenericParam>();
        decl->span = span;
        decl->name = name;
        decl->attributes = attributes;
        return decl;
    }

    ValueGenericParam* ValueGenericParam::clone(Arena &arena) const
    {
        auto* decl = arena.create<ValueGenericParam>();
        decl->span = span;
        decl->name = name;
        decl->type = type->clone(arena);
        decl->resolvedType = resolvedType;
        decl->attributes = attributes;
        return decl;
    }
}
