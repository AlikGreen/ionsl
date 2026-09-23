#pragma once
#include <algorithm>
#include <utility>
#include <variant>

#include "constantValue.h"
#include "qualifiedName.h"

namespace ionsl
{
using AttribArgValue = std::variant<QualifiedName, ConstantValue>;

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

    const AttributeArg& getArgOr(const SymbolId argName, const AttributeArg& val) const
    {
        for (const auto& arg : args)
        {
            if (arg.name == argName)
                return arg;
        }

        return val;
    }
};

class Attributes
{
public:
    Attributes() = default;
    // ReSharper disable once CppNonExplicitConvertingConstructor
    Attributes(const std::vector<Attribute> &attributes) : m_attributes(attributes) { } // NOLINT(*-explicit-constructor)

    [[nodiscard]] const std::vector<Attribute>& attributes() const { return m_attributes; }

    [[nodiscard]] bool contains(const QualifiedName &name) const;
    [[nodiscard]] bool contains(const std::string &name, const SymbolTable& symbols) const;

    [[nodiscard]] const Attribute* find(const std::string &name, const SymbolTable& symbols) const;
private:
    std::vector<Attribute> m_attributes;
};
}
