#include "scopeTable.h"

#include <stack>
#include <unordered_set>

namespace ionsl
{
    constexpr ScopeId ScopeId::None = ScopeId(0);
    constexpr ScopeId ScopeId::Error = ScopeId(1);

    ScopeId ScopeTable::create(const ScopeId parent)
    {
        Scope scope{};
        scope.parent = parent;

        auto id = ScopeId(m_nextScopeId++);
        m_scopes.emplace(id, scope);
        return id;
    }

    const Scope & ScopeTable::getScope(const ScopeId id)
    {
        return m_scopes.at(id);
    }

    void ScopeTable::registerDecl(const ScopeId scopeId, const SymbolId name, const DeclId id)
    {
        if(scopeId == ScopeId::None) return; // maybe should be more explicit
        m_scopes[scopeId].decls[name].push_back(id);
    }


    std::span<const DeclId> ScopeTable::find(ScopeId scopeId, const SymbolId name) const
    {
        while(scopeId != ScopeId::Error && scopeId != ScopeId::None)
        {
            const Scope& scope = m_scopes.at(scopeId);

            auto it = scope.decls.find(name);

            if(it != scope.decls.end())
                return it->second;

            scopeId = scope.parent;
        }

        return {};
    }

}
