#include "typeSyntax.h"

#include "expressions.h"

namespace ionsl
{
    TypeArgumentType* TypeArgumentType::clone(Arena &arena) const
    {
        auto* newType = arena.create<TypeArgumentType>();
        newType->span = span;
        newType->type = type->clone(arena);
        return newType;
    }

    TypeArgumentValue* TypeArgumentValue::clone(Arena &arena) const
    {
        auto* newType = arena.create<TypeArgumentValue>();
        newType->span = span;
        newType->expression = expression->clone(arena);
        return newType;
    }

    NamedTypeSyntax* NamedTypeSyntax::clone(Arena &arena) const
    {
        auto* newType = arena.create<NamedTypeSyntax>();
        newType->span = span;
        newType->name = name;

        for(const auto* arg : arguments)
            newType->arguments.push_back(arg->clone(arena));

        return newType;
    }

    ArrayTypeSyntax* ArrayTypeSyntax::clone(Arena &arena) const
    {
        auto* newType = arena.create<ArrayTypeSyntax>();
        newType->span = span;
        newType->size = size->clone(arena);
        newType->elementType = elementType->clone(arena);
        return newType;
    }
}
