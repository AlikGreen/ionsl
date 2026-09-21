#pragma once
#include "../common/diagnostics.h"

namespace ionsl
{
enum class TokenKind
{
    Identifier, NumberLiteral, StringLiteral,
    LBracket, RBracket, LParen, RParen, LBrace, RBrace, LAngle, RAngle,
    LBracketLBracket, RBracketRBracket,
    Comma, Colon, ColonColon, Semicolon, Dot, Arrow,
    Exclamation, Minus, Plus, Star, Slash,
    Equal, PlusEqual, MinusEqual, StarEqual, SlashEqual,
    EqualEqual, ExclamationEqual, LessEqual,
    GreaterEqual, AmpAmp, PipePipe, MinusMinus, PlusPlus,
    AmpEqual, PipeEqual, LAngleLAngle, RAngleRAngle,
    LAngleLAngleEqual, RAngleRAngleEqual, CaretEqual,
    Percent, PercentEqual, ColonColonLAngle,
    KwStruct, KwTrue, KwFalse, KwElse, KwBreak, KwContinue,
    KwFunction, KwVar, KwIf, KwWhile, KwFor, KwReturn, KwMut,
    KwInterface, KwType, KwOperator, KwPrefix, KwPostfix,
    KwCast,
    EndOfFile, Unknown
};

struct Token
{
    TokenKind   kind;
    std::string_view text;
    SourceSpan  span;
};

constexpr std::string_view tokenKindDisplayName(const TokenKind kind)
{
    switch (kind)
    {
        case TokenKind::Identifier:         return "an identifier";
        case TokenKind::NumberLiteral:       return "a number literal";
        case TokenKind::StringLiteral:       return "a string literal";

        case TokenKind::LBracket:            return "'['";
        case TokenKind::RBracket:            return "']'";
        case TokenKind::LParen:              return "'('";
        case TokenKind::RParen:              return "')'";
        case TokenKind::LBrace:               return "'{'";
        case TokenKind::RBrace:               return "'}'";
        case TokenKind::LAngle:               return "'<'";
        case TokenKind::RAngle:               return "'>'";
        case TokenKind::LBracketLBracket:      return "'[['";
        case TokenKind::RBracketRBracket:      return "']]'";

        case TokenKind::Comma:                return "','";
        case TokenKind::Colon:                return "':'";
        case TokenKind::ColonColon:            return "'::'";
        case TokenKind::Semicolon:             return "';'";
        case TokenKind::Dot:                   return "'.'";
        case TokenKind::Arrow:                 return "'->'";

        case TokenKind::Exclamation:           return "'!'";
        case TokenKind::Minus:                 return "'-'";
        case TokenKind::Plus:                  return "'+'";
        case TokenKind::Star:                  return "'*'";
        case TokenKind::Slash:                 return "'/'";

        case TokenKind::Equal:                 return "'='";
        case TokenKind::PlusEqual:              return "'+='";
        case TokenKind::MinusEqual:             return "'-='";
        case TokenKind::StarEqual:              return "'*='";
        case TokenKind::SlashEqual:             return "'/='";

        case TokenKind::EqualEqual:             return "'=='";
        case TokenKind::ExclamationEqual:       return "'!='";
        case TokenKind::LessEqual:              return "'<='";
        case TokenKind::GreaterEqual:           return "'>='";
        case TokenKind::AmpAmp:                 return "'&&'";
        case TokenKind::PipePipe:               return "'||'";
        case TokenKind::MinusMinus:             return "'--'";
        case TokenKind::PlusPlus:               return "'++'";

        case TokenKind::AmpEqual:               return "'&='";
        case TokenKind::PipeEqual:              return "'|='";
        case TokenKind::LAngleLAngle:           return "'<<'";
        case TokenKind::RAngleRAngle:           return "'>>'";
        case TokenKind::LAngleLAngleEqual:      return "'<<='";
        case TokenKind::RAngleRAngleEqual:      return "'>>='";
        case TokenKind::CaretEqual:             return "'^=";
        case TokenKind::Percent:                return "'%'";
        case TokenKind::PercentEqual:           return "'%='";
        case TokenKind::ColonColonLAngle:       return "'::<'";

        case TokenKind::KwStruct:               return "'struct'";
        case TokenKind::KwTrue:                 return "'true'";
        case TokenKind::KwFalse:                return "'false'";
        case TokenKind::KwElse:                 return "'else'";
        case TokenKind::KwBreak:                return "'break'";
        case TokenKind::KwContinue:             return "'continue'";
        case TokenKind::KwFunction:             return "'fn'";
        case TokenKind::KwVar:                  return "'var'";
        case TokenKind::KwIf:                   return "'if'";
        case TokenKind::KwWhile:                return "'while'";
        case TokenKind::KwFor:                  return "'for'";
        case TokenKind::KwReturn:               return "'return'";
        case TokenKind::KwMut:                  return "'mut'";
        case TokenKind::KwInterface:            return "'interface'";
        case TokenKind::KwType:                 return "'type'";
        case TokenKind::KwOperator:             return "'operator'";
        case TokenKind::KwPrefix:               return "'prefix'";
        case TokenKind::KwPostfix:              return "'postfix'";
        case TokenKind::KwCast:                 return "'cast'";

        case TokenKind::EndOfFile:               return "end of file";
        case TokenKind::Unknown:                 return "an unknown token";
    }

    return "an unknown token";
}
}
