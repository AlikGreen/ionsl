#include <utility>

#include "parser.h"

namespace ionsl
{
    TypeSyntax* Parser::parseType()
    {
        const SourceSpan startSpan = peek().span;

        QualifiedName name = parseName();

        TypeSyntax* type = parseNamedType(std::move(name), startSpan);

        if(type == nullptr)
            return nullptr;

        while(check(TokenKind::LBracket))
        {
            type = parseArrayType(type, startSpan);

            if(type == nullptr)
            {
                return nullptr;
            }
        }

        return type;
    }

    NamedTypeSyntax * Parser::parseNamedType(QualifiedName name, const SourceSpan &start)
    {
        auto* type = m_ast.arena.create<NamedTypeSyntax>();
        type->name = std::move(name);

        if(match(TokenKind::LAngle))
        {
            if(check(TokenKind::RAngle))
            {
                reportError(peek().span, "expected generic argument");
                return nullptr;
            }

            do
            {
                type->arguments.push_back(parseGenericArgument());
            }while(match(TokenKind::Comma));

            if(!expect(TokenKind::RAngle))
                return nullptr;
        }

        type->span = SourceSpan::between(start, previous().span);
        return type;
    }

    ArrayTypeSyntax * Parser::parseArrayType(TypeSyntax* elementType, const SourceSpan &start)
    {
        auto* type = m_ast.arena.create<ArrayTypeSyntax>();
        expect(TokenKind::LBracket);

        auto* size = parseExpression();

        if(!expect(TokenKind::RBracket))
            return nullptr;

        type->size = size;
        type->elementType = elementType;
        type->span = SourceSpan::between(start, previous().span);

        return type;
    }

    TypeArgument* Parser::parseGenericArgument()
    {
        if (startsUnambiguousConst())
        {
            auto* expr = parseExpression();

            auto* arg = create<TypeArgumentValue>();
            arg->span = expr->span;
            arg->expression = expr;
            return arg;
        }

        if (check(TokenKind::Identifier)) // Types all start with identifiers
        {
            const auto snapshot = saveState();

            if (auto* type = parseType())
            {
                auto* arg = create<TypeArgumentType>();
                arg->span = type->span;
                arg->type = type;
                return arg;
            }

            restoreState(snapshot);
        }

        auto* expr = parseExpression();

        auto* arg = create<TypeArgumentValue>();
        arg->span = expr->span;
        arg->expression = expr;
        return arg;
    }

    NamedTypeSyntax* Parser::createVoidType(const SourceSpan &span)
    {
        auto* type = m_ast.arena.create<NamedTypeSyntax>();
        type->name = QualifiedName::single(m_symbolTable.intern(std::string_view("void")));
        type->span = span;

        return type;
    }

    bool Parser::startsUnambiguousConst() const
    {
        if(check(TokenKind::NumberLiteral) ||
            check(TokenKind::StringLiteral) ||
            check(TokenKind::KwTrue) ||
            check(TokenKind::KwFalse) ||
            check(TokenKind::LParen) ||
            check(TokenKind::Exclamation) ||
            check(TokenKind::Minus))
        {
            return true;
        }

        return false;
    }
}
