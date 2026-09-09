#pragma once
#include <span>

#include "../ast/scopeTable.h"
#include "../ast/type.h"

namespace ionsl
{
class GenericParam;

struct SemaContext
{
    ScopeId scope{};
    std::span<GenericParam* const> visibleGenericParams{};
    const std::unordered_map<DeclId, TypeId>* substitutions = nullptr;

    [[nodiscard]] SemaContext withScope(ScopeId s) const;
    [[nodiscard]] SemaContext withSubs(const std::unordered_map<DeclId, TypeId>& subs) const;
    [[nodiscard]] SemaContext forGenericDecl(std::span<GenericParam* const> params, const std::unordered_map<DeclId, TypeId>& subs) const;
};
}
