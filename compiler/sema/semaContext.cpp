#include "semaContext.h"

namespace ionsl
{
    SemaContext SemaContext::withScope(const ScopeId s) const
    {
        auto c = *this; c.scope = s; return c;
    }

    SemaContext SemaContext::withSubs(const std::unordered_map<DeclId, TypeId> &subs) const
    {
        auto c = *this;
        c.substitutions = &subs;
        return c;
    }

    SemaContext SemaContext::forGenericDecl(const std::span<GenericParam * const> params,
        const std::unordered_map<DeclId, TypeId> &subs) const
    {
        auto c = *this;
        c.visibleGenericParams = params;
        c.substitutions = &subs;
        return c;
    }
}