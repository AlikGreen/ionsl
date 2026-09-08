#pragma once
#include <unordered_map>

#include "type.h"

namespace ionsl
{
class TypeTable
{
public:
    TypeTable();
    TypeId getPrimitiveType(PrimitiveKind kind);

    TypeId getVectorType(TypeId scalarKind, uint32_t dimension);
    TypeId getMatrixType(TypeId scalarKind, uint32_t rows, uint32_t columns);
    TypeId getArrayType(TypeId elementType, std::optional<uint32_t> size);

    TypeId getStructType(DeclId id);
    TypeId getInterfaceType(DeclId id);

    [[nodiscard]] TypeInfo getInfo(TypeId id) const;
    [[nodiscard]] bool isIntegral(TypeId id) const;
private:
    TypeId addType(TypeInfo info);

    template<typename T>
    requires std::is_constructible_v<TypeInfo, T>
    TypeId addType(T type)
    {
        const TypeId typeId{m_types.size()};
        m_types.emplace_back(type);
        return typeId;
    }

    void addPrimitiveType(PrimitiveKind kind, TypeId id);

    std::vector<TypeInfo> m_types;

    std::unordered_map<VectorType, TypeId> m_vectorTypes;
    std::unordered_map<MatrixType, TypeId> m_matrixTypes;
    std::unordered_map<ArrayType, TypeId> m_arrayTypes;
    std::unordered_map<PrimitiveKind, TypeId> m_primitiveTypes;
    std::unordered_map<DeclId, TypeId> m_structTypes;
    std::unordered_map<DeclId, TypeId> m_interfaceTypes;
};
}
