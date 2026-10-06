#include "module.h"

#include "declarations.h"
#include "../compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../sema/semaContext.h"
#include "../sema/semanticAnalyzer.h"

namespace ionsl
{
    Module Module::clone() const
    {
        Module newModule{*m_compiler};
        clone(newModule);
        return std::move(newModule);
    }

    void Module::clone(Module &newModule) const
    {
        for(const auto* decl : m_declarations)
            newModule.m_declarations.push_back(decl->clone(*newModule.m_arena));
    }

    Module::Module(Compiler& compiler)
        : m_arena(std::make_unique<Arena>()), m_compiler(&compiler)
    {

    }

    std::optional<DeclId> Module::findTopLevelDecl(const std::string &name) const
    {
        const auto symbolId = m_compiler->m_symbolTable.find(name);
        if(!symbolId) return std::nullopt;

        for(const auto* decl : m_declarations)
            if(decl->name == *symbolId)
                return decl->id;

        return std::nullopt;
    }

    std::optional<TypeId> Module::findType(const std::string &name) const
    {
        // FIXME PLEASE
        auto tokens = Lexer::tokenize(name);
        Parser parser{tokens, *m_compiler};
        auto type = parser.parseType();

        DeclTable declTable{};

        GlobalScope globalScope{*this};

        Module m{};
        DeclAllocator d{};
        SemanticAnalyzer analyzer{m, m_compiler->m_symbolTable, m_compiler->m_typeSystem, m_compiler->m_scopeTable, d};
        return analyzer.resolveType(*type, SemaContext{});
    }
}
