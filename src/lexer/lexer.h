#pragma once
#include <vector>

#include "token.h"
#include "../common/diagnostics.h"

namespace ionsl
{
class Lexer
{
public:
    explicit Lexer(std::string_view source);
    std::vector<Token> tokenize();

    static std::vector<Token> tokenize(std::string_view source);
private:
    std::string_view m_source;
    SourceLocation m_loc{};

    Token nextToken();

    [[nodiscard]] Token makeToken(TokenKind kind, SourceLocation startLoc) const;

    void skipWhitespace();

    static bool isWhitespace(char c);
    char advance();
    [[nodiscard]] char peek(size_t offset = 0) const;
    [[nodiscard]] bool done() const;
};
}
