#pragma once
#include <cstdint>
#include <optional>

#include "lexer.h"

namespace ionsl
{
enum class BinaryOp : uint8_t
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,

    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LogicalAnd,
    LogicalOr,

    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    ShiftLeft,
    ShiftRight,

    Assign,
    ModuloAssign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign,
    BitwiseAndAssign,
    BitwiseOrAssign,
    BitwiseXorAssign,
    ShiftLeftAssign,
    ShiftRightAssign,
};

inline std::optional<BinaryOp> tokenToBinaryOp(const TokenKind type)
{
    switch (type)
    {
        case TokenKind::Plus:             return BinaryOp::Add;
        case TokenKind::Minus:            return BinaryOp::Subtract;
        case TokenKind::Star:             return BinaryOp::Multiply;
        case TokenKind::Slash:            return BinaryOp::Divide;
        case TokenKind::EqualEqual:       return BinaryOp::Equal;
        case TokenKind::ExclamationEqual: return BinaryOp::NotEqual;
        case TokenKind::LAngle:             return BinaryOp::Less;
        case TokenKind::LessEqual:        return BinaryOp::LessEqual;
        case TokenKind::RAngle:          return BinaryOp::Greater;
        case TokenKind::GreaterEqual:     return BinaryOp::GreaterEqual;
        case TokenKind::AmpAmp:           return BinaryOp::LogicalAnd;
        case TokenKind::PipePipe:         return BinaryOp::LogicalOr;

        case TokenKind::Percent:          return BinaryOp::Modulo;
        case TokenKind::PercentEqual:         return BinaryOp::ModuloAssign;

        case TokenKind::Equal:            return BinaryOp::Assign;
        case TokenKind::PlusEqual:        return BinaryOp::AddAssign;
        case TokenKind::MinusEqual:       return BinaryOp::SubAssign;
        case TokenKind::StarEqual:        return BinaryOp::MulAssign;
        case TokenKind::SlashEqual:       return BinaryOp::DivAssign;

        case TokenKind::AmpEqual:         return BinaryOp::BitwiseAndAssign;
        case TokenKind::PipeEqual:        return BinaryOp::BitwiseOrAssign;
        case TokenKind::CaretEqual:       return BinaryOp::BitwiseXorAssign;
        case TokenKind::LAngleLAngle:     return BinaryOp::ShiftLeft;
        case TokenKind::RAngleRAngle:     return BinaryOp::ShiftRight;
        case TokenKind::LAngleLAngleEqual: return BinaryOp::ShiftLeftAssign;
        case TokenKind::RAngleRAngleEqual: return BinaryOp::ShiftRightAssign;

        default:                          return std::nullopt;
    }
}

enum class UnaryOp : uint8_t
{
    Negate,
    LogicalNot,
    BitwiseNot,
    PreIncrement,
    PreDecrement,
    PostIncrement,
    PostDecrement
};
}
