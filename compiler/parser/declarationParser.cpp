#include "parser.h"

namespace ionsl
{
    Declaration* Parser::parseDeclaration()
    {
        try
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
                case TokenKind::KwEnum:
                    return parseEnumDecl();
                case TokenKind::KwAttribute:
                    return parseAttributeDecl();
                default:
                {
                    advance();
                    return create<ErrorDecl>();
                }
            }
        }
        catch (const ParseError& e)
        {
            // TODO sync
            return create<ErrorDecl>();
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

        if(check(TokenKind::LAngle))
        {
            decl->genericParams = parseGenericParams();
        }

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
        else
            expect(TokenKind::Semicolon);

        decl->span = SourceSpan::between(start, previous().span);
        return decl;
    }

    StructDecl* Parser::parseStructDecl()
    {
        const SourceSpan start = peek().span;

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

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    InterfaceDecl* Parser::parseInterfaceDecl()
    {
        const SourceSpan start = peek().span;

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

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    ValueDecl* Parser::parseVarDecl()
    {
        const SourceSpan start = peek().span;
        expect(TokenKind::KwVar);
        match(TokenKind::KwMut); // FIXME
        auto* var = parseValueDecl();
        expect(TokenKind::Semicolon);

        var->span = SourceSpan::between(start, previous().span);
        return var;
    }

    ValueDecl* Parser::parseValueDecl()
    {
        const SourceSpan start = peek().span;
        parseAttributes();
        auto* decl = createDecl<ValueDecl>();
        decl->attributes = takeAttributes();
        decl->name = m_symbolTable.intern(consume(TokenKind::Identifier).text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);
        expect(TokenKind::Colon);
        decl->type = parseType();

        if(match(TokenKind::Equal))
            decl->initializer = parseExpression();

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    AliasDecl* Parser::parseAliasDecl()
    {
        const SourceSpan start = peek().span;
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

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    EnumDecl* Parser::parseEnumDecl()
    {
        const SourceSpan start = peek().span;
        expect(TokenKind::KwEnum);

        auto* decl = createDecl<EnumDecl>();
        decl->name = m_symbolTable.intern(consume(TokenKind::Identifier).text);

        if (match(TokenKind::Colon))
        {
            decl->underlyingType = parseType();
        }

        expect(TokenKind::LBrace);

        do
        {
            EnumMember member{};
            member.name = m_symbolTable.intern(consume(TokenKind::Identifier).text);

            if (match(TokenKind::Equal))
                member.initializer = parseExpression();

            decl->members.push_back(member);
        }
        while (!match(TokenKind::Comma));

        expect(TokenKind::RBrace);

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    AttributeDecl* Parser::parseAttributeDecl()
    {
        const SourceSpan start = peek().span;
        expect(TokenKind::KwAttribute);

        auto* decl = createDecl<AttributeDecl>();
        decl->name = m_symbolTable.intern(consume(TokenKind::Identifier).text);

        expect(TokenKind::LParen);

        bool first = true;

        while (!match(TokenKind::RParen))
        {
            if (!first) expect(TokenKind::Comma);
            first = false;
            decl->fields.push_back(parseValueDecl());
        }

        decl->span = SourceSpan::between(start, previous().span);

        return decl;
    }

    GenericParam* Parser::parseGenericParam()
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
