#pragma once
#include <cstdint>

#include "astWalker.h"


namespace ionsl
{
class Declaration;
using DeclId = uint32_t;

constexpr DeclId InvalidDeclId = ~0u;

class DeclarationIdAllocator
{
public:
    DeclId allocate()
    {
        return m_nextId++;
    }

private:
    uint64_t m_nextId = 0;
};

class DeclTable
{
public:
    DeclTable() = default;
    explicit DeclTable(Module& module);
    [[nodiscard]] Declaration* get(DeclId id) const;
private:
    std::unordered_map<DeclId, Declaration*> m_map{};
};
}
