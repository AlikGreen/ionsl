#include "scopeTable.h"

#include <stack>
#include <unordered_set>

namespace ionsl
{
    ScopeId ScopeTable::create(const ScopeId parent)
    {
        Scope scope{};
        scope.parent = parent;

        ScopeId id = m_nextScopeId++;
        m_scopes.emplace(id, scope);
        return id;
    }

    const Scope & ScopeTable::getScope(const ScopeId id)
    {
        return m_scopes.at(id);
    }

    void ScopeTable::registerDecl(const ScopeId scopeId, const SymbolId name, const DeclId id)
    {
        m_scopes[scopeId].decls[name].push_back(id);
    }

    std::vector<DeclId> ScopeTable::findDecls(ScopeId scopeId, const QualifiedName &name) const
    {
        return findUnqualifiedDecls(scopeId, name.parts.at(0));
    }

    std::vector<DeclId> ScopeTable::findUnqualifiedDecls(ScopeId scopeId, const SymbolId symbol) const
    {
        while(scopeId != ScopeIdInvalid)
        {
            const Scope& scope = m_scopes.at(scopeId);

            auto it = scope.decls.find(symbol);

            if(it != scope.decls.end())
                return it->second;

            scopeId = scope.parent;
        }

        return {};
    }
}
