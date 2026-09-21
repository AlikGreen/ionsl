#pragma once

#include <memory>
#include <optional>

#include "decl.h"
#include "type.h"

#include "../common/arena.h"
#include "../common/diagnostics.h"


namespace ionsl
{
class Compiler;
class Declaration;

class Module
{
public:
    Module() = default;
    explicit Module(size_t arenaSize, Compiler& compiler);

    [[nodiscard]] Module clone() const;
    void clone(Module& newModule) const;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    Module(Module&&) noexcept = default;
    Module& operator=(Module&&) noexcept = default;

    [[nodiscard]] const DiagnosticSink& diagnostics() const { return m_diagnostics; }
    DiagnosticSink& diagnostics() { return m_diagnostics; }

    [[nodiscard]] const std::vector<Declaration*>& declarations() const { return m_declarations; }
    std::vector<Declaration*>& declarations() { return m_declarations; }

    [[nodiscard]] Arena& arena() const { return *m_arena; }

    [[nodiscard]] std::optional<DeclId> findTopLevelDecl(const std::string &name) const;

    // warning this is a very bad function and should not be run often
    [[nodiscard]] std::optional<TypeId> findType(const std::string &name) const; // FIXME
private:
    std::unique_ptr<Arena> m_arena;
    DiagnosticSink m_diagnostics;

    std::vector<Declaration*> m_declarations;

    Compiler* m_compiler{};

};
}
