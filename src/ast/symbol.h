#pragma once
#include <cstdint>
#include <functional>

namespace ionsl
{
class SymbolId
{
public:
    SymbolId() = default;
    explicit constexpr SymbolId(const uint32_t val) : m_value(val) { }

    [[nodiscard]] uint32_t value() const { return m_value; }
    friend constexpr bool operator==(SymbolId, SymbolId) = default;

    static const SymbolId Invalid;
private:
    uint32_t m_value = 0;
};
}

template<>
struct std::hash<ionsl::SymbolId>
{
    size_t operator()(const ionsl::SymbolId& value) const noexcept
    {
        return value.value();
    }
};
