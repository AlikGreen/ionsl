#pragma once
#include <unordered_map>

#include "type.h"

namespace ionsl
{
class TypeTable
{
public:
    TypeId getVectorType(TypeId scalarKind, uint32_t dimension);
    TypeId getMatrixType(TypeId scalarKind, uint32_t rows, uint32_t columns);
private:
    std::vector<TypeInfo> m_types;
};
}
