#include "parser.h"

namespace ionsl
{
    Declaration* Parser::parseDeclaration()
    {
        switch (peek().kind)
        {
            case TokenKind::KwStruct:
                return parseStructDecl();
            case TokenKind::KwInterface:
                return parseInterfaceDecl();
            case TokenKind::KwFunction:
                return parseFunctionDecl();
            case TokenKind::KwVar:
                return parseVarDecl();
            default:
            {
                advance();
                return create<ErrorDecl>();
            }
        }
    }

    FunctionDecl* Parser::parseFunctionDecl()
    {
        auto* decl = createDecl<FunctionDecl>();
        const SourceSpan start = peek().span;
        decl->attributes = parseAttributes();
        expect(TokenKind::KwFunction);
        decl->name = m_symbolTable.intern(advance().text);
        expect(TokenKind::LParen);

        if(!check(TokenKind::RParen))
            do
            {
                decl->params.push_back(parseValueDecl());
            }
            while(match(TokenKind::Comma));

        expect(TokenKind::RParen);

        if(match(TokenKind::Arrow))
            decl->returnType = parseType();
        else
            decl->returnType = createVoidType(start);

        if(check(TokenKind::LBrace))
            decl->body = parseBlockStmt();

        decl->span = SourceSpan::between(start, previous().span);
        return decl;
    }

    StructDecl* Parser::parseStructDecl()
    {
        auto* decl = createDecl<StructDecl>();
        decl->attributes = parseAttributes();
        expect(TokenKind::KwStruct);
        decl->name = m_symbolTable.intern(advance().text);

        if(match(TokenKind::Colon))
        {
            decl->interfaces.push_back(parseType());

            while (!check(TokenKind::LBrace))
            {
                expect(TokenKind::Comma);
                decl->interfaces.push_back(parseType());
            }
        }

        expect(TokenKind::LBrace);

        while(!match(TokenKind::RBrace))
        {
            if(check(TokenKind::KwFunction))
            {
                decl->methods.push_back(parseFunctionDecl());
            }else
            {
                decl->fields.push_back(parseValueDecl());
                expect(TokenKind::Comma);
            }
        }

        return decl;
    }

    InterfaceDecl* Parser::parseInterfaceDecl()
    {
        auto* decl = createDecl<InterfaceDecl>();
        decl->attributes = parseAttributes();
        expect(TokenKind::KwInterface);
        decl->name = m_symbolTable.intern(advance().text);

        expect(TokenKind::LBrace);

        while(!match(TokenKind::RBrace))
        {
            decl->methods.push_back(parseFunctionDecl());
        }

        return decl;
    }

    ValueDecl* Parser::parseVarDecl()
    {
        expect(TokenKind::KwVar);
        match(TokenKind::KwMut); // FIXME
        auto* var = parseValueDecl();
        expect(TokenKind::Semicolon);
        return var;
    }

    ValueDecl* Parser::parseValueDecl()
    {
        auto* decl = createDecl<ValueDecl>();
        decl->attributes = parseAttributes();
        decl->name = m_symbolTable.intern(advance().text);
        expect(TokenKind::Colon);
        decl->type = parseType();

        if(match(TokenKind::Equal))
            decl->initializer = parseExpression();

        return decl;
    }

}
