#pragma once

#include "symbolTable.h"
#include "typeTable.h"
#include "../common/arena.h"
#include "../common/diagnostics.h"

namespace ionsl
{
class Declaration;

class Module
{
public:
    Arena arena;
    DiagnosticSink diagnostics;
    TypeTable typeTable;
    DeclTable declTable;

    std::vector<Declaration*> declarations;

    [[nodiscard]] Module clone() const;

    explicit Module(size_t arenaSize);

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    Module(Module&&) noexcept = default;
    Module& operator=(Module&&) noexcept = default;
};
}
