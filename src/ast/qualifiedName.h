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

    void add(SymbolId id)
    {
        parts.push_back(id);
    }

    std::string string(const SymbolTable& table)
    {
        std::string fullName;

        for(const auto& part : parts)
            fullName += table.get(part);

        return fullName;
    }

    bool matchesStart(const QualifiedName& other) const
    {
        if(other.parts.size() > parts.size()) return false;

        for(size_t i = 0; i < other.parts.size(); i++)
        {
            if(parts[i] != other.parts[i]) return false;
        }

        return true;
    }

    bool operator==(const QualifiedName& other) const
    {
        if(other.parts.size() != parts.size()) return false;

        for(size_t i = 0; i < parts.size(); i++)
        {
            if(parts[i] != other.parts[i]) return false;
        }

        return true;
    }
};
}
