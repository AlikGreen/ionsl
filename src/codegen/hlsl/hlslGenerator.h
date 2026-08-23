#pragma once
#include "../codeGenerator.h"

namespace ionsl
{
class HlslGenerator final : public CodeGenerator
{
public:
    explicit HlslGenerator(const Module &module, const SymbolTable& symbolTable)
        : CodeGenerator(module, symbolTable) { }

    std::string generate() override;
};
}
