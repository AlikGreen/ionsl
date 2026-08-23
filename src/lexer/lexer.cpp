#include "lexer.h"
#include <string>
#include <unordered_map>

namespace ionsl
{
    static const std::unordered_map<std::string_view, TokenKind> keywords
    {
        { "struct", TokenKind::KwStruct }, { "mut", TokenKind::KwMut },
        { "true",  TokenKind::KwTrue }, { "false",  TokenKind::KwFalse },
        { "fn",  TokenKind::KwFunction }, { "var",  TokenKind::KwVar },
        { "if",  TokenKind::KwIf }, { "else",  TokenKind::KwElse },
        { "while",  TokenKind::KwWhile }, { "interface", TokenKind::KwInterface },
        { "for",  TokenKind::KwFor }, { "return",  TokenKind::KwReturn },
        { "break",  TokenKind::KwBreak }, { "continue",  TokenKind::KwContinue },
        { "type", TokenKind::KwType }, { "operator", TokenKind::KwOperator },
        { "cast", TokenKind::KwCast }, { "prefix", TokenKind::KwPrefix },
        { "postfix", TokenKind::KwPostfix },
    };

    static const std::unordered_map<std::string_view, TokenKind> symbols
    {
        { "(", TokenKind::LParen }, { ")", TokenKind::RParen },
        { "[", TokenKind::LBracket }, { "]", TokenKind::RBracket },
        { "{", TokenKind::LBrace }, { "}", TokenKind::RBrace },
        { "<", TokenKind::LAngle }, { ">", TokenKind::RAngle },
        { ",", TokenKind::Comma }, { ":", TokenKind::Colon },
        { ";", TokenKind::Semicolon }, { "=", TokenKind::Equal },
        { "::", TokenKind::ColonColon }, { ".", TokenKind::Dot },
        { "-", TokenKind::Minus }, { "+", TokenKind::Plus },
        { "->", TokenKind::Arrow }, { "!", TokenKind::Exclamation },
        { "*", TokenKind::Star }, { "/", TokenKind::Slash },
        { "==", TokenKind::EqualEqual }, { "!=", TokenKind::ExclamationEqual },
        { "<=", TokenKind::LessEqual },{ ">=", TokenKind::GreaterEqual },
        { "&&", TokenKind::AmpAmp }, { "||", TokenKind::PipePipe },
        { "--", TokenKind::MinusMinus }, { "++", TokenKind::PlusPlus },
        { "+=", TokenKind::PlusEqual }, { "-=", TokenKind::MinusEqual },
        { "*=", TokenKind::StarEqual }, { "/=", TokenKind::SlashEqual },
        { "[[", TokenKind::LBracketLBracket }, { "]]", TokenKind::RBracketRBracket },
        { "&=", TokenKind::AmpEqual }, { "|=", TokenKind::PipeEqual },
        { "<<", TokenKind::LAngleLAngle }, { ">>", TokenKind::RAngleRAngle },
        { "<<=", TokenKind::LAngleLAngleEqual }, { ">>=", TokenKind::RAngleRAngleEqual },
        { "^=", TokenKind::CaretEqual }, { "%=", TokenKind::PercentEqual },
        { "%", TokenKind::Percent },
    };


    static constexpr uint32_t kMaxSymbolLength = 3;

    Lexer::Lexer(const std::string_view source)
        : m_source(source)  { }

    std::vector<Token> Lexer::tokenize()
    {
        std::vector<Token> tokens{};

        while(!done())
        {
            Token token = nextToken();
            tokens.push_back(token);
            if(token.kind == TokenKind::EndOfFile)
                break;
        }

        return tokens;
    }

    std::vector<Token> Lexer::tokenize(const std::string_view source)
    {
        Lexer lexer{source};
        return lexer.tokenize();
    }

    Token Lexer::nextToken()
    {
        skipWhitespace();

        const SourceLocation startLoc = m_loc;

        if(done())
            return makeToken(TokenKind::EndOfFile, startLoc);

        // Skip comments
        if(peek() == '/')
        {
            if(peek(1) == '/')
            {
                while(!done() && peek() != '\n')
                    advance();
            }
            else if(peek(1) == '*')
            {
                while(!done() && !(peek() == '*' && peek(1) == '/'))
                    advance();
            }
        }

        if(peek() == '"')
        {
            advance();

            while(!done() && peek() != '"')
                advance();

            advance();

            return makeToken(TokenKind::StringLiteral, startLoc);
        }

        for(size_t i = std::min(kMaxSymbolLength, static_cast<uint32_t>(m_source.size() - m_loc.offset)); i >= 1; i--)
        {
            const std::string_view text = m_source.substr(startLoc.offset, i);

            if(const auto it = symbols.find(text); it != symbols.end())
            {
                for(size_t j = 0; j < i; j++)
                    advance();

                return makeToken(it->second, startLoc);
            }
        }

        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X'))
        {
            advance(); advance(); // consume "0x"
            while (std::isxdigit(peek())) advance();

            // consume suffix eg 'u'
            while (std::isalpha(peek())) advance();

            return makeToken(TokenKind::NumberLiteral, startLoc);
        }

        if(std::isdigit(peek()) || peek() == '-')
        {
            while (std::isdigit(peek())) advance();

            if (peek() == '.' && std::isdigit(peek(1)))
            {
                advance();
                while (std::isdigit(peek())) advance();
            }

            // consume exponent
            if (peek() == 'e' || peek() == 'E')
            {
                advance();
                if (peek() == '+' || peek() == '-') advance();
                while (std::isdigit(peek())) advance();
            }

            // consume suffix eg 'u'
            while (std::isalpha(peek())) advance();

            return makeToken(TokenKind::NumberLiteral, startLoc);
        }

        if(std::isalpha(peek()) || peek() == '_')
        {
            while(std::isalnum(peek()) || peek() =='_')
                advance();

            const std::string_view text = m_source.substr(startLoc.offset, m_loc.offset - startLoc.offset);

            if(const auto it = keywords.find(text); it != keywords.end())
                return makeToken(it->second, startLoc);

            return makeToken(TokenKind::Identifier, startLoc);
        }


        advance();
        return makeToken(TokenKind::Unknown, startLoc);
    }

    Token Lexer::makeToken(const TokenKind kind, const SourceLocation startLoc) const
    {
        Token token{};
        token.kind = kind;
        token.text = m_source.substr(startLoc.offset, m_loc.offset - startLoc.offset);
        token.span = SourceSpan{ startLoc, m_loc };
        return token;
    }

    void Lexer::skipWhitespace()
    {
        while(!done() && isWhitespace(peek()))
            advance();
    }

    bool Lexer::isWhitespace(char c)
    {
        return std::string("\n\r\t ").contains(c);
    }

    char Lexer::advance()
    {
        const char c = m_source[m_loc.offset++];
        if (c == '\n') { m_loc.line++; m_loc.column = 1; }
        else            { m_loc.column++; }
        return c;
    }

    char Lexer::peek(size_t offset) const
    {
        return m_source.at(m_loc.offset + offset);
    }

    bool Lexer::done() const
    {
        return m_loc.offset >= m_source.size();
    }
}
