#pragma once
#include <unordered_set>

#include "../ast/declarations.h"
#include "../ast/module.h"
#include "../ast/typeSystem.h"
#include "../ast/typeTable.h"

namespace ionsl
{
class Extractor
{
public:
    Extractor(const Module& module, Compiler& compiler, const std::vector<DeclId> &entrypoints);
    Module extract();
private:
    const Module& m_module;
    Compiler& m_compiler;
    TypeTable& m_types;
    TypeSystem& m_typeSystem;
    SymbolTable& m_symbols;
    DeclAllocator& m_declAllocator;
    DeclTable m_decls;
    std::vector<DeclId> m_entrypoints;

    std::unordered_set<DeclId> m_extracted;
    std::unordered_set<DeclId> m_topLevelDecls;
    std::unordered_set<uint32_t> m_takenBindingIndices;
    std::unordered_map<uint32_t, uint32_t> m_nextBindingIndices;

    std::unordered_map<size_t, ResourceDecl*> m_resourceCache;

    void extractEntrypoint(FunctionDecl& decl, Module& out);
    void extractDecl(const Declaration& decl, Module& out);
    void extractType(TypeId id, Module& out);

    std::unordered_map<DeclId, DeclId> extractResources(FunctionDecl& declaration, Module& out);
    ResourceDecl* extractResourceDecl(const ValueDecl& param, const FunctionDecl &function, Module& out);
    ResourceDecl* findResourceDecl(const ValueDecl& param) const;
    void remapIdentifiers(BlockStmt& block, std::unordered_map<DeclId, DeclId> resourceMap);
    TypeId getResourceType(const ValueDecl& param) const;

    static size_t hashResource(TypeId type, SymbolId name);

    std::optional<ResourceKind> getResourceKind(const ValueDecl& param) const;
    uint32_t nextBindingIndex(uint32_t space);
};

}