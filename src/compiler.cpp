#include "compiler.h"

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/semanticAnalyzer.h"

namespace ionsl
{
    Compiler::Compiler()
        : m_typeSystem(m_typeTable, m_declTable, m_symbolTable)
    {
    }

    std::vector<Token> Compiler::tokenize(const std::string &source)
    {
        return Lexer::tokenize(source);
    }

    Module Compiler::parse(std::span<Token> tokens)
    {
        return Parser::parse(tokens, m_declAllocator, m_symbolTable, m_scopeTable, m_declTable);
    }

    void Compiler::link(Module& module)
    {
        SemanticAnalyzer::analyze(module, m_symbolTable, m_typeSystem, m_declTable, m_scopeTable);
    }
}
