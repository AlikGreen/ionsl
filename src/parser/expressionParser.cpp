#include <format>

#include "parser.h"

namespace ionsl
{
    static std::unordered_map<BinaryOp, std::pair<uint8_t, uint8_t>> kInfixBindingPower
    {
        { BinaryOp::Assign,        { 2, 1 } },
        { BinaryOp::AddAssign,     { 2, 1 } },
        { BinaryOp::SubAssign,     { 2, 1 } },
        { BinaryOp::MulAssign,     { 2, 1 } },
        { BinaryOp::DivAssign,     { 2, 1 } },

        { BinaryOp::ModuloAssign,     { 2, 1 } },
        { BinaryOp::BitwiseAndAssign,  { 2, 1 } },
        { BinaryOp::BitwiseOrAssign,   { 2, 1 } },
        { BinaryOp::BitwiseXorAssign,  { 2, 1 } },
        { BinaryOp::ShiftLeftAssign,     { 2, 1 } },
        { BinaryOp::ShiftRightAssign,     { 2, 1 } },

        { BinaryOp::LogicalOr,     { 3, 4 } },
        { BinaryOp::LogicalAnd,    { 5, 6 } },
        { BinaryOp::BitwiseOr,     { 7, 8 } },
        { BinaryOp::BitwiseXor,    { 9, 10 } },
        { BinaryOp::BitwiseAnd,    { 11, 12 } },

        { BinaryOp::Equal,         { 13, 14 } },
        { BinaryOp::NotEqual,      { 13, 14 } },

        { BinaryOp::Less,          { 15, 16 } },
        { BinaryOp::LessEqual,     { 15, 16 } },
        { BinaryOp::Greater,       { 15, 16 } },
        { BinaryOp::GreaterEqual,  { 15, 16 } },

        { BinaryOp::ShiftLeft,     { 17, 18 } },
        { BinaryOp::ShiftRight,    { 17, 18 } },

        { BinaryOp::Add,           { 19, 20 } },
        { BinaryOp::Subtract,      { 19, 20 } },

        { BinaryOp::Multiply,      { 21, 22 } },
        { BinaryOp::Divide,        { 21, 22 } },
        { BinaryOp::Modulo,        { 21, 22 } }
    };

    constexpr uint8_t kPrefixUnaryBindingPower = 23;

    std::optional<std::pair<uint32_t, uint32_t>> Parser::getBindingPower(const TokenKind kind)
    {
        if (const auto op = tokenToBinaryOp(kind))
        {
            return kInfixBindingPower[op.value()];
        }

        switch (kind)
        {
            case TokenKind::LParen:
            case TokenKind::ColonColonLAngle:
            case TokenKind::LBracket:
            case TokenKind::Dot:
                return std::pair<uint32_t, uint32_t>{100, 0};

            default:
                return std::nullopt;
        }
    }

    Expression* Parser::parseExpression(const uint32_t minBP, const std::unordered_set<TokenKind>& stopTokens)
    {
        auto lhs = parsePrefixExpr();

        while(true)
        {
            if(stopTokens.contains(peek().kind))
                break;

            auto bp = getBindingPower(peek().kind);
            if(!bp.has_value()) break;

            auto [lBP, rBP] = bp.value();

            if(lBP < minBP)
                break;

            Token opToken = advance();
            lhs = parseInfixExpr(lhs, opToken.kind);
        }
        return lhs;
    }

    Expression* Parser::parseInfixExpr(Expression* left, TokenKind opKind)
    {
        switch (opKind)
        {
            case TokenKind::Dot:
            {
                const auto fieldName = m_symbolTable.intern(advance().text);

                auto* fieldAccess = create<FieldAccessExpr>();

                fieldAccess->object = left;
                fieldAccess->memberName = fieldName;
                fieldAccess->span = SourceSpan::between(left->span, previous().span);

                return fieldAccess;
            }
            case TokenKind::LParen:
            case TokenKind::ColonColonLAngle:
            {
                auto* call = create<CallExpr>();

                if(opKind == TokenKind::ColonColonLAngle)
                {
                    call->genericArgs = parseGenericArgs();
                }

                while (!match(TokenKind::RParen))
                {
                    call->args.push_back(parseExpression());
                    match(TokenKind::Comma);
                }

                call->callee = left;
                call->span = SourceSpan::between(left->span, previous().span);

                return call;
            }
            case TokenKind::LBracket:
            {
                Expression* index = parseExpression();
                match(TokenKind::RBracket);

                auto indexExpr = create<IndexExpr>();
                indexExpr->array = left;
                indexExpr->index = index;
                indexExpr->span  = SourceSpan::between(left->span, previous().span);

                return indexExpr;
            }
            default:
            {
                if (auto op = tokenToBinaryOp(opKind))
                {
                    auto [lBP, rBP] = kInfixBindingPower[op.value()];
                    Expression* right = parseExpression(rBP);

                    auto* binary = create<BinaryExpr>();
                    binary->op = op.value();
                    binary->left = left;
                    binary->right = right;
                    binary->span = SourceSpan::between(left->span, previous().span);

                    return binary;
                }
                return left;
            }
        }
    }

    Expression* Parser::parsePrefixExpr()
    {
        Token token = peek();

        switch (token.kind)
        {
            case TokenKind::NumberLiteral:
            case TokenKind::StringLiteral:
            case TokenKind::KwTrue:
            case TokenKind::KwFalse:
                return parseLiteralExpr();

            case TokenKind::Identifier:
                return parseIdentifierExpr();

            case TokenKind::Minus:
            case TokenKind::Exclamation:
            case TokenKind::MinusMinus:
            case TokenKind::PlusPlus:
            {
                advance();
                Expression* operand = parseExpression(kPrefixUnaryBindingPower);
                return makeUnaryPrefixExpr(token, std::move(operand));
            }

            case TokenKind::LParen:
            {
                advance();
                Expression* expr = parseExpression();
                match(TokenKind::RParen);
                return expr;
            }

            default:
            {
                advance();
                reportError(previous().span, std::format("expected an expression found {}", tokenKindDisplayName(previous().kind)));
                return create<ErrorExpr>();
            }
        }
    }

    Expression* Parser::makeUnaryPrefixExpr(const Token &operatorToken, Expression* operand)
    {
        UnaryOp op;

        switch (operatorToken.kind)
        {
            case TokenKind::Exclamation:
                op = UnaryOp::LogicalNot;
            break;
            case TokenKind::Minus:
                op = UnaryOp::Negate;
            break;
            case TokenKind::MinusMinus:
                op = UnaryOp::PreDecrement;
            break;
            case TokenKind::PlusPlus:
                op = UnaryOp::PreIncrement;
            break;
            default:
                return create<ErrorExpr>(); // TODO diagnostics
        }

        UnaryExpr* expr = create<UnaryExpr>();
        expr->op = op;
        expr->operand = operand;
        expr->span = SourceSpan::between(operatorToken.span, previous().span);

        return expr;
    }


    LiteralExpr* Parser::parseLiteralExpr()
    {
        auto* expr = create<LiteralExpr>();
        expr->literal = parseLiteral();
        return expr;
    }

    IdentifierExpr * Parser::parseIdentifierExpr()
    {
        auto* expr = create<IdentifierExpr>();
        expr->name = parseName();
        return expr;
    }
}
