#pragma once
#include <utility>
#include <variant>

#include "literalValue.h"
#include "qualifiedName.h"

namespace ionsl
{
using AttribArgValue = std::variant<QualifiedName, LiteralValue>;

struct AttributeArg
{
    AttributeArg(const SymbolId name, AttribArgValue value)
        : name(name), value(std::move(value)) { }

    explicit AttributeArg(AttribArgValue value)
        : name(SymbolId::Invalid), value(std::move(value)) { }

    SymbolId name{};
    AttribArgValue value;
};

struct Attribute
{
    QualifiedName name;
    std::vector<AttributeArg> args;
};

class Attributes
{
public:
    Attributes() = default;
    Attributes(const std::vector<Attribute> &attributes) : m_attributes(attributes) { }

    const std::vector<Attribute>& attributes() { return m_attributes; }

    [[nodiscard]] bool contains(const QualifiedName &name) const
    {
        for(const auto& attr : m_attributes)
        {
            if(attr.name == name)
                return true;
        }
        return false;
    }

    [[nodiscard]] bool contains(const std::string &name, const SymbolTable& symbols) const
    {
        for(const auto& attr : m_attributes)
        {
            if(attr.name.string(symbols) == name)
                return true;
        }
        return false;
    }
private:
    std::vector<Attribute> m_attributes;
};
}
