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
            m_ast.diagnostics().add(e.diagnostic);
            return create<ErrorDecl>();
        }
    }

    FunctionDecl* Parser::parseFunctionDecl()
    {
        auto* decl = createDecl<FunctionDecl>();
        const SourceSpan start = peek().span;
        expect(TokenKind::KwFunction);
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

        if(check(TokenKind::LAngle))
        {
            decl->genericParams = parseGenericParams();
        }

        expect(TokenKind::LParen);

        m_currentScope = m_scopeTable.create(m_currentScope);
        decl->paramsScope = m_currentScope;

        if(!check(TokenKind::RParen))
        {
            do
            {
                decl->params.push_back(parseValueDecl());
            }
            while(match(TokenKind::Comma));
        }

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

        m_currentScope = m_scopeTable.getScope(m_currentScope).parent;
        return decl;
    }

    StructDecl* Parser::parseStructDecl()
    {
        const SourceSpan start = peek().span;

        auto* decl = createDecl<StructDecl>();
        expect(TokenKind::KwStruct);
        decl->name = m_symbolTable.intern(advance().text);
        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

        ScopeId outerScope = m_currentScope;
        m_currentScope = m_scopeTable.create(m_currentScope);

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
                auto* funcDecl = parseFunctionDecl();
                decl->methods.push_back(funcDecl);
                m_scopeTable.registerDecl(m_currentScope, funcDecl->name, funcDecl->id);
            }else
            {
                auto* fieldDecl = parseValueDecl();
                decl->fields.push_back(fieldDecl);
                m_scopeTable.registerDecl(m_currentScope, fieldDecl->name, fieldDecl->id);
                expect(TokenKind::Comma);
            }
        }

        decl->span = SourceSpan::between(start, previous().span);

        m_currentScope = outerScope;

        return decl;
    }

    InterfaceDecl* Parser::parseInterfaceDecl()
    {
        const SourceSpan start = peek().span;
        auto* decl = createDecl<InterfaceDecl>();
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
        decl->scope = m_currentScope;
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

        ScopeId membersScope = m_scopeTable.create(m_currentScope);
        decl->membersScope = membersScope;

        if (match(TokenKind::Colon))
        {
            decl->underlyingType = parseType();
        }

        expect(TokenKind::LBrace);

        while (!match(TokenKind::RBrace))
        {
            auto member = createDecl<EnumMember>();
            member->name = m_symbolTable.intern(consume(TokenKind::Identifier).text);
            member->parentId = decl->id;

            if (match(TokenKind::Equal))
                member->initializer = parseExpression();

            m_scopeTable.registerDecl(membersScope, member->name, member->id);
            decl->members.push_back(member);

            expect(TokenKind::Comma);
        }


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

        expect(TokenKind::Semicolon);

        return decl;
    }

    GenericParam* Parser::parseGenericParam()
    {
        // TODO implement value type params
        // TODO implement constraints eg interfaces

        auto* decl = createDecl<TypeGenericParam>();
        const SourceSpan startSpan = peek().span;

        decl->name = m_symbolTable.intern(peek().text);
        expect(TokenKind::Identifier);

        m_scopeTable.registerDecl(m_currentScope, decl->name, decl->id);

        if (match(TokenKind::Colon))
        {
            do
            {
                decl->constraints.push_back(parseName());
            }
            while (match(TokenKind::Amp));
        }

        decl->span = SourceSpan::between(startSpan, previous().span);

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
