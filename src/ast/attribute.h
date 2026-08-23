#pragma once
#include <variant>

#include "literalValue.h"
#include "qualifiedName.h"

namespace ionsl
{
using AttributeArg = std::variant<QualifiedName, LiteralValue>;

struct Attribute
{
    QualifiedName name;
    std::vector<AttributeArg> args;
};
}
