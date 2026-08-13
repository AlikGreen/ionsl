#pragma once
#include <optional>

#include "types.h"

namespace ionsl
{
class TypeSystem
{
public:
    TypeSystem();
    [[nodiscard]] std::optional<uint32_t> getConversionCost(const Type& fromType, const Type& toType);
    bool isCompatible(const Type& from, const Type& to);
private:
    std::unordered_map<PrimitiveKind, std::unordered_map<PrimitiveKind, std::optional<uint32_t>>> m_primitiveConversionCosts;
};
}
