#include "attribute.h"

namespace ionsl
{
    AttributeArg * AttributeArg::clone(Arena &arena) const
    {
        auto* newArg = arena.create<AttributeArg>();
        newArg->name = name;
        newArg->value = value;
        newArg->span = span;

        if (expression)
            newArg->expression = expression->clone(arena);

        return newArg;
    }

    Attribute* Attribute::clone(Arena &arena) const
    {
        auto* attrib = arena.create<Attribute>();
        attrib->name = name;
        attrib->decl = decl;
        attrib->span = span;

        for (const auto* arg : args)
            attrib->args.push_back(arg->clone(arena));

        return attrib;
    }

    const AttributeArg* Attribute::getArg(const std::string &argName, const SymbolTable& symbols) const
    {
        for (const auto arg : args)
        {
            if (symbols.get(arg->name) == argName)
                return arg;
        }

        return nullptr;
    }

    bool Attributes::contains(const QualifiedName &name) const
    {
        return std::ranges::any_of(attributes, [&name](const Attribute* attr) { return attr->name == name; });
    }

    bool Attributes::contains(const std::string &name, const SymbolTable &symbols) const
    {
        return std::ranges::any_of(attributes, [&name, &symbols](const Attribute* attr) { return attr->name.string(symbols) == name; });
    }

    const Attribute * Attributes::find(const std::string &name, const SymbolTable &symbols) const
    {
        for(const auto& attr : attributes)
        {
            if(attr->name.string(symbols) == name)
                return attr;
        }
        return nullptr;
    }

    Attributes * Attributes::clone(Arena &arena) const
    {
        auto* attribs = arena.create<Attributes>();

        for (const auto* attrib : attributes)
            attribs->attributes.push_back(attrib->clone(arena));

        return attribs;
    }
}
