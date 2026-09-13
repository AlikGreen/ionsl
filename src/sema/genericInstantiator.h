#pragma once
#include "semaContext.h"
#include "../ast/declarations.h"
#include "../ast/typeSystem.h"

namespace ionsl
{
class GenericInstantiator
{
public:
    FunctionDecl* instantiate(const FunctionDecl *genericDecl, const std::vector<TypeArgument*> &typeArgs);
private:
    [[nodiscard]] size_t hashFuncInst(DeclId genericId, const std::vector<TypeArgument*> &typeArgs) const;

    Arena& m_arena;
    TypeSystem& m_types;
    ScopeTable& m_scopes;
    DeclAllocator& m_declAllocator;
    Module& m_module;

    std::unordered_map<size_t, FunctionDecl*> m_cache;
};
}
