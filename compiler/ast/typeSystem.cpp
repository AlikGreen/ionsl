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

    TypeSystem::TypeSystem(TypeTable &typeTable, const SymbolTable &symbolTable)
        : m_types(typeTable), m_symbols(symbolTable)
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
            if(!cost)
            {
                return std::nullopt;
            }
            conversionSum += *cost;
        }

        return conversionSum;
    }

    std::optional<uint32_t> TypeSystem::primitiveConversionCost(PrimitiveKind from, PrimitiveKind to) const
    {
        static std::unordered_map<std::pair<PrimitiveKind, PrimitiveKind>, uint32_t, PrimitivePairHash> costs
        {
            { { PrimitiveKind::Float16, PrimitiveKind::Float16 }, 0 },
            { { PrimitiveKind::Float16, PrimitiveKind::Float32 }, 1 },
            { { PrimitiveKind::Float16, PrimitiveKind::Float64 }, 2 },

            { { PrimitiveKind::Float32, PrimitiveKind::Float16 }, 1 },
            { { PrimitiveKind::Float32, PrimitiveKind::Float32 }, 0 },
            { { PrimitiveKind::Float32, PrimitiveKind::Float64 }, 1 },

            { { PrimitiveKind::Float64, PrimitiveKind::Float16 }, 2 },
            { { PrimitiveKind::Float64, PrimitiveKind::Float32 }, 1 },
            { { PrimitiveKind::Float64, PrimitiveKind::Float64 }, 0 },

            { { PrimitiveKind::UInt8, PrimitiveKind::UInt8 }, 0 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt16 }, 1 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt32 }, 2 },
            { { PrimitiveKind::UInt8, PrimitiveKind::UInt64 }, 3 },

            { { PrimitiveKind::UInt16, PrimitiveKind::UInt8 }, 1 },
            { { PrimitiveKind::UInt16, PrimitiveKind::UInt16 }, 0 },
            { { PrimitiveKind::UInt16, PrimitiveKind::UInt32 }, 1 },
            { { PrimitiveKind::UInt16, PrimitiveKind::UInt64 }, 2 },

            { { PrimitiveKind::UInt32, PrimitiveKind::UInt8 }, 2 },
            { { PrimitiveKind::UInt32, PrimitiveKind::UInt16 }, 1 },
            { { PrimitiveKind::UInt32, PrimitiveKind::UInt32 }, 0 },
            { { PrimitiveKind::UInt32, PrimitiveKind::UInt64 }, 1 },

            { { PrimitiveKind::UInt64, PrimitiveKind::UInt8 }, 3 },
            { { PrimitiveKind::UInt64, PrimitiveKind::UInt16 }, 2 },
            { { PrimitiveKind::UInt64, PrimitiveKind::UInt32 }, 1 },
            { { PrimitiveKind::UInt64, PrimitiveKind::UInt64 }, 0 },

            { { PrimitiveKind::Int8, PrimitiveKind::Int8 }, 0 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int16 }, 1 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int32 }, 2 },
            { { PrimitiveKind::Int8, PrimitiveKind::Int64 }, 3 },

            { { PrimitiveKind::Int16, PrimitiveKind::Int8 }, 1 },
            { { PrimitiveKind::Int16, PrimitiveKind::Int16 }, 0 },
            { { PrimitiveKind::Int16, PrimitiveKind::Int32 }, 1 },
            { { PrimitiveKind::Int16, PrimitiveKind::Int64 }, 2 },

            { { PrimitiveKind::Int32, PrimitiveKind::Int8 }, 2 },
            { { PrimitiveKind::Int32, PrimitiveKind::Int16 }, 1 },
            { { PrimitiveKind::Int32, PrimitiveKind::Int32 }, 0 },
            { { PrimitiveKind::Int32, PrimitiveKind::Int64 }, 1 },

            { { PrimitiveKind::Int64, PrimitiveKind::Int8 }, 3 },
            { { PrimitiveKind::Int64, PrimitiveKind::Int16 }, 2 },
            { { PrimitiveKind::Int64, PrimitiveKind::Int32 }, 1 },
            { { PrimitiveKind::Int64, PrimitiveKind::Int64 }, 0 },

            { { PrimitiveKind::UInt8, PrimitiveKind::Int8 }, 4 },
            { { PrimitiveKind::UInt8, PrimitiveKind::Int16 }, 4 },
            { { PrimitiveKind::UInt8, PrimitiveKind::Int32 }, 5 },
            { { PrimitiveKind::UInt8, PrimitiveKind::Int64 }, 6 },

            { { PrimitiveKind::UInt16, PrimitiveKind::Int8 }, 5 },
            { { PrimitiveKind::UInt16, PrimitiveKind::Int16 }, 4 },
            { { PrimitiveKind::UInt16, PrimitiveKind::Int32 }, 4 },
            { { PrimitiveKind::UInt16, PrimitiveKind::Int64 }, 5 },

            { { PrimitiveKind::UInt32, PrimitiveKind::Int8 }, 6 },
            { { PrimitiveKind::UInt32, PrimitiveKind::Int16 }, 5 },
            { { PrimitiveKind::UInt32, PrimitiveKind::Int32 }, 4 },
            { { PrimitiveKind::UInt32, PrimitiveKind::Int64 }, 4 },

            { { PrimitiveKind::UInt64, PrimitiveKind::Int8 }, 7 },
            { { PrimitiveKind::UInt64, PrimitiveKind::Int16 }, 6 },
            { { PrimitiveKind::UInt64, PrimitiveKind::Int32 }, 5 },
            { { PrimitiveKind::UInt64, PrimitiveKind::Int64 }, 4 },

            { { PrimitiveKind::Int8, PrimitiveKind::UInt8 }, 4 },
            { { PrimitiveKind::Int8, PrimitiveKind::UInt16 }, 4 },
            { { PrimitiveKind::Int8, PrimitiveKind::UInt32 }, 5 },
            { { PrimitiveKind::Int8, PrimitiveKind::UInt64 }, 6 },

            { { PrimitiveKind::Int16, PrimitiveKind::UInt8 }, 5 },
            { { PrimitiveKind::Int16, PrimitiveKind::UInt16 }, 4 },
            { { PrimitiveKind::Int16, PrimitiveKind::UInt32 }, 4 },
            { { PrimitiveKind::Int16, PrimitiveKind::UInt64 }, 5 },

            { { PrimitiveKind::Int32, PrimitiveKind::UInt8 }, 6 },
            { { PrimitiveKind::Int32, PrimitiveKind::UInt16 }, 5 },
            { { PrimitiveKind::Int32, PrimitiveKind::UInt32 }, 4 },
            { { PrimitiveKind::Int32, PrimitiveKind::UInt64 }, 4 },

            { { PrimitiveKind::Int64, PrimitiveKind::UInt8 }, 7 },
            { { PrimitiveKind::Int64, PrimitiveKind::UInt16 }, 6 },
            { { PrimitiveKind::Int64, PrimitiveKind::UInt32 }, 5 },
            { { PrimitiveKind::Int64, PrimitiveKind::UInt64 }, 4 },

            { { PrimitiveKind::Int8, PrimitiveKind::Float16 }, 3 },
            { { PrimitiveKind::Int8, PrimitiveKind::Float32 }, 4 },
            { { PrimitiveKind::Int8, PrimitiveKind::Float64 }, 5 },

            { { PrimitiveKind::Int16, PrimitiveKind::Float16 }, 4 },
            { { PrimitiveKind::Int16, PrimitiveKind::Float32 }, 3 },
            { { PrimitiveKind::Int16, PrimitiveKind::Float64 }, 4 },

            { { PrimitiveKind::Int32, PrimitiveKind::Float16 }, 5 },
            { { PrimitiveKind::Int32, PrimitiveKind::Float32 }, 3 },
            { { PrimitiveKind::Int32, PrimitiveKind::Float64 }, 4 },

            { { PrimitiveKind::Int64, PrimitiveKind::Float16 }, 6 },
            { { PrimitiveKind::Int64, PrimitiveKind::Float32 }, 4 },
            { { PrimitiveKind::Int64, PrimitiveKind::Float64 }, 3 },

            { { PrimitiveKind::Float16, PrimitiveKind::Int8 }, 3 },
            { { PrimitiveKind::Float16, PrimitiveKind::Int16 }, 4 },
            { { PrimitiveKind::Float16, PrimitiveKind::Int32 }, 5 },
            { { PrimitiveKind::Float16, PrimitiveKind::Int64 }, 6 },

            { { PrimitiveKind::Float32, PrimitiveKind::Int8 }, 4 },
            { { PrimitiveKind::Float32, PrimitiveKind::Int16 }, 3 },
            { { PrimitiveKind::Float32, PrimitiveKind::Int32 }, 3 },
            { { PrimitiveKind::Float32, PrimitiveKind::Int64 }, 4 },

            { { PrimitiveKind::Float64, PrimitiveKind::Int8 }, 5 },
            { { PrimitiveKind::Float64, PrimitiveKind::Int16 }, 4 },
            { { PrimitiveKind::Float64, PrimitiveKind::Int32 }, 4 },
            { { PrimitiveKind::Float64, PrimitiveKind::Int64 }, 3 },

            { { PrimitiveKind::UInt8, PrimitiveKind::Float16 }, 3 },
            { { PrimitiveKind::UInt8, PrimitiveKind::Float32 }, 4 },
            { { PrimitiveKind::UInt8, PrimitiveKind::Float64 }, 5 },

            { { PrimitiveKind::UInt16, PrimitiveKind::Float16 }, 4 },
            { { PrimitiveKind::UInt16, PrimitiveKind::Float32 }, 3 },
            { { PrimitiveKind::UInt16, PrimitiveKind::Float64 }, 4 },

            { { PrimitiveKind::UInt32, PrimitiveKind::Float16 }, 5 },
            { { PrimitiveKind::UInt32, PrimitiveKind::Float32 }, 3 },
            { { PrimitiveKind::UInt32, PrimitiveKind::Float64 }, 4 },

            { { PrimitiveKind::UInt64, PrimitiveKind::Float16 }, 6 },
            { { PrimitiveKind::UInt64, PrimitiveKind::Float32 }, 4 },
            { { PrimitiveKind::UInt64, PrimitiveKind::Float64 }, 3 },

            { { PrimitiveKind::Float16, PrimitiveKind::UInt8 }, 3 },
            { { PrimitiveKind::Float16, PrimitiveKind::UInt16 }, 4 },
            { { PrimitiveKind::Float16, PrimitiveKind::UInt32 }, 5 },
            { { PrimitiveKind::Float16, PrimitiveKind::UInt64 }, 6 },

            { { PrimitiveKind::Float32, PrimitiveKind::UInt8 }, 4 },
            { { PrimitiveKind::Float32, PrimitiveKind::UInt16 }, 3 },
            { { PrimitiveKind::Float32, PrimitiveKind::UInt32 }, 3 },
            { { PrimitiveKind::Float32, PrimitiveKind::UInt64 }, 4 },

            { { PrimitiveKind::Float64, PrimitiveKind::UInt8 }, 5 },
            { { PrimitiveKind::Float64, PrimitiveKind::UInt16 }, 4 },
            { { PrimitiveKind::Float64, PrimitiveKind::UInt32 }, 4 },
            { { PrimitiveKind::Float64, PrimitiveKind::UInt64 }, 3 },
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
                    TypeId::Bool,
                };
            }
            case BinaryOp::LogicalAnd:
            case BinaryOp::LogicalOr:
            {
                if(left != TypeId::Bool ||
                   right != TypeId::Bool)
                {
                    return std::nullopt;
                }

                return BinaryTypeResult{
                    TypeId::Bool,
                    TypeId::Bool,
                    TypeId::Bool
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
            if(operand != TypeId::Bool)
                return std::nullopt;

            return UnaryResultType {
                TypeId::Bool,
                TypeId::Bool
            };
        }

        if(!m_types.isIntegral(operand))
        {
            if (op == UnaryOp::Negate && TypeTable::isFloat(operand))
                return UnaryResultType {
                    operand,
                    operand
                };


            return std::nullopt;
        }

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
