#include "parser.h"

#include <format>

#include "../compiler.h"

namespace ionsl
{
    Parser::Parser(const std::span<Token> tokens, Compiler& compiler)
        : m_tokens(tokens), m_ast(10*1024*1024, compiler), m_symbolTable(compiler.m_symbolTable), m_scopeTable(compiler.m_scopeTable), m_declAllocator(compiler.m_declAllocator)
    {
        m_currentScope = ScopeId::None;
    }

    Module Parser::parse(const std::span<Token> tokens, Compiler& compiler)
    {
        Parser parser{tokens, compiler};
        return parser.parse();
    }

    Module Parser::parse()
    {
        while(!atEnd())
        {
            m_ast.declarations().push_back(parseDeclaration());
        }

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
                if (text.contains('.') || text.contains('e') || text.contains('E'))
                {
                    if(text.ends_with("f"))
                        text.remove_suffix(1);

                    double value;
                    text.remove_prefix(2);
                    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
                    // TODO handle error
                    return value;
                }
                if(text.starts_with("-"))
                {
                    int64_t value;
                    text.remove_prefix(1);
                    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
                    // TODO handle error
                    return -value;
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
        if(check(TokenKind::Identifier))
        {
            auto name = parseName();
            AttribArgValue value;
            if(match(TokenKind::Equal))
            {
                if(check(TokenKind::Identifier))
                    return AttributeArg{ name.parts.front(), parseName() };

                return AttributeArg{ parseLiteral() };
            }

            return AttributeArg{ name };
        }

        return AttributeArg{ parseLiteral() };
    }

    void Parser::reportError(const SourceSpan &span, const std::string &message)
    {
        m_ast.diagnostics().error(span, "{}", message);
    }

    ParserState Parser::saveState() const
    {
        return {
            m_pos,
            m_ast.diagnostics().logs().size()
        };
    }

    void Parser::restoreState(const ParserState &state)
    {
        m_pos = state.m_tokenIndex;
        m_ast.diagnostics().logs().resize(state.m_diagnosticCount);
    }

    bool Parser::atEnd() const
    {
        return m_pos >= m_tokens.size() || m_tokens[m_pos].kind == TokenKind::EndOfFile;
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
