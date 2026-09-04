#include "parser.h"

#include <format>

namespace ionsl
{
    Parser::Parser(const std::span<Token> tokens, DeclarationIdAllocator& declAllocator, SymbolTable& symbolTable, ScopeTable& scopeTable, DeclTable& declTable)
        : m_tokens(tokens), m_declAllocator(declAllocator), m_symbolTable(symbolTable), m_scopeTable(scopeTable), m_declTable(declTable)
    {
         m_currentScope = m_scopeTable.create(ScopeIdInvalid);
    }

    Module Parser::parse(const std::span<Token> tokens, DeclarationIdAllocator& declAllocator, SymbolTable& symbolTable, ScopeTable& scopeTable, DeclTable& declTable)
    {
        Parser parser{tokens, declAllocator, symbolTable, scopeTable, declTable};
        return parser.parse();
    }

    Module Parser::parse()
    {
        while(!atEnd())
        {
            m_ast.declarations.push_back(parseDeclaration());
        }

        m_declTable.regenerate(m_ast);
        return std::move(m_ast); // maybe should clone?
    }

    QualifiedName Parser::parseName()
    {
        QualifiedName qualified;
        do
        {
            auto name = std::string(consume(TokenKind::Identifier).text);
            qualified.parts.push_back(m_symbolTable.intern(name));
        }
        while(match(TokenKind::ColonColon));
        return qualified;
    }

    LiteralValue Parser::parseLiteral()
    {
        switch (advance().kind)
        {
            case TokenKind::NumberLiteral:
            {
                std::string_view text = previous().text;
                if(text.starts_with("0x"))
                {
                    uint64_t value;
                    text.remove_prefix(2);
                    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
                    // TODO handle error
                    return value;
                }
                if(text.contains(".eE"))
                {
                    if(text.ends_with("f"))
                        text.remove_prefix(1);

                    double value;
                    text.remove_prefix(2);
                    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
                    // TODO handle error
                    return value;
                }
                if(text.starts_with("-"))
                {
                    int64_t value;
                    text.remove_prefix(2);
                    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
                    // TODO handle error
                    return value;
                }

                if(text.ends_with("u"))
                    text.remove_prefix(1);

                uint64_t value;
                auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
                // TODO handle error
                return value;
            }
            case TokenKind::StringLiteral:
                return std::string(previous().text);
            case TokenKind::KwTrue:
                return true;
            case TokenKind::KwFalse:
                return false;
            default:
                return (uint64_t)0;
        }
    }

    void Parser::parseAttributes()
    {
        while(match(TokenKind::LBracketLBracket))
        {
            Attribute attr;
            attr.name = parseName();
            if(match(TokenKind::LParen))
            {
                do
                {
                    attr.args.push_back(parseAttribArg());
                }
                while (!match(TokenKind::RParen));
            }
            expect(TokenKind::RBracketRBracket);
            m_pendingAttributes.push_back(attr);
        }
    }

    std::vector<Attribute> Parser::takeAttributes()
    {
        auto pending = m_pendingAttributes;
        m_pendingAttributes.clear();
        return pending;
    }

    AttributeArg Parser::parseAttribArg()
    {
        if(match(TokenKind::Identifier))
            return parseName();

        return parseLiteral();
    }

    void Parser::reportError(const SourceSpan &span, const std::string &message)
    {
        m_ast.diagnostics.add(message, span, Severity::Error);
    }

    ParserState Parser::saveState() const
    {
        return {
            m_pos,
            m_ast.diagnostics.diagnostics().size()
        };
    }

    void Parser::restoreState(const ParserState &state)
    {
        m_pos = state.m_tokenIndex;
        m_ast.diagnostics.diagnostics().resize(state.m_diagnosticCount);
    }

    bool Parser::atEnd() const
    {
        return m_tokens[m_pos].kind == TokenKind::EndOfFile || m_pos >= m_tokens.size();
    }

    const Token& Parser::peek() const
    {
        return m_tokens[m_pos];
    }

    const Token & Parser::previous() const
    {
        return m_tokens[m_pos - 1];
    }

    const Token& Parser::advance()
    {
        m_pos++;
        return previous();
    }

    bool Parser::check(const TokenKind kind) const
    {
        return peek().kind == kind;
    }

    bool Parser::match(const TokenKind kind)
    {
        if(atEnd() || peek().kind != kind)
            return false;

        advance();
        return true;
    }

    bool Parser::expect(const TokenKind kind)
    {
        advance();

        if(previous().kind != kind)
            reportError(peek().span, std::format("Expected '{}' found '{}'", tokenKindDisplayName(kind), tokenKindDisplayName(previous().kind)));

        return previous().kind == kind;
    }

    const Token& Parser::consume(const TokenKind kind, std::string_view message)
    {
        advance();

        if(previous().kind != kind)
            reportError(peek().span, std::format("Expected '{}' found '{}': {}", tokenKindDisplayName(kind), tokenKindDisplayName(previous().kind), message));

        return previous();
    }
}
