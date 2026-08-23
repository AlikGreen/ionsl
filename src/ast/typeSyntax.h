#pragma once
#include "astNode.h"
#include "qualifiedName.h"

namespace ionsl
{
class Expression;

class TypeSyntax : public AstNode
{
public:
    TypeSyntax* clone(Arena& arena) const override = 0;
};

class TypeArgument : public AstNode
{
public:
    TypeArgument* clone(Arena& arena) const override = 0;
};

class TypeArgumentType final : public TypeArgument
{
public:
    TypeSyntax* type{};

    TypeArgumentType* clone(Arena &arena) const override;
};

class TypeArgumentValue final : public TypeArgument
{
public:
    Expression* expression{};

    TypeArgumentValue* clone(Arena &arena) const override;
};

class NamedTypeSyntax final : public TypeSyntax
{
public:
    QualifiedName name;
    std::vector<TypeArgument*> arguments;

    NamedTypeSyntax* clone(Arena &arena) const override;
};

class ArrayTypeSyntax final : public TypeSyntax
{
public:
    TypeSyntax* elementType{};
    Expression* size{};

    ArrayTypeSyntax* clone(Arena &arena) const override;
};
}
