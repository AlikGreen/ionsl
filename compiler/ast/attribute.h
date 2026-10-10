#pragma once
#include <algorithm>
#include <utility>
#include <variant>

#include "constantValue.h"
#include "expressions.h"
#include "qualifiedName.h"
#include "../common/arena.h"

namespace ionsl
{
class AttributeArg : public AstNode
{
public:
    AttributeArg() = default;

    AttributeArg* clone(Arena& arena) const override;

    SymbolId name = SymbolId::Invalid;
    SourceSpan span{};
    Expression* expression{};
    std::optional<ConstantValue> value{};
};

class Attribute : public AstNode
{
public:
    Attribute() = default;

    Attribute* clone(Arena& arena) const override;

    QualifiedName name;
    SourceSpan span;
    std::vector<AttributeArg*> args;
    DeclId decl = DeclId::Error;

    const AttributeArg* getArg(const std::string &argName, const SymbolTable& symbols) const;
};

class Attributes : public AstNode
{
public:
    Attributes() = default;
    // ReSharper disable once CppNonExplicitConvertingConstructor
    Attributes(std::vector<Attribute*> attributes) : attributes(std::move(attributes)) { } // NOLINT(*-explicit-constructor)

    [[nodiscard]] bool contains(const QualifiedName &name) const;
    [[nodiscard]] bool contains(const std::string &name, const SymbolTable& symbols) const;

    [[nodiscard]] const Attribute* find(const std::string &name, const SymbolTable& symbols) const;

    Attributes* clone(Arena& arena) const override;

    std::vector<Attribute*> attributes;
};
}
