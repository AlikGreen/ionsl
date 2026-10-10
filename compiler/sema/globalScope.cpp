#include "globalScope.h"

#include "../ast/declarations.h"
#include "../ast/module.h"

namespace ionsl
{
    GlobalScope::GlobalScope(const Module &module)
    {
        for(const auto decl : module.declarations())
            registerDecl(decl->name, decl->id);
    }

    void GlobalScope::registerDecl(const SymbolId name, const DeclId id)
    {
        m_decls[name].push_back(id);
    }

    std::vector<DeclId> GlobalScope::find(const SymbolId name) const
    {
        const auto it = m_decls.find(name);
        return it != m_decls.end() ? it->second : std::vector<DeclId>{};
    }
}
