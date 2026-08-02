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
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign
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

        // case TokenKind::Percent:          return BinaryOp::Modulo;
        // case TokenKind::Amp:              return BinaryOp::BitwiseAnd;
        // case TokenKind::Pipe:             return BinaryOp::BitwiseOr;
        // case TokenKind::Caret:            return BinaryOp::BitwiseXor;
        // case TokenKind::LessLess:         return BinaryOp::ShiftLeft;
        // case TokenKind::GreaterGreater:   return BinaryOp::ShiftRight;

        case TokenKind::Equal:            return BinaryOp::Assign;
        case TokenKind::PlusEqual:        return BinaryOp::AddAssign;
        case TokenKind::MinusEqual:       return BinaryOp::SubAssign;
        case TokenKind::StarEqual:        return BinaryOp::MulAssign;
        case TokenKind::SlashEqual:       return BinaryOp::DivAssign;
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
