#pragma once
#include <span>
#include <unordered_map>

#include "../ast/decl.h"
#include "../ast/qualifiedName.h"
#include "../ast/symbol.h"

namespace ionsl
{
class GlobalScope
{
public:
    explicit GlobalScope(const Module& module);
    void registerDecl(SymbolId name, DeclId id);
    [[nodiscard]] std::span<const DeclId> find(SymbolId name) const;
private:
    std::unordered_map<SymbolId, std::vector<DeclId>> m_decls;
};
}
