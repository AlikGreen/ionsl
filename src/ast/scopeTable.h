#pragma once
#include "decl.h"
#include "qualifiedName.h"


namespace ionsl
{
using ScopeId = uint32_t;
constexpr ScopeId ScopeIdInvalid = ~0u;

class Scope
{
public:
    ScopeId parent;
    std::unordered_map<SymbolId, std::vector<DeclId>> decls;
};

class ScopeTable
{
public:
    ScopeId create(ScopeId parent);
    const Scope& getScope(ScopeId id);

    void registerDecl(ScopeId scopeId, SymbolId name, DeclId id);

    // FIXME split into findValueDecls and findTypeDecls
    std::vector<DeclId> findDecls(ScopeId scopeId, const QualifiedName& name) const;

private:
    ScopeId m_nextScopeId = 0;
    std::unordered_map<ScopeId, Scope> m_scopes;

    [[nodiscard]] std::vector<DeclId> findUnqualifiedDecls(ScopeId scopeId, SymbolId symbol) const;
};
}
