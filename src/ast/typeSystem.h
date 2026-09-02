#pragma once
#include "symbolTable.h"
#include "typeTable.h"
#include "../common/enums.h"

namespace ionsl
{
struct BinaryTypeResult
{
    TypeId leftType;
    TypeId rightType;
    TypeId resultType;
};

struct UnaryResultType
{
    TypeId operandType;
    TypeId resultType;
};

class TypeSystem
{
public:
    explicit TypeSystem(const TypeTable& typeTable, const DeclTable& declTable, const SymbolTable& symbolTable);

    [[nodiscard]] std::optional<uint32_t> conversionCost(TypeId from, TypeId to) const;
    [[nodiscard]] std::optional<uint32_t> conversionCost(const std::vector<TypeId> &from, const std::vector<TypeId> &to) const;

    [[nodiscard]] std::optional<uint32_t> primitiveConversionCost(PrimitiveKind from, PrimitiveKind to) const;

    [[nodiscard]] std::optional<TypeId> findCommonType(TypeId left, TypeId right) const;

    [[nodiscard]] std::optional<BinaryTypeResult> resolveBinaryTypes(BinaryOp op, TypeId left, TypeId right) const;
    [[nodiscard]] std::optional<UnaryResultType> resolveUnaryType(UnaryOp op, TypeId operand) const;
private:
    const TypeTable& m_types;
    const DeclTable& m_declTable;
    const SymbolTable& m_symbols;
};
}
