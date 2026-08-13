#pragma once
#include <ranges>
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

    std::vector<FunctionDecl*> getFunctionsByName(const std::string &name)
    {
        std::vector<FunctionDecl*> funcs;

        for (const auto decl: m_byId | std::views::values)
        {
            if (auto funcDecl = std::get_if<FunctionDecl>(&decl->decl))
            {
                if (funcDecl->name == name)
                    funcs.push_back(funcDecl);
            }
        }

        return funcs;
    }
private:
    std::unordered_map<DeclId, DeclNode*> m_byId;
};
}
