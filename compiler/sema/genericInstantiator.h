#pragma once
#include "semaContext.h"
#include "../ast/declarations.h"
#include "../ast/typeSystem.h"

namespace ionsl
{
class GenericInstantiator
{
public:
    GenericInstantiator(TypeSystem &m_types, DeclAllocator &m_declAllocator, Module &m_module)
        : m_types(m_types),
          m_declAllocator(m_declAllocator),
          m_module(m_module)
    {
    }

    FunctionDecl* instantiate(const FunctionDecl& genericDecl, const std::vector<TypeArgument*> &typeArgs);
    FunctionDecl* instantiate(const FunctionDecl& genericDecl, const std::vector<TypeId>& typeArgs);
private:
    [[nodiscard]] size_t hashFuncInst(DeclId genericId, const std::vector<TypeId> &typeArgs) const;

    TypeSystem& m_types;
    DeclAllocator& m_declAllocator;
    Module& m_module;

    std::unordered_map<size_t, FunctionDecl*> m_cache{};
};
}
