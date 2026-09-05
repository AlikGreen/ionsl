#pragma once
#include <string>

#include "../ast/module.h"

namespace ionsl
{
class CodeGenerator
{
public:
    virtual ~CodeGenerator() = default;

    explicit CodeGenerator(const Module& module, const SymbolTable& symbolTable, const TypeTable& typeTable, const DeclTable& declTable)
        : m_module(module), m_symbols(symbolTable), m_typeTable(typeTable), m_declTable(declTable) { }

    virtual std::string generate() = 0;
protected:
    const Module& m_module;
    const SymbolTable& m_symbols;
    const TypeTable& m_typeTable;
    const DeclTable& m_declTable;
};
}
