#pragma once
#include <cstdint>

#include "astWalker.h"


namespace ionsl
{
class Declaration;

class DeclId
{
public:
    DeclId() = default;
    explicit DeclId(const uint32_t val) : m_value(val) { }

    [[nodiscard]] uint32_t value() const { return m_value; }
    friend constexpr bool operator==(DeclId, DeclId) = default;

    static const DeclId Error;
private:
    uint32_t m_value = 0;
};
}

template<>
struct std::hash<ionsl::DeclId>
{
    size_t operator()(const ionsl::DeclId& value) const noexcept
    {
        return value.value();
    }
};

namespace ionsl
{
class DeclarationIdAllocator
{
public:
    DeclId allocate()
    {
        return DeclId{m_nextId++};
    }

private:
    uint32_t m_nextId = 1;
};

class DeclTable
{
public:
    DeclTable() = default;
    void regenerate(const Module& module);
    [[nodiscard]] Declaration* get(DeclId id) const;
private:
    std::unordered_map<DeclId, Declaration*> m_map{};
};
}