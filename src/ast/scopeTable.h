#pragma once
#include "decl.h"
#include "qualifiedName.h"


namespace ionsl
{
class ScopeId
{
public:
    ScopeId() = default;
    explicit ScopeId(const uint32_t val) : m_value(val) { }

    [[nodiscard]] uint32_t value() const { return m_value; }
    friend constexpr bool operator==(ScopeId, ScopeId) = default;

    static const ScopeId Error;
private:
    uint32_t m_value = 0;
};
}

template<>
struct std::hash<ionsl::ScopeId>
{
    size_t operator()(const ionsl::ScopeId& value) const noexcept
    {
        return value.value();
    }
};


namespace ionsl
{
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
    [[nodiscard]] std::vector<DeclId> findDecls(ScopeId scopeId, const QualifiedName& name) const;
private:
    uint32_t m_nextScopeId = 1;
    std::unordered_map<ScopeId, Scope> m_scopes;

    [[nodiscard]] std::vector<DeclId> findUnqualifiedDecls(ScopeId scopeId, SymbolId symbol) const;
};
}
