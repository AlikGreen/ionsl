#include "typeSystem.h"
#include <ranges>

namespace ionsl
{
    struct PrimitivePairHash
    {
        size_t operator()(const std::pair<PrimitiveKind, PrimitiveKind>& pair) const
        {
            return std::hash<int>{}(static_cast<int>(pair.first))
                 ^ (std::hash<int>{}(static_cast<int>(pair.second)) << 1);
        }
    };

    TypeSystem::TypeSystem(TypeTable &typeTable, const DeclTable &declTable, const SymbolTable &symbolTable)
        : m_types(typeTable), m_declTable(declTable), m_symbols(symbolTable)
    {
    }

    std::optional<uint32_t> TypeSystem::conversionCost(const TypeId from, const TypeId to) const
    {
        if(from == to) return 0;

        TypeInfo fromInfo = m_types.getInfo(from);
        TypeInfo toInfo = m_types.getInfo(to);

        if(fromInfo.kind.index() != toInfo.kind.index()) return std::nullopt;
        if(!fromInfo.is<PrimitiveType>()) return std::nullopt;

        const PrimitiveKind fromPrimitive = fromInfo.as<PrimitiveType>()->kind;
        const PrimitiveKind toPrimitive = toInfo.as<PrimitiveType>()->kind;

        auto cost = primitiveConversionCost(fromPrimitive, toPrimitive);
        if(cost.has_value())
            return cost.value();

        return std::nullopt;
    }

    std::optional<uint32_t> TypeSystem::conversionCost(const std::vector<TypeId>& from, const std::vector<TypeId>& to) const
    {
        if(from.size() != to.size()) return std::nullopt;

        uint32_t conversionSum = 0;

        for (const auto& [fromId, toId] : std::views::zip(from, to))
        {
            auto cost = conversionCost(fromId, toId);
            if(!cost) return std::nullopt;
            conversionSum += *cost;
        }

        return conversionSum;
    }

    std::optional<uint32_t> TypeSystem::primitiveConversionCost(PrimitiveKind from, PrimitiveKind to) const
    {
        std::unordered_map<std::pair<PrimitiveKind, PrimitiveKind>, uint32_t, PrimitivePairHash> costs
        {
            { { PrimitiveKind::Float16, PrimitiveKind::Float16 }, 0 },
            { { PrimitiveKind::Float16, PrimitiveKind::Float32 }, 1 },
            { { PrimitiveKind::Float16, PrimitiveKind::Float64 }, 2 },

            { { PrimitiveKind::Float32, PrimitiveKind::Float32 }, 0 },
            { { PrimitiveKind::Float32, PrimitiveKind::Float64 }, 1 },

            { { PrimitiveKind::Float64, PrimitiveKind::Float64 }, 0 },

            { { PrimitiveKind::UInt8, PrimitiveKind::UInt8 }, 0 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt16 }, 1 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt32 }, 2 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt64 }, 3 },

            { { PrimitiveKind::UInt16, PrimitiveKind::UInt16 }, 0 },
            { { PrimitiveKind::UInt16, PrimitiveKind::UInt32 }, 1 },
            { { PrimitiveKind::UInt16, PrimitiveKind::UInt64 }, 2 },

            { { PrimitiveKind::UInt32, PrimitiveKind::UInt32 }, 0 },
            { { PrimitiveKind::UInt32, PrimitiveKind::UInt64 }, 1 },

            { { PrimitiveKind::UInt64, PrimitiveKind::UInt64 }, 0 },

            { { PrimitiveKind::Int8, PrimitiveKind::Int8 }, 0 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int16 }, 1 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int32 }, 2 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int64 }, 3 },

            { { PrimitiveKind::Int16, PrimitiveKind::Int16 }, 0 },
            { { PrimitiveKind::Int16, PrimitiveKind::Int32 }, 1 },
            { { PrimitiveKind::Int16, PrimitiveKind::Int64 }, 2 },

            { { PrimitiveKind::Int32, PrimitiveKind::Int32 }, 0 },
            { { PrimitiveKind::Int32, PrimitiveKind::Int64 }, 1 },

            { { PrimitiveKind::Int64, PrimitiveKind::Int64 }, 0 },

            // TODO add other conversions like int to uint and int to float
        };

        if(const auto it = costs.find({from, to}); it != costs.end())
            return it->second;

        return std::nullopt;
    }

    std::optional<TypeId> TypeSystem::findCommonType(TypeId left, TypeId right) const
    {
        const auto leftToRight = conversionCost(left, right);
        const auto rightToLeft = conversionCost(right, left);

        if(!leftToRight && !rightToLeft)
            return std::nullopt;

        if(leftToRight && !rightToLeft)
            return right;

        if(rightToLeft && !leftToRight)
            return left;

        if(*leftToRight <= *rightToLeft)
            return right;

        return left;
    }

    std::optional<BinaryTypeResult> TypeSystem::resolveBinaryTypes(const BinaryOp op, const TypeId left, const TypeId right) const
    {
        // TODO resolve custom operators

        switch (op)
        {
            case BinaryOp::Add:
            case BinaryOp::Subtract:
            case BinaryOp::Multiply:
            case BinaryOp::Divide:
            case BinaryOp::Modulo:
            {
                const auto commonType = findCommonType(left, right);

                if(!commonType)
                    return std::nullopt;

                return BinaryTypeResult{
                    *commonType,
                    *commonType,
                    *commonType
                };
            }
            case BinaryOp::Assign:
            case BinaryOp::AddAssign:
            case BinaryOp::SubAssign:
            case BinaryOp::MulAssign:
            case BinaryOp::DivAssign:
            case BinaryOp::ModuloAssign:
            {
                if(!conversionCost(right, left))
                    return std::nullopt;

                return BinaryTypeResult{
                    left,
                    left,
                    left
                };
            }
            case BinaryOp::Equal:
            case BinaryOp::NotEqual:
            case BinaryOp::Less:
            case BinaryOp::LessEqual:
            case BinaryOp::Greater:
            case BinaryOp::GreaterEqual:
            {
                const auto commonType = findCommonType(left, right);

                if(!commonType)
                    return std::nullopt;

                return BinaryTypeResult{
                    *commonType,
                    *commonType,
                    TypeIdBool,
                };
            }
            case BinaryOp::LogicalAnd:
            case BinaryOp::LogicalOr:
            {
                if(left != TypeIdBool ||
                   right != TypeIdBool)
                {
                    return std::nullopt;
                }

                return BinaryTypeResult{
                    TypeIdBool,
                    TypeIdBool,
                    TypeIdBool
                };
            }
            case BinaryOp::BitwiseAnd:
            case BinaryOp::BitwiseOr:
            case BinaryOp::BitwiseXor:
            {
                const auto commonType = findCommonType(left, right);

                if(!commonType || !m_types.isIntegral(*commonType))
                    return std::nullopt;

                return BinaryTypeResult{
                    *commonType,
                    *commonType,
                    *commonType
                };
            }

            case BinaryOp::ShiftLeft:
            case BinaryOp::ShiftRight:
            {
                if(!m_types.isIntegral(left) || !m_types.isIntegral(right))
                    return std::nullopt;

                return BinaryTypeResult{
                    left,
                    right,
                    left
                };
            }

            default:
                return std::nullopt;
        }
    }

    std::optional<UnaryResultType> TypeSystem::resolveUnaryType(const UnaryOp op, const TypeId operand) const
    {
        if(op == UnaryOp::LogicalNot)
        {
            if(operand != TypeIdBool)
                return std::nullopt;

            return UnaryResultType {
                TypeIdBool,
                TypeIdBool
            };
        }

        if(!m_types.isIntegral(operand))
            return std::nullopt;

        return UnaryResultType {
            operand,
            operand
        };
    }

    TypeTable& TypeSystem::types() const
    {
        return m_types;
    }
}
