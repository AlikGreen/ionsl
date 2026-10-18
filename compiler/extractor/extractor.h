#pragma once
#include <unordered_set>

#include "../ast/declarations.h"
#include "../ast/module.h"
#include "../ast/typeTable.h"

namespace ionsl
{
class Extractor
{
public:
    Extractor(const Module& module, Compiler& compiler, TypeTable& typeTable, const std::vector<DeclId> &entrypoints);
    Module extract();
private:
    const Module& m_module;
    Compiler& m_compiler;
    TypeTable& m_types;
    DeclTable m_decls;
    std::vector<DeclId> m_entrypoints;

    std::unordered_set<DeclId> m_extracted;
    std::unordered_set<DeclId> m_topLevelDecls;

    void extractEntrypoint(FunctionDecl& decl, Module& out);
    void extractDecl(const Declaration& decl, Module& out);
    void extractStructDecl(const StructDecl& decl, Module& out);
    void extractType(TypeId id, Module& out);
};
}
