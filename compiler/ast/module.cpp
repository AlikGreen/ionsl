#include "module.h"

#include "declarations.h"
#include "../compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../sema/semaContext.h"
#include "../sema/typeResolver.h"

namespace ionsl
{
    Module Module::clone() const
    {
        Module newModule{m_arena->capacity(), *m_compiler};
        clone(newModule);
        return std::move(newModule);
    }

    void Module::clone(Module &newModule) const
    {
        for(const auto* decl : m_declarations)
            newModule.m_declarations.push_back(decl->clone(*newModule.m_arena));
    }

    Module::Module(const size_t arenaSize, Compiler& compiler)
        : m_arena(std::make_unique<Arena>(arenaSize)), m_compiler(&compiler)
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
        auto tokens = Lexer::tokenize(name);
        Parser parser{tokens, *m_compiler};
        auto type = parser.parseType();

        DeclTable declTable{};

        ConstantEvaluator eval{declTable, m_compiler->m_typeSystem};
        GlobalScope globalScope{*this};

        TypeResolver resolver{m_compiler->m_typeSystem, eval, m_compiler->m_symbolTable, m_compiler->m_scopeTable, globalScope, declTable};
        return resolver.resolveType(*type, SemaContext{});
    }
}
