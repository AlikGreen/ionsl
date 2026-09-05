#pragma once
#include <span>

#include "ast/module.h"
#include "ast/decl.h"
#include "ast/symbolTable.h"
#include "ast/typeSystem.h"
#include "codegen/codeGenerator.h"
#include "lexer/token.h"

namespace ionsl
{
class Compiler
{
public:
    Compiler();

    std::vector<Token> tokenize(const std::string &source);
    Module parse(std::span<Token> tokens);
    void link(Module& module);

    template<typename T>
    requires std::is_base_of_v<CodeGenerator, T> && std::is_constructible_v<T, const Module&, const SymbolTable&, const TypeTable&, const DeclTable&>
    std::string generate(const Module& module)
    {
        return T(module, m_symbolTable, m_typeTable, m_declTable).generate();
    }

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;

    Compiler(Compiler&&) noexcept = default;
    Compiler& operator=(Compiler&&) noexcept = default;
private:
    DeclarationIdAllocator m_declAllocator;
    SymbolTable m_symbolTable;
    TypeTable m_typeTable;
    DeclTable m_declTable;
    ScopeTable m_scopeTable;
    TypeSystem m_typeSystem;
};
}
