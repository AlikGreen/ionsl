#pragma once
#include <vector>

#include "ast.h"

namespace ionsl
{
class DeclTable
{
public:
    DeclTable() = default;
    explicit DeclTable(std::vector<DeclNode>& decls)
    {
        for (auto& decl : decls)
            m_byId[decl.id] = &decl;
    }

    DeclNode* get(const DeclId id)
    {
        if (const auto it = m_byId.find(id); it != m_byId.end())
            return it->second;

        return nullptr;
    }
private:
    std::unordered_map<DeclId, DeclNode*> m_byId;
};
}
