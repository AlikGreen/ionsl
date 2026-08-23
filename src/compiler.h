#pragma once
#include <span>

#include "ast/module.h"
#include "ast/decl.h"
#include "ast/symbolTable.h"
#include "codegen/codeGenerator.h"
#include "lexer/token.h"

namespace ionsl
{
class Compiler
{
public:
    std::vector<Token> tokenize(const std::string &source);
    Module parse(std::span<Token> tokens);

    template<typename T>
    requires std::is_base_of_v<CodeGenerator, T> && std::is_constructible_v<T, const Module&, const SymbolTable&>
    std::string generate(const Module& module)
    {
        return T(module, symbolTable).generate();
    }

    Compiler() = default;

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;

    Compiler(Compiler&&) noexcept = default;
    Compiler& operator=(Compiler&&) noexcept = default;
private:
    DeclarationIdAllocator declAllocator;
    SymbolTable symbolTable;
};
}
