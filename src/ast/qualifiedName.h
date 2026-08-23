#pragma once
#include <string>
#include <vector>

#include "symbol.h"
#include "symbolTable.h"

namespace ionsl
{
struct QualifiedName
{
    std::vector<SymbolId> parts;

    static QualifiedName single(SymbolId name)
    {
        QualifiedName q;
        q.parts.push_back(name);
        return q;
    }

    std::string string(const SymbolTable& table)
    {
        std::string fullName;

        for(const auto& part : parts)
            fullName += table.get(part);

        return fullName;
    }
};
}
