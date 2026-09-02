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
        decl->initializer = initializer->clone(arena);
        decl->name = name;

        decl->type = type->clone(arena);
        decl->resolvedType = resolvedType;

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
        decl->resolvedReturnType = resolvedReturnType;

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

    ErrorDecl* ErrorDecl::clone(Arena &arena) const
    {
        auto* decl = arena.create<ErrorDecl>();
        decl->span = span;
        return decl;
    }
}
