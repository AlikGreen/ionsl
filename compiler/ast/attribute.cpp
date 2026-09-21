#include "attribute.h"

namespace ionsl
{
    bool Attributes::contains(const QualifiedName &name) const
    {
        return std::ranges::any_of(m_attributes, [&name](const Attribute& attr) { return attr.name == name; });
    }

    bool Attributes::contains(const std::string &name, const SymbolTable &symbols) const
    {
        return std::ranges::any_of(m_attributes, [&name, &symbols](const Attribute& attr) { return attr.name.string(symbols) == name; });
    }

    const Attribute * Attributes::find(const std::string &name, const SymbolTable &symbols) const
    {
        for(const auto& attr : m_attributes)
        {
            if(attr.name.string(symbols) == name)
                return &attr;
        }
        return nullptr;
    }
}
