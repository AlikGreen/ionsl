#include "compiler.h"

#include "lexer/lexer.h"
#include "parser/parser.h"

namespace ionsl
{
    std::vector<Token> Compiler::tokenize(const std::string &source)
    {
        return Lexer::tokenize(source);
    }

    Ast Compiler::parse(std::span<Token> tokens)
    {
        return Parser::parse(tokens, declAllocator, symbolTable);
    }
}
