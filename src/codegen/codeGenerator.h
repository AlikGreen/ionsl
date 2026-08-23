#pragma once
#include <string>

#include "../ast/module.h"

namespace ionsl
{
class CodeGenerator
{
public:
    virtual ~CodeGenerator() = default;

    explicit CodeGenerator(const Module& module, const SymbolTable& symbolTable)
        : m_module(module), m_symbols(symbolTable) { }

    virtual std::string generate() = 0;
protected:
    const Module& m_module;
    const SymbolTable& m_symbols;
};
}
