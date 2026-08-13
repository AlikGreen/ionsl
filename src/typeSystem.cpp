#include "typeSystem.h"

namespace ionsl
{
    TypeSystem::TypeSystem()
    {
        // TODO add all primitive type conversions here

        m_primitiveConversionCosts[PrimitiveKind::Int8][PrimitiveKind::Int8] = 0;
        m_primitiveConversionCosts[PrimitiveKind::Int8][PrimitiveKind::Int16] = 1;
        m_primitiveConversionCosts[PrimitiveKind::Int8][PrimitiveKind::Int32] = 2;
        m_primitiveConversionCosts[PrimitiveKind::Int8][PrimitiveKind::Int64] = 3;

        m_primitiveConversionCosts[PrimitiveKind::Int16][PrimitiveKind::Int16] = 0;
        m_primitiveConversionCosts[PrimitiveKind::Int16][PrimitiveKind::Int32] = 1;
        m_primitiveConversionCosts[PrimitiveKind::Int16][PrimitiveKind::Int64] = 2;

        m_primitiveConversionCosts[PrimitiveKind::Int32][PrimitiveKind::Int32] = 0;
        m_primitiveConversionCosts[PrimitiveKind::Int32][PrimitiveKind::Int64] = 1;
    }

    std::optional<uint32_t> TypeSystem::getConversionCost(const Type &fromType, const Type &toType)
    {
        return std::visit(
        [this](const auto& from, const auto& to) -> std::optional<uint32_t>
        {
            using From = std::decay_t<decltype(from)>;
            using To = std::decay_t<decltype(to)>;

            if constexpr (!std::is_same_v<From, To>)
                return std::nullopt;
            if constexpr (std::is_same_v<From, PrimitiveType> && std::is_same_v<To, PrimitiveKind>)
                return m_primitiveConversionCosts[from.kind][to.kind];
            // if constexpr (std::is_same_v<From, VectorType>) have to evaluate the dimension to make sure they are the same

            // TODO add support for cast operator overloading

            return std::nullopt;
        },
        fromType.kind,
        toType.kind);
    }

    bool TypeSystem::isCompatible(const Type &from, const Type &to)
    {
        return getConversionCost(from, to).has_value();
    }
}