#include "parser.h"

namespace ionsl
{
    Declaration* Parser::parseDeclaration()
    {
        parseAttributes();

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
            case TokenKind::KwType:
                return parseAliasDecl();
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
        decl->attributes = takeAttributes();
        expect(TokenKind::KwFunction);
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);
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
        decl->attributes = takeAttributes();
        expect(TokenKind::KwStruct);
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

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
        decl->attributes = takeAttributes();
        expect(TokenKind::KwInterface);
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

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
        parseAttributes();
        auto* decl = createDecl<ValueDecl>();
        decl->attributes = takeAttributes();
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);
        expect(TokenKind::Colon);
        decl->type = parseType();

        if(match(TokenKind::Equal))
            decl->initializer = parseExpression();

        return decl;
    }

    AliasDecl* Parser::parseAliasDecl()
    {
        expect(TokenKind::KwType);
        auto* decl = createDecl<AliasDecl>();

        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);


        if(check(TokenKind::LAngle))
        {
            decl->genericParams = parseGenericParams();
        }

        expect(TokenKind::Equal);

        decl->targetType = parseType();

        expect(TokenKind::Semicolon);

        return decl;
    }

    GenericParam * Parser::parseGenericParam()
    {
        // TODO implement value type params
        // TODO implement requirements eg interfaces

        auto* decl = createDecl<TypeGenericParam>();

        decl->span = peek().span;
        decl->name = m_symbolTable.intern(peek().text);
        expect(TokenKind::Identifier);

        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

        return decl;
    }

    std::vector<GenericParam*> Parser::parseGenericParams()
    {
        expect(TokenKind::LAngle);

        std::vector<GenericParam*> params;

        do
        {
            params.push_back(parseGenericParam());
        }while(match(TokenKind::Comma));

        expect(TokenKind::RAngle);

        return params;
    }
}
