#pragma once
#include <span>

#include "ast/module.h"
#include "ast/decl.h"
#include "ast/scopeTable.h"
#include "ast/symbolTable.h"
#include "ast/typeSystem.h"
#include "codegen/codeGenerator.h"

namespace ionsl
{
struct LinkDescription
{
    std::vector<Module*> modules;
    std::unordered_map<DeclId, std::vector<TypeId>> specializations;
};

class Compiler
{
public:
    Compiler();

    Module compile(const std::string &source);
    Module link(const LinkDescription& desc);

    template<typename T>
    requires std::is_base_of_v<CodeGenerator, T> && std::is_constructible_v<T, const Module&, const SymbolTable&, const TypeTable&>
    std::string generate(const Module& module)
    {
        return T(module, m_symbolTable, m_typeTable).generate();
    }

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;

    Compiler(Compiler&&) noexcept = default;
    Compiler& operator=(Compiler&&) noexcept = default;
private:
    friend class Module;
    friend class Parser;

    SymbolTable m_symbolTable;
    TypeTable m_typeTable;
    ScopeTable m_scopeTable;
    TypeSystem m_typeSystem;
    DeclAllocator m_declAllocator;

    Module m_stdlib{};
};
}
