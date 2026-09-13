#include "compiler.h"

#include <filesystem>
#include <fstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/globalScope.h"
#include "sema/semanticAnalyzer.h"

namespace ionsl
{
    std::optional<std::string> loadFile(const std::string& path)
    {
        std::ifstream file{path, std::ios::in | std::ios::binary};
        if (!file)
            return std::nullopt;

        std::ostringstream buffer;
        buffer << file.rdbuf();

        if (file.bad())
            return std::nullopt;

        return buffer.str();
    }

    Compiler::Compiler()
        : m_typeSystem(m_typeTable, m_symbolTable)
    {
        m_stdlib = compile(*loadFile(R"(C:\Users\alikg\CLionProjects\ionsl\std\core.ionsl)"));
    }

    Module Compiler::compile(const std::string &source)
    {
        auto tokens = Lexer::tokenize(source);
        return Parser::parse(tokens, m_symbolTable, m_scopeTable, m_declAllocator);
    }

    Module Compiler::link(const LinkDescription &desc)
    {
        Module module{10*1024*1024};

        for(const Module* m : desc.modules)
            m->clone(module);

        m_stdlib.clone(module);

        SemanticAnalyzer::analyze(module, m_symbolTable, m_typeSystem, m_scopeTable, m_declAllocator);

        return std::move(module);
    }

}
