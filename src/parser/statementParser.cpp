#include "parser.h"

namespace ionsl
{
    Statement* Parser::parseStatement()
    {
        switch (peek().kind)
        {
            case TokenKind::KwFor:
                return parseForStmt();
            case TokenKind::KwWhile:
                return parseWhileStmt();
            case TokenKind::KwIf:
                return parseIfStmt();
            case TokenKind::KwReturn:
                return parseReturnStmt();
            case TokenKind::KwBreak:
                return parseBreakStmt();
            case TokenKind::KwContinue:
                return parseContinueStmt();
            case TokenKind::LBrace:
                return parseBlockStmt();
            case TokenKind::KwVar:
                return parseDeclStmt();
            default:
                return parseExprStmt();
        }
    }

    BlockStmt* Parser::parseBlockStmt()
    {
        auto* stmt = create<BlockStmt>();
        const auto start = peek().span;

        expect(TokenKind::LBrace);

        m_currentScope = m_scopeTable.create(m_currentScope);
        stmt->scope = m_currentScope;

        while(!match(TokenKind::RBrace))
        {
            stmt->statements.push_back(parseStatement());
        }

        m_currentScope = m_scopeTable.getScope(m_currentScope).parent;

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    WhileStmt* Parser::parseWhileStmt()
    {
        auto* stmt = create<WhileStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwWhile);

        expect(TokenKind::LParen);
        stmt->condition = parseExpression();
        expect(TokenKind::RParen);
        stmt->body = parseBlockStmt();

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    ForStmt* Parser::parseForStmt()
    {
        auto* stmt = create<ForStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwFor);

        expect(TokenKind::LParen);
        stmt->init = parseDeclStmt();
        stmt->condition = parseExpression();
        expect(TokenKind::Semicolon);
        stmt->increment = parseExpression();
        expect(TokenKind::RParen);
        stmt->body = parseBlockStmt();

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    IfStmt* Parser::parseIfStmt()
    {
        auto* stmt = create<IfStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwIf);

        expect(TokenKind::LParen);
        stmt->condition = parseExpression();
        expect(TokenKind::RParen);
        stmt->thenBranch = parseBlockStmt();

        if(match(TokenKind::KwElse))
            stmt->elseBranch = parseBlockStmt();

        stmt->span = SourceSpan::between(start, previous().span);

        return stmt;
    }

    ReturnStmt* Parser::parseReturnStmt()
    {
        auto* stmt = create<ReturnStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwReturn);

        if(!check(TokenKind::Semicolon))
            stmt->expr = parseExpression();

        expect(TokenKind::Semicolon);

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    BreakStmt* Parser::parseBreakStmt()
    {
        auto* stmt = create<BreakStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwBreak);
        expect(TokenKind::Semicolon);

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    ContinueStmt* Parser::parseContinueStmt()
    {
        auto* stmt = create<ContinueStmt>();
        const auto start = peek().span;

        expect(TokenKind::KwContinue);
        expect(TokenKind::Semicolon);

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    DeclStmt* Parser::parseDeclStmt()
    {
        auto* stmt = create<DeclStmt>();
        const auto start = peek().span;

        stmt->decl = parseDeclaration();

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }

    ExprStmt* Parser::parseExprStmt()
    {
        auto* stmt = create<ExprStmt>();
        const auto start = peek().span;

        stmt->expr = parseExpression();
        expect(TokenKind::Semicolon);

        stmt->span = SourceSpan::between(start, previous().span);
        return stmt;
    }
}
