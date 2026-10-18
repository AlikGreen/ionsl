#pragma once
#include <span>

#include "ast/module.h"
#include "ast/decl.h"
#include "ast/scopeTable.h"
#include "ast/symbolTable.h"
#include "ast/typeSystem.h"
#include "codegen/codeGenerator.h"
#include "reflection/reflection.h"

namespace ionsl
{
struct EntrypointSpecialization
{
    std::vector<TypeId> typeArgs;
    std::string newName;
};

struct LinkDescription
{
    std::vector<Module*> modules;
    std::unordered_map<DeclId, EntrypointSpecialization> specializations;
};

class Compiler
{
public:
    Compiler();

    Module parse(const std::string &source);
    Module link(const LinkDescription& desc);
    Module extract(const Module& module, const std::vector<DeclId> &entryPoints);
    ShaderReflection reflect(const Module& module);

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
